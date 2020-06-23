#include "vcShader.h"

#include "udWorkerPool.h"
#include "udStringUtil.h"
#include "udFile.h"

struct AsyncShaderLoadInfo
{
  udResult loadResult;
  vcShader **ppShader;

  const char *pVertexFilename;
  const char *pFragmentFilename;
  const vcVertexLayoutTypes *pInputTypes;
  uint32_t totalInputs;
  udWorkerPoolCallback pOnLoadedCallback;

  const char *pVertexShaderText;
  const char *pFragmentShaderText;
};

void vcShader_AsyncLoadWorkerThreadWork(void *pShaderLoadInfo)
{
  udResult result;
  AsyncShaderLoadInfo *pLoadInfo = (AsyncShaderLoadInfo*)pShaderLoadInfo;

  UD_ERROR_CHECK(udFile_Load(udTempStr("%s%s", pLoadInfo->pVertexFilename, pVertexFileExtension), &pLoadInfo->pVertexShaderText));
  UD_ERROR_CHECK(udFile_Load(udTempStr("%s%s", pLoadInfo->pFragmentFilename, pFragmentFileExtension), &pLoadInfo->pFragmentShaderText));

  result = udR_Success;
epilogue:

  pLoadInfo->loadResult = result;
  udFree(pLoadInfo->pVertexFilename);
  udFree(pLoadInfo->pFragmentFilename);
}

void vcShader_AsyncLoadMainThreadWork(void *pShaderLoadInfo)
{
  udResult result;
  AsyncShaderLoadInfo *pLoadInfo = (AsyncShaderLoadInfo*)pShaderLoadInfo;
  UD_ERROR_CHECK(pLoadInfo->loadResult);

  UD_ERROR_IF(!vcShader_CreateFromText(pLoadInfo->ppShader, pLoadInfo->pVertexShaderText, pLoadInfo->pFragmentShaderText, pLoadInfo->pInputTypes, pLoadInfo->totalInputs), udR_InternalError);

  if (pLoadInfo->pOnLoadedCallback != nullptr)
    pLoadInfo->pOnLoadedCallback(pLoadInfo->ppShader);

  result = udR_Success;
epilogue:

  udFree(pLoadInfo->pVertexShaderText);
  udFree(pLoadInfo->pFragmentShaderText);
  pLoadInfo->loadResult = result;
}

bool vcShader_CreateFromFileAsync(vcShader **ppShader, udWorkerPool *pWorkerPool, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs, udWorkerPoolCallback pOnLoadedCallback)
{
  if (ppShader == nullptr || pWorkerPool == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr || pInputTypes == nullptr || totalInputs == 0)
    return false;

  udResult result;

  AsyncShaderLoadInfo *pLoadInfo = udAllocType(AsyncShaderLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppShader = ppShader;
  pLoadInfo->pVertexFilename = udStrdup(pVertexShader);
  pLoadInfo->pFragmentFilename = udStrdup(pFragmentShader);
  pLoadInfo->pInputTypes = pInputTypes;
  pLoadInfo->totalInputs = totalInputs;
  pLoadInfo->pOnLoadedCallback = pOnLoadedCallback;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pWorkerPool, vcShader_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcShader_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppShader = nullptr;
epilogue:

  return result == udR_Success;
}

bool vcShader_CreateFromFile(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs)
{
  const char *pVertexShaderText = nullptr;
  const char *pFragmentShaderText = nullptr;

  udFile_Load(udTempStr("%s%s", pVertexShader, pVertexFileExtension), &pVertexShaderText);
  udFile_Load(udTempStr("%s%s", pFragmentShader, pFragmentFileExtension), &pFragmentShaderText);

  bool success = vcShader_CreateFromText(ppShader, pVertexShaderText, pFragmentShaderText, pInputTypes, totalInputs);

  udFree(pFragmentShaderText);
  udFree(pVertexShaderText);

  return success;
}
