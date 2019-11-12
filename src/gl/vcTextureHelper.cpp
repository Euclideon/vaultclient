#include "vcTexture.h"

#include "udWorkerPool.h"
#include "udStringUtil.h"
#include "udFile.h"

#include "stb_image.h"

struct AsyncTextureLoadInfo
{
  bool loadedSuccess = false;

  vcTexture **ppTexture;
  const char *pFilename;
  vcTextureFilterMode filterMode;
  bool hasMips;
  vcTextureWrapMode wrapMode;

  uint32_t limitTextureSize;

  // hmm
  const void* pPixels;
  vcTextureFormat format;
  uint32_t width;
  uint32_t height;
};

void vcTexture_AsyncLoadWorkerThreadWork(void *pTextureLoadInfo)
{
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;
  void* pData = nullptr;
  int64_t dataLen = 0;
  uint32_t width, height, channelCount;

  pLoadInfo->loadedSuccess = true;

  if (pLoadInfo->pFilename != nullptr)
  {
   // printf("Loading pixels from file: %s...\n", pLoadInfo->pFilename);
    if (udFile_Load(pLoadInfo->pFilename, &pData, &dataLen) != udR_Success)
      pLoadInfo->loadedSuccess = false;

    //printf("Decoding pixels...\n");
    uint8_t* pPixels = stbi_load_from_memory((stbi_uc*)pData, (int)dataLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

    if (pPixels == nullptr)
      pLoadInfo->loadedSuccess = false;

    pLoadInfo->pPixels = udMemDup(pPixels, width * height * 4, 0, udAF_Zero);
    pLoadInfo->width = width;
    pLoadInfo->height = height;
    pLoadInfo->format = vcTextureFormat_RGBA8;

    udFree(pData);
    stbi_image_free(pPixels);
  }

  if (pLoadInfo->loadedSuccess && (pLoadInfo->width > pLoadInfo->limitTextureSize || pLoadInfo->height > pLoadInfo->limitTextureSize))
  {
    const void* pResizedPixels = nullptr;
    uint32_t resizedWidth = 0;
    uint32_t resizedHeight = 0;

    vcTexture_ResizePixels(pLoadInfo->pPixels, pLoadInfo->width, pLoadInfo->height, pLoadInfo->limitTextureSize, &pResizedPixels, &resizedWidth, &resizedHeight);
   // printf("Resized from %dx%d to %dx%d...\n", pLoadInfo->width, pLoadInfo->height, resizedWidth, resizedHeight);

    udFree(pLoadInfo->pPixels);

    pLoadInfo->pPixels = pResizedPixels;
    pLoadInfo->width = resizedWidth;
    pLoadInfo->height = resizedHeight;
  }

  udFree(pLoadInfo->pFilename);
}

void vcTexture_AsyncLoadMainThreadWork(void *pTextureLoadInfo)
{
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;

  //vcTexture_CreateFromMemory(pLoadInfo->ppTexture, pLoadInfo->pData, pLoadInfo->dataLen, nullptr, nullptr, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode, vcTCF_None, 0, pLoadInfo->limitTextureSize);

  if (pLoadInfo->loadedSuccess)
    vcTexture_Create(pLoadInfo->ppTexture, pLoadInfo->width, pLoadInfo->height, pLoadInfo->pPixels, pLoadInfo->format, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode);
  else
  {
    // texture failed to load, load a 1x1 instead
    uint32_t whitePixel = 0xffffffff;
    vcTexture_Create(pLoadInfo->ppTexture, 1, 1, &whitePixel);
  }

  udFree(pLoadInfo->pPixels);
}

//void vcTexture_AsyncLoadMainThreadWorkCreate(void* pTextureLoadInfo)
//{
//  AsyncTextureLoadInfo* pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;
//
//  vcTexture_Create(pLoadInfo->ppTexture, pLoadInfo->width, pLoadInfo->height, pLoadInfo->pPixels, pLoadInfo->format, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode, vcTCF_None, 0, pLoadInfo->limitTextureSize);
//
//  udFree(pLoadInfo->pPixels);
//}

udResult vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  *ppTexture = nullptr;

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pFilename = udStrdup(pFilename);
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;

epilogue:
  return result;
}

udResult vcTexture_AsyncCreate(vcTexture** ppTexture, udWorkerPool* pPool, uint32_t width, uint32_t height, const void* pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /* = -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr)
    return udR_InvalidParameter_;

  udResult result;
  int pixelBytes = 4;

  AsyncTextureLoadInfo* pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  *ppTexture = nullptr;

  
  //vcTexture_GetFormatAndPixelSize(format, &pixelBytes);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pPixels = udMemDup(pPixels, width * height * pixelBytes, 0, udAF_None);
  pLoadInfo->width = width;
  pLoadInfo->height = height;
  pLoadInfo->format = format;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;

epilogue:
  return result;
}

udResult vcTexture_ResizePixels(const void* pPixels, uint32_t width, uint32_t height, uint32_t maxDimensionSize, const void **ppResultPixels, uint32_t *pResultWidth, uint32_t *pResultHeight)
{
  udResult result;
  uint32_t maxDimension = udMax(width, height);
  int passCount = 0;
  const void* pLastPixels = pPixels;
  uint32_t lastWidth = width;
  uint32_t lastHeight = height;

  while ((maxDimension >> passCount) > maxDimensionSize)
  {
    lastWidth >>= 1;
    lastHeight >>= 1;
    uint32_t* pDownsampledPixels = udAllocType(uint32_t, lastWidth * lastHeight, udAF_Zero);
    UD_ERROR_NULL(pDownsampledPixels, udR_MemoryAllocationFailure);

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
        pDownsampledPixels[y * lastWidth + x] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
      }
    }

    if (passCount != 0)
      udFree(pLastPixels);

    ++passCount;
    pLastPixels = pDownsampledPixels;
  }

  result = udR_Success;
  *ppResultPixels = pLastPixels;
  *pResultWidth = lastWidth;
  *pResultHeight = lastHeight;

epilogue:
  return result;
}
