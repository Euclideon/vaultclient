#include "vcSHP.h"

#include "udMath.h"
#include "udChunkedArray.h"
#include "udFile.h"
#include "vcDBF.h"
#include <udStringUtil.h>

enum class vcSHP_Type
{
  Empty = 0,
  Point = 1,
  Polyline = 3,
  Polygon = 5,
  MultiPoint = 8,
  PointZ = 11,
  PolylineZ = 13,
  PolygonZ = 15,
  MultiPointZ = 18,
  PointM = 21,
  PolylineM = 23,
  PolygonM = 25,
  MultiPointM = 28,
  MultiPatch = 31
};

// file header, 100 bytes
struct vcSHP_Header
{
  uint32_t fileLength;
  int32_t version;
  vcSHP_Type shapeType;
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
  udDouble2 *points; // x, y
};

struct vcSHP_RecordMultiPoint
{
  udDouble2 MBRmin;
  udDouble2 MBRmax;

  int32_t pointsCount;
  udDouble2 *points;
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

  // optional:
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
  udDouble2 *points;

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
  udDouble2 *points;

  // optional:
  bool MProvided;
  udDouble2 MRange;
  double *MValue;
};


union vcSHP_RecordData
{
  udDouble2 point; // (x, y)
  vcSHP_RecordPoly poly;// polyline and polygon
  vcSHP_RecordMultiPoint multiPoint;

  vcSHP_RecordPointZ pointZ; // (x, y, Z, M)
  vcSHP_RecordPolyZ polyZ; // polylineZ, polygonZ and multipatch
  vcSHP_RecordMultiPointZ multiPointZ;

  udDouble3 pointM; // (x, y, M)
  vcSHP_RecordPolyM polyM; // polylineM and polygonM
  vcSHP_RecordMultiPointM multiPointM;
};

struct vcSHP_Record
{
  int32_t id;
  uint32_t length;
  vcSHP_Type shapeType;
  vcSHP_RecordData data;
};

struct vcDBF;
struct vcSHP
{
  bool localBigEndian;
  bool dbfLoad;
  bool prjLoad;

  vcSHP_Header shpHeader;
  udChunkedArray<vcSHP_Record> shpRecords;
  vcDBF *pDBF;

  udResult projectionLoad;
  char *WKTString = nullptr;
};

void vcSHP_ReleaseRecord(vcSHP_Record &record)
{
  switch (record.shapeType)
  {
  case vcSHP_Type::Polyline:
  case vcSHP_Type::Polygon:
  {
    udFree(record.data.poly.parts);
    udFree(record.data.poly.points);
  }
  break;
  case vcSHP_Type::MultiPoint:
  {
    udFree(record.data.multiPoint.points);
  }
  break;
  case vcSHP_Type::PolylineZ:
  case vcSHP_Type::PolygonZ:
  case vcSHP_Type::MultiPatch:
  {
    udFree(record.data.polyZ.parts);
    udFree(record.data.polyZ.points);

    if (record.data.polyZ.MValue != nullptr)
      udFree(record.data.polyZ.MValue);
  }
  break;
  case vcSHP_Type::MultiPointZ:
  {
    udFree(record.data.multiPointZ.points);
    if (record.data.multiPointZ.MValue != nullptr)
      udFree(record.data.multiPointZ.MValue);
  }
  break;
  case vcSHP_Type::PolylineM:
  case vcSHP_Type::PolygonM:
  {
    udFree(record.data.polyM.parts);
    udFree(record.data.polyM.points);

    if (record.data.polyM.MValue != nullptr)
      udFree(record.data.polyM.MValue);
  }
  break;
  case vcSHP_Type::MultiPointM:
  {
    udFree(record.data.multiPointM.points);
    if (record.data.multiPointM.MValue != nullptr)
      udFree(record.data.multiPointM.MValue);
  }
  break;
  default:
  break;
  }
}


void vcSHP_Release(vcSHP *pSHP)
{
  // release .shp file
  for (uint32_t i = 0; i < (uint32_t)pSHP->shpRecords.length; ++i)
    vcSHP_ReleaseRecord(pSHP->shpRecords[i]);

  pSHP->shpRecords.Deinit();

  // release .dbf file
  vcDBF_Destroy(&pSHP->pDBF);

  // release .prj file
  if(pSHP->projectionLoad == udR_Success)
    udFree(pSHP->WKTString);
}

