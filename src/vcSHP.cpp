#include "vcSHP.h"

#include "udMath.h"
#include "udChunkedArray.h"
#include "udFile.h"
#include "vcDBF.h"
#include "udStringUtil.h"

enum vcSHPConstant
{
  vcSHP_MemoryCache = 1024 * 1000 * 100, // 100M
};

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

struct vcSHP_Record
{
  int32_t id;
  uint32_t length;
  vcSHPType shapeType;

  udDouble4 MBRMin;
  udDouble4 MBRMax;

  int32_t partsCount;
  int32_t pointsCount;

  int32_t *parts;
  udDouble3 *points; // x, y, z
  double *MValues;

  void Init()
  {
    id = 0;
    length = 0;

    MBRMin = udDouble4::zero();
    MBRMax = udDouble4::zero();

    partsCount = 0;
    pointsCount = 0;

    parts = nullptr;
    points = nullptr;
    MValues = nullptr;
  }
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
  udFree(pRecord->parts);
  udFree(pRecord->points);
  udFree(pRecord->MValues);
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

void ReadRange(udDouble2 *min, udDouble2 *max, uint8_t **ppSrc)
{
  memcpy(&min->x, ppSrc, sizeof(double));
  *ppSrc += sizeof(double);
  memcpy(&min->y, ppSrc, sizeof(double));
  *ppSrc += sizeof(double);
  memcpy(&max->x, ppSrc, sizeof(double));
  *ppSrc += sizeof(double);
  memcpy(&max->y, ppSrc, sizeof(double));
  *ppSrc += sizeof(double);
}

udResult vcSHP_LoadShpRecord(vcSHP *pSHP, uint8_t **ppReadPosition, vcSHPType defaultShape, int32_t *pLeftLength, int32_t *pRecordLength)
{
  udResult result = udR_Success;
  uint8_t id[4] = {};
  uint8_t length[4] = {};
  vcSHP_Record record = {};
  uint8_t *pRecordEnd = nullptr;

  record.Init();
  
  UD_ERROR_IF(*pLeftLength < int32_t(sizeof(int32_t) * 2), udR_ReadFailure);
  memcpy(id, *ppReadPosition, sizeof(int32_t));
  memcpy(length, *ppReadPosition + sizeof(int32_t), sizeof(int32_t));

  record.id = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];

  // check if there is a full record
  record.length = (length[0] << 24) | (length[1] << 16) | (length[2] << 8) | length[3];
  record.length *= 2; //the file stores the length in shorts.

  *pRecordLength = sizeof(int32_t) * 2 + record.length;
  UD_ERROR_IF(*pLeftLength < *pRecordLength, udR_ReadFailure);
  pRecordEnd = *ppReadPosition + *pRecordLength;
  *ppReadPosition += sizeof(int32_t) * 2; // skip id and length

  // shape type check
  memcpy(&record.shapeType, *ppReadPosition, sizeof(int32_t));
  *ppReadPosition += sizeof(int32_t);
  UD_ERROR_IF(record.shapeType != defaultShape, udR_ParseError);

