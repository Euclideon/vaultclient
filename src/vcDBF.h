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

udResult vcDBF_Create(vcDBF **ppDBF);
udResult vcDBF_Destroy(vcDBF **ppDBF);

udResult vcDBF_Load(vcDBF **ppDBF, const char *pFilename);
udResult vcDBF_Save(vcDBF *pDBF, const char *pFilename);

udResult vcDBF_FindFieldIndex(vcDBF *pDBF, const char *pFieldName, uint16_t *pIndex);
udResult vcDBF_GetFieldIndex(vcDBF *pDBF, const char *pFieldName, uint16_t *pIndex);
uint16_t vcDBF_GetFieldCount(vcDBF *pDBF);
uint32_t vcDBF_GetRecordCount(vcDBF *pDBF);

// These two can only be performed if there's no data in pDBF
udResult vcDBF_AddField(vcDBF *pDBF, char *pFieldName, char fieldType, uint8_t fieldLen);
udResult vcDBF_RemoveField(vcDBF *pDBF, uint16_t fieldIndex);

// Record indexes here start from 0 not 1, for SHP files need to pass in recNumber - 1
udResult vcDBF_GetRecord(vcDBF *pDBF, vcDBF_Record **ppRecord, uint32_t recordIndex);
udResult vcDBF_CreateRecord(vcDBF *pDBF, vcDBF_Record **ppRecord);
udResult vcDBF_DeleteRecord(vcDBF *pDBF, vcDBF_Record *pRecord);
udResult vcDBF_DeleteRecord(vcDBF *pDBF, uint32_t recordIndex);

udResult vcDBF_RecordReadFieldBool(vcDBF_Record *pRecord, bool *pValue, uint16_t fieldIndex);
udResult vcDBF_RecordReadFieldDouble(vcDBF_Record *pRecord, double *pValue, uint16_t fieldIndex);
udResult vcDBF_RecordReadFieldInt(vcDBF_Record *pRecord, int32_t *pValue, uint16_t fieldIndex);
udResult vcDBF_RecordReadFieldString(vcDBF_Record *pRecord, const char **ppValue, uint16_t fieldIndex);
udResult vcDBF_RecordReadFieldMemo(vcDBF *pDBF, vcDBF_Record *pRecord, const char **ppValue, uint16_t fieldIndex);

udResult vcDBF_RecordWriteFieldBool(vcDBF_Record *pRecord, bool value, uint16_t fieldIndex);
udResult vcDBF_RecordWriteFieldDouble(vcDBF_Record *pRecord, double value, uint16_t fieldIndex);
udResult vcDBF_RecordWriteFieldInt(vcDBF_Record *pRecord, int32_t value, uint16_t fieldIndex);
udResult vcDBF_RecordWriteFieldString(vcDBF_Record *pRecord, const char *pValue, uint16_t fieldIndex);
udResult vcDBF_RecordWriteFieldMemo(vcDBF_Record *pRecord, vcDBF *pDBF, const char *pValue, uint16_t fieldIndex);

#endif //vcDBase_h__
