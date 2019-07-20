#include "udResult.h"
#include "udStringUtil.h"

#include "vcDBase.h"
#include "vcModals.h"

#include <time.h>

enum vcDBF_FieldTypes
{
  vcDBF_Character = 'C',
  vcDBF_Date = 'D',
  vcDBF_Float = 'F',
  vcDBF_Numeric = 'N',
  vcDBF_Logical = 'L',
  vcDBF_DateTime = 'T',
  vcDBF_Integer = 'I',
  vcDBF_Currency = 'Y',
  vcDBF_Memo = 'M',
  vcDBF_Count = 9
};

struct vcDBF_Header
{
  int8_t flags;
  int8_t YMD[3];
  int32_t records;
  int16_t headerBytes;
  int16_t recordBytes;
  //int8_t reserved[20];
};

struct vcDBF_MemoFile
{
  uint32_t firstAvailableIndex;
  uint16_t blockSize;
  int64_t fileLength;
  char *pData;

  char *pLastRead;
};

struct vcDBF_MemoBlock
{
  uint32_t type;
  uint32_t length;
  char *pData;
};

struct vcDBF_Reader
{
  udFile *pFile;

  vcDBF_Tuple *pLastRead;
  uint8_t fieldCount;
  vcDBF_FieldDesc *pFields;
  vcDBF_DataMappedType *pTypes;
  int32_t recordNum;
  int32_t totalRecords;
  bool finished;

  bool memo;
  vcDBF_MemoFile *pMemo;
  uint8_t memoFieldCount;
  char **ppStrings;
};

struct vcDBF_Writer
{
  udFile *pFile;

  uint8_t fieldCount;
  vcDBF_FieldDesc *pFields;
  int32_t recordNum;
  char spaceBuffer[255] = { ' ' };

  bool memo;
  uint32_t memoBlockIndex;
  uint32_t blockSize;
  udFile *pMemo;
};

template <typename T>
udResult vcDBase_ReadBigEndian(udFile *pFile, T *pOut)
{
  udResult result = udR_Failure_;

  uint8_t bytes[sizeof(T)];

  UD_ERROR_CHECK(udFile_Read(pFile, bytes, sizeof(T)));

  for (int i = 0; i < sizeof(T); ++i)
    *pOut |= bytes[i] << 8 * (sizeof(T) - (i + 1));

  result = udR_Success;

epilogue:

  return result;
}

