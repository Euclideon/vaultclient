//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, April 2014
//

#define _FILE_OFFSET_BITS 64
#if defined(_MSC_VER)
# define _CRT_SECURE_NO_WARNINGS
# define fseeko _fseeki64
# define ftello _ftelli64
# if !defined(_OFF_T_DEFINED)
    typedef __int64 _off_t;
    typedef _off_t off_t;
#   define _OFF_T_DEFINED
# endif //_OFF_T_DEFINED
#elif defined(__linux__)
# if !defined(_LARGEFILE_SOURCE )
  // This must be set for linux to expose fseeko and ftello
# define _LARGEFILE_SOURCE
#endif

#endif


#include "udFile.h"
#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <sys/stat.h>

#if UDPLATFORM_NACL
# define fseeko fseek
# define ftello ftell
#endif

#define FILE_DEBUG 0

// Declarations of the fall-back standard handler that uses crt FILE as a back-end
static udFile_SeekReadHandlerFunc   udFileHandler_FILESeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_FILESeekWrite;
static udFile_ReleaseHandlerFunc    udFileHandler_FILERelease;
static udFile_CloseHandlerFunc      udFileHandler_FILEClose;
volatile int32_t g_udFileHandler_FILEHandleCount;
#if FILE_DEBUG
#pragma optimize("", off)
#endif

// The udFile derivative for supporting standard runtime library FILE i/o
struct udFile_FILE : public udFile
{
  FILE *pCrtFile;
  udMutex *pMutex;                        // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads
};


// ----------------------------------------------------------------------------
static FILE *OpenWithFlags(const char *pFilename, udFileOpenFlags flags)
{
  const char *pMode = "";
  FILE *pFile = nullptr;

  if ((flags & udFOF_Read) && (flags & udFOF_Write) && (flags & udFOF_Create))
    pMode = "w+b";  // Read/write, any existing file destroyed
  else if ((flags & udFOF_Read) && (flags & udFOF_Write))
    pMode = "r+b"; // Read/write, but file must already exist
  else if (flags & udFOF_Read)
    pMode = "rb"; // Read, file must already exist
  else if ((flags & udFOF_Write) || (flags & udFOF_Create))
    pMode = "wb"; // Write, any existing file destroyed (Create flag treated as Write in this case)
  else
    return nullptr;

#if UDPLATFORM_WINDOWS
  pFile = _wfopen(udOSString(pFilename), udOSString(pMode));
#else
  pFile = fopen(pFilename, pMode);
#endif

  if (pFile)
    udInterlockedPreIncrement(&g_udFileHandler_FILEHandleCount);
#if FILE_DEBUG
  if (pFile)
    udDebugPrintf("Opening %s (%s) handleCount=%d\n", pFilename, pMode, g_udFileHandler_FILEHandleCount);
  else
    udDebugPrintf("Error opening %s (%s) handleCount=%d\n", pFilename, pMode, g_udFileHandler_FILEHandleCount);
#endif

  return pFile;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt FILE i/o functions
udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  UDTRACE();
  udFile_FILE *pFile = nullptr;
  udResult result;

  pFile = udAllocType(udFile_FILE, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  result = udFileExists(pFilename, &pFile->fileLength);
  if (result != udR_Success)
    pFile->fileLength = 0;

  pFile->fpRead = udFileHandler_FILESeekRead;
  pFile->fpWrite = udFileHandler_FILESeekWrite;
  pFile->fpRelease = udFileHandler_FILERelease;
  pFile->fpClose = udFileHandler_FILEClose;

  if (!(flags & udFOF_FastOpen)) // With FastOpen flag, just don't open the file, let the first read do that
  {
    pFile->pCrtFile = OpenWithFlags(pFilename, flags);
    // File open failures shouldn't trigger breakpoints with BREAK_ON_ERROR defined.
    if (!pFile->pCrtFile)
      UD_ERROR_SET_NO_BREAK(udR_OpenFailure);
  }

  if (flags & udFOF_Multithread)
  {
    pFile->pMutex = udCreateMutex();
    UD_ERROR_NULL(pFile->pMutex, udR_InternalError);
  }

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
  {
    if (pFile->pCrtFile)
    {
      fclose(pFile->pCrtFile);
      udInterlockedPreDecrement(&g_udFileHandler_FILEHandleCount);
    }
    udFree(pFile);
  }
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of SeekReadHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILESeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);
  udResult result;
  size_t actualRead;

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);

  if (pFILE->pCrtFile == nullptr)
  {
#if FILE_DEBUG
    udDebugPrintf("Reopening handle for %s (handleCount=%d)\n", pFile->pFilenameCopy, g_udFileHandler_FILEHandleCount);
#endif
    pFILE->pCrtFile = OpenWithFlags(pFile->pFilenameCopy, pFile->flagsCopy);
    UD_ERROR_NULL(pFILE->pCrtFile, udR_OpenFailure);
  }

  fseeko(pFILE->pCrtFile, seekOffset, SEEK_SET);
  actualRead = bufferLength ? fread(pBuffer, 1, bufferLength, pFILE->pCrtFile) : 0;
  if (pActualRead)
    *pActualRead = actualRead;
  UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_ReadFailure);

  result = udR_Success;

epilogue:
  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of SeekWriteHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILESeekWrite(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualWritten)
{
  UDTRACE();
  udResult result;
  size_t actualWritten;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);

  UD_ERROR_NULL(pFILE->pCrtFile, udR_OpenFailure);

  fseeko(pFILE->pCrtFile, seekOffset, SEEK_SET);
  actualWritten = fwrite(pBuffer, 1, bufferLength, pFILE->pCrtFile);
  if (pActualWritten)
    *pActualWritten = actualWritten;
  UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_WriteFailure);

  result = udR_Success;

epilogue:
  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2016
// Implementation of Release to release the underlying file handle
static udResult udFileHandler_FILERelease(udFile *pFile)
{
  udResult result;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);

  // Early-exit that doesn't involve locking the mutex
  if (!pFILE->pCrtFile)
    return udR_NothingToDo;

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);

  // Another check after the mutex lock to catch the case where another thread released while this thread waited on the mutex
  UD_ERROR_IF(!pFILE->pCrtFile, udR_NothingToDo);

  // Don't support release/reopen on files for create/writing
  UD_ERROR_IF(!pFile->pFilenameCopy || (pFile->flagsCopy & (udFOF_Create|udFOF_Write)), udR_InvalidConfiguration);

#if FILE_DEBUG
  udDebugPrintf("Releasing handle for %s (handleCount=%d) pCrtFile=%p\n", pFile->pFilenameCopy, g_udFileHandler_FILEHandleCount, pFILE->pCrtFile);
#endif
  fclose(pFILE->pCrtFile);
  pFILE->pCrtFile = nullptr;
  udInterlockedPreDecrement(&g_udFileHandler_FILEHandleCount);

  result = udR_Success;

epilogue:

  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of CloseHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILEClose(udFile **ppFile)
{
  UDTRACE();
  udResult result = udR_Success;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(*ppFile);
  *ppFile = nullptr;

  if (pFILE->pCrtFile)
  {
    result = (fclose(pFILE->pCrtFile) != 0) ? udR_CloseFailure : udR_Success;
    pFILE->pCrtFile = nullptr;
    udInterlockedPreDecrement(&g_udFileHandler_FILEHandleCount);
  }

  if (pFILE->pMutex)
    udDestroyMutex(&pFILE->pMutex);
  udFree(pFILE);

  return result;
}


