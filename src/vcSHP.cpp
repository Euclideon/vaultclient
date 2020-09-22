#include "vcSHP.h"

#include "udMath.h"
#include "udChunkedArray.h"
#include "udFile.h"
#include "vcDBF.h"
#include "udStringUtil.h"

enum vcSHPType
{
  vcSHPType_Empty = 0,
  vcSHPType_Point = 1,
  vcSHPType_Polyline = 3,
  vcSHPType_Polygon = 5,
  vcSHPType_MultiPoint = 8,
  vcSHPType_PointZ = 11,
  vcSHPType_PolylineZ = 13,
  vcSHPType_PolygonZ = 15,
  vcSHPType_MultiPointZ = 18,
  vcSHPType_PointM = 21,
  vcSHPType_PolylineM = 23,
  vcSHPType_PolygonM = 25,
  vcSHPType_MultiPointM = 28,
  vcSHPType_MultiPatch = 31
};

// file header, 100 bytes
struct vcSHP_Header
{
  uint32_t fileLength;
  int32_t version;
  vcSHPType shapeType;
  // Minimum bounding rectangle (MBR) of all shapes contained within the dataset; four doubles in the following order: min X, min Y, max X, max Y
  udDouble2 MBRmin;
  udDouble2 MBRmax;
  // Range of Z; two doubles in the following order : min Z, max Z
  udDouble2 ZRange;
  // Range of M; two doubles in the following order: min M, max M
  udDouble2 MRange;
};

struct vcSHP_RecordPoly
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;

  int32_t partsCount;
  int32_t pointsCount;

  int32_t *parts;
  udDouble3 *points; // x, y
};

struct vcSHP_RecordMultiPoint
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;

  int32_t pointsCount;
  udDouble3 *points;
};

struct vcSHP_RecordPointZ
{
  udDouble3 point;
  double MValue;
};

struct vcSHP_RecordPolyZ
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;
  udDouble2 ZRange;

  int32_t partsCount;
  int32_t pointsCount;

  int32_t *parts;
  udDouble3 *points; // x, y, z

  // optional: loaded but didn't use
  bool MProvided;
  udDouble2 MRange;
  double *MValue;
};

struct vcSHP_RecordMultiPointZ
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;
  udDouble2 ZRange;

  int32_t pointsCount;
  udDouble3 *points;

  // optional:
  bool MProvided;
  udDouble2 MRange;
  double *MValue;
};

struct vcSHP_RecordPolyM
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;
  int32_t partsCount;
  int32_t pointsCount;

  int32_t *parts;
  udDouble3 *points;

  // optional:
  bool MProvided;
  udDouble2 MRange;
  double *MValue;
};

struct vcSHP_RecordMultiPointM
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;

  int32_t pointsCount;
  udDouble3 *points;

  // optional:
  bool MProvided;
  udDouble2 MRange;
  double *MValue;
};


union vcSHP_RecordData
{
  udDouble3 point; // (x, y, 0)
  vcSHP_RecordPoly poly;// polyline and polygon (x, y, 0)
  vcSHP_RecordMultiPoint multiPoint; // (x, y, 0)

  vcSHP_RecordPointZ pointZ; // (x, y, Z), M
  vcSHP_RecordPolyZ polyZ; // polylineZ, polygonZ and multipatch (x, y, Z), M
  vcSHP_RecordMultiPointZ multiPointZ; // (x, y, Z), M

  vcSHP_RecordPointZ pointM; // (x, y, 0), M
  vcSHP_RecordPolyM polyM; // polylineM and polygonM (x, y, 0), M
  vcSHP_RecordMultiPointM multiPointM;// (x, y, 0), M
};

struct vcSHP_Record
{
  int32_t id;
  uint32_t length;
  vcSHPType shapeType;
  vcSHP_RecordData data;
};

struct vcDBF;
struct vcSHP
{
  bool dbfLoad;
  bool prjLoad;

  vcSHP_Header shpHeader;
  udChunkedArray<vcSHP_Record> shpRecords;
  vcDBF *pDBF;

