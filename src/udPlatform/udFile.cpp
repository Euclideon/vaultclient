//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
//

#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include "udMath.h"

#if UDPLATFORM_WINDOWS
# include <ShlObj.h>
#else
# include <pwd.h>
#endif

#define MAX_HANDLERS 16
#define CONTENT_LOAD_CHUNK_SIZE 65536 // When loading an entire file of unknown size, read in chunks of this many bytes

udFile_OpenHandlerFunc udFileHandler_FILEOpen;     // Default crt FILE based handler

struct udFileHandler
{
  udFile_OpenHandlerFunc *fpOpen;
  char prefix[16];              // The prefix that this handler will respond to, eg 'http:', or an empty string for regular filenames
};

static udFileHandler s_handlers[MAX_HANDLERS] =
{
  { udFileHandler_FILEOpen, "" }    // Default file handler
};
static int s_handlersCount = 1;

// ****************************************************************************
// Author: Dave Pevreal, Ocober 2014
udResult udFile_Load(const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  udResult result;
  udFile *pFile = nullptr;
  char *pMemory = nullptr;
  int64_t length = 0;
  size_t actualRead;

  UD_ERROR_NULL(pFilename, udR_InvalidParameter_);
  UD_ERROR_NULL(ppMemory, udR_InvalidParameter_);
  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Read, &length)); // NOTE: Length can be zero. Chrome does this on cached files.

  if (length)
  {
    pMemory = (char*)udAlloc((size_t)length + 1); // Note always allocating 1 extra byte
    UD_ERROR_CHECK(udFile_Read(pFile, pMemory, (size_t)length, 0, udFSW_SeekCur, &actualRead));
    UD_ERROR_IF(actualRead != (size_t)length, udR_ReadFailure);
  }
  else
  {
    udDebugPrintf("udFile_Load: %s open succeeded, length unknown\n", pFilename);
    size_t alreadyRead = 0, attemptRead = 0;
    length = CONTENT_LOAD_CHUNK_SIZE;
    for (actualRead = 0; attemptRead == actualRead; alreadyRead += actualRead)
    {
      if (alreadyRead > (size_t)length)
        length += CONTENT_LOAD_CHUNK_SIZE;
      void *pNewMem = udRealloc(pMemory, (size_t)length + 1); // Note always allocating 1 extra byte
      UD_ERROR_NULL(pNewMem, udR_MemoryAllocationFailure);
      pMemory = (char*)pNewMem;

      attemptRead = (size_t)length + 1 - alreadyRead; // Note attempt to read 1 extra byte so EOF is detected
      UD_ERROR_CHECK(udFile_Read(pFile, pMemory + alreadyRead, attemptRead, 0, udFSW_SeekCur, &actualRead));
    }
    UDASSERT((size_t)length >= alreadyRead, "Logic error in read loop");
    if ((size_t)length != alreadyRead)
    {
      length = alreadyRead;
      void *pNewMem = udRealloc(pMemory, (size_t)length + 1);
      UD_ERROR_NULL(pNewMem, udR_MemoryAllocationFailure);
      pMemory = (char*)pNewMem;
    }
  }
  pMemory[length] = 0; // A nul-terminator for text files

  if (pFileLengthInBytes) // Pass length back if requested
    *pFileLengthInBytes = length;

  // Success, pass the memory back to the caller
  *ppMemory = pMemory;
  pMemory = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFile_Close(&pFile);
  udFree(pMemory);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
udResult udFile_Save(const char *pFilename, void *pBuffer, size_t length)
{
  udResult result;
  udFile *pFile = nullptr;

  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Create|udFOF_Write));
  UD_ERROR_CHECK(udFile_Write(pFile, pBuffer, (size_t)length));

epilogue:
  udFile_Close(&pFile);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  udResult result;
  const char *pNewFilename = nullptr;
  UD_ERROR_NULL(ppFile, udR_InvalidParameter_);
  UD_ERROR_NULL(pFilename, udR_InvalidParameter_);

  *ppFile = nullptr;
  if (pFileLengthInBytes)
    *pFileLengthInBytes = 0;

  // TODO: Figure out how to only do this for the FILE handler requires that
  //       pFilenameCopy isn't set to `udStrdup(pFilename)`
#if !UDPLATFORM_EMSCRIPTEN
  if (udFile_TranslatePath(&pNewFilename, pFilename) != udR_Success)
