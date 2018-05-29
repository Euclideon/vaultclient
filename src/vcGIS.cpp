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
  double n0, zeta, eta, chi;
  double zetad, etad, lats;
  double A;
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

  for (int i = 0; i < UDARRAYSIZE(n); ++i)
    n[i] = udPow(n0, i);

  A = (params.a / (n[1] + 1)) * (1 + 1 / 4 * n[2] + 1 / 64 * n[4] + 1 / 256 * n[6] + 25 / 16384 * n[8]);

  zeta = y / (params.k*A);
  eta = x / (params.k*A);
  zetad = zeta;
  etad = eta;

  double beta[] = { 1 / 2 * n[1] - 2 / 3 * n[2] + 37 / 96 * n[3] - 1 / 360 * n[4] - 81 / 512 * n[5],
    1 / 48 * n[2] + 1 / 15 * n[3] - 437 / 1440 * n[4] + 46 / 105 * n[5],
    17 / 480 * n[3] - 37 / 840 * n[4] - 209 / 4480 * n[5] };

  double delta[] = { 2 * n[1] - 2 / 3 * n[2],
    7 / 3 * n[2] - 8 / 5 * n[3],
    56 / 15 * n[3] };

  for (int j = 0; j < 3; ++j)
  {
    zetad -= beta[j] * udSin(2*j*zeta)*udCosh(2*j*eta);
    etad -= beta[j] * udCos(2*j*zeta)*udSinh(2*j*eta);
  }
  chi = udASin(udSin(zetad) / udCosh(etad));
  lats = 0;
  for (int j = 0; j < 3; ++j)
    lats += delta[j] * udSin(2*j*chi);
  latitude = chi + lats;
  longitude = udATan2(udSinh(etad), udCos(zetad));


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
