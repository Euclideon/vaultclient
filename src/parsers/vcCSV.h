#ifndef vcCSV_h__
#define vcCSV_h__

#include "udResult.h"
#include "udChunkedArray.h"

enum vcCSV_ColumnType
{
  vcCSVCT_Name,
  vcCSVCT_X,
  vcCSVCT_Y,
  vcCSVCT_Z,
  vcCSVCT_ID,
  vcCSVCT_ParentID,
  vcCSVCT_Skip
};

enum vcCSV_Delimeter
{
  vcCSVD_Comma,
  vcCSVD_Tab,
  vcCSVD_Space,
  vcCSVD_Semicolon,
  vcCSVD_CustomCharacter,
  vcCSVD_FixedSize,

  vcCSVD_Count
};

struct vcCSVImportSettings
{
  char delimeter;
  int fixedSizeDelimeterSpacing;
  int skipEntries;
  int zoneSRID;
};

struct vcCSV
{
  udResult readResult;

  const char *pFilename;
  vcCSVImportSettings importSettings;

  bool headerRead;
  bool completeRead;
  bool cancel;

  int64_t totalBytesRead;
  int skippedEntries;

  int entryCount; // rows
  int elementCount;

  struct Buffer
  {
    char *pData;
    int64_t usageBytes;
    int64_t capacityBytes;
  };
  Buffer data;

  udChunkedArray<vcCSV_ColumnType> columns;
};

udResult vcCSV_Create(vcCSV **ppCSV, const char *pFilename, const vcCSVImportSettings &importSettings);
udResult vcCSV_ReadHeader(vcCSV *pCSV);
udResult vcCSV_Read(vcCSV *pCSV, uint64_t readAmountBytes = -1);

void vcCSV_Destroy(vcCSV **ppCSV);

#endif// vcCSV_h__
