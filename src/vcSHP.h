#ifndef vcSHP_h__
#define vcSHP_h__

#include "udResult.h"
#include "udMath.h"
#include "udChunkedArray.h"

#include <unordered_map>

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

  udDouble4 pointZ; // (x, y, Z, M)
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

struct vcSHP_Projection
{

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

  vcSHP_Projection projection;
};

void vcSHP_ReleaseRecord(vcSHP_Record &record);

udResult vcSHP_Load(vcSHP *pSHP, const char *pFilename);
void vcSHP_Release(vcSHP *pSHP);

udResult vcSHP_LoadShpFile(vcSHP *pSHP, const char *pFilename);
void vcSHP_LoadDbfFile(vcSHP *pSHP, const char *pFilename);
void vcSHP_LoadPrjFile(vcSHP *pSHP, const char *pFilename);


#endif