  udResult projectionLoad;
  char *WKTString;
};

uint16_t vcSHP_GetFirstDBFStringFieldIndex(vcDBF *pDBF)
{
  char type;
  udResult result = udR_Failure_;

  for (uint16_t i = 0; i < vcDBF_GetFieldCount(pDBF); i++)
  {
    result = vcDBF_GetFieldType(pDBF, i, &type);
    if (result == udR_Success && type == 'C')
      return i;
  }

  return (uint16_t)-1;
}

void vcSHP_ReleaseRecord(vcSHP_Record *pRecord)
{
  switch (pRecord->shapeType)
  {
  case vcSHPType::vcSHPType_Polyline:
  case vcSHPType::vcSHPType_Polygon:
  {
    udFree(pRecord->data.poly.parts);
    udFree(pRecord->data.poly.points);
  }
  break;
  case vcSHPType::vcSHPType_MultiPoint:
  {
    udFree(pRecord->data.multiPoint.points);
  }
  break;
  case vcSHPType::vcSHPType_PolylineZ:
  case vcSHPType::vcSHPType_PolygonZ:
  case vcSHPType::vcSHPType_MultiPatch:
  {
    udFree(pRecord->data.polyZ.parts);
    udFree(pRecord->data.polyZ.points);

    if (pRecord->data.polyZ.MValue != nullptr)
      udFree(pRecord->data.polyZ.MValue);
  }
  break;
  case vcSHPType::vcSHPType_MultiPointZ:
  {
    udFree(pRecord->data.multiPointZ.points);
    if (pRecord->data.multiPointZ.MValue != nullptr)
      udFree(pRecord->data.multiPointZ.MValue);
  }
  break;
  case vcSHPType::vcSHPType_PolylineM:
  case vcSHPType::vcSHPType_PolygonM:
  {
    udFree(pRecord->data.polyM.parts);
    udFree(pRecord->data.polyM.points);

    if (pRecord->data.polyM.MValue != nullptr)
      udFree(pRecord->data.polyM.MValue);
  }
  break;
  case vcSHPType::vcSHPType_MultiPointM:
  {
    udFree(pRecord->data.multiPointM.points);
    if (pRecord->data.multiPointM.MValue != nullptr)
      udFree(pRecord->data.multiPointM.MValue);
  }
  break;

  }
}


void vcSHP_Release(vcSHP *pSHP)
{
  // release .shp file
  for (uint32_t i = 0; i < (uint32_t)pSHP->shpRecords.length; ++i)
    vcSHP_ReleaseRecord(&pSHP->shpRecords[i]);

  pSHP->shpRecords.Deinit();

  // release .dbf file
  vcDBF_Destroy(&pSHP->pDBF);

  // release .prj file
  if(pSHP->projectionLoad == udR_Success)
    udFree(pSHP->WKTString);
}

static uint32_t UnpackDouble(uint32_t readPos, double *dest, uint8_t *src)
{
  memcpy(dest, src + readPos, 8);
  return readPos + 8;
}

static uint32_t UnpackInt32(uint32_t readPos, int32_t *dest, uint8_t *src)
{
  memcpy(dest, src + readPos, 4);
  return readPos + 4;
}

