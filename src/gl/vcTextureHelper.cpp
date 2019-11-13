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

  // TODO: texture downsize goes here

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

udResult vcTexture_AsyncCreate(vcTexture** ppTexture, udWorkerPool* pPool, uint32_t width, uint32_t height, const void* pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/)
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

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/)
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

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncCreateFromMemory(vcTexture** ppTexture, udWorkerPool* pPool, void* pFileData, size_t fileLength, vcTextureFilterMode filterMode /*= vcTFM_Linear */, bool hasMipmaps /*= false */, vcTextureWrapMode wrapMode /*= vcTWM_Repeat */)
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

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}
