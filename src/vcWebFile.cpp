#include "vcWebFile.h"

#include "vdkWeb.h"

#include "udPlatformUtil.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udStringUtil.h"

static udResult vcWebFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udResult result = udR_Success;
  vdkWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;

  options.method = vdkWM_GET;
  options.rangeBegin = (uint64_t)seekOffset;
  options.rangeEnd = (uint64_t)(seekOffset + bufferLength - 1);

  UD_ERROR_IF(vdkWeb_RequestAdv(pFile->pFilenameCopy, options, &pData, &dataLength, &responseCode) != vE_Success, udR_ReadFailure);
  UD_ERROR_IF(dataLength > bufferLength, udR_ReadFailure);

  memcpy(pBuffer, pData, dataLength);
  if (pActualRead)
    *pActualRead = dataLength;

epilogue:
  if (pData)
    vdkWeb_ReleaseResponse(&pData);
  return result;
}

static udResult vcWebFile_Close(udFile **ppFile)
{
  if (ppFile == nullptr)
    return udR_InvalidParameter_;

  udFree(*ppFile);

  return udR_Success;
}

udResult vcWebFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  UDTRACE();
  udFile *pFile = nullptr;
  udResult result;
  vdkWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;

  options.method = vdkWM_HEAD;

  UD_ERROR_IF(flags & udFOF_Write, udR_NotAllowed);
  UD_ERROR_IF(flags & udFOF_Create, udR_NotAllowed);

  pFile = udAllocType(udFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->fpRead = vcWebFile_SeekRead;
  pFile->fpClose = vcWebFile_Close;

  if (!(flags & udFOF_FastOpen))
  {
    UD_ERROR_IF(vdkWeb_RequestAdv(pFilename, options, &pData, &dataLength, &responseCode) != vE_Success, udR_OpenFailure);

    // TODO: (EVC-615) JIRA task to expand these
    if (responseCode == 403)
      UD_ERROR_SET(udR_NotAllowed);
    else if (responseCode == 503)
      UD_ERROR_SET(udR_Pending);
    else if (responseCode >= 500 && responseCode <= 599)
      UD_ERROR_SET(udR_ServerError);
    else if (responseCode < 200 || responseCode >= 300)
      UD_ERROR_SET(udR_OpenFailure);

    pFile->totalBytes = dataLength;
    pFile->fileLength = dataLength;
  }

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile != nullptr)
    vcWebFile_Close(&pFile);

  return result;
}

udResult vcWebFile_RegisterFileHandlers()
{
  udResult result = udR_Failure_;

  UD_ERROR_CHECK(udFile_RegisterHandler(vcWebFile_Open, "http://"));
  UD_ERROR_CHECK(udFile_RegisterHandler(vcWebFile_Open, "https://"));
  UD_ERROR_CHECK(udFile_RegisterHandler(vcWebFile_Open, "ftp://"));
  UD_ERROR_CHECK(udFile_RegisterHandler(vcWebFile_Open, "ftps://"));

  result = udR_Success;
epilogue:
  return result;
}

#if (UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR)
// See vcWebFile.mm for implementation
#elif UDPLATFORM_EMSCRIPTEN
void vcWebFile_OpenBrowser(const char *pWebpageAddress)
{
  // TODO: Open new tab with link
  udUnused(pWebpageAddress);
}
#else
void vcWebFile_OpenBrowser(const char *pWebpageAddress)
{
  const char *pBuffer = nullptr;

#if UDPLATFORM_WINDOWS
  udSprintf(&pBuffer, "START %s", pWebpageAddress);
#elif UDPLATFORM_OSX
  udSprintf(&pBuffer, "open %s", pWebpageAddress);
#elif UDPLATFORM_LINUX
  udSprintf(&pBuffer, "xdg-open %s", pWebpageAddress);
#endif

  if (pBuffer != nullptr)
  {
    //Have to do this because some compilers require you do something with the return value of system
    int retval = system(pBuffer);
    udUnused(retval);

    udFree(pBuffer);
  }
}
#endif