udResult vcSHP_LoadShpRecord(vcSHP *pSHP, udFile *pFile, uint32_t &offset, vcSHPType defaultShape)
{
  udResult result;
  uint8_t id[4] = {};
  uint8_t length[4] = {};
  uint8_t *buffer = nullptr;
  uint32_t readPosition = 0;
  int32_t type = 0;
  int32_t leftBytes = 0;

  vcSHP_Record record = {};
  UD_ERROR_CHECK(udFile_Read(pFile, id, 4 * sizeof(uint8_t)));
  record.id = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];
  offset += 4;

  UD_ERROR_CHECK(udFile_Read(pFile, length, 4 * sizeof(uint8_t)));
  record.length = (length[0] << 24) | (length[1] << 16) | (length[2] << 8) | length[3];
  record.length *= 2;
  offset += 4;

  buffer = udAllocType(uint8_t, record.length, udAF_Zero);
  UD_ERROR_NULL(buffer, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(udFile_Read(pFile, buffer, record.length * sizeof(uint8_t)));

  // shape type check
  readPosition = UnpackInt32(readPosition, &type, buffer);
  record.shapeType = (vcSHPType)type;
  UD_ERROR_IF(record.shapeType != defaultShape, udR_ParseError);

  switch (record.shapeType)
  {
  case vcSHPType::vcSHPType_Point:
  {
    readPosition = UnpackDouble(readPosition, &record.data.point.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.point.y, buffer);
  }
  break;
  case vcSHPType::vcSHPType_Polyline:
  case vcSHPType::vcSHPType_Polygon:
  {
    readPosition = UnpackDouble(readPosition, &record.data.poly.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.poly.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.poly.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.poly.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.poly.partsCount, buffer);
    readPosition = UnpackInt32(readPosition, &record.data.poly.pointsCount, buffer);

    record.data.poly.parts = udAllocType(int32_t, record.data.poly.partsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.poly.parts, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.poly.partsCount; i++)
      readPosition = UnpackInt32(readPosition, &record.data.poly.parts[i], buffer);

    record.data.poly.points = udAllocType(udDouble3, record.data.poly.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.poly.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.poly.pointsCount; i++)
    {
      udDouble3 *p = &record.data.poly.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }

  }
  break;
  case vcSHPType::vcSHPType_MultiPoint:
  {
    readPosition = UnpackDouble(readPosition, &record.data.multiPoint.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPoint.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPoint.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPoint.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.multiPoint.pointsCount, buffer);
    record.data.multiPoint.points = udAllocType(udDouble3, record.data.multiPoint.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.multiPoint.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.multiPoint.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPoint.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }
  }
  break;
  case vcSHPType::vcSHPType_PointZ:
  {
    readPosition = UnpackDouble(readPosition, &record.data.pointZ.point.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.pointZ.point.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.pointZ.point.z, buffer); // Z
    readPosition = UnpackDouble(readPosition, &record.data.pointZ.MValue, buffer); // M
  }
  break;
  case vcSHPType::vcSHPType_PolylineZ:
  case vcSHPType::vcSHPType_PolygonZ:
  case vcSHPType::vcSHPType_MultiPatch:
  {
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.polyZ.partsCount, buffer);
    readPosition = UnpackInt32(readPosition, &record.data.polyZ.pointsCount, buffer);

    record.data.polyZ.parts = udAllocType(int32_t, record.data.polyZ.partsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.polyZ.parts, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.polyZ.partsCount; i++)
      readPosition = UnpackInt32(readPosition, &record.data.polyZ.parts[i], buffer);

    record.data.polyZ.points = udAllocType(udDouble3, record.data.polyZ.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.polyZ.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.polyZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.polyZ.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }

    // z
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.ZRange.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyZ.ZRange.y, buffer);
    for (int i = 0; i < record.data.polyZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.polyZ.points[i];
      readPosition = UnpackDouble(readPosition, &p->z, buffer);
    }

    // optional:
    record.data.polyZ.MProvided = false;
    record.data.polyZ.MValue = nullptr;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.polyZ.pointsCount))
    {
      record.data.polyZ.MProvided = true;
      readPosition = UnpackDouble(readPosition, &record.data.polyZ.MRange.x, buffer);
      readPosition = UnpackDouble(readPosition, &record.data.polyZ.MRange.y, buffer);

      record.data.polyZ.MValue = udAllocType(double, record.data.polyZ.pointsCount, udAF_Zero);
      UD_ERROR_NULL(record.data.polyZ.MValue, udR_MemoryAllocationFailure);
      for (int i = 0; i < record.data.polyZ.pointsCount; i++)
        readPosition = UnpackDouble(readPosition, &record.data.polyZ.MValue[i], buffer);
    }

  }
  break;
  case vcSHPType::vcSHPType_MultiPointZ:
  {
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.multiPointZ.pointsCount, buffer);
    record.data.multiPointZ.points = udAllocType(udDouble3, record.data.multiPointZ.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.multiPointZ.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPointZ.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }

    // z
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.ZRange.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.ZRange.y, buffer);
    for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPointZ.points[i];
      readPosition = UnpackDouble(readPosition, &p->z, buffer);
    }

    // optional:
    record.data.multiPointZ.MProvided = false;
    record.data.multiPointZ.MValue = nullptr;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.multiPointZ.pointsCount))
    {
      record.data.multiPointZ.MProvided = true;
      readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MRange.x, buffer);
      readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MRange.y, buffer);

      record.data.multiPointZ.MValue = udAllocType(double, record.data.multiPointZ.pointsCount, udAF_Zero);
      UD_ERROR_NULL(record.data.multiPointZ.MValue, udR_MemoryAllocationFailure);
      for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
        readPosition = UnpackDouble(readPosition, &record.data.multiPointZ.MValue[i], buffer);
    }

  }
  break;
  case vcSHPType::vcSHPType_PointM:
  {
    readPosition = UnpackDouble(readPosition, &record.data.pointM.point.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.pointM.point.y, buffer);
    record.data.pointM.point.z = 0;
    readPosition = UnpackDouble(readPosition, &record.data.pointM.MValue, buffer); // M
  }
  break;
  case vcSHPType::vcSHPType_PolylineM:
  case vcSHPType::vcSHPType_PolygonM:
  {
    readPosition = UnpackDouble(readPosition, &record.data.polyM.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyM.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyM.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.polyM.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.polyM.partsCount, buffer);
    readPosition = UnpackInt32(readPosition, &record.data.polyM.pointsCount, buffer);

    record.data.polyM.parts = udAllocType(int32_t, record.data.polyM.partsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.polyM.parts, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.polyM.partsCount; i++)
      readPosition = UnpackInt32(readPosition, &record.data.polyM.parts[i], buffer);

    record.data.polyM.points = udAllocType(udDouble3, record.data.polyM.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.polyM.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.polyM.pointsCount; i++)
    {
      udDouble3 *p = &record.data.polyM.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }

    // optional:
    record.data.polyM.MProvided = false;
    record.data.polyM.MValue = nullptr;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.polyM.pointsCount))
    {
      record.data.polyM.MProvided = true;
      readPosition = UnpackDouble(readPosition, &record.data.polyM.MRange.x, buffer);
      readPosition = UnpackDouble(readPosition, &record.data.polyM.MRange.y, buffer);

      record.data.polyM.MValue = udAllocType(double, record.data.polyM.pointsCount, udAF_Zero);
      UD_ERROR_NULL(record.data.polyM.MValue, udR_MemoryAllocationFailure);
      for (int i = 0; i < record.data.polyM.pointsCount; i++)
        readPosition = UnpackDouble(readPosition, &record.data.polyM.MValue[i], buffer);
    }
  }
  break;
  case vcSHPType::vcSHPType_MultiPointM:
  {
    readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MBRmin.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MBRmin.y, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MBRmax.x, buffer);
    readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MBRmax.y, buffer);

    readPosition = UnpackInt32(readPosition, &record.data.multiPointM.pointsCount, buffer);
    record.data.multiPointM.points = udAllocType(udDouble3, record.data.multiPointM.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.data.multiPointM.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.data.multiPointM.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPointM.points[i];
      readPosition = UnpackDouble(readPosition, &p->x, buffer);
      readPosition = UnpackDouble(readPosition, &p->y, buffer);
      p->z = 0;
    }
    // optional:
    record.data.multiPointM.MProvided = false;
    record.data.multiPointM.MValue = nullptr;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.multiPointM.pointsCount))
    {
      record.data.multiPointM.MProvided = true;
      readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MRange.x, buffer);
      readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MRange.y, buffer);

      record.data.multiPointM.MValue = udAllocType(double, record.data.multiPointM.pointsCount, udAF_Zero);
      UD_ERROR_NULL(record.data.multiPointM.MValue, udR_MemoryAllocationFailure);
      for (int i = 0; i < record.data.multiPointM.pointsCount; i++)
        readPosition = UnpackDouble(readPosition, &record.data.multiPointM.MValue[i], buffer);
    }
  }
  break;
  default:
  break;
  }

  pSHP->shpRecords.PushBack(record);
  offset += readPosition;

