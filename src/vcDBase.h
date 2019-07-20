#ifndef vcDBase_h__
#define vcDBase_h__

#include "udResult.h"
#include "udFile.h"

enum vcDBF_DataMappedType
{
  vcDBFDMT_String,
  vcDBFDMT_Float,
  vcDBFDMT_Logical,
  vcDBFDMT_Integer,
  vcDBFDMT_Memo,
  vcDBFDMT_Invalid
};

struct vcDBF_Reader;
struct vcDBF_Writer;

struct vcDBF_RecordField
{
  char *pFieldName;
  vcDBF_DataMappedType type;
  union
  {
    char *pString;
    double floaty;
    bool logical;
    int integer;
  } data;
};

struct vcDBF_Tuple
{
  vcDBF_RecordField *pFields;
  uint8_t numFields;
};

struct vcDBF_FieldDesc
{
  char fieldName[11];
  char fieldType;
  //uint32_t reserved;
  uint8_t fieldLen;
  uint8_t fieldCount;
  //char reserved[2];
  uint8_t workAreaID;
  //char reserved[2];
  uint8_t setFieldsFlag;
  //char reserved[8];
};

typedef void (DBFReadFunction)(vcDBF_Tuple &, vcDBF_Reader *, void *);

udResult vcDBase_CreateReader(vcDBF_Reader **ppReader, const char *pFilename);

// Takes ownership of the vcDBF_Tuple
udResult vcDBase_ReadTuple(vcDBF_Reader *pReader, vcDBF_Tuple *pTuple);
udResult vcDBase_DestroyReader(vcDBF_Reader **ppReader);

template <typename T>
void vcDBase_FlipEndian(T *pData)
{
  T out = 0;

  for (uint32_t i = 0; i < sizeof(T); ++i)
    out |= ((uint8_t *)pData)[i] << 8 * (sizeof(T) - (i + 1));

  *pData = out;
};

template <typename T>
udResult vcDBase_ReadBigEndian(udFile *pFile, T *pOut);

template <typename T>
udResult vcDBase_ReadLittleEndian(udFile *pFile, T *pOut);

udResult vcDBase_CreateWriter(vcDBF_Writer **ppWriter, const char *pFilename, vcDBF_FieldDesc *pFields, uint8_t fieldCount, int32_t recordCount, bool memo);

udResult vcDBase_WriteString(vcDBF_Writer *pWriter, const char *pString);
udResult vcDBase_WriteBool(vcDBF_Writer *pWriter, const bool input);
udResult vcDBase_WriteDouble(vcDBF_Writer *pWriter, const double input);
udResult vcDBase_WriteInteger(vcDBF_Writer *pWriter, const int32_t input);

udResult vcDBase_DestroyWriter(vcDBF_Writer **ppWriter);


#endif //vcDBase_h__
