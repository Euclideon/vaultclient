#include "vcGIS.h"
struct vcGIS_EPSGParameters
{
  int zone;
  double meridian;
  int falseNorthing;
  int falseEasting;
  double f;
  double a;
  double k;
};

bool vcGIS_PopulateEPSGParameters(vcGIS_EPSGParameters *pParams, uint16_t epsgCode)
{
  if (pParams == nullptr)
    return false;

  if (epsgCode > 32600 && epsgCode < 32661)
  {
    // WGS84 Northern Hemisphere
    pParams->zone = epsgCode - 32600;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 0;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257223563;
    pParams->a = 6378137;
    pParams->k = 0.9996;
  }
  else if (epsgCode > 32700 && epsgCode < 32761)
  {
    // WGS84 Southern Hemisphere
    pParams->zone = epsgCode - 32700;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 10000000;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257223563;
    pParams->a = 6378137;
    pParams->k = 0.9996;
  }
  else
  {
    return false;
  }

  return true;
}

bool vcGIS_LocalZoneToLatLong(uint16_t epsgCode, udDouble3 localSpace, udDouble3 *pLatLong)
{

  double x, y;
  double b, e; // ellipse parameters
  double n0, mu, B;
  double c, c2, v, t4, t5;
  double latitude, longitude;
  double easting = localSpace[0];
  double northing = localSpace[1];
  double n[5];

  vcGIS_EPSGParameters params;

  vcGIS_PopulateEPSGParameters(&params, epsgCode);

  x = easting - params.falseEasting;
  y = northing - params.falseNorthing;

  b = params.a*(1 - params.f);
  e = (udPow(params.a, 2) - udPow(b, 2)) / udPow(b, 2);
  n0 = params.f / (2 - params.f);

  for (int i = 0; i < 5; ++i)
    n[i] = udPow(n0, i);

  double D[] = { 0, 0, 3 / 2 * n[1] - 27 / 32 * n[3], 0, 21 / 16 * n[2] - 55 / 32 * n[4], 0, 151 / 96 * n[3], 0, 1097 / 512 * n[4] };
  double d = 0;
  for (int i = 2; i < 9; i += 2)
    d += D[i];

  B = b*(1 + n[1] + 5 / 4 * n[2] + 5 / 4 * n[3]);

  mu = UD_PI * x / (2 * (UD_PI / 2) * B *params.k);

  c = udCos(mu + d);
  c2 = udPow(c, 2);
  v = params.a*udSqrt((1 + e) / (1 + e*udPow(c,2)));
  x = x / (params.k*v);
  t4 = udATan2(udSinh(x), c);
  t5 = udATan(udTan(mu + d)*udCos(t4));

  latitude = (1 + e*c2)*(t5 - e / 24 * udPow(x,4) * udTan(mu + d)*(9 - 10 * c2)) - e*(mu + d)*c2;
  longitude = t4 - e / 60 * udPow(x, 3) * c *(10 - (4 * udPow(x, 2)) / c2 + udPow(x, 2) * c2);

  pLatLong->x = UD_RAD2DEG(latitude);
  pLatLong->y = params.meridian + UD_RAD2DEG(longitude);

  return true;
}

bool vcGIS_LatLongToLocalZone(uint16_t epsgCode, udDouble3 latLong, udDouble3 *pLocalSpace)
{
  udUnused(epsgCode);
  udUnused(latLong);
  udUnused(pLocalSpace);

  return false;
}

bool vcGIS_LatLongToSlippyTileIDs(udInt2 *pTileIDs, udDouble2 latLong, int zoomLevel)
{
  if (pTileIDs == nullptr)
    return false;

  pTileIDs->x = (int)udFloor((latLong.x + 180.0) / 360.0 * udPow(2.0, zoomLevel)); // Long
  pTileIDs->y = (int)(udFloor((1.0 - udLogN(udTan(latLong.y * UD_PI / 180.0) + 1.0 / udCos(latLong.y * UD_PI / 180.0)) / UD_PI) / 2.0 * udPow(2.0, zoomLevel))); //Lat

  return true;
}

bool vcGIS_SlippyTileIDsToLatLong(udDouble2 *pLatLong, udInt2 tileID, int zoomLevel)
{
  if (pLatLong == nullptr)
    return false;

  double n = UD_PI - 2.0 * UD_PI * tileID.y / udPow(2.0, zoomLevel);

  pLatLong->x = tileID.x / udPow(2.0, zoomLevel) * 360.0 - 180;
  pLatLong->y = 180.0 / UD_PI * udATan(0.5 * (exp(n) - exp(-n)));

  return true;
}