template <typename T>
udResult vcDBase_ReadLittleEndian(udFile *pFile, T *pOut)
{
  udResult result = udR_Failure_;

  uint8_t bytes[sizeof(T)];

  UD_ERROR_CHECK(udFile_Read(pFile, bytes, sizeof(T)));

  for (int i = 0; i < sizeof(T); ++i)
    *pOut |= bytes[i] << (i * 8);

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_ReadHeader(vcDBF_Reader *pReader, vcDBF_Header *pHeader)
{
  udResult result = udR_Failure_;

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &pHeader->flags, sizeof(pHeader->flags)));
  UD_ERROR_IF((pHeader->flags & 0b00000011) != 3, udR_VersionMismatch); // DBase IV

  if ((pHeader->flags & 0b01000000) == 0b01000000)
    pReader->memo = true;

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, pHeader->YMD, sizeof(pHeader->YMD)));
  UD_ERROR_IF(pHeader->YMD[0] < 0 || pHeader->YMD[1] > 12 || pHeader->YMD[2] > 31, udR_ParseError);

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &pHeader->records, sizeof(pHeader->records)));
  UD_ERROR_IF(pHeader->records < 1, udR_NothingToDo);

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &pHeader->headerBytes, sizeof(pHeader->headerBytes)));
  UD_ERROR_IF(pHeader->headerBytes < 31, udR_NothingToDo);

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &pHeader->recordBytes, sizeof(pHeader->recordBytes)));
  UD_ERROR_IF(pHeader->recordBytes < 1, udR_NothingToDo);

  int8_t trash[20];
  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &trash, sizeof(uint8_t) * 20)); // Seek forward, 20 reserved bytes

  pReader->totalRecords = pHeader->records;

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_ReadFieldDesc(udFile *pFile, vcDBF_FieldDesc *pDesc, vcDBF_DataMappedType *pType)
{
  if (pFile == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  vcDBF_DataMappedType type = vcDBFDMT_Invalid;
  uint8_t trash[10];

  // All in-use fields are single byte sized, no endianness concerns
  UD_ERROR_CHECK(udFile_Read(pFile, pDesc->fieldName, sizeof(char)));

  UD_ERROR_IF(pDesc->fieldName[0] == '\x0d', udR_NothingToDo); // Field terminator

  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->fieldName[1], sizeof(pDesc->fieldName) - sizeof(pDesc->fieldName[1])));
  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->fieldType, sizeof(char)));

  UD_ERROR_CHECK(udFile_Read(pFile, &trash, sizeof(uint32_t)));

  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->fieldLen, sizeof(uint8_t)));
  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->fieldCount, sizeof(uint8_t)));

  UD_ERROR_CHECK(udFile_Read(pFile, &trash, sizeof(uint16_t)));

  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->workAreaID, sizeof(uint8_t)));

  UD_ERROR_CHECK(udFile_Read(pFile, &trash, sizeof(uint8_t) * 10));

  UD_ERROR_CHECK(udFile_Read(pFile, &pDesc->setFieldsFlag, sizeof(uint8_t)));

  switch (pDesc->fieldType)
  {
  case vcDBF_Character: // Falls through
  case vcDBF_Date:
  case vcDBF_DateTime:
    type = vcDBFDMT_String;
    break;

  case vcDBF_Float: // Falls through
  case vcDBF_Currency:
  case vcDBF_Numeric:
    type = vcDBFDMT_Float;
    break;

  case vcDBF_Logical:
    type = vcDBFDMT_Logical;
    break;

  case vcDBF_Integer:
    type = vcDBFDMT_Integer;
    break;

  case vcDBF_Memo:
    type = vcDBFDMT_Memo;
  }

  UD_ERROR_IF(type == vcDBFDMT_Invalid, udR_ParseError);
  *pType = type;

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_ReadMemoBlock(vcDBF_Reader *pReader, uint32_t block, char *pOut)
{
  if (pReader->pMemo == nullptr || pOut == nullptr || block < pReader->pMemo->firstAvailableIndex || block > (pReader->pMemo->fileLength / pReader->pMemo->blockSize) - 1)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  uint32_t memoLength = 0;

  pOut = pReader->pMemo->pData + (pReader->pMemo->blockSize * block) + sizeof(uint32_t) * 2;
  memoLength = *((uint32_t *)pOut - 1);

  // In the unlikely event of the memolength matching the blocksize we wont be able to null terminate the string in the loaded memo file data
  if (memoLength % pReader->pMemo->blockSize == 0)
  {
    char *pNewOut = udAllocType(char, memoLength + 1, udAF_Zero);
    UD_ERROR_NULL(pNewOut, udR_MemoryAllocationFailure);

    for (uint32_t i = 0; i < memoLength; ++i)
      pNewOut[i] = pOut[i];

    pOut = pNewOut;

    for (uint32_t i = 0; ; ++i)
    {
      if (pReader->ppStrings[i] == nullptr)
      {
        pReader->ppStrings[i] = pOut;
        break;
      }
    }
  }

  pOut[memoLength] = '\0';

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_LoadMemoFile(vcDBF_Reader *pReader, const char *pFilename)
{
  if (pReader == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  pReader->pMemo = udAllocType(vcDBF_MemoFile, 1, udAF_Zero);
  UD_ERROR_NULL(pReader->pMemo, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(udFile_Load(pFilename, &pReader->pMemo->pData, &pReader->pMemo->fileLength));

  pReader->pMemo->firstAvailableIndex = *(uint32_t *)pReader->pMemo->pData;
  vcDBase_FlipEndian(&pReader->pMemo->firstAvailableIndex);

  pReader->pMemo->blockSize = *((uint16_t *)pReader->pMemo->pData + 2);

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_CreateReader(vcDBF_Reader **ppReader, const char *pFilename)
{
  if (ppReader == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  vcDBF_Header DBF = {};
  int fieldCount = 0;
  int headerBytesRemaining = 0;
  uint64_t trash = 0;

  vcDBF_Reader *pReader = udAllocType(vcDBF_Reader, 1, udAF_Zero);
  UD_ERROR_NULL(pReader, udR_MemoryAllocationFailure);
  pReader->recordNum = 1;

  UD_ERROR_CHECK(udFile_Open(&pReader->pFile, pFilename, udFOF_Read));

  UD_ERROR_CHECK(vcDBase_ReadHeader(pReader, &DBF));

  if (pReader->memo)
  {
    uint32_t len = (uint32_t)udStrlen(pFilename);
    char *pDBT = udStrdup(pFilename);
    pDBT[len - 1] = 'T'; // X.DBF --> X.DBT

    UD_ERROR_CHECK(vcDBase_LoadMemoFile(pReader, pDBT));

    udFree(pDBT);
  }

  fieldCount = DBF.headerBytes / 32; // Header & field descriptors are 32 bytes, overestimate by 1 to make readfielddesc process the field terminator
  UD_ERROR_IF(fieldCount < 1, udR_ParseError);
  pReader->fieldCount = (uint8_t)udMin(fieldCount, 255);

  pReader->pFields = udAllocType(vcDBF_FieldDesc, pReader->fieldCount, udAF_Zero);
  UD_ERROR_NULL(pReader->pFields, udR_MemoryAllocationFailure);

  pReader->pTypes = udAllocType(vcDBF_DataMappedType, pReader->fieldCount, udAF_Zero);
  UD_ERROR_NULL(pReader->pTypes, udR_MemoryAllocationFailure);

  headerBytesRemaining = DBF.headerBytes - 32;
  for (uint8_t i = 0; i < pReader->fieldCount; ++i)
  {
    result = vcDBase_ReadFieldDesc(pReader->pFile, &pReader->pFields[i], &pReader->pTypes[i]);
    if (result == udR_NothingToDo)
    {
      pReader->fieldCount = i;
      result = udR_Success;
      --headerBytesRemaining;
      break;
    }
    else
    {
      if (pReader->pFields[i].fieldType == 'M')
        ++pReader->memoFieldCount;

      headerBytesRemaining -= 32;
      UD_ERROR_CHECK(result);
    }
  }

  if (pReader->memo && pReader->memoFieldCount > 0)
    pReader->ppStrings = udAllocType(char *, pReader->memoFieldCount, udAF_Zero);

  // If we're not at the end of the header, seek to the end
  while (headerBytesRemaining > 0)
  {
    UD_ERROR_CHECK(udFile_Read(pReader->pFile, &trash, udMin((int)sizeof(trash), headerBytesRemaining)));
    headerBytesRemaining -= sizeof(trash);
  }

  *ppReader = pReader;

  result = udR_Success;

epilogue:

  if (result != udR_Success && pReader != nullptr)
  {
    udFile_Close(&pReader->pFile);

    udFree(pReader->ppStrings);
    udFree(pReader->pTypes);
    udFree(pReader->pFields);
    udFree(pReader);
  }

  return result;
}

udResult vcDBase_ReadTuple(vcDBF_Reader *pReader, vcDBF_Tuple *pTuple)
{
  if (pReader == nullptr || pTuple == nullptr)
    return udR_InvalidParameter_;

  if (pReader->finished)
    return udR_NothingToDo;

  udResult result = udR_Failure_;
  char marker = '\0';
  uint32_t blockIndex = 0;

  if (pReader->pLastRead != nullptr)
  {
    for (int i = 0; i < pReader->pLastRead->numFields; ++i)
    {
      if (pReader->pLastRead->pFields[i].type == vcDBFDMT_String)
        udFree(pReader->pLastRead->pFields[i].data.pString);
    }
    udFree(pReader->pLastRead->pFields);
  }

  for (int i = 0; i < pReader->memoFieldCount; ++i)
  {
    if (pReader->ppStrings[i] == nullptr)
      break;

    udFree(pReader->ppStrings[i]);
    pReader->ppStrings[i] = nullptr;
  }

  UD_ERROR_CHECK(udFile_Read(pReader->pFile, &marker, sizeof(char)));

  UD_ERROR_IF(marker == '\x2A', udR_Success); // Deleted record
  UD_ERROR_IF(marker == '\x1A', udR_NothingToDo); // End of file reached
  UD_ERROR_IF(marker != '\x20', udR_ParseError); // Record marker

  pTuple->numFields = pReader->fieldCount;
  pTuple->pFields = udAllocType(vcDBF_RecordField, pTuple->numFields, udAF_Zero);
  UD_ERROR_NULL(pTuple->pFields, udR_MemoryAllocationFailure);

  pReader->pLastRead = pTuple;

  for (uint32_t i = 0; i < pReader->fieldCount; ++i)
  {
    uint32_t j = 0;

    pTuple->pFields[i].pFieldName = pReader->pFields[i].fieldName;
    pTuple->pFields[i].type = pReader->pTypes[i];

    char buffer[256];
    UD_ERROR_CHECK(udFile_Read(pReader->pFile, buffer, pReader->pFields[i].fieldLen));
    buffer[pReader->pFields[i].fieldLen] = '\0';

    // Skip padding to content
    for (; j < pReader->pFields[i].fieldLen; ++j)
      if (buffer[j] != ' ')
        break;

    // If no content within field length set to null
    if (j == pReader->pFields[i].fieldLen)
    {
      j = 0;
      buffer[0] = '\0';
    }

    switch (pTuple->pFields[i].type)
    {
    case vcDBFDMT_String: // String types shouldn't be space padded, they're padded with nulls at the end
      pTuple->pFields[i].data.pString = udStrdup(buffer);
      break;

    case vcDBFDMT_Float:
      pTuple->pFields[i].data.floaty = udStrAtof(buffer + j);
      break;

    case vcDBFDMT_Logical:
      UD_ERROR_IF(buffer[j] == '?', udR_Success);
      pTuple->pFields[i].data.logical = (buffer[j] == 'Y' || buffer[j] == 'y' || buffer[j] == 'T' || buffer[j] == 't');

      UD_ERROR_IF(!pTuple->pFields[i].data.logical && (buffer[j] != 'N' && buffer[j] != 'n' && buffer[j] != 'F' && buffer[j] != 'f'), udR_ParseError);
      break;

    case vcDBFDMT_Integer:
      pTuple->pFields[i].data.integer = udStrAtoi(buffer + j);
      break;

    case vcDBFDMT_Memo:
      UD_ERROR_IF(!pReader->memo, udR_ParseError);

      if (pReader->pFields[i].fieldLen == 4) // memo block index stored as uint32_t
        memcpy(&blockIndex, buffer, sizeof(uint32_t));
      else
        blockIndex = udStrAtoi(buffer + j);

      UD_ERROR_CHECK(vcDBase_ReadMemoBlock(pReader, blockIndex, pTuple->pFields[i].data.pString));
      break;

    case vcDBFDMT_Invalid:
      break;
    }
  }

  result = udR_Success;

epilogue:

  ++pReader->recordNum;

  pReader->finished = (result == udR_NothingToDo || pReader->recordNum > pReader->totalRecords); // > because recordNum starts at 1

  return result;
}

udResult vcDBase_DestroyReader(vcDBF_Reader **ppReader)
{
  if (ppReader == nullptr || *ppReader == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  if ((*ppReader)->pLastRead != nullptr)
  {
    for (int i = 0; i < (*ppReader)->pLastRead->numFields; ++i)
    {
      if ((*ppReader)->pLastRead->pFields[i].type == vcDBFDMT_String)
        udFree((*ppReader)->pLastRead->pFields[i].data.pString);
    }
    udFree((*ppReader)->pLastRead->pFields);
  }

  if ((*ppReader)->memo)
  {
    for (int i = 0; i < (*ppReader)->memoFieldCount; ++i)
    {
      if ((*ppReader)->ppStrings[i] == nullptr)
        break;

      udFree((*ppReader)->ppStrings[i]);
      (*ppReader)->ppStrings[i] = nullptr;
    }

    udFree((*ppReader)->ppStrings);
  }

  UD_ERROR_CHECK(udFile_Close(&(*ppReader)->pFile));

  udFree((*ppReader)->pTypes);
  udFree((*ppReader)->pFields);
  udFree(*ppReader);

  *ppReader = nullptr;

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_WriteFieldDesc(udFile *pFile, const vcDBF_FieldDesc *pDesc)
{
  if (pFile == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  uint8_t blank[10] = {};
  uint8_t strLen = 0;
  for (; pDesc->fieldName[strLen] != '\0' && strLen < 11; ++strLen);

  // All these fields are single byte numbers or unused, no need to flip endian
  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->fieldName, strLen));

  if (strLen < 11)
    UD_ERROR_CHECK(udFile_Write(pFile, blank, 11 - strLen));

  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->fieldType, 1));
  UD_ERROR_CHECK(udFile_Write(pFile, blank, sizeof(uint32_t)));
  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->fieldLen, 1));
  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->fieldCount, 1));
  UD_ERROR_CHECK(udFile_Write(pFile, blank, 2));
  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->workAreaID, 1));
  UD_ERROR_CHECK(udFile_Write(pFile, blank, 2));
  UD_ERROR_CHECK(udFile_Write(pFile, &pDesc->setFieldsFlag, 1));
  UD_ERROR_CHECK(udFile_Write(pFile, blank, 8));

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_CreateWriter(vcDBF_Writer **ppWriter, const char *pFilename, vcDBF_FieldDesc *pFields, uint8_t fieldCount, int32_t recordCount, bool memo)
{
  if (ppWriter == nullptr || pFilename == nullptr || pFields == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  vcDBF_Header dbfh = {};
  time_t currTime = time_t(0);
  tm *pLocalTime;
  uint8_t blank[20] = {};

  int filenameLen = 0;
  char *pMemoname = nullptr;
  const char *pFullFilename = pFilename;
  if (!udStrEndsWithi(pFullFilename, ".DBF"))
    pFullFilename = udTempStr("%s.dbf", pFullFilename);

  filenameLen = udStrlen(pFullFilename);

  vcDBF_Writer *pWriter = udAllocType(vcDBF_Writer, 1, udAF_Zero);
  UD_ERROR_NULL(pWriter, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(udFile_Open(&pWriter->pFile, pFullFilename, udFOF_Write));

  // DBF File header
  memset(&dbfh, 0, sizeof(vcDBF_Header));
  dbfh.flags = 3; // TODO: Revisit other flags, check if any are necessary

  if (memo)
    dbfh.flags |= 0b01000000;

  dbfh.records = recordCount;
  dbfh.headerBytes = 32 * fieldCount + 1;

  pLocalTime = localtime(&currTime);

  dbfh.YMD[0] = (int8_t)pLocalTime->tm_year;
  dbfh.YMD[1] = (int8_t)(pLocalTime->tm_mon + 1);
  dbfh.YMD[2] = (int8_t)pLocalTime->tm_mday;

  for (int i = 0; i < fieldCount; ++i)
    dbfh.recordBytes += pFields[i].fieldLen;

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, &dbfh, sizeof(vcDBF_Header)));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, blank, 20)); // Reserved bytes

  for (int i = 0; i < fieldCount; ++i)
    UD_ERROR_CHECK(vcDBase_WriteFieldDesc(pWriter->pFile, &pFields[i]));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, "\x0d", sizeof(char)));

  pWriter->memo = memo;

  if (memo)
  {
    pMemoname = udAllocType(char, filenameLen + 1, udAF_Zero);

    if (!udStrEndsWithi(pFilename, ".DBF"))
    {
      udSprintf(pMemoname, filenameLen + 1, "%s.dbt", pFilename);
    }
    else
    {
      udStrcpy(pMemoname, filenameLen + 1, pFilename);
      pMemoname[filenameLen - 1] = 't';
    }

    UD_ERROR_CHECK(udFile_Open(&pWriter->pMemo, pMemoname, udFOF_Write));

    pWriter->blockSize = 512;
    pWriter->memoBlockIndex = 1;
  }

  *ppWriter = pWriter;

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_WriteString(vcDBF_Writer *pWriter, const char *pString)
{
  if (pWriter == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  uint32_t fieldNum = (uint32_t)pWriter->recordNum % pWriter->fieldCount;
  uint8_t fieldLen = pWriter->pFields[fieldNum].fieldLen;
  uint32_t outputLen = (uint32_t) udStrlen(pString);

  if (outputLen > fieldLen)
    outputLen = fieldLen; // Write out as much as will fit
  else if (outputLen < fieldLen)
    UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pWriter->spaceBuffer, fieldLen - outputLen));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pString, outputLen));

  result = udR_Success;
  ++pWriter->recordNum;

