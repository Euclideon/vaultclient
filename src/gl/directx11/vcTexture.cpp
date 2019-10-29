#include "vcD3D11.h"

#include "gl/vcTexture.h"
#include "gl/vcLayout.h"

#include "vcSettings.h"

#include "udFile.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include "stb_image.h"

enum
{
  MaxMipLevels = 4,
};

void vcTexture_GetFormatAndPixelSize(const vcTextureFormat format, bool isRenderTarget, int *pPixelSize = nullptr, DXGI_FORMAT *pTextureFormat = nullptr)
{
  DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
  int pixelSize = 0;

  switch (format)
  {
  case vcTextureFormat_RGBA8:
    textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    pixelSize = 4;
    break;
  case vcTextureFormat_BGRA8:
    textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    pixelSize = 4;
    break;
  case vcTextureFormat_D24S8:
    if (isRenderTarget)
      textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
    else
      textureFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pixelSize = 4;
    break;
  case vcTextureFormat_D32F:
    if (isRenderTarget)
      textureFormat = DXGI_FORMAT_R32_TYPELESS;
    else
      textureFormat = DXGI_FORMAT_R32_FLOAT;
    pixelSize = 4;
    break;

  case vcTextureFormat_Unknown: // fall through
  case vcTextureFormat_Count:
    break;
  }

  if (pPixelSize != nullptr)
    *pPixelSize = pixelSize;

  if (pTextureFormat != nullptr)
    *pTextureFormat = textureFormat;
}

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0 || format == vcTextureFormat_Unknown || format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  // only allow mip maps for certain formats
  if (format != vcTextureFormat_RGBA8)
    hasMipmaps = false;

  udResult result = udR_Success;
  DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN;
  int pixelBytes = 0;
  bool isDynamic = ((flags & vcTCF_Dynamic) == vcTCF_Dynamic);
  bool isRenderTarget = ((flags & vcTCF_RenderTarget) == vcTCF_RenderTarget);
  vcTexture_GetFormatAndPixelSize(format, isRenderTarget, &pixelBytes, &texFormat);

  UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA subResource[MaxMipLevels] = {};
  D3D11_SUBRESOURCE_DATA *pSubData = nullptr;
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));

  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);
  UD_ERROR_NULL(pTexture, udR_MemoryAllocationFailure);

  if (isRenderTarget)
  {
    if (format == vcTextureFormat_D32F || format == vcTextureFormat_D24S8)
      bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    else
      bindFlags |= D3D11_BIND_RENDER_TARGET;
  }

  // Upload texture to graphics system
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = hasMipmaps ? MaxMipLevels : 1;
  desc.ArraySize = 1;
  desc.Format = texFormat;
  desc.SampleDesc.Count = 1;
  desc.Usage = (isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
  desc.BindFlags = bindFlags;
  desc.CPUAccessFlags = (isDynamic ? D3D11_CPU_ACCESS_WRITE : 0);

  if (pPixels != nullptr)
  {
    subResource[0].pSysMem = pPixels;
    subResource[0].SysMemPitch = desc.Width * pixelBytes;
    subResource[0].SysMemSlicePitch = 0;
    pSubData = subResource;

    // Manually generate MaxMipLevels levels of mip maps
    if (hasMipmaps && !isRenderTarget)
    {
      const void *pLastPixels = pPixels;
      uint32_t lastWidth = width;
      uint32_t lastHeight = height;

      for (int i = 1; i < MaxMipLevels; ++i)
      {
        lastWidth >>= 1;
        lastHeight >>= 1;
        uint32_t *pMippedPixels = udAllocType(uint32_t, lastWidth * lastHeight, udAF_Zero);

        for (uint32_t y = 0; y < lastHeight; ++y)
        {
          for (uint32_t x = 0; x < lastWidth; ++x)
          {
            uint8_t r = 0, g = 0, b = 0, a = 0;

            // 4x4 bilinear sampling
            for (int s = 0; s < 4; ++s)
            {
              uint32_t sample = ((uint32_t *)pLastPixels)[(y * 2 + s / 2) * lastWidth * 2 + (x * 2 + s % 2)];
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

  UD_ERROR_IF(g_pd3dDevice->CreateTexture2D(&desc, pSubData, &pTexture->pTextureD3D) != S_OK, udR_InternalError);

  // Free mip map memory
  if (hasMipmaps && pPixels && !isRenderTarget)
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
    if (isRenderTarget)
    {
      if (format == vcTextureFormat_D24S8)
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      else if (format == vcTextureFormat_D32F)
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    }

    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    UD_ERROR_IF(g_pd3dDevice->CreateShaderResourceView(pTexture->pTextureD3D, &srvDesc, &pTexture->pTextureView) != S_OK, udR_InternalError);

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
    UD_ERROR_IF(g_pd3dDevice->CreateSamplerState(&sampDesc, &pTexture->pSampler) != S_OK, udR_InternalError);
  }

  if ((flags & vcTCF_AsynchronousRead) == vcTCF_AsynchronousRead)
  {
    // Create two staging textures to be ping-ponged between for asynchronous reads
    D3D11_TEXTURE2D_DESC stagingDesc;
    ZeroMemory(&stagingDesc, sizeof(stagingDesc));
    stagingDesc.Width = width;
    stagingDesc.Height = height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = texFormat;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    for (int i = 0; i < 2; ++i)
      UD_ERROR_IF(g_pd3dDevice->CreateTexture2D(&stagingDesc, nullptr, &pTexture->pStagingTextureD3D[i]) != S_OK, udR_InternalError);
  }

  pTexture->flags = flags;
  pTexture->format = format;
  pTexture->width = width;
  pTexture->height = height;
  pTexture->isDynamic = isDynamic;
  pTexture->isRenderTarget = isRenderTarget;
  pTexture->d3dFormat = desc.Format;
  vcGLState_ReportGPUWork(0, 0, size_t((pTexture->width * pTexture->height * pixelBytes) * (hasMipmaps ? 1.3333f : 1.0f)));

  *ppTexture = pTexture;
  pTexture = nullptr;

epilogue:
  if (pTexture != nullptr)
    vcTexture_Destroy(&pTexture);

  return result;
}


bool vcTexture_CreateFromMemory(vcTexture **ppTexture, void *pFileData, size_t fileLength, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFileData == nullptr || fileLength == 0)
    return false;

  uint32_t width, height, channelCount;
  vcTexture *pTexture = nullptr;

  uint8_t *pData = stbi_load_from_memory((stbi_uc *)pFileData, (int)fileLength, (int *)& width, (int *)& height, (int *)& channelCount, 4);

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

bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFilename == nullptr)
    return false;

  void *pFileData;
  int64_t fileLen;

  if (udFile_Load(pFilename, &pFileData, &fileLen) != udR_Success)
    return false;

  bool result = vcTexture_CreateFromMemory(ppTexture, pFileData, fileLen, pWidth, pHeight, filterMode, hasMipmaps, wrapMode, flags, aniFilter);

  udFree(pFileData);
  return result;
}

udResult vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height)
{
  if (pTexture == nullptr || !pTexture->isDynamic || pTexture->width != width || pTexture->height != height || pTexture->pTextureD3D == nullptr)
    return udR_InvalidParameter_;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, pTexture->isRenderTarget, &pixelBytes, &texFormat);

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  UD_ERROR_IF(g_pd3dDeviceContext->Map(pTexture->pTextureD3D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK, udR_InternalError);
  memcpy(mappedResource.pData, pPixels, width * height * pixelBytes);
  g_pd3dDeviceContext->Unmap(pTexture->pTextureD3D, 0);

  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * pixelBytes);

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

  for (int i = 0; i < 2; ++i)
  {
    if ((*ppTexture)->pStagingTextureD3D[i] != nullptr)
      (*ppTexture)->pStagingTextureD3D[i]->Release();
  }

  udFree(*ppTexture);
  *ppTexture = nullptr;
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

bool vcTexture_BeginReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels, vcFramebuffer *pFramebuffer)
{
  udUnused(pFramebuffer);

  if (pTexture == nullptr || pPixels == nullptr || int(x + width) > pTexture->width || int(y + height) > pTexture->height)
    return false;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count)
    return false;

  udResult result = udR_Success;
  if ((pTexture->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead && pTexture->pStagingTextureD3D[pTexture->stagingIndex] == nullptr)
  {
    // Texture not configured for pixel read back, create a single temporary staging texture.
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = pTexture->d3dFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    // Create a full res texture, because subsequent reads may work with different regions. 
    // Might be some optimizations here, if not D24S8 format, only recreate staging texture if sizes do not match.
    desc.Width = pTexture->width;
    desc.Height = pTexture->height;

    UD_ERROR_IF(g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pTexture->pStagingTextureD3D[pTexture->stagingIndex]) != S_OK, udR_InternalError);
  }

  // Begin asynchronous copy
  if (pTexture->format == vcTextureFormat_D24S8 || pTexture->format == vcTextureFormat_D32F)
  {
    // If you use CopySubresourceRegion with a depth-stencil buffer or a multisampled resource, you must copy the whole subresource
    // TODO: What about D32F format? (its not a depth-stencil)
    g_pd3dDeviceContext->CopyResource(pTexture->pStagingTextureD3D[pTexture->stagingIndex], pTexture->pTextureD3D);
  }
  else
  {
    // left, top, front, right, bottom, back
    D3D11_BOX srcBox = { x, y, 0, (x + width), (y + height), 1 };
    g_pd3dDeviceContext->CopySubresourceRegion(pTexture->pStagingTextureD3D[pTexture->stagingIndex], 0, x, y, 0, pTexture->pTextureD3D, 0, &srcBox);
  }

  // If not configured for asynchronous read, perform copy immediately
  if ((pTexture->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead)
  {
    UD_ERROR_IF(!vcTexture_EndReadPixels(pTexture, x, y, width, height, pPixels), udR_InternalError);
    pTexture->stagingIndex = 0; // force only using single staging texture
  }

epilogue:
  return result == udR_Success;
}

bool vcTexture_EndReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pTexture == nullptr || pPixels == nullptr || int(x + width) > pTexture->width || int(y + height) > pTexture->height)
    return false;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count || pTexture->pStagingTextureD3D[pTexture->stagingIndex] == nullptr)
    return false;

  udResult result = udR_Success;
  int pixelBytes = 0;
  uint32_t *pPixelData = nullptr;
  D3D11_MAPPED_SUBRESOURCE msr;
  vcTexture_GetFormatAndPixelSize(pTexture->format, pTexture->isRenderTarget, &pixelBytes);

  ID3D11Texture2D *pStagingTexture = pTexture->pStagingTextureD3D[pTexture->stagingIndex];
  pTexture->stagingIndex = (pTexture->stagingIndex + 1) & 1;

  UD_ERROR_IF(g_pd3dDeviceContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &msr) != S_OK, udR_InternalError);

  pPixelData = ((uint32_t *)msr.pData) + (x + y * pTexture->width);
  memcpy(pPixels, pPixelData, width * height * pixelBytes);

epilogue:
  if (pStagingTexture != nullptr)
    g_pd3dDeviceContext->Unmap(pStagingTexture, 0); // unmapping a non-mapped texture seems fine

  return result == udR_Success;
}