epilogue:
  udFree(buffer);

  if (result != udR_Success)
    vcSHP_ReleaseRecord(&record);

  return result;
}

udResult vcSHP_LoadShpFile(vcSHP *pSHP, const char *pFilename)
{
  udResult result;

  udFile *pFile = nullptr;
  uint8_t *headerBuffer = nullptr;
  uint32_t offset = 0;

  vcSHP_Header &header = pSHP->shpHeader;
  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Read));

  headerBuffer = udAllocType(uint8_t, 100, udAF_Zero);
  UD_ERROR_NULL(headerBuffer, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(udFile_Read(pFile, headerBuffer, 100 * sizeof(uint8_t)));
  // 00 00 27 0a
  UD_ERROR_IF((headerBuffer[0] != 0 || headerBuffer[1] != 0 || headerBuffer[2] != 0x27 || headerBuffer[3] != 0x0a), udR_ParseError);

  // the file stores the length in shorts
  header.fileLength = (headerBuffer[24] << 24) | (headerBuffer[25] << 16) | (headerBuffer[26] << 8) | headerBuffer[27];
  header.fileLength = 2 * udMin(header.fileLength, UINT32_MAX / 2);

  header.version = (headerBuffer[31] << 24) | (headerBuffer[30] << 16) | (headerBuffer[29] << 8) | headerBuffer[28];
  header.shapeType = (vcSHPType)((int32_t)((headerBuffer[35] << 24) | (headerBuffer[34] << 16) | (headerBuffer[33] << 8) | headerBuffer[32]));

  offset = 36;
  offset = UnpackDouble(offset, &header.MBRmin.x, headerBuffer);
  offset = UnpackDouble(offset, &header.MBRmin.y, headerBuffer);
  offset = UnpackDouble(offset, &header.MBRmax.x, headerBuffer);
  offset = UnpackDouble(offset, &header.MBRmax.y, headerBuffer);
  offset = UnpackDouble(offset, &header.ZRange.x, headerBuffer);
  offset = UnpackDouble(offset, &header.ZRange.y, headerBuffer);
  offset = UnpackDouble(offset, &header.MRange.x, headerBuffer);
  offset = UnpackDouble(offset, &header.MRange.y, headerBuffer);

  UD_ERROR_IF(offset != 100, udR_ParseError);
  udFree(headerBuffer);

  UD_ERROR_CHECK(pSHP->shpRecords.Init(32));
  while (offset < header.fileLength)
  {
    result = vcSHP_LoadShpRecord(pSHP, pFile, offset, header.shapeType);
    UD_ERROR_CHECK(result);
  }

epilogue:
  udFile_Close(&pFile);

  if (headerBuffer)
    udFree(headerBuffer);

  return result;
}

