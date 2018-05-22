#include "vcGIS.h"

bool vcGIS_LocalZoneToLatLong(uint16_t epsgCode, udDouble3 localSpace, udDouble3 *pLatLong)
{
  int zone = 1;
  double meridian = 0;
  int falseNorthing = 0;
  int falseEasting = 0;
  double x, y;
  double f, a, b, e; // ellipse parameters
  double k, n0, mu, B;
  double c, c2, v, t4, t5;
  double latitude, longitude;
  double easting = localSpace[0];
  double northing = localSpace[1];
  double n[5];


  if (epsgCode > 32600 && epsgCode < 32661)
  {
    // WGS84 Northern Hemisphere
    zone = epsgCode - 32600;
    meridian = zone * 6 - 183;
    falseNorthing = 0;
    falseEasting = 500000;
    f = 1 / 298.257223563;
    a = 6378137;
    k = 0.9996;
  }
  else if (epsgCode > 32700 && epsgCode < 32761)
  {
    // WGS84 Southern Hemisphere
    zone = epsgCode - 32700;
    meridian = zone * 6 - 183;
    falseNorthing = 10000000;
    falseEasting = 500000;
    f = 1 / 298.257223563;
    a = 6378137;
    k = 0.9996;
  }
  else
  {
    return false;
  }

  x = easting - falseEasting;
  y = northing - falseNorthing;

  b = a*(1 - f);
  e = (udPow(a, 2) - udPow(b, 2)) / udPow(b, 2);
  n0 = f / (2 - f);

  for (int i = 0; i < 5; ++i)
  {
    n[i] = udPow(n0, i);
  }

  double D[] = { 0, 0, 3 / 2 * n[1] - 27 / 32 * n[3], 0, 21 / 16 * n[2] - 55 / 32 * n[4], 0, 151 / 96 * n[3], 0, 1097 / 512 * n[4] };
  double d = 0;
  for (int i = 2; i < 9; i += 2)
    d += D[i];

  B = b*(1 + n[1] + 5 / 4 * n[2] + 5 / 4 * n[3]);

  mu = UD_PI * x / (2 * (UD_PI / 2) * B *k);

  c = udCos(mu + d);
  c2 = udPow(c, 2);
  v = a*udSqrt((1 + e) / (1 + e*udPow(c,2)));
  x = x / (k*v);
  t4 = udATan2(udSinh(x), c);
  t5 = udATan(udTan(mu + d)*udCos(t4));

  latitude = (1 + e*c2)*(t5 - e / 24 * udPow(x,4) * udTan(mu + d)*(9 - 10 * c2)) - e*(mu + d)*c2;
  longitude = t4 - e / 60 * udPow(x, 3) * c *(10 - (4 * udPow(x, 2)) / c2 + udPow(x, 2) * c2);

  pLatLong->x = UD_RAD2DEG(latitude);
  pLatLong->y = meridian + UD_RAD2DEG(longitude);

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
