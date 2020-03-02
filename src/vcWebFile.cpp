#include "vcWebFile.h"

#include "vdkWeb.h"

#include "udPlatformUtil.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udStringUtil.h"
#include "vcHTTP.h"

static udResult vcWebFile_Load(udFile *pFile, void **ppBuffer, int64_t *pBufferLength)
{
  UDTRACE();
  vdkWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;
  udResult result = udR_Success;
  vdkError err = vE_Success;

  options.method = vdkWM_GET;

  err = vdkWeb_RequestAdv(pFile->pFilenameCopy, options, &pData, &dataLength, &responseCode);
  result = vcHTTP_StatusToudResult((vcHTTPStatus)responseCode);
  if (result == udR_Failure_)
    result = udR_ReadFailure;

  if (err != vE_Success || result != udR_Success)
    UD_ERROR_HANDLE();

  *ppBuffer = udMemDup(pData, dataLength, 0, udAF_None);
  UD_ERROR_NULL(*ppBuffer, udR_MemoryAllocationFailure);

  if (pBufferLength != nullptr)
    *pBufferLength = (int64_t)dataLength;

epilogue:
  if (pData)
    vdkWeb_ReleaseResponse(&pData);
  return result;
}

static udResult vcWebFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  vdkWebOptions options = {};
  const char *pData = nullptr;
  const char *pDataOffset = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;
  udResult result = udR_Success;
  vdkError err = vE_Success;

  options.method = vdkWM_GET;
  options.rangeBegin = (uint64_t)seekOffset;
  options.rangeEnd = (uint64_t)(seekOffset + bufferLength - 1);

  err = vdkWeb_RequestAdv(pFile->pFilenameCopy, options, &pData, &dataLength, &responseCode);
  result = vcHTTP_StatusToudResult((vcHTTPStatus)responseCode);
  if (result == udR_Failure_)
    result = udR_ReadFailure;

  if (err != vE_Success || result != udR_Success)
    UD_ERROR_HANDLE();

  pDataOffset = pData;

  // If the range was specified and the server responded with a 200 then this handles getting the correct part of the buffer
  // This is acceptable according to the standard: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
  if (pData != nullptr && responseCode == 200 && seekOffset != 0)
  {
    pDataOffset += seekOffset;
    dataLength = udMin<int64_t>(dataLength - seekOffset, bufferLength - 1);
  }

  UD_ERROR_IF(dataLength > bufferLength, udR_ReadFailure);
  memcpy(pBuffer, pDataOffset, dataLength);
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
  vdkWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;
  udResult result = udR_Success;
  vdkError err = vE_Success;

  options.method = vdkWM_HEAD;

  UD_ERROR_IF(flags & udFOF_Write, udR_NotAllowed);
  UD_ERROR_IF(flags & udFOF_Create, udR_NotAllowed);

  pFile = udAllocType(udFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->fpLoad = vcWebFile_Load;
  pFile->fpRead = vcWebFile_SeekRead;
  pFile->fpClose = vcWebFile_Close;

  err = vdkWeb_RequestAdv(pFilename, options, &pData, &dataLength, &responseCode);
  result = vcHTTP_StatusToudResult((vcHTTPStatus)responseCode);
  if (result == udR_Failure_)
    result = udR_OpenFailure;

  if (err != vE_Success || result != udR_Success)
    UD_ERROR_HANDLE();

  pFile->totalBytes = dataLength;
  pFile->fileLength = dataLength;

  *ppFile = pFile;
  pFile = nullptr;

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

