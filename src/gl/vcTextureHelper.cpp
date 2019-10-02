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

  vcTexture_CreateFromMemory(pLoadInfo->ppTexture, pLoadInfo->pData, pLoadInfo->dataLen, nullptr, nullptr, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode);

  udFree(pLoadInfo->pFilename);
  udFree(pLoadInfo->pData);
}

udResult vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/)
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

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;

epilogue:
  return result;
}