static void SwapByteOrder(int length, uint8_t *src)
{
  for (int i = 0; i < length / 2; i++)
  {
    uint8_t temp = src[i];
    src[i] = src[length - i - 1];
    src[length - i - 1] = temp;
  }
}

static uint32_t UnpackDouble(bool localBigEndian, uint32_t readPos, double *dest, uint8_t *src)
{
  if (localBigEndian)
    SwapByteOrder(8, src + readPos);

  memcpy(dest, src + readPos, 8);
  return readPos + 8;
}

static uint32_t UnpackInt32(bool localBigEndian, uint32_t readPos, int32_t *dest, uint8_t *src)
{
  if (localBigEndian)
    SwapByteOrder(4, src + readPos);

  memcpy(dest, src + readPos, 4);
  return readPos + 4;
}

udResult vcSHP_LoadShpRecord(vcSHP *pSHP, udFile *pFile, uint32_t &offset, vcSHP_Type defaultShape)
{
  udResult result;
  uint8_t id[4] = {};
  uint8_t length[4] = {};
  uint8_t *buffer = nullptr;
  uint32_t readPosition = 0;
  int32_t type = 0;
  int32_t leftBytes = 0;

  vcSHP_Record record;
  UD_ERROR_CHECK(udFile_Read(pFile, id, 4 * sizeof(uint8_t)));
  record.id = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];
  offset += 4;

  UD_ERROR_CHECK(udFile_Read(pFile, length, 4 * sizeof(uint8_t)));
  record.length = (length[0] << 24) | (length[1] << 16) | (length[2] << 8) | length[3];
  record.length *= 2;
  offset += 4;

  buffer = udAllocType(uint8_t, record.length, udAF_Zero);
  UD_ERROR_CHECK(udFile_Read(pFile, buffer, record.length * sizeof(uint8_t)));

  // shape type check
  readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &type, buffer);
  record.shapeType = (vcSHP_Type)type;
  UD_ERROR_IF(record.shapeType != defaultShape, udR_ParseError);

  switch (record.shapeType)
  {
  case vcSHP_Type::Point:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.point.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.point.y, buffer);
  }
  break;
  case vcSHP_Type::Polyline:
  case vcSHP_Type::Polygon:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.poly.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.poly.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.poly.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.poly.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.poly.partsCount, buffer);
    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.poly.pointsCount, buffer);

    record.data.poly.parts = udAllocType(int32_t, record.data.poly.partsCount, udAF_Zero);
    for (int i = 0; i < record.data.poly.partsCount; i++)
      readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.poly.parts[i], buffer);

    record.data.poly.points = udAllocType(udDouble2, record.data.poly.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.poly.pointsCount; i++)
    {
      udDouble2 *p = &record.data.poly.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }

  }
  break;
  case vcSHP_Type::MultiPoint:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPoint.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPoint.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPoint.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPoint.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.multiPoint.pointsCount, buffer);
    record.data.multiPoint.points = udAllocType(udDouble2, record.data.multiPoint.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.multiPoint.pointsCount; i++)
    {
      udDouble2 *p = &record.data.multiPoint.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }
  }
  break;
  case vcSHP_Type::PointZ:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointZ.point.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointZ.point.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointZ.point.z, buffer); // Z
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointZ.MValue, buffer); // M
  }
  break;
  case vcSHP_Type::PolylineZ:
  case vcSHP_Type::PolygonZ:
  case vcSHP_Type::MultiPatch:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyZ.partsCount, buffer);
    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyZ.pointsCount, buffer);

    record.data.polyZ.parts = udAllocType(int32_t, record.data.polyZ.partsCount, udAF_Zero);
    for (int i = 0; i < record.data.polyZ.partsCount; i++)
      readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyZ.parts[i], buffer);

    record.data.polyZ.points = udAllocType(udDouble3, record.data.polyZ.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.polyZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.polyZ.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }

    // z
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.ZRange.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.ZRange.y, buffer);
    for (int i = 0; i < record.data.polyZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.polyZ.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->z, buffer);
    }

    // optional:
    record.data.polyZ.MProvided = false;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.polyZ.pointsCount))
    {
      record.data.polyZ.MProvided = true;
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MRange.x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MRange.y, buffer);

      record.data.polyZ.MValue = udAllocType(double, record.data.polyZ.pointsCount, udAF_Zero);
      for (int i = 0; i < record.data.polyZ.pointsCount; i++)
        readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyZ.MValue[i], buffer);
    }

  }
  break;
  case vcSHP_Type::MultiPointZ:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.pointsCount, buffer);
    record.data.multiPointZ.points = udAllocType(udDouble3, record.data.multiPointZ.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPointZ.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }

    // z
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.ZRange.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.ZRange.y, buffer);
    for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
    {
      udDouble3 *p = &record.data.multiPointZ.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->z, buffer);
    }

    // optional:
    record.data.multiPointZ.MProvided = false;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.multiPointZ.pointsCount))
    {
      record.data.multiPointZ.MProvided = true;
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MRange.x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MRange.y, buffer);

      record.data.multiPointZ.MValue = udAllocType(double, record.data.multiPointZ.pointsCount, udAF_Zero);
      for (int i = 0; i < record.data.multiPointZ.pointsCount; i++)
        readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointZ.MValue[i], buffer);
    }

  }
  break;
  case vcSHP_Type::PointM:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointM.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointM.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.pointM.z, buffer); // M
  }
  break;
  case vcSHP_Type::PolylineM:
  case vcSHP_Type::PolygonM:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyM.partsCount, buffer);
    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyM.pointsCount, buffer);

    record.data.polyM.parts = udAllocType(int32_t, record.data.polyM.partsCount, udAF_Zero);
    for (int i = 0; i < record.data.polyM.partsCount; i++)
      readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.polyM.parts[i], buffer);

    record.data.polyM.points = udAllocType(udDouble2, record.data.polyM.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.polyM.pointsCount; i++)
    {
      udDouble2 *p = &record.data.polyM.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }

    // optional:
    record.data.polyM.MProvided = false;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.polyM.pointsCount))
    {
      record.data.polyM.MProvided = true;
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MRange.x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MRange.y, buffer);

      record.data.polyM.MValue = udAllocType(double, record.data.polyM.pointsCount, udAF_Zero);
      for (int i = 0; i < record.data.polyM.pointsCount; i++)
        readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.polyM.MValue[i], buffer);
    }
  }
  break;
  case vcSHP_Type::MultiPointM:
  {
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MBRmin.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MBRmin.y, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MBRmax.x, buffer);
    readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MBRmax.y, buffer);

    readPosition = UnpackInt32(pSHP->localBigEndian, readPosition, &record.data.multiPointM.pointsCount, buffer);
    record.data.multiPointM.points = udAllocType(udDouble2, record.data.multiPointM.pointsCount, udAF_Zero);
    for (int i = 0; i < record.data.multiPointM.pointsCount; i++)
    {
      udDouble2 *p = &record.data.multiPointM.points[i];
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &p->y, buffer);
    }
    // optional:
    record.data.multiPointM.MProvided = false;
    leftBytes = record.length - readPosition;
    if (leftBytes == 8 * (2 + record.data.multiPointM.pointsCount))
    {
      record.data.multiPointM.MProvided = true;
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MRange.x, buffer);
      readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MRange.y, buffer);

      record.data.multiPointM.MValue = udAllocType(double, record.data.multiPointM.pointsCount, udAF_Zero);
      for (int i = 0; i < record.data.multiPointM.pointsCount; i++)
        readPosition = UnpackDouble(pSHP->localBigEndian, readPosition, &record.data.multiPointM.MValue[i], buffer);
    }
  }
  break;
  default:
  break;
  }

  pSHP->shpRecords.PushBack(record);
  offset += readPosition;

