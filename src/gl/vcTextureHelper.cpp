#include "vcTexture.h"

#include "udWorkerPool.h"
#include "udStringUtil.h"
#include "udFile.h"

#include "stb_image.h"

struct AsyncTextureLoadInfo
{
  udResult loadResult;

  vcTexture **ppTexture;
  vcTextureFilterMode filterMode;
  bool hasMips;
  vcTextureWrapMode wrapMode;
  uint32_t limitTextureSize;

  // optional filename to load
  const char* pFilename;

  // optional raw data to create pixels from
  void* pMemory;
  int64_t memoryLen;

  // resulting unpacked pixel data
  const void* pPixels;
  vcTextureFormat format;
  uint32_t width;
  uint32_t height;
};

void vcTexture_AsyncLoadWorkerThreadWork(void* pTextureLoadInfo)
{
  udResult result;
  AsyncTextureLoadInfo* pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;

  if (pLoadInfo->pFilename)
    UD_ERROR_CHECK(udFile_Load(pLoadInfo->pFilename, &pLoadInfo->pMemory, &pLoadInfo->memoryLen));

  if (pLoadInfo->pMemory)
  {
    uint32_t width, height, channelCount;
    uint8_t* pPixels = stbi_load_from_memory((stbi_uc*)pLoadInfo->pMemory, (int)pLoadInfo->memoryLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
    UD_ERROR_NULL(pPixels, udR_InternalError);

    pLoadInfo->pPixels = udMemDup(pPixels, width * height * 4, 0, udAF_Zero);
    pLoadInfo->format = vcTextureFormat_RGBA8;
    pLoadInfo->width = width;
    pLoadInfo->height = height;

    stbi_image_free(pPixels);
  }

  if (pLoadInfo->width > pLoadInfo->limitTextureSize || pLoadInfo->height > pLoadInfo->limitTextureSize)
  {
    const void *pResizedPixels = nullptr;
    uint32_t resizedWidth = 0;
    uint32_t resizedHeight = 0;

    // On failure, just continue without resize
    if (vcTexture_ResizePixels(pLoadInfo->pPixels, pLoadInfo->width, pLoadInfo->height, pLoadInfo->limitTextureSize, &pResizedPixels, &resizedWidth, &resizedHeight) == udR_Success)
    {
      udFree(pLoadInfo->pPixels);

      pLoadInfo->pPixels = pResizedPixels;
      pLoadInfo->width = resizedWidth;
      pLoadInfo->height = resizedHeight;
      pLoadInfo->limitTextureSize = udMax(pLoadInfo->width, pLoadInfo->height); // adjust limit to match
    }
  }

  result = udR_Success;
epilogue:

  pLoadInfo->loadResult = result;
  udFree(pLoadInfo->pFilename);
  udFree(pLoadInfo->pMemory);
}

void vcTexture_AsyncLoadMainThreadWork(void* pTextureLoadInfo)
{
  udResult result;
  AsyncTextureLoadInfo* pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;
  UD_ERROR_CHECK(pLoadInfo->loadResult);
  
  UD_ERROR_CHECK(vcTexture_Create(pLoadInfo->ppTexture, pLoadInfo->width, pLoadInfo->height, pLoadInfo->pPixels, pLoadInfo->format, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode));

  result = udR_Success;
epilogue:

  if (result != udR_Success)
  {
    // texture failed to load, load a 1x1 white texture
    uint32_t whitePixel = 0xffffffff;
    vcTexture_Create(pLoadInfo->ppTexture, 1, 1, &whitePixel);
  }
  pLoadInfo->loadResult = result;
  udFree(pLoadInfo->pPixels);
}

udResult vcTexture_AsyncCreate(vcTexture** ppTexture, udWorkerPool* pPool, uint32_t width, uint32_t height, const void* pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || width == 0 || height == 0 || pPixels == nullptr || format == vcTextureFormat_Unknown || format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udResult result;
  int pixelBytes = 4; // assumptions

  AsyncTextureLoadInfo* pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

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
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pFilename = udStrdup(pFilename);
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncCreateFromMemory(vcTexture** ppTexture, udWorkerPool* pPool, void* pFileData, size_t fileLength, vcTextureFilterMode filterMode /*= vcTFM_Linear */, bool hasMipmaps /*= false */, vcTextureWrapMode wrapMode /*= vcTWM_Repeat */, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || pFileData == nullptr || fileLength == 0)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo* pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pMemory = udMemDup(pFileData, fileLength, 0, udAF_None);
  pLoadInfo->memoryLen = fileLength;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_ResizePixels(const void *pPixels, uint32_t width, uint32_t height, uint32_t targetSize, const void **ppResultPixels, uint32_t *pResultWidth, uint32_t *pResultHeight)
{
  // TODO: Add ability to upscale
  udResult result;

  const void *pLastPixels = pPixels;
  uint32_t lastWidth = width;
  uint32_t lastHeight = height;

  uint32_t totalPassCount = uint32_t(udLog2(float(udMax(width, height))) - udLog2(float(targetSize)));
  UD_ERROR_IF(totalPassCount <= 0, udR_Failure_);

  for (uint32_t p = 0; p < totalPassCount; ++p)
  {
    lastWidth >>= 1;
    lastHeight >>= 1;
    uint32_t *pDownsampledPixels = udAllocType(uint32_t, lastWidth * lastHeight, udAF_Zero);
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

    if (p != 0)
      udFree(pLastPixels);

    pLastPixels = pDownsampledPixels;
  }

  result = udR_Success;
  *ppResultPixels = pLastPixels;
  *pResultWidth = lastWidth;
  *pResultHeight = lastHeight;

epilogue:
  return result;
}
