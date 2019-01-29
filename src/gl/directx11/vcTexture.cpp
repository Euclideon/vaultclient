#include "vcD3D11.h"

#include "gl/vcTexture.h"

#include "vcSettings.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"
#include "stb_image.h"

enum
{
  MaxMipLevels = 4,
};

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  // only allow mip maps for certain formats
  if (format != vcTextureFormat_RGBA8)
    hasMipmaps = false;

  udResult result = udR_Success;

  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);

  DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN;
  int pixelBytes = 4;

  pTexture->isDynamic = ((flags & vcTCF_Dynamic) == vcTCF_Dynamic);
  pTexture->isRenderTarget = ((flags & vcTCF_RenderTarget) == vcTCF_RenderTarget);

  switch (format)
  {
  case vcTextureFormat_RGBA8:
    texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    pixelBytes = 4;
    break;
  case vcTextureFormat_BGRA8:
    texFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    pixelBytes = 4;
    break;
  case vcTextureFormat_D24S8:
    texFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pixelBytes = 4;
    break;
  case vcTextureFormat_D32F:
    if (pTexture->isRenderTarget)
      texFormat = DXGI_FORMAT_R32_TYPELESS;
    else
      texFormat = DXGI_FORMAT_R32_FLOAT;
    pixelBytes = 4;
    break;
  default:
    UD_ERROR_SET(udR_InvalidParameter_);
  }

  UINT bindFlags = 0;
  if (pTexture->isRenderTarget && (format == vcTextureFormat_D32F || format == vcTextureFormat_D24S8))
    bindFlags = D3D11_BIND_DEPTH_STENCIL;
  else if (pTexture->isRenderTarget)
    bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  else
    bindFlags = D3D11_BIND_SHADER_RESOURCE;

  // Upload texture to graphics system
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = hasMipmaps ? MaxMipLevels : 1;
  desc.ArraySize = 1;
  desc.Format = texFormat;
  desc.SampleDesc.Count = 1;
  desc.Usage = (pTexture->isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
  desc.BindFlags = bindFlags;
  desc.CPUAccessFlags = (pTexture->isDynamic ? D3D11_CPU_ACCESS_WRITE : 0);

  D3D11_SUBRESOURCE_DATA subResource[MaxMipLevels];
  D3D11_SUBRESOURCE_DATA *pSubData = nullptr;

  if (pPixels != nullptr)
  {
    subResource[0].pSysMem = pPixels;
    subResource[0].SysMemPitch = desc.Width * pixelBytes;
    subResource[0].SysMemSlicePitch = 0;
    pSubData = subResource;

    // Manually generate MaxMipLevels levels of mip maps
    if (hasMipmaps && !pTexture->isRenderTarget)
    {
      const void *pLastPixels = pPixels;
      uint32_t lastWidth = width;
      uint32_t lastHeight = height;

      for (int i = 1; i < MaxMipLevels; ++i)
      {
        lastWidth >>= 1;
        lastHeight >>= 1;
        uint32_t *pMippedPixels = udAllocType(uint32_t, lastWidth  * lastHeight, udAF_Zero);

        for (uint32_t y = 0; y < lastHeight; ++y)
        {
          for (uint32_t x = 0; x < lastWidth; ++x)
          {
            uint8_t r = 0, g = 0, b = 0, a = 0;

            // 4x4 bilinear sampling
            for (int s = 0; s < 4; ++s)
            {
              uint32_t sample = ((uint32_t*)pLastPixels)[(y * 2 + s / 2) * lastWidth * 2 + (x * 2 + s % 2)];
              r += ((sample >> 0) & 0xff) >> 2;
              g += ((sample >> 8) & 0xff) >> 2;
              b += ((sample >> 16) & 0xff) >> 2;
              a += ((sample >> 24) & 0xff) >> 2;
            }
            pMippedPixels[y * lastWidth + x] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
          }
        }

        subResource[i].pSysMem = pMippedPixels;
        subResource[i].SysMemPitch = lastWidth * pixelBytes;
        subResource[i].SysMemSlicePitch = 0;

        pLastPixels = pMippedPixels;
      }
    }
  }

  g_pd3dDevice->CreateTexture2D(&desc, pSubData, &pTexture->pTextureD3D);

  // Free mip map memory
  if (hasMipmaps && pPixels && !pTexture->isRenderTarget)
  {
    for (int i = 1; i < MaxMipLevels; ++i)
    {
      udFree(subResource[i].pSysMem);
    }
  }

  // Create texture view (if desired)
  if ((bindFlags & D3D11_BIND_SHADER_RESOURCE) == D3D11_BIND_SHADER_RESOURCE)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = texFormat;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    g_pd3dDevice->CreateShaderResourceView(pTexture->pTextureD3D, &srvDesc, &pTexture->pTextureView);

    uint32_t maxAnisotropic = vcGLState_GetMaxAnisotropy(aniFilter);

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = maxAnisotropic == 0 ? vcTFMToD3D[filterMode] : D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = vcTWMToD3D[wrapMode];
    sampDesc.AddressV = vcTWMToD3D[wrapMode];
    sampDesc.AddressW = vcTWMToD3D[wrapMode];
    sampDesc.MipLODBias = 0.f;
    sampDesc.MaxAnisotropy = maxAnisotropic;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    for (int i = 0; i < 4; ++i)
      sampDesc.BorderColor[i] = 1.f;
    sampDesc.MinLOD = 0.f;
    sampDesc.MaxLOD = hasMipmaps ? 4.0f : 0.f;
    g_pd3dDevice->CreateSamplerState(&sampDesc, &pTexture->pSampler);
  }

  pTexture->d3dFormat = desc.Format;
  pTexture->format = format;
  pTexture->width = width;
  pTexture->height = height;

  *ppTexture = pTexture;

  pTexture = nullptr;