#endif
  {
    pNewFilename = udStrdup(pFilename);
    UD_ERROR_NULL(pNewFilename, udR_MemoryAllocationFailure);
  }

  for (int i = s_handlersCount - 1; i >= 0; --i)
  {
    udFileHandler *pHandler = s_handlers + i;
    if (udStrBeginsWith(pNewFilename, pHandler->prefix))
    {
      result = pHandler->fpOpen(ppFile, pNewFilename, flags);
      if (result == udR_Success)
      {
        udFree((*ppFile)->pFilenameCopy); // In rare circumstances this can already be assigned, so free to prevent a leak
        (*ppFile)->pFilenameCopy = pNewFilename;
        pNewFilename = nullptr;
        (*ppFile)->flagsCopy = flags;
        if (pFileLengthInBytes)
          *pFileLengthInBytes = (*ppFile)->fileLength;
        // Successfully opened
        UD_ERROR_SET(udR_Success);
      }
    }
  }
  // Getting here indicates no handler succeeded
  result = udR_OpenFailure;

epilogue:
  udFree(pNewFilename);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, July 2016
void udFile_SetSeekBase(udFile *pFile, int64_t seekBase, int64_t newLength)
{
  if (pFile)
  {
    pFile->seekBase = seekBase;
    if (newLength)
      pFile->fileLength = newLength;
    pFile->filePos = seekBase;  // Move the current position to the base in case a udFSW_SeekCur read is issued
  }
}

