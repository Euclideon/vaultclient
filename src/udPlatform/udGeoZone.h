#ifndef UDGEOZONE_H
#define UDGEOZONE_H

#include "udMath.h"

struct udGeoZone
{
  udDouble2 latLongBoundMin;
  udDouble2 latLongBoundMax;
  double meridian;
  double parallel;
  double flattening;
  double semiMajorAxis;
  double semiMinorAxis;
  double thirdFlattening;
  double eccentricity;
  double eccentricitySq;
  double radius;
  double scaleFactor;
  double n[10];
  double alpha[7];
  double beta[7];
  double firstParallel;
  double secondParallel;
  double falseNorthing;
  double falseEasting;
  int32_t zone;
  int32_t padding;
};

// Find an appropriate SRID code for a given lat/long within UTM/WGS84 (for example as a default value)
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat = false);

// Set the zone structure parameters from a given srid code
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode);

// Convert a point from lat/long to the cartesian coordinate system defined by the zone
udDouble3 udGeoZone_ToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat = false);

// Convert a point from the cartesian coordinate system defined by the zone to lat/long
udDouble3 udGeoZone_ToLatLong(const udGeoZone &zone, const udDouble3 &position, bool flipToLongLat = false);

#endif // UDGEOZONE_H
