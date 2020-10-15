#include "vcCSV.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udWorkerPool.h"
#include "udFile.h"

#define MAX_COLUMN_COUNT 16
#define CAPACITY_CHUNK_SIZE (1024 << 4) 

bool vcCSV_ShouldSkipLine(char *pToken)
{
  // Skip over comments etc
  return pToken[0] == 0 || pToken[0] == ' ' || udStrBeginsWith(pToken, "//") || udStrBeginsWith(pToken, "!");
}

void vcCSV_InferColumnTypes(vcCSV *pCSV)
{
  // read a single line to determine initial column types
  int pointIndex = 0;
  bool hasID = false;
  bool hasParentID = false;
  bool hasName = false;
  int curOffset = 0;
  pCSV->columns.Init(32);

  int columnCount = pCSV->elementCount / pCSV->entryCount;
  for (int i = 0; i < columnCount; ++i)
  {
    int charCount = 0;

    char *pText = &pCSV->data.pData[curOffset];
    int length = (int)udStrlen(pText); // null terminator
    curOffset += length + 1;  // null terminator

    // skip
    if (udStrlen(pText) == 0 || *pText == ' ' || *pText == '\r')
    {
      pCSV->columns.PushBack(vcCSVCT_Skip);
      continue;
    }

    // check for potential ID first
    udStrAtoi64(pText, &charCount);
    if (charCount > 0 && length == charCount)
    {
      if (!hasID)
      {
        hasID = true;
        pCSV->columns.PushBack(vcCSVCT_ID);
      }
      else if (!hasParentID)
      {
        hasParentID = true;
        pCSV->columns.PushBack(vcCSVCT_ParentID);
      }
      else
      {
        // Unknown type
        pCSV->columns.PushBack(vcCSVCT_Skip);
      }
      continue;
    }

    // check for double (positional)
    udStrAtof64(pText, &charCount);
    if (charCount > 0 && length == charCount)
    {
      if (pointIndex <= 2)
      {
        pCSV->columns.PushBack((vcCSV_ColumnType)(vcCSVCT_X + pointIndex));
        ++pointIndex;
      }
      else
      {
        // Unknown type
        pCSV->columns.PushBack(vcCSVCT_Skip);
      }
      continue;
    }

    if (!hasName)
    {
      hasName = true;
      pCSV->columns.PushBack(vcCSVCT_Name);
    }
    else
    {
      pCSV->columns.PushBack(vcCSVCT_Skip);
    }
  }
}

void vcCSV_AppendData(vcCSV *pCSV, const char *pData, int64_t dataLength)
{
  if (pCSV->data.usageBytes + dataLength >= pCSV->data.capacityBytes)
  {
    pCSV->data.capacityBytes += CAPACITY_CHUNK_SIZE;
    pCSV->data.pData = udReallocType(pCSV->data.pData, char, pCSV->data.capacityBytes);
  }
  memcpy(pCSV->data.pData + pCSV->data.usageBytes, pData, dataLength);
  pCSV->data.usageBytes += dataLength;
}

