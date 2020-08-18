#include "vcWebFile.h"

#include "udWeb.h"

#include "udPlatformUtil.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udStringUtil.h"

#if UDPLATFORM_WINDOWS
# include <shellapi.h>
#endif

static udResult vcWebFile_Load(udFile *pFile, void **ppBuffer, int64_t *pBufferLength)
{
  UDTRACE();
  udResult result = udR_Success;
  udWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;

  options.method = udWM_GET;

  UD_ERROR_IF(udWeb_RequestAdv(pFile->pFilenameCopy, options, &pData, &dataLength, &responseCode) != udE_Success, udR_ReadFailure);

  *ppBuffer = udMemDup(pData, dataLength, 1, udAF_None);
  UD_ERROR_NULL(*ppBuffer, udR_MemoryAllocationFailure);
  ((char *)*ppBuffer)[dataLength] = 0;

  if (pBufferLength != nullptr)
    *pBufferLength = (int64_t)dataLength;

epilogue:
  if (pData)
    udWeb_ReleaseResponse(&pData);
  return result;
}

static udResult vcWebFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udResult result = udR_Success;
  udWebOptions options = {};
  const char *pData = nullptr;
  const char *pDataOffset = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;

  options.method = udWM_GET;
  options.rangeBegin = (uint64_t)seekOffset;
  options.rangeEnd = (uint64_t)(seekOffset + bufferLength - 1);

  if ((int64_t)options.rangeEnd >= pFile->fileLength && pFile->fileLength > 0)
    options.rangeEnd = (uint64_t)(pFile->fileLength - 1);

  UD_ERROR_IF(udWeb_RequestAdv(pFile->pFilenameCopy, options, &pData, &dataLength, &responseCode) != udE_Success, udR_ReadFailure);
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
    udWeb_ReleaseResponse(&pData);
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
  udWebOptions options = {};
  const char *pData = nullptr;
  uint64_t dataLength = 0;
  int responseCode = 0;

  options.method = udWM_HEAD;

  UD_ERROR_IF(flags & udFOF_Write, udR_NotAllowed);
  UD_ERROR_IF(flags & udFOF_Create, udR_NotAllowed);

  pFile = udAllocType(udFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->fpLoad = vcWebFile_Load;
  pFile->fpRead = vcWebFile_SeekRead;
  pFile->fpClose = vcWebFile_Close;

  if ((flags & udFOF_FastOpen) == 0)
  {
    UD_ERROR_IF(udWeb_RequestAdv(pFilename, options, &pData, &dataLength, &responseCode) != udE_Success, udR_OpenFailure);

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
  else
  {
    pFile->totalBytes = 0;
    pFile->fileLength = 0;
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

epilogue:
  return result;
}

#if (UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR)
// See vcWebFile.mm for implementation
#else
void vcWebFile_OpenBrowser(const char *pWebpageAddress)
{
#if UDPLATFORM_WINDOWS
  ShellExecute(NULL, L"open", udOSString(pWebpageAddress), NULL, NULL, SW_SHOWNORMAL);
#elif UDPLATFORM_EMSCRIPTEN
  MAIN_THREAD_EM_ASM(window.open(UTF8ToString($0)), pWebpageAddress);
#else
  const char *pBuffer = nullptr;

#if UDPLATFORM_OSX
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
#endif
}
#endif