epilogue:

  return result;
}

udResult vcDBase_WriteBool(vcDBF_Writer *pWriter, const bool input)
{
  if (pWriter == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  uint32_t fieldNum = pWriter->recordNum % pWriter->fieldCount;
  uint8_t fieldLen = pWriter->pFields[fieldNum].fieldLen;

  if (fieldLen > 1)
    UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pWriter->spaceBuffer, fieldLen - 1));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, (input) ? "T" : "F", 1));

  result = udR_Success;
  ++pWriter->recordNum;

epilogue:

  return result;
}

udResult vcDBase_WriteDouble(vcDBF_Writer *pWriter, const double input)
{
  if (pWriter == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  uint32_t outputLen = 0;
  uint32_t fieldNum = pWriter->recordNum % pWriter->fieldCount;
  uint8_t fieldLen = pWriter->pFields[fieldNum].fieldLen;
  char *pOutput = udAllocType(char, fieldLen + 1, udAF_Zero);
  UD_ERROR_NULL(pOutput, udR_MemoryAllocationFailure);

  outputLen = (uint32_t)udStrFtoa(pOutput, pWriter->pFields[fieldNum].fieldLen + 1, input, 5) - 1; //TODO: How much precision?

  if (outputLen > fieldLen)
    outputLen = fieldLen; // Write out as much as will fit
  else if (outputLen < fieldLen)
    UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pWriter->spaceBuffer, fieldLen - outputLen));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pOutput, outputLen));

  result = udR_Success;
  ++pWriter->recordNum;