udResult vcCSV_Read(vcCSV *pCSV, uint64_t readAmountBytes /* = -1*/)
{
  if (pCSV->completeRead)
    return udR_Success;

  udResult result;
  int64_t fileLength = 0;
  udFile *pFile = nullptr;
  uint64_t totalBytesRead = 0;
  const int maxChunkSizeBytes = CAPACITY_CHUNK_SIZE;
  char chunkMemory[maxChunkSizeBytes + 1] = {}; // + 1 for nul terminator
  int64_t bytesRemaining = 0; // from previous chunk
  size_t readChunkLength = 0;
  const char *pDelimeter = nullptr;
  uint64_t resumeReadOffset = pCSV->totalBytesRead; // from previous vcCSV_Read()

  UD_ERROR_CHECK(udFile_Open(&pFile, pCSV->pFilename, udFOF_Read, &fileLength));

  // assumed its fixed spacing if delimeter null
  if (pCSV->importSettings.delimeter != '\0')
    udSprintf(&pDelimeter, "%c", pCSV->importSettings.delimeter);

  while (!pCSV->cancel && udFile_Read(pFile, chunkMemory + bytesRemaining, maxChunkSizeBytes - bytesRemaining, resumeReadOffset, udFSW_SeekCur, &readChunkLength) == udR_Success)
  {
    if (readChunkLength == 0)
      break; // done

    resumeReadOffset = 0;

    // factor in any remainder from last chunk
    int64_t actualChunkLength = readChunkLength + bytesRemaining;

    // avoid mangled read of last data in the chunk
    if (actualChunkLength < fileLength)
    {
      // nul terminate on the last newline, and factor any remaining data into the start of the next chunk
      size_t lastEntryNewline = 0;
      udStrrchr(chunkMemory, "\n", &lastEntryNewline);
      chunkMemory[lastEntryNewline] = '\0';

      bytesRemaining = actualChunkLength - (int64_t)(lastEntryNewline + 1); // skip the newline character
    }

    for (char *pLine = strtok(chunkMemory, "\r\n"); pLine; pLine = strtok(nullptr, "\r\n"))
    {
      if (vcCSV_ShouldSkipLine(pLine))
        continue;

      // skip entries if specified
      if (pCSV->importSettings.skipEntries > pCSV->skippedEntries)
      {
        ++pCSV->skippedEntries;
        continue;
      } 

      if (*pLine)
      {
        pCSV->entryCount++;

        if (pDelimeter != nullptr)
        {
          char *pTokenArray[MAX_COLUMN_COUNT] = {};
          int count = udStrTokenSplit(pLine, pDelimeter, pTokenArray, MAX_COLUMN_COUNT);

          pCSV->elementCount += count;
          for (int c = 0; c < count; ++c)
          {
            int64_t dataLen = (int64_t)udStrlen(pTokenArray[c]) + 1; // include null terminator
            vcCSV_AppendData(pCSV, pTokenArray[c], dataLen);
          }
        }
        else
        {
          // assumed its fixed spacing
          int count = (int)udStrlen(pLine) / pCSV->importSettings.fixedSizeDelimeterSpacing;
          pCSV->elementCount += count;
          for (int c = 0; c < count; ++c)
          {
            int dataLen = pCSV->importSettings.fixedSizeDelimeterSpacing;
            int offset = (c * pCSV->importSettings.fixedSizeDelimeterSpacing);
            vcCSV_AppendData(pCSV, pLine + offset, dataLen);
            vcCSV_AppendData(pCSV, "\0", 1); // append null terminator between data
          }
        }
      }
    }

    totalBytesRead += actualChunkLength - bytesRemaining;
    if (totalBytesRead >= readAmountBytes)
      break;

    if (bytesRemaining > 0)
    {
      // move
      size_t bytesRead = actualChunkLength - bytesRemaining;
      memmove(chunkMemory, chunkMemory + bytesRead, bytesRemaining);
      memset(chunkMemory + bytesRemaining, 0, actualChunkLength - bytesRemaining);
    }
  }

  pCSV->totalBytesRead += totalBytesRead;

  if (!pCSV->cancel)
    result = udR_Success;
  else
    result = udR_Cancelled;

  if (result == udR_Success && (int64_t)pCSV->totalBytesRead == fileLength)
    pCSV->completeRead = true;

epilogue:

  if (pCSV)
    pCSV->readResult = result;

  udFile_Close(&pFile);

  udFree(pDelimeter);
  return result;
}

udResult vcCSV_ReadHeader(vcCSV *pCSV)
{
  udResult result;

  while (!pCSV->completeRead)
  {
    const int64_t minimumHeaderReadSize = CAPACITY_CHUNK_SIZE; // 8kb
    UD_ERROR_CHECK(vcCSV_Read(pCSV, (size_t)minimumHeaderReadSize));

    // keep going until read enough
    if (pCSV->data.usageBytes >= minimumHeaderReadSize)
      break;
  }

  UD_ERROR_IF(pCSV->entryCount == 0, udR_InternalError);
  vcCSV_InferColumnTypes(pCSV);

  pCSV->headerRead = true;
  result = udR_Success;
epilogue:

  return result;
}

udResult vcCSV_Create(vcCSV **ppCSV, const char *pFilename, const vcCSVImportSettings &importSettings)
{
  udResult result;
  vcCSV *pCSV = nullptr;

  UD_ERROR_NULL(ppCSV, udR_InvalidParameter_);
  UD_ERROR_NULL(pFilename, udR_InvalidParameter_);

  pCSV = udAllocType(vcCSV, 1, udAF_Zero);
  UD_ERROR_NULL(pCSV, udR_MemoryAllocationFailure);

  pCSV->pFilename = udStrdup(pFilename);
  pCSV->importSettings = importSettings;

  pCSV->data.pData = udAllocType(char, CAPACITY_CHUNK_SIZE, udAF_Zero);
  pCSV->data.capacityBytes = CAPACITY_CHUNK_SIZE;

  result = udR_Success;
  *ppCSV = pCSV;
  pCSV = nullptr;
epilogue:

  if (pCSV != nullptr)
    vcCSV_Destroy(&pCSV);

  return result;
}

void vcCSV_Destroy(vcCSV **ppCSV)
{
  if (ppCSV == nullptr || *ppCSV == nullptr)
    return;

  vcCSV *pCSV = *ppCSV;
  *ppCSV = nullptr;

  udFree(pCSV->pFilename);
  udFree(pCSV->data.pData);
  pCSV->columns.Deinit();

  udFree(pCSV);
}