  if (record.shapeType == vcSHPType_Point || record.shapeType == vcSHPType_PointZ || record.shapeType == vcSHPType_PointM)
  {
    record.pointsCount = 1;
    record.points = udAllocType(udDouble3, 1, udAF_Zero);
    record.MValues = nullptr;

    record.points[0] = udDouble3::zero();
    memcpy(&record.points[0].x, *ppReadPosition, sizeof(double));
    *ppReadPosition += sizeof(double);
    memcpy(&record.points[0].y, *ppReadPosition, sizeof(double));
    *ppReadPosition += sizeof(double);

    if (record.shapeType == vcSHPType_PointZ)
    {
      memcpy(&record.points[0].z, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
    }
    
    if (record.shapeType == vcSHPType_PointZ || record.shapeType == vcSHPType_PointM)
    {
      record.MValues = udAllocType(double, 1, udAF_Zero);
      memcpy(&record.MValues[0], *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
    }
  }
  else
  {
    //read MBR
    if (record.shapeType == vcSHPType_Polyline || record.shapeType == vcSHPType_Polygon || record.shapeType == vcSHPType_MultiPoint || record.shapeType == vcSHPType_PolylineZ || record.shapeType == vcSHPType_PolygonZ || record.shapeType == vcSHPType_MultiPointZ || record.shapeType == vcSHPType_PolylineM || record.shapeType == vcSHPType_PolygonM || record.shapeType == vcSHPType_MultiPointM || record.shapeType == vcSHPType_MultiPatch)
    {
      memcpy(&record.MBRMin.x, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      memcpy(&record.MBRMin.y, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      memcpy(&record.MBRMax.x, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      memcpy(&record.MBRMax.y, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
    }

    // read parts count
    if (record.shapeType == vcSHPType_Polyline || record.shapeType == vcSHPType_Polygon || record.shapeType == vcSHPType_PolylineZ || record.shapeType == vcSHPType_PolygonZ || record.shapeType == vcSHPType_PolylineM || record.shapeType == vcSHPType_PolygonM || record.shapeType == vcSHPType_MultiPatch)
    {
      memcpy(&record.partsCount, *ppReadPosition, sizeof(int32_t));
      *ppReadPosition += sizeof(int32_t);
    }

    // read points count
    memcpy(&record.pointsCount, *ppReadPosition, sizeof(int32_t));
    *ppReadPosition += sizeof(int32_t);

    // read parts
    if (record.shapeType == vcSHPType_Polyline || record.shapeType == vcSHPType_Polygon || record.shapeType == vcSHPType_PolylineZ || record.shapeType == vcSHPType_PolygonZ || record.shapeType == vcSHPType_PolylineM || record.shapeType == vcSHPType_PolygonM || record.shapeType == vcSHPType_MultiPatch)
    {
      record.parts = udAllocType(int32_t, record.partsCount, udAF_Zero);
      UD_ERROR_NULL(record.parts, udR_MemoryAllocationFailure);
      memcpy(record.parts, *ppReadPosition, sizeof(int32_t) * record.partsCount);
      *ppReadPosition += sizeof(int32_t) * record.partsCount;
    }

    // read points x, y
    record.points = udAllocType(udDouble3, record.pointsCount, udAF_Zero);
    UD_ERROR_NULL(record.points, udR_MemoryAllocationFailure);
    for (int i = 0; i < record.pointsCount; i++)
    {
      record.points[i] = udDouble3::zero();
      memcpy(&record.points[i].x, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      memcpy(&record.points[i].y, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
    }

    // read z value
    if (record.shapeType == vcSHPType_PolylineZ || record.shapeType == vcSHPType_PolygonZ || record.shapeType == vcSHPType_MultiPointZ || record.shapeType == vcSHPType_MultiPatch)
    {
      memcpy(&record.MBRMin.z, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      memcpy(&record.MBRMax.z, *ppReadPosition, sizeof(double));
      *ppReadPosition += sizeof(double);
      for (int i = 0; i < record.pointsCount; i++)
      {
        memcpy(&record.points[i].z, *ppReadPosition, sizeof(double));
        *ppReadPosition += sizeof(double);
      }

    }

    // read m value
    if (record.shapeType == vcSHPType_PolylineZ || record.shapeType == vcSHPType_PolygonZ || record.shapeType == vcSHPType_MultiPointZ || record.shapeType == vcSHPType_PolylineM || record.shapeType == vcSHPType_PolygonM || record.shapeType == vcSHPType_MultiPointM || record.shapeType == vcSHPType_MultiPatch)
    {
      size_t leftBytes = pRecordEnd - *ppReadPosition;
      if (leftBytes == sizeof(double) * 2 + sizeof(double)*record.pointsCount)
      {
        memcpy(&record.MBRMin.w, *ppReadPosition, sizeof(double));
        *ppReadPosition += sizeof(double);
        memcpy(&record.MBRMax.w, *ppReadPosition, sizeof(double));
        *ppReadPosition += sizeof(double);

        record.MValues = udAllocType(double, record.pointsCount, udAF_Zero);
        UD_ERROR_NULL(record.MValues, udR_MemoryAllocationFailure);
        for (int i = 0; i < record.pointsCount; i++)
        {
          memcpy(&record.MValues[i], *ppReadPosition, sizeof(double));
          *ppReadPosition += sizeof(double);
        }
      }
    }
  }

  pSHP->shpRecords.PushBack(record);

epilogue:

  // set the pLeftLength to next record
  if (result == udR_Success)
    *pLeftLength -= *pRecordLength;
  else
    vcSHP_ReleaseRecord(&record);

  return result;
}

udResult vcSHP_LoadShpFile(vcSHP *pSHP, const char *pFilename)
{
  udResult result = udR_Success;

  udFile *pFile = nullptr;
  uint8_t *headerBuffer = nullptr;
  uint8_t *tempPosition = nullptr;
  uint8_t *cache0 = nullptr;
  uint8_t *cache1 = nullptr;
  int32_t totalRecordLength = 0;
  uint8_t *pReadPosition = nullptr;

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

  tempPosition = headerBuffer + 36;
  memcpy(&header.MBRmin.x, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.MBRmin.y, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.MBRmax.x, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.MBRmax.y, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.ZRange.x, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.ZRange.y, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.MRange.x, tempPosition, sizeof(double));
  tempPosition += sizeof(double);
  memcpy(&header.MRange.y, tempPosition, sizeof(double));

  UD_ERROR_CHECK(pSHP->shpRecords.Init(32));

  totalRecordLength = header.fileLength - 100;
  if (totalRecordLength < vcSHP_MemoryCache)
  {
    int32_t recordLength = 0;
    int32_t leftLength = totalRecordLength;
    cache0 = udAllocType(uint8_t, totalRecordLength, udAF_Zero);
    UD_ERROR_CHECK(udFile_Read(pFile, cache0, totalRecordLength));

    pReadPosition = cache0;

    while (leftLength > 0)
    {
      result = vcSHP_LoadShpRecord(pSHP, &pReadPosition, header.shapeType, &leftLength, &recordLength);
      UD_ERROR_CHECK(result);
    }

  }
  else
  {
    int32_t bytesToRead = totalRecordLength;
    uint8_t **ppCurrentCache = nullptr;
    int32_t recordLength = 0;
    int32_t leftLength = 0;
    uint8_t *lastCache = nullptr;
    int32_t lastCacheSize = 0;

    while (bytesToRead > 0)
    {
      if (ppCurrentCache != &cache0)
        ppCurrentCache = &cache0;
      else
        ppCurrentCache = &cache1;

      //Check if the cache size needs to be expanded
      int32_t cacheSize = vcSHP_MemoryCache;
      if (result == udR_ReadFailure && recordLength > vcSHP_MemoryCache) // record larger than vcSHP_MemoryCache
        cacheSize = recordLength;
      if (leftLength + bytesToRead < vcSHP_MemoryCache)
        cacheSize = leftLength + bytesToRead;

      //allocate cache
      *ppCurrentCache = udAllocType(uint8_t, cacheSize, udAF_Zero);
      UD_ERROR_NULL(*ppCurrentCache, udR_MemoryAllocationFailure);

      //copy left record(part of)
      if (leftLength > 0)
        memcpy(*ppCurrentCache, lastCache + lastCacheSize - leftLength, leftLength);

      // release last cache
      if (lastCache == cache0)
        udFree(cache0);
      else
        udFree(cache1);
      lastCache = nullptr;
      lastCacheSize = 0;

      //read some bytes
      UD_ERROR_CHECK(udFile_Read(pFile, *ppCurrentCache + leftLength, cacheSize-leftLength));
      bytesToRead -= (cacheSize - leftLength);

      //read cache to records
      pReadPosition = *ppCurrentCache;
      leftLength = cacheSize;
      while (leftLength > 0)
      {
        result = vcSHP_LoadShpRecord(pSHP, &pReadPosition, header.shapeType, &leftLength, &recordLength);
        if (result != udR_Success)
          break;

        recordLength = 0;
      }

      // read error
      if (result != udR_Success && result != udR_ReadFailure)
        goto epilogue;

      // to continue read
      lastCache = *ppCurrentCache;
      lastCacheSize = cacheSize;
    }

  }


epilogue:
  udFile_Close(&pFile);
  udFree(headerBuffer);
  udFree(cache0);
  udFree(cache1);

  return result;
}

udResult vcSHP_LoadFileGroup(vcSHP *pSHP, const char *pFileName)
{
  if (pSHP == nullptr || pFileName == nullptr)
    return udR_InvalidParameter_;

  udFilename loadFile(pFileName);
  if(!loadFile.HasFilename())
    return udR_InvalidParameter_;

  // load shp file
  udResult result = udR_Failure_;
  UD_ERROR_CHECK(vcSHP_LoadShpFile(pSHP, pFileName));

  // load dbf file
  loadFile.SetExtension(".dbf");
  vcDBF_Load(&pSHP->pDBF, loadFile.GetPath());

  // load prj file
  loadFile.SetExtension(".prj");
  pSHP->WKTString = nullptr;
  pSHP->projectionLoad = udFile_Load(loadFile.GetPath(), &pSHP->WKTString);

epilogue:
  return result;
}

void vcSHP_AddModel(vcState *pProgramState, const udGeoZone& sourceZone, udProjectNode *pParentNode, vcSHP_Record *pRecord, vcDBF_Record *pDBFRecord, uint16_t stringIndex)
{
  char buffer[256] = {};
  if (pDBFRecord != nullptr && !pDBFRecord->deleted && stringIndex != (uint16_t)(-1))
  {
    udStrcpy(buffer, pDBFRecord->pFields[stringIndex].pString);
    udStrStripWhiteSpace(buffer);
  }
  else
  {
    udSprintf(buffer, "POI %d", pRecord->id);
  }

  udProjectNode *pNode = nullptr;
  udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParentNode, "POI", buffer, nullptr, nullptr);

  switch (pRecord->shapeType)
  {
  case vcSHPType_Point:
  case vcSHPType_PointZ:
  case vcSHPType_PointM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, sourceZone, udPGT_Point, &pRecord->points[0], 1);
  break;
  case vcSHPType_Polyline:
  case vcSHPType_PolylineZ:
  case vcSHPType_PolylineM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, sourceZone, udPGT_LineString, pRecord->points, pRecord->pointsCount);
  break;
  case vcSHPType_Polygon:
  case vcSHPType_PolygonZ:
  case vcSHPType_PolygonM:
  case vcSHPType_MultiPatch:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, sourceZone, udPGT_Polygon, pRecord->points, pRecord->pointsCount);
  break;
  case vcSHPType_MultiPoint:
  case vcSHPType_MultiPointZ:
  case vcSHPType_MultiPointM:
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, sourceZone, udPGT_MultiPoint, pRecord->points, pRecord->pointsCount);
  break;
  case vcSHPType_Empty:
  break;
  }
  
}

udResult vcSHP_Load(vcState *pProgramState, const char *pFilename)
{
  vcSHP shp = {};
  udProjectNode *pParentNode = nullptr;
  uint16_t stringIndex = 0;
  vcDBF_Record *pDBFRecord = nullptr;
  udGeoZone sourceZone = {};  

  udResult result = udR_Failure_;
  UD_ERROR_CHECK(vcSHP_LoadFileGroup(&shp, pFilename));

  if (shp.projectionLoad == udR_Success)
  {
    udGeoZone_SetFromWKT(&sourceZone, shp.WKTString);
    if (sourceZone.srid == 0 && udStrEqual(sourceZone.datumName, "D_WGS_1984") && sourceZone.datum == udGZGD_WGS84)
      udGeoZone_SetFromSRID(&sourceZone, vcPSZ_StandardGeoJSON);
  }
  else
  {
    udGeoZone_SetFromSRID(&sourceZone, vcPSZ_NotGeolocated);
  }

  if(sourceZone.srid != 0)
    vcProject_CreateBlankScene(pProgramState, "SHP Import", vcPSZ_WGS84ECEF);
  else
    vcProject_CreateBlankScene(pProgramState, "SHP Import", vcPSZ_NotGeolocated);

  pParentNode = pProgramState->activeProject.pRoot;
  stringIndex = vcSHP_GetFirstDBFStringFieldIndex(shp.pDBF);
  for (size_t i = 0; i < shp.shpRecords.length; ++i)
  {
    if (stringIndex != (uint16_t)(-1))
      vcDBF_GetRecord(shp.pDBF, &pDBFRecord, (uint32_t)i);

    vcSHP_AddModel(pProgramState, sourceZone, pParentNode, &shp.shpRecords[i], pDBFRecord, stringIndex);
  }

epilogue:
  vcSHP_Release(&shp);

  return result;
}
