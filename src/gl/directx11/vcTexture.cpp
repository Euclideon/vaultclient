#include "vcD3D11.h"

#include "gl/vcTexture.h"

#include "vcSettings.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"
#include "stb_image.h"

bool vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return false;

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
  case vcTextureFormat_D24:
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
  }

  UINT bindFlags = 0;
  if (pTexture->isRenderTarget && (format == vcTextureFormat_D32F || format == vcTextureFormat_D24))
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
  desc.MipLevels = hasMipmaps ? 4 : 1; // 4 choose by random
  desc.ArraySize = 1;
  desc.Format = texFormat;
  desc.SampleDesc.Count = 1;
  desc.Usage = (pTexture->isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
  desc.BindFlags = bindFlags;
  desc.CPUAccessFlags = (pTexture->isDynamic ? D3D11_CPU_ACCESS_WRITE : 0);

  D3D11_SUBRESOURCE_DATA subResource;
  D3D11_SUBRESOURCE_DATA *pSubData = nullptr;

  if (pPixels != nullptr)
  {
    subResource.pSysMem = pPixels;
    subResource.SysMemPitch = desc.Width * pixelBytes;
    subResource.SysMemSlicePitch = 0;
    pSubData = &subResource;
  }

  g_pd3dDevice->CreateTexture2D(&desc, pSubData, &pTexture->pTextureD3D);

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

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = vcTFMToD3D[filterMode];
    sampDesc.AddressU = vcTWMToD3D[wrapMode];
    sampDesc.AddressV = vcTWMToD3D[wrapMode];
    sampDesc.AddressW = vcTWMToD3D[wrapMode];
    sampDesc.MipLODBias = 0.f;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MinLOD = 0.f;
    sampDesc.MaxLOD = 0.f;
    g_pd3dDevice->CreateSamplerState(&sampDesc, &pTexture->pSampler);
  }

  pTexture->d3dFormat = desc.Format;
  pTexture->format = format;
  pTexture->width = width;
  pTexture->height = height;

  *ppTexture = pTexture;
  return true;
}

bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/)
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
    vcTexture_Create(&pTexture, width, height, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  *ppTexture = pTexture;

  return (pTexture != nullptr);
}

void vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height)
{
  if (pTexture == nullptr || !pTexture->isDynamic || pTexture->width != width || pTexture->height != height || pTexture->pTextureD3D == nullptr)
    return;

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  g_pd3dDeviceContext->Map(pTexture->pTextureD3D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  memcpy(mappedResource.pData, pPixels, width * height * 4);
  g_pd3dDeviceContext->Unmap(pTexture->pTextureD3D, 0);
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
}

bool vcTexture_LoadCubemap(vcTexture **ppTexture, const char *pFilename)
{
  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);
  udFilename fileName(pFilename);

  const char* names[] = { "_LF", "_RT", "_FR", "_BK", "_UP", "_DN" };
  pTexture->format = vcTextureFormat_Cubemap;

  size_t filenameLen = udStrlen(pFilename);

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  const char skyboxPath[] = ASSETDIR;
#elif UDPLATFORM_OSX
  char *pSkyboxPath;
  char *pBaseDir = SDL_GetBasePath();
  if (pBaseDir)
    pSkyboxPath = pBaseDir;
  else
    pSkyboxPath = SDL_strdup("./");
  char skyboxPath[1024] = "";
  udSprintf(skyboxPath, 1024, "%s", pSkyboxPath);
#else
  const char skyboxPath[] = ASSETDIR "skyboxes/";
#endif

  size_t pathLen = udStrlen(skyboxPath);
  char *pFilePath = udStrdup(skyboxPath, filenameLen + 5);

  uint8_t *pFacePixels[6];

  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    int width, height, depth;

    char fileNameNoExt[256] = "";
    fileName.ExtractFilenameOnly(fileNameNoExt,UDARRAYSIZE(fileNameNoExt));
    udSprintf(pFilePath, filenameLen + 5 + pathLen, "%s%s%s%s", skyboxPath, fileNameNoExt, names[i], fileName.GetExt());
    pFacePixels[i] = stbi_load(pFilePath, &width, &height, &depth, 4);

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
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = desc.MipLevels;
  srvDesc.Texture2D.MostDetailedMip = 0;

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

  udFree(pFilePath);

#if UDPLATFORM_OSX
  SDL_free(pBaseDir);
#endif

  *ppTexture = pTexture;
  return true;
}