// ****************************************************************************
// Author: Dave Pevreal, November 2014
const char *udFile_GetFilename(udFile *pFile)
{
  if (pFile)
    return pFile->pFilenameCopy;
  else
    return nullptr;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_GetPerformance(udFile *pFile, udFilePerformance *pPerformance)
{
  UDTRACE();
  if (!pFile || !pPerformance)
    return udR_InvalidParameter_;

  pPerformance->throughput = pFile->totalBytes;
  pPerformance->mbPerSec = pFile->mbPerSec;
  pPerformance->requestsInFlight = pFile->requestsInFlight;

  return udR_Success;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
static void udUpdateFilePerformance(udFile *pFile, size_t actualRead)
{
  UDTRACE();
  pFile->msAccumulator += udGetTimeMs();
  pFile->totalBytes += actualRead;
  if (--pFile->requestsInFlight == 0)
    pFile->mbPerSec = float((pFile->totalBytes/1048576.0) / (pFile->msAccumulator / 1000.0));
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Read(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead, int64_t *pFilePos, udFilePipelinedRequest *pPipelinedRequest)
{
  UDTRACE();
  udResult result;
  size_t actualRead = 0;
  int64_t offset;
  void *pCipherText = nullptr;

  UD_ERROR_NULL(pFile, udR_InvalidParameter_);
  UD_ERROR_NULL(pFile->fpRead, udR_InvalidConfiguration);

  switch (seekWhence)
  {
    case udFSW_SeekSet: offset = seekOffset + pFile->seekBase; break;
    case udFSW_SeekCur: offset = pFile->filePos + seekOffset; break;
    case udFSW_SeekEnd: offset = pFile->fileLength + seekOffset + pFile->seekBase; break;
    default:
      UD_ERROR_SET(udR_InvalidParameter_);
  }

  ++pFile->requestsInFlight;
  pFile->msAccumulator -= udGetTimeMs();
    result = pFile->fpRead(pFile, pBuffer, bufferLength, offset, &actualRead, pFile->fpBlockPipedRequest ? pPipelinedRequest : nullptr);
  pFile->filePos = offset + actualRead;

  // Save off the actualRead in the request for the case where the handler doesn't support piped requests
  if (pPipelinedRequest && (!pFile->fpBlockPipedRequest))
  {
    pPipelinedRequest->reserved[0] = (uint64_t)actualRead;
    pPipelinedRequest = nullptr;
  }

  // Update the performance stats unless it's a supported pipelined request (in which case the stats are updated in the block function)
  if (!pPipelinedRequest || !pFile->fpBlockPipedRequest)
    udUpdateFilePerformance(pFile, actualRead);

  if (pActualRead)
    *pActualRead = actualRead;
  if (pFilePos)
    *pFilePos = pFile->filePos - pFile->seekBase;

  // If the caller isn't checking the actual read (ie it's null), and it's not the requested amount, return an error when full amount isn't actually read
  if (result == udR_Success && pActualRead == nullptr && actualRead != bufferLength)
    result = udR_ReadFailure;

epilogue:
  if (pCipherText != nullptr && pCipherText != pBuffer)
    udFree(pCipherText);
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Write(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten, int64_t *pFilePos)
{
  UDTRACE();
  udResult result;
  size_t actualWritten = 0; // Assign to zero to avoid incorrect compiler warning;
  int64_t offset;

  UD_ERROR_NULL(pFile, udR_InvalidParameter_);
  UD_ERROR_NULL(pFile->fpRead, udR_InvalidConfiguration);

  switch (seekWhence)
  {
  case udFSW_SeekSet: offset = seekOffset + pFile->seekBase; break;
  case udFSW_SeekCur: offset = pFile->filePos + seekOffset; break;
  case udFSW_SeekEnd: offset = pFile->fileLength + seekOffset; break;
  default:
    UD_ERROR_SET(udR_InvalidParameter_);
  }

  ++pFile->requestsInFlight;
  pFile->msAccumulator -= udGetTimeMs();
  result = pFile->fpWrite(pFile, pBuffer, bufferLength, offset, &actualWritten);
  pFile->filePos = offset + actualWritten;

  // Update the performance stats unless it's a supported pipelined request (in which case the stats are updated in the block function)
  udUpdateFilePerformance(pFile, actualWritten);

  if (pActualWritten)
    *pActualWritten = actualWritten;
  if (pFilePos)
    *pFilePos = pFile->filePos - pFile->seekBase;

  // If the caller isn't checking the actual written (ie it's null), and it's not the requested amount, return an error when full amount isn't actually written
  if (result == udR_Success && pActualWritten == nullptr && actualWritten != bufferLength)
    result = udR_WriteFailure;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_BlockForPipelinedRequest(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead)
{
  UDTRACE();
  udResult result;

  if (pFile->fpBlockPipedRequest)
  {
    size_t actualRead;
    result = pFile->fpBlockPipedRequest(pFile, pPipelinedRequest, &actualRead);
    udUpdateFilePerformance(pFile, actualRead);
    if (pActualRead)
      *pActualRead = actualRead;
  }
  else
  {
    if (pActualRead)
      *pActualRead = (size_t)pPipelinedRequest->reserved[0];
    result = udR_Success;
  }

  return result;
}

udResult udFile_Release(udFile *pFile)
{
  udResult result;
  UD_ERROR_NULL(pFile, udR_InvalidParameter_);

  result = (pFile->fpRelease) ? pFile->fpRelease(pFile) : udR_Success;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Close(udFile **ppFile)
{
  UDTRACE();
  if (ppFile == nullptr)
    return udR_InvalidParameter_;

  if (*ppFile != nullptr && (*ppFile)->fpClose != nullptr)
  {
    udFree((*ppFile)->pFilenameCopy);
    return (*ppFile)->fpClose(ppFile);
  }
  else
  {
    return udR_CloseFailure;
  }
}


// ****************************************************************************
// Author: Samuel Surtees, July 2018
udResult udFile_TranslatePath(const char **ppNewPath, const char *pPath)
{
  udResult result = udR_ObjectNotFound;
  UD_ERROR_NULL(ppNewPath, udR_InvalidParameter_);
  UD_ERROR_NULL(pPath, udR_InvalidParameter_);

  // TODO: Process environment variables when passed in via `%env%` and `$env`
  {
#if UDPLATFORM_WINDOWS
    PWSTR pHomeDirW = nullptr;
    UD_ERROR_IF(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &pHomeDirW) != S_OK, udR_ObjectNotFound);
    udOSString temp(pHomeDirW);
    const char *pHomeDir = temp;

    if (pHomeDirW)
      CoTaskMemFree(pHomeDirW);
#else
    struct passwd *pPw = getpwuid(getuid());
    UD_ERROR_NULL(pPw, udR_ObjectNotFound);
    const char *pHomeDir = pPw->pw_dir;
#endif
    size_t homeDirLength = udStrlen(pHomeDir);

    if (pPath[0] == '~' && (pPath[1] == '\0' || pPath[1] == '\\' || pPath[1] == '/'))
    {
      size_t filenameLength = udStrlen(pPath);
      size_t newSize = homeDirLength + (filenameLength - 1) + 1;
      char *pTemp = udAllocType(char, newSize, udAF_None);
      UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

      udStrcpy(pTemp, newSize, pHomeDir);
      udStrcat(pTemp, newSize, pPath + 1);

      result = udR_Success;
      *ppNewPath = pTemp;
    }
  }

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc *fpHandler, const char *pPrefix)
{
  UDTRACE();
  if (s_handlersCount >= MAX_HANDLERS)
    return udR_CountExceeded;
  s_handlers[s_handlersCount].fpOpen = fpHandler;
  udStrcpy(s_handlers[s_handlersCount].prefix, sizeof(s_handlers[s_handlersCount].prefix), pPrefix);
  ++s_handlersCount;
  return udR_Success;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_DeregisterHandler(udFile_OpenHandlerFunc *fpHandler)
{
  UDTRACE();
  for (int handlerIndex = 0; handlerIndex < s_handlersCount; ++handlerIndex)
  {
    if (s_handlers[handlerIndex].fpOpen == fpHandler)
    {
      if (++handlerIndex < s_handlersCount)
        memcpy(s_handlers + handlerIndex - 1, s_handlers + handlerIndex, (s_handlersCount - handlerIndex) * sizeof(s_handlers[0]));
      --s_handlersCount;
      return udR_Success;
    }
  }

  return udR_ObjectNotFound;
}