epilogue:
  if (buffer)
    udFree(buffer);

  if (result != udR_Success)
    vcSHP_ReleaseRecord(record);

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
  UD_ERROR_CHECK(udFile_Read(pFile, headerBuffer, 100 * sizeof(uint8_t)));
  // 00 00 27 0a
  UD_ERROR_IF((headerBuffer[0] != 0 || headerBuffer[1] != 0 || headerBuffer[2] != 0x27 || headerBuffer[3] != 0x0a), udR_ParseError);

  header.fileLength = (headerBuffer[24] << 24) | (headerBuffer[25] << 16) | (headerBuffer[26] << 8) | headerBuffer[27];
  if (header.fileLength < UINT_MAX / 2)
    header.fileLength *= 2;
  else
    header.fileLength = (UINT_MAX / 2) * 2;

  header.version = (headerBuffer[31] << 24) | (headerBuffer[30] << 16) | (headerBuffer[29] << 8) | headerBuffer[28];
  header.shapeType = (vcSHP_Type)((int32_t)((headerBuffer[35] << 24) | (headerBuffer[34] << 16) | (headerBuffer[33] << 8) | headerBuffer[32]));

  offset = 36;
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MBRmin.x, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MBRmin.y, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MBRmax.x, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MBRmax.y, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.ZRange.x, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.ZRange.y, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MRange.x, headerBuffer);
  offset = UnpackDouble(pSHP->localBigEndian, offset, &header.MRange.y, headerBuffer);

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

