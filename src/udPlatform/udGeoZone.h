#ifndef UDGEOZONE_H
#define UDGEOZONE_H

#include "udMath.h"

enum udGeoZoneEllipsoid
{
  udGZE_WGS84,          //EPSG:7030
  udGZE_Airy1830,       //EPSG:7001
  udGZE_AiryModified,   //EPSG:7002
  udGZE_Bessel1841,     //EPSG:7004
  udGZE_Clarke1866,     //EPSG:7008
  udGZE_Clarke1880IGN,  //EPSG:7011
  udGZE_GRS80,          //EPSG:7019
  udGZE_Intl1924,       //EPSG:7022 (Same numbers as Hayford)
  udGZE_WGS72,          //EPSG:7043
  udGZE_CGCS2000,       //EPSG:1024

  udGZE_Count
};

enum udGeoZoneGeodeticDatum
{
  udGZGD_WGS84,      //EPSG:4326
  udGZGD_ED50,       //EPSG:4230
  udGZGD_ETRS89,     //EPSG:4258
  udGZGD_TM75,       //EPSG:4300
  udGZGD_NAD27,      //EPSG:4267
  udGZGD_NAD83,      //EPSG:4269
  udGZGD_NTF,        //EPSG:4275
  udGZGD_OSGB36,     //EPSG:4277
  udGZGD_Potsdam,    //EPSG:4746
  udGZGD_Tokyo,      //EPSG:7414
  udGZGD_WGS72,      //EPSG:4322
  udGZGD_JGD2000,    //EPSG:4612
  udGZGD_JGD2011,    //EPSG:6668
  udGZGD_GDA94,      //EPSG:4283
  udGZGD_RGF93,      //EPSG:4171
  udGZGD_NAD83_HARN, //EPSG:4152
  udGZGD_CGCS2000,   //EPSG:4490

  udGZGD_Count
};

enum udGeoZoneProjectionType
{
  udGZPT_TransverseMercator,
  udGZPT_LambertConformalConic2SP,

  udGZPT_Count
};

struct udGeoZone
{
  udGeoZoneGeodeticDatum datum;
  udGeoZoneProjectionType projection;
  udDouble2 latLongBoundMin;
  udDouble2 latLongBoundMax;
  double meridian;
  double parallel;            // Parallel of origin for Transverse Mercator
  double flattening;
  double semiMajorAxis;
  double semiMinorAxis;
  double thirdFlattening;
  double eccentricity;
  double eccentricitySq;
  double radius;
  double scaleFactor;
  double n[10];
  double alpha[9];
  double beta[9];
  double firstParallel;
  double secondParallel;
  double falseNorthing;
  double falseEasting;
  double unitMetreScale;      // 1.0 for metres, 0.3048006096012192 for feet
  int32_t zone;
  int32_t srid;
  char datumShortName[16];
  char datumName[64];
  char zoneName[64]; // Only 33 characters required for longest known name "Japan Plane Rectangular CS XVIII"
};

// Find an appropriate SRID code for a given lat/long within UTM/WGS84 (for example as a default value)
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Set the zone structure parameters from a given srid code
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode);

// Get geozone from well known text
udResult udGeoZone_SetFromWKT(udGeoZone *pZone, const char *pWKT);

// Get the Well Known Text for a zone
udResult udGeoZone_GetWellKnownText(const char **ppWKT, const udGeoZone &zone);

// Convert a point from lat/long to the cartesian coordinate system defined by the zone
udDouble3 udGeoZone_ToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Convert a point from the cartesian coordinate system defined by the zone to lat/long
udDouble3 udGeoZone_ToLatLong(const udGeoZone &zone, const udDouble3 &position, bool flipToLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Convert a lat/long pair in one datum to another datum
udDouble3 udGeoZone_ConvertDatum(udDouble3 latLong, udGeoZoneGeodeticDatum currentDatum, udGeoZoneGeodeticDatum newDatum, bool flipToLongLat = false);

// Transform a point from one zone to another
udDouble3 udGeoZone_TransformPoint(const udDouble3 &point, const udGeoZone &sourceZone, const udGeoZone &destZone);

// Transform a matrix from one zone to another
udDouble4x4 udGeoZone_TransformMatrix(const udDouble4x4 &matrix, const udGeoZone &sourceZone, const udGeoZone &destZone);

#endif // UDGEOZONE_H