udResult vcSHP_LoadFileGroup(vcSHP *pSHP, const udFilename &fileName)
{
  if (pSHP == nullptr || !fileName.HasFilename())
    return udR_InvalidParameter_;

  udFilename dbfName = fileName;
  dbfName.SetExtension(".dbf");

  udFilename prjName = fileName;
  prjName.SetExtension(".prj");

  // load shp file
  udResult result = udR_Failure_;
  UD_ERROR_CHECK(vcSHP_LoadShpFile(pSHP, fileName.GetPath()));

  // load dbf file
  vcDBF_Load(&pSHP->pDBF, dbfName.GetPath());

  // load prj file
  pSHP->WKTString = nullptr;
  pSHP->projectionLoad = udFile_Load(prjName.GetPath(), &pSHP->WKTString);

epilogue:

  return result;
}

void vcUDP_AddModel(vcState *pProgramState, udProjectNode *pParentNode, vcSHP_Record &record, vcDBF_Record *pDBFRecord, uint16_t stringIndex)
{
  char buffer[256] = {};
  if (pDBFRecord != nullptr && !pDBFRecord->deleted && stringIndex >= 0)
  {
    udStrcpy(buffer, pDBFRecord->pFields[stringIndex].pString);
    udStrStripWhiteSpace(buffer);
  }
  else
  {
    udSprintf(buffer, "POI %d", record.id);
  }

  udProjectNode *pNode = nullptr;
  udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", buffer, nullptr, nullptr);

  switch (record.shapeType)
  {
  case vcSHPType::vcSHPType_Point:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &record.data.point, 1);
  break;
  case vcSHPType::vcSHPType_Polyline:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_LineString, record.data.poly.points, record.data.poly.pointsCount);
  break;
  case vcSHPType::vcSHPType_Polygon:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, record.data.poly.points, record.data.poly.pointsCount);
  break;
  case vcSHPType::vcSHPType_MultiPoint:
  vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_MultiPoint, record.data.multiPointZ.points, record.data.multiPointZ.pointsCount);
  break;
  case vcSHPType::vcSHPType_PointZ:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &record.data.pointZ.point, 1);
  break;
  case vcSHPType::vcSHPType_PolylineZ:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_LineString, record.data.polyZ.points, record.data.polyZ.pointsCount);
  break;
  case vcSHPType::vcSHPType_PolygonZ:
  case vcSHPType::vcSHPType_MultiPatch:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, record.data.polyZ.points, record.data.polyZ.pointsCount);
  break;
  case vcSHPType::vcSHPType_MultiPointZ:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_MultiPoint, record.data.multiPointZ.points, record.data.multiPointZ.pointsCount);
  break;
  case vcSHPType::vcSHPType_PointM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &record.data.pointM.point, 1);
  break;
  case vcSHPType::vcSHPType_PolylineM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_LineString, record.data.polyM.points, record.data.polyM.pointsCount);
  break;
  case vcSHPType::vcSHPType_PolygonM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, record.data.polyM.points, record.data.polyM.pointsCount);
  break;
  case vcSHPType::vcSHPType_MultiPointM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_MultiPoint, record.data.multiPointM.points, record.data.multiPointM.pointsCount);
  break;
  default:
    printf("invalid shape type %d\n", (int)record.shapeType);
  break;
  }
  
}