epilogue:

  udFree(pOutput);

  return result;
}

udResult vcDBase_WriteInteger(vcDBF_Writer *pWriter, const int32_t input)
{
  if (pWriter == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  uint32_t outputLen = 0;
  uint32_t fieldNum = pWriter->recordNum % pWriter->fieldCount;
  uint8_t fieldLen = pWriter->pFields[fieldNum].fieldLen;
  char *pOutput = udAllocType(char, fieldLen + 1, udAF_Zero);
  UD_ERROR_NULL(pOutput, udR_MemoryAllocationFailure);

  outputLen = (uint32_t)udStrItoa(pOutput, pWriter->pFields[fieldNum].fieldLen + 1, input) - 1;

  if (outputLen > fieldLen)
    outputLen = fieldLen; // Write out as much as will fit
  else if (outputLen < fieldLen)
    UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pWriter->spaceBuffer, fieldLen - outputLen));

  UD_ERROR_CHECK(udFile_Write(pWriter->pFile, pOutput, outputLen));

  result = udR_Success;
  ++pWriter->recordNum;

epilogue:

  udFree(pOutput);

  return result;
}

udResult vcDBase_WriteMemoBlock(vcDBF_Writer *pWriter, const char *pString)
{
  if (pWriter == nullptr || !pWriter->memo || pString == nullptr)
    return udR_Failure_;

  udResult result = udR_Failure_;

  uint32_t strLen = udStrlen(pString);

  pWriter->memoBlockIndex += 1 + (int)((strLen + 2) / pWriter->blockSize); // +2 Terminating characters

  UD_ERROR_CHECK(udFile_Write(pWriter->pMemo, pString, strLen));
  UD_ERROR_CHECK(udFile_Write(pWriter->pMemo, "\x1A\x1A", sizeof(char) * 2));

  strLen = pWriter->blockSize - (strLen % pWriter->blockSize);

  while (strLen > 0)
  {
    UD_ERROR_CHECK(udFile_Write(pWriter->pMemo, pWriter->spaceBuffer, udMin(strLen, (uint32_t)sizeof(pWriter->spaceBuffer))));
    strLen -= sizeof(pWriter->spaceBuffer);
  }

  result = udR_Success;

epilogue:

  return result;
}

