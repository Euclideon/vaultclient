#include "vcTexture.h"

#include "udWorkerPool.h"
#include "udStringUtil.h"
#include "udFile.h"

struct AsyncTextureLoadInfo
{
  bool loadedSuccess = false;

  vcTexture **ppTexture;
  const char *pFilename;
  vcTextureFilterMode filterMode;
  bool hasMips;
  vcTextureWrapMode wrapMode;

  void *pData;
  int64_t dataLen;

  uint32_t limitTextureSize;
};

void vcTexture_AsyncLoadWorkerThreadWork(void *pTextureLoadInfo)
{
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;

  if (udFile_Load(pLoadInfo->pFilename, &pLoadInfo->pData, &pLoadInfo->dataLen) == udR_Success)
    pLoadInfo->loadedSuccess = true;
}

void vcTexture_AsyncLoadMainThreadWork(void *pTextureLoadInfo)
{
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;

  vcTexture_CreateFromMemory(pLoadInfo->ppTexture, pLoadInfo->pData, pLoadInfo->dataLen, nullptr, nullptr, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode, vcTCF_None, 0, pLoadInfo->limitTextureSize);

  udFree(pLoadInfo->pFilename);
  udFree(pLoadInfo->pData);
}

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

#include <stdio.h>

void vcTexture_ResizePixels(const void* pPixels, uint32_t width, uint32_t height, uint32_t maxDimensionSize, const void **ppResultPixels, uint32_t *pResultWidth, uint32_t *pResultHeight)
{
  uint32_t maxSize = udMax(width, height);
  int passes = 0;
  //while ((maxSize >> passes) > maxDimensionSize)
  //  ++passes;

  const void* pLastPixels = pPixels;
  uint32_t lastWidth = width;
  uint32_t lastHeight = height;

  //for (int i = 0; i < passes; ++i)
  while ((maxSize >> passes) > maxDimensionSize)
  {
    lastWidth >>= 1;
    lastHeight >>= 1;
    uint32_t* pDownsampledPixels = udAllocType(uint32_t, lastWidth * lastHeight, udAF_Zero);

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

    if (passes != 0)// && i != passes - 1)
      udFree(pLastPixels);

    pLastPixels = pDownsampledPixels;
    ++passes;
  }

  printf("%d x %d...passes=%d, %d x %d\n", width, height, passes, lastWidth, lastHeight);

  *ppResultPixels = pLastPixels;
  *pResultWidth = lastWidth;
  *pResultHeight = lastHeight;
}