void vcSHP_LoadDbfFile(vcSHP *pSHP, const char *pFilename)
{
  vcDBF_Create(&pSHP->pDBF);
  vcDBF_Load(&pSHP->pDBF, pFilename);
}

udResult vcSHP_LoadFileGroup(vcSHP *pSHP, const char *pFilename)
{
  if (pSHP == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  int i = 1;
  if (*((uint8_t *)&i) == 1)
    pSHP->localBigEndian = false;
  else
    pSHP->localBigEndian = true;

  // load shp file
  udResult result;
  result = vcSHP_LoadShpFile(pSHP, pFilename);
  if (result != udR_Success)
  {
    vcSHP_Release(pSHP);
    return result;
  }

  printf("shape file load successfully %s\n", pFilename);

  char extraFileName[256] = "";
  int32_t nameLen = strlen(pFilename) - 3;
  memcpy(extraFileName, pFilename, nameLen);
  memcpy(extraFileName + nameLen, "dbf", 3);
  vcSHP_LoadDbfFile(pSHP, extraFileName);

  memcpy(extraFileName + nameLen, "prj", 3);
  pSHP->projectionLoad = udFile_Load(extraFileName, &pSHP->WKTString);

  return result;
}

void vcUDP_AddModel(vcState *pProgramState, udProjectNode *pParentNode, vcSHP_Record &record, vcDBF_Record *pDBFRecord)
{
  char buffer[256] = {};
  const char *nodeName = "";
  if (pDBFRecord != nullptr && !pDBFRecord->deleted)
  {
    // for TEST only
    size_t len = strlen(pDBFRecord->pFields[0].pString);
    memcpy(buffer, pDBFRecord->pFields[0].pString, len);
    udStrStripWhiteSpace(buffer);
    nodeName = udTempStr("%s", buffer);
  }
  else
  {
    nodeName = udTempStr("POI %d", record.id);
  }

  udProjectNode *pNode = nullptr;
  udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", nodeName, nullptr, nullptr);

  switch (record.shapeType)
  {
  case vcSHP_Type::Point:
  {

  }
  break;
  case vcSHP_Type::Polyline:
  case vcSHP_Type::Polygon:
  {

  }
  break;
  case vcSHP_Type::MultiPoint:
  {

  }
  break;
  case vcSHP_Type::PointZ:
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &record.data.pointZ.point, 1);
  }
  break;
  case vcSHP_Type::PolylineZ:
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_MultiPoint, record.data.polyZ.points, record.data.polyZ.pointsCount);
  }
  break;
  case vcSHP_Type::PolygonZ:
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, record.data.polyZ.points, record.data.polyZ.pointsCount);
  }
  break;
  case vcSHP_Type::MultiPointZ:
  {

  }
  break;
  case vcSHP_Type::PointM:
  {
    
  }
  break;
  case vcSHP_Type::PolylineM:
  case vcSHP_Type::PolygonM:
  {

  }
  break;
  case vcSHP_Type::MultiPointM:
  {

  }
  break;
  case vcSHP_Type::MultiPatch:
  {

  }
  break;
  default:
  break;
  }
  
}

udResult vcSHP_Load(vcState *pProgramState, const char *pFilename)
{
  udResult result;

  vcSHP shp;
  result = vcSHP_LoadFileGroup(&shp, pFilename);

  printf("load file done. name: %s result: %d\n", pFilename, result);  

  // TODO: create scene from .prj file  
  vcProject_CreateBlankScene(pProgramState, "SHP Import", vcPSZ_NotGeolocated);
  if (shp.projectionLoad == udR_Success)
  {
    printf("%s \n", shp.WKTString);
    udGeoZone zone = {};
    udGeoZone_SetFromWKT(&zone, shp.WKTString);
    vcGIS_ChangeSpace(&pProgramState->geozone, zone);
  }
  
  udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;

  vcDBF_Record *pDBFRecord = nullptr;
  for (size_t i = 0; i < shp.shpRecords.length; ++i)
  {
    vcDBF_GetRecord(shp.pDBF, &pDBFRecord, i);
    vcUDP_AddModel(pProgramState, pParentNode, shp.shpRecords[i], pDBFRecord);
  }

  return result;
}