epilogue:
  if (pTexture != nullptr)
    vcTexture_Destroy(&pTexture);

  return result;
}

bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFilename == nullptr)
    return false;

  uint32_t width, height, channelCount;
  vcTexture *pTexture = nullptr;

  void *pFileData;
  int64_t fileLen;

  if (udFile_Load(pFilename, &pFileData, &fileLen) != udR_Success)
    return false;

  uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
  udFree(pFileData);

  if (pData)
    vcTexture_Create(&pTexture, width, height, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags, aniFilter);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  *ppTexture = pTexture;

  return (pTexture != nullptr);
}

udResult vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height)
{
  if (pTexture == nullptr || !pTexture->isDynamic || pTexture->width != width || pTexture->height != height || pTexture->pTextureD3D == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  UD_ERROR_IF(g_pd3dDeviceContext->Map(pTexture->pTextureD3D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK, udR_InternalError);
  memcpy(mappedResource.pData, pPixels, width * height * 4);
  g_pd3dDeviceContext->Unmap(pTexture->pTextureD3D, 0);

epilogue:
  return result;
}

void vcTexture_Destroy(vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr)
    return;

  if ((*ppTexture)->pSampler != nullptr)
    (*ppTexture)->pSampler->Release();

  if ((*ppTexture)->pTextureD3D != nullptr)
    (*ppTexture)->pTextureD3D->Release();

  if ((*ppTexture)->pTextureView != nullptr)
    (*ppTexture)->pTextureView->Release();

  udFree(*ppTexture);
  *ppTexture = nullptr;
}

bool vcTexture_LoadCubemap(vcTexture **ppTexture, const char *pFilename)
{
  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);
  udFilename fileName(pFilename);

  const char* names[] = { "_LF", "_RT", "_FR", "_BK", "_UP", "_DN" };
  pTexture->format = vcTextureFormat_Cubemap;
  uint8_t *pFacePixels[6];

  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    int width, height, depth;

    char fileNameNoExt[256] = "";
    fileName.ExtractFilenameOnly(fileNameNoExt, (int)udLengthOf(fileNameNoExt));
    pFacePixels[i] = stbi_load(vcSettings_GetAssetPath(udTempStr("assets/skyboxes/%s%s%s", fileNameNoExt, names[i], fileName.GetExt())), &width, &height, &depth, 4);

    pTexture->height = height;
    pTexture->width = width;

    VERIFY_GL();
  }

  // Create D3D11 texture
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = pTexture->width;
  desc.Height = pTexture->height;
  desc.MipLevels = 1;
  desc.ArraySize = 6;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;

  ID3D11Texture2D *pTextureD3D = nullptr;

  D3D11_SUBRESOURCE_DATA subResource[6];

  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    subResource[i].pSysMem = pFacePixels[i];
    subResource[i].SysMemPitch = desc.Width * 4;
    subResource[i].SysMemSlicePitch = 0;
  }

  g_pd3dDevice->CreateTexture2D(&desc, subResource, &pTextureD3D);

  // Create texture view
  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
  ZeroMemory(&srvDesc, sizeof(srvDesc));
  srvDesc.Format = desc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
  srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
  srvDesc.Texture2DArray.MostDetailedMip = 0;
  srvDesc.Texture2DArray.ArraySize = desc.ArraySize;

  g_pd3dDevice->CreateShaderResourceView(pTextureD3D, &srvDesc, &pTexture->pTextureView);

  if (pTextureD3D != nullptr)
    pTextureD3D->Release();

  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(sampDesc));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.MipLODBias = 0.f;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
  sampDesc.MinLOD = 0.f;
  sampDesc.MaxLOD = 0.f;
  g_pd3dDevice->CreateSamplerState(&sampDesc, &pTexture->pSampler);

  pTexture->d3dFormat = desc.Format;
  pTexture->format = vcTextureFormat_RGBA8;

  *ppTexture = pTexture;

  // Unload temp memory
  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    stbi_image_free(pFacePixels[i]);
  }

#if UDPLATFORM_OSX
  SDL_free(pBaseDir);
#endif

  *ppTexture = pTexture;
  return true;
}

udResult vcTexture_GetSize(vcTexture *pTexture, int *pWidth, int *pHeight)
{
  if (pTexture == nullptr)
    return udR_InvalidParameter_;

  if (pWidth != nullptr)
    *pWidth = pTexture->width;

  if (pHeight != nullptr)
    *pHeight = pTexture->height;

  return udR_Success;
}
