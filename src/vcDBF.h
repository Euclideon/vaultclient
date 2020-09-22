#ifndef vcDBF_h__
#define vcDBF_h__

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

struct vcDBF;

struct vcDBF_Record
{
  bool deleted;
  union vcDBF_RecordField
  {
    char *pString;
    double floaty;
    bool logical;
    int integer;
  } *pFields;
};

template <typename T>
void vcDBF_FlipEndian(T *pData)
{
  uint8_t bytes[sizeof(T)];
  memcpy(bytes, pData, sizeof(T));

  for (uint32_t i = 0; i < (sizeof(T) / 2); ++i)
  {
    uint8_t temp = bytes[i];
    bytes[i] = bytes[sizeof(T) - i - 1];
    bytes[sizeof(T) - i - 1] = temp;
  }

  memcpy(pData, bytes, sizeof(T));
};

udResult vcDBF_Create(vcDBF **ppDBF);
void vcDBF_Destroy(vcDBF **ppDBF);

udResult vcDBF_Load(vcDBF **ppDBF, const char *pFilename);
udResult vcDBF_Save(vcDBF *pDBF, const char *pFilename);

udResult vcDBF_FindFieldIndex(vcDBF *pDBF, const char *pFieldName, uint16_t *pIndex);
udResult vcDBF_GetFieldIndex(vcDBF *pDBF, const char *pFieldName, uint16_t *pIndex);
uint16_t vcDBF_GetFieldCount(vcDBF *pDBF);
uint32_t vcDBF_GetRecordCount(vcDBF *pDBF);
udResult vcDBF_GetFieldType(vcDBF *pDBF, uint16_t fieldIndex, char* pType);

// These two can only be performed if there's no data in pDBF
udResult vcDBF_AddField(vcDBF *pDBF, char *pFieldName, char fieldType, uint8_t fieldLen);
udResult vcDBF_RemoveField(vcDBF *pDBF, uint16_t fieldIndex);

// Record indexes here start from 0
udResult vcDBF_GetRecord(vcDBF *pDBF, vcDBF_Record **ppRecord, uint32_t recordIndex);
udResult vcDBF_CreateRecord(vcDBF *pDBF, vcDBF_Record **ppRecord);
udResult vcDBF_DeleteRecord(vcDBF *pDBF, vcDBF_Record *pRecord);
udResult vcDBF_DeleteRecord(vcDBF *pDBF, uint32_t recordIndex);

udResult vcDBF_RecordReadFieldBool(vcDBF_Record *pRecord, uint16_t fieldIndex, bool *pValue);
udResult vcDBF_RecordReadFieldDouble(vcDBF_Record *pRecord, uint16_t fieldIndex, double *pValue);
udResult vcDBF_RecordReadFieldInt(vcDBF_Record *pRecord, uint16_t fieldIndex, int32_t *pValue);
udResult vcDBF_RecordReadFieldString(vcDBF_Record *pRecord, uint16_t fieldIndex, const char **ppValue);
udResult vcDBF_RecordReadFieldMemo(vcDBF *pDBF, vcDBF_Record *pRecord, uint16_t fieldIndex, const char **ppValue);

udResult vcDBF_RecordWriteFieldBool(vcDBF_Record *pRecord, uint16_t fieldIndex, bool value);
udResult vcDBF_RecordWriteFieldDouble(vcDBF_Record *pRecord, uint16_t fieldIndex, double value);
udResult vcDBF_RecordWriteFieldInt(vcDBF_Record *pRecord, uint16_t fieldIndex, int32_t value);
udResult vcDBF_RecordWriteFieldString(vcDBF_Record *pRecord, uint16_t fieldIndex, const char *pValue);
udResult vcDBF_RecordWriteFieldMemo(vcDBF *pDBF, vcDBF_Record *pRecord, uint16_t fieldIndex, const char *pValue);

#endif //vcDBF_h__
