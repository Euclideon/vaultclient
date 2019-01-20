#include "vcWebFile.h"

#include "vdkWeb.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udFileHandler.h"

struct vcWebFile : public udFile
{
  const char *pData;
  uint32_t dataLength;
};

static udResult vcWebFile_SeekRead(udFile *pIntFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  vcWebFile *pFile = static_cast<vcWebFile*>(pIntFile);
  udResult result = udR_Failure_;
  uint32_t actualRead = 0;

  UD_ERROR_IF(pFile->dataLength <= seekOffset, udR_ReadFailure);

  actualRead = udMin((uint32_t)bufferLength, (uint32_t)(pFile->dataLength - seekOffset));

  if (pActualRead)
    *pActualRead = actualRead;

  memcpy(pBuffer, &pFile->pData[seekOffset], actualRead);

  result = udR_Success;

epilogue:
  return result;
}

static udResult vcWebFile_Close(udFile **ppFile)
{
  vcWebFile *pFile = static_cast<vcWebFile*>(*ppFile);

  if (pFile->pData)
    vdkWeb_ReleaseResponse(&pFile->pData);

  udFree(pFile);

  return udR_Success;
}

udResult vcWebFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  UDTRACE();
  vcWebFile *pFile = nullptr;
  udResult result;
  int responseCode = 0;

  UD_ERROR_IF(flags & udFOF_Write, udR_NotAllowed);
  UD_ERROR_IF(flags & udFOF_Create, udR_NotAllowed);

  pFile = udAllocType(vcWebFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->fpRead = vcWebFile_SeekRead;
  pFile->fpClose = vcWebFile_Close;

  UD_ERROR_IF(vdkWeb_GetRequest(pFilename, &pFile->pData, &pFile->dataLength, &responseCode) != vE_Success, udR_OpenFailure);
  UD_ERROR_IF(responseCode != 200, udR_OpenFailure);

  pFile->totalBytes = pFile->dataLength;
  pFile->fileLength = pFile->dataLength;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile != nullptr)
    vcWebFile_Close((udFile**)&pFile);

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