udResult vcDBase_WriteTuple(vcDBF_Writer *pWriter, vcDBF_Tuple *pTuple)
{
  udResult result = udR_Failure_;

  for (int i = 0; i < pWriter->fieldCount; ++i)
  {
    switch(pTuple->pFields[i].type)
    {
    case vcDBFDMT_String: // String types shouldn't be space padded, they're padded with nulls at the end
      vcDBase_WriteString(pWriter, pTuple->pFields[i].data.pString);
      break;

    case vcDBFDMT_Float:
      vcDBase_WriteDouble(pWriter, pTuple->pFields[i].data.floaty);
      break;

    case vcDBFDMT_Logical:
      vcDBase_WriteBool(pWriter, pTuple->pFields[i].data.logical);
      break;

    case vcDBFDMT_Integer:
      vcDBase_WriteInteger(pWriter, pTuple->pFields[i].data.integer);
      break;

    case vcDBFDMT_Memo:
      UD_ERROR_IF(!pWriter->memo, udR_Failure_);

      if (pWriter->pFields[i].fieldLen == 4) // write memo num
        UD_ERROR_CHECK(udFile_Write(pWriter->pMemo, &pWriter->memoBlockIndex, sizeof(pWriter->memoBlockIndex)));
      else
        vcDBase_WriteInteger(pWriter, pWriter->memoBlockIndex);

      UD_ERROR_CHECK(vcDBase_WriteMemoBlock(pWriter, pTuple->pFields[i].data.pString));
      break;

    case vcDBFDMT_Invalid:
      break;
    }
  }

  result = udR_Success;

epilogue:

  return result;
}


udResult vcDBase_DestroyWriter(vcDBF_Writer **ppWriter)
{
  udResult result = udR_Failure_;

  UD_ERROR_CHECK(udFile_Write((*ppWriter)->pFile, "\x1A", sizeof(char)));

  UD_ERROR_CHECK(udFile_Close(&(*ppWriter)->pFile));

  result = udR_Success;

epilogue:

  return result;
}