udResult vcSHP_Load(vcState *pProgramState, const udFilename &fileName)
{
  vcSHP shp = {};
  udProjectNode *pParentNode = nullptr;
  uint16_t stringIndex = 0;
  vcDBF_Record *pDBFRecord = nullptr;
  udGeoZone zone = {};

  udResult result = udR_Failure_;
  UD_ERROR_CHECK(vcSHP_LoadFileGroup(&shp, fileName));

  vcProject_CreateBlankScene(pProgramState, "SHP Import", vcPSZ_NotGeolocated);
  // keep for future requirements. if the zone is set to no geolocated, all coordinates inside shapre files need to transfer from local position.
  //if (shp.projectionLoad == udR_Success)
  //{
  //  udGeoZone_SetFromWKT(&zone, shp.WKTString); // srid missing
  //  vcGIS_ChangeSpace(&pProgramState->geozone, zone);
  //}
  // ====
  
  pParentNode = pProgramState->activeProject.pRoot;
  stringIndex = vcSHP_GetFirstDBFStringFieldIndex(shp.pDBF);
  for (size_t i = 0; i < shp.shpRecords.length; ++i)
  {
    if (stringIndex != (uint16_t)(-1))
      vcDBF_GetRecord(shp.pDBF, &pDBFRecord, (uint32_t)i);

    vcUDP_AddModel(pProgramState, pParentNode, shp.shpRecords[i], pDBFRecord, stringIndex);
  }

epilogue:
  vcSHP_Release(&shp);

  return result;
}
