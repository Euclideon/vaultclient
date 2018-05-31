#include "vcGIS.h"

#include "udPlatform/udPlatformUtil.h"

struct vcGIS_SRIDParameters
{
  int zone;
  double meridian;
  int falseNorthing;
  int falseEasting;
  double f;
  double a;
  double k;
  char hemisphere;
};

bool vcGIS_PopulateSRIDParameters(vcGIS_SRIDParameters *pParams, uint16_t sridCode)
{
  if (pParams == nullptr)
    return false;

  if (sridCode > 32600 && sridCode < 32661)
  {
    // WGS84 Northern Hemisphere
    pParams->zone = sridCode - 32600;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 0;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257223563;
    pParams->a = 6378137;
    pParams->k = 0.9996;
    pParams->hemisphere = 'N';
  }
  else if (sridCode > 32700 && sridCode < 32761)
  {
    // WGS84 Southern Hemisphere
    pParams->zone = sridCode - 32700;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 10000000;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257223563;
    pParams->a = 6378137;
    pParams->k = 0.9996;
    pParams->hemisphere = 'S';
  }
  else if (sridCode > 26900 && sridCode < 26924)
  {
    // NAD83 Northern Hemisphere
    pParams->zone = sridCode - 26900;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 0;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257222101;
    pParams->a = 6378137;
    pParams->k = 0.9996;
    pParams->hemisphere = 'N';
  }
  else if (sridCode > 28347 && sridCode < 28357)
  {
    // GDA94 Southern Hemisphere (for MGA)
    pParams->zone = sridCode - 28300;
    pParams->meridian = pParams->zone * 6 - 183;
    pParams->falseNorthing = 10000000;
    pParams->falseEasting = 500000;
    pParams->f = 1 / 298.257222101;
    pParams->a = 6378137;
    pParams->k = 0.9996;
    pParams->hemisphere = 'S';
  }
  else
  {
    return false;
  }

  return true;
}

bool vcGIS_LocalZoneToLatLong(uint16_t sridCode, udDouble3 localSpace, udDouble3 *pLatLong)
{
  double arc;
  double mu;
  double ei;
  double ca, cb, cc, cd;
  double n0, r0, t0;
  double _a1, _a2;
  double dd0;
  double Q0;
  double lof1, lof2, lof3;
  double phi1;
  double fact1, fact2, fact3, fact4;
  double zoneCM;

  double easting = localSpace[0];
  double northing = localSpace[1];

  vcGIS_SRIDParameters params;

  if (!vcGIS_PopulateSRIDParameters(&params, sridCode))
    return false;

  double b = params.a * (1 - params.f);
  double e = udSqrt((udPow(params.a, 2) - udPow(b, 2)) / udPow(params.a, 2));
  double e1sq = (udPow(params.a, 2) - udPow(b, 2)) / udPow(b, 2);

  if (params.hemisphere == 'S')
    northing = params.falseNorthing - northing;

  // Set Variables
  arc = northing / params.k;
  mu = arc / (params.a * (1 - udPow(e, 2) / 4.0 - 3 * udPow(e, 4) / 64.0 - 5 * udPow(e, 6) / 256.0));

  ei = (1 - udPow((1 - e * e), (1 / 2.0))) / (1 + udPow((1 - e * e), (1 / 2.0)));

  ca = 3 * ei / 2 - 27 * udPow(ei, 3) / 32.0;
  cb = 21 * udPow(ei, 2) / 16 - 55 * udPow(ei, 4) / 32;
  cc = 151 * udPow(ei, 3) / 96;
  cd = 1097 * udPow(ei, 4) / 512;

  phi1 = mu + ca * udSin(2 * mu) + cb * udSin(4 * mu) + cc * udSin(6 * mu) + cd * udSin(8 * mu);
  n0 = params.a / udPow((1 - udPow((e * udSin(phi1)), 2)), (1 / 2.0));
  r0 = params.a * (1 - e * e) / udPow((1 - udPow((e * udSin(phi1)), 2)), (3 / 2.0));
  _a1 = 500000 - easting;
  dd0 = _a1 / (n0 * params.k);
  t0 = udPow(udTan(phi1), 2);
  Q0 = e1sq * udPow(udCos(phi1), 2);

  fact1 = n0 * udTan(phi1) / r0;
  fact2 = dd0 * dd0 / 2;
  fact3 = (5 + 3 * t0 + 10 * Q0 - 4 * Q0 * Q0 - 9 * e1sq) * udPow(dd0, 4) / 24;
  fact4 = (61 + 90 * t0 + 298 * Q0 + 45 * t0 * t0 - 252 * e1sq - 3 * Q0 * Q0) * udPow(dd0, 6) / 720;

  lof1 = _a1 / (n0 * params.k);
  lof2 = (1 + 2 * t0 + Q0) * udPow(dd0, 3) / 6.0;
  lof3 = (5 - 2 * Q0 + 28 * t0 - 3 * udPow(Q0, 2) + 8 * e1sq + 24 * udPow(t0, 2)) * udPow(dd0, 5) / 120;
  _a2 = (lof1 - lof2 + lof3) / udCos(phi1);

  // Latitude
  pLatLong->x = 180 * (phi1 - fact1 * (fact2 + fact3 + fact4)) / UD_PI;
  if (params.hemisphere == 'S')
    pLatLong->x = -pLatLong->x;

  // Longitude
  if (params.zone > 0)
    zoneCM = 6 * params.zone - 183.0;
  else
    zoneCM = 3.0;

  pLatLong->y = zoneCM - UD_RAD2DEG(_a2);

  return true;
}

bool vcGIS_LatLongToLocalZone(uint16_t sridCode, udDouble3 latLong, udDouble3 *pLocalSpace)
{
  double latitude = latLong[0];
  double longitude = latLong[1];

  vcGIS_SRIDParameters params;

  if (!vcGIS_PopulateSRIDParameters(&params, sridCode))
    return false;

  double a = params.a;
  double b = a * (1 - params.f);

  double k0 = params.k;

  // Lat Lon to UTM variables
  double equatorialRadius = a;
  double polarRadius = b;

  // eccentricity
  double e = udSqrt(1 - udPow(polarRadius / equatorialRadius, 2));
  double e1sq = e * e / (1 - e * e);

  double rho = 6368573.744; // r curv 1 WGS84 only
  double nu = 6389236.914; // r curv 2 WGS84 only

  // Calculate Meridional Arc Length / Meridional Arc
  double S = 5103266.421;
  double A0 = 6367449.146;
  double B0 = 16038.42955;
  double C0 = 16.83261333;
  double D0 = 0.021984404;
  double E0 = 0.000312705;

  // Calculation Constants
  // Delta Long
  double p = -0.483084;
  double sin1 = 4.84814E-06;

  // Coefficients for UTM Coordinates
  double K1 = 5101225.115;
  double K2 = 3750.291596;
  double K3 = 1.397608151;
  double K4 = 214839.3105;
  double K5 = -2.995382942;
  double A6 = -1.00541E-07;

  // Set Variables
  latitude = UD_DEG2RAD(latitude);
  rho = equatorialRadius * (1 - e * e) / udPow(1 - udPow(e * udSin(latitude), 2), 3 / 2.0);
  nu = equatorialRadius / udPow(1 - udPow(e * udSin(latitude), 2), (1 / 2.0));

  double var1 = params.zone;
  double var2 = (6 * var1) - 183;
  double var3 = longitude - var2;

  p = var3 * 3600 / 10000;

  S = A0 * latitude - B0 * udSin(2 * latitude) + C0 * udSin(4 * latitude) - D0 * udSin(6 * latitude) + E0 * udSin(8 * latitude);

  K1 = S * k0;
  K2 = nu * udSin(latitude) * udCos(latitude) * udPow(sin1, 2) * k0 * (100000000) / 2;
  K3 = ((udPow(sin1, 4) * nu * udSin(latitude) * udPow(udCos(latitude), 3)) / 24) * (5 - udPow(udTan(latitude), 2) + 9 * e1sq * udPow(udCos(latitude), 2) + 4 * udPow(e1sq, 2) * udPow(udCos(latitude), 4)) * k0 * (10000000000000000L);
  K4 = nu * udCos(latitude) * sin1 * k0 * 10000;
  K5 = udPow(sin1 * udCos(latitude), 3) * (nu / 6) * (1 - udPow(udTan(latitude), 2) + e1sq * udPow(udCos(latitude), 2)) * k0 * 1000000000000L;

  A6 = (udPow(p * sin1, 6) * nu * udSin(latitude) * udPow(udCos(latitude), 5) / 720) * (61 - 58 * udPow(udTan(latitude), 2) + udPow(udTan(latitude), 4) + 270 * e1sq * udPow(udCos(latitude), 2) - 330 * e1sq * udPow(udSin(latitude), 2)) * k0 * (1E+24);

  //NORTHING
  double northing = K1 + K2 * p * p + K3 * udPow(p, 4);
  if (latitude < 0.0)
    northing = params.falseNorthing + northing;

  pLocalSpace->x = params.falseEasting + (K4 * p + K5 * udPow(p, 3));
  pLocalSpace->y = northing;

  return false;
}

bool vcGIS_LatLongToSlippyTileIDs(udInt2 *pTileIDs, udDouble2 longLat, int zoomLevel)
{
  if (pTileIDs == nullptr)
    return false;

  pTileIDs->x = (int)udFloor((longLat.x + 180.0) / 360.0 * udPow(2.0, zoomLevel)); // Long
  pTileIDs->y = (int)(udFloor((1.0 - udLogN(udTan(longLat.y * UD_PI / 180.0) + 1.0 / udCos(longLat.y * UD_PI / 180.0)) / UD_PI) / 2.0 * udPow(2.0, zoomLevel))); //Lat

  return true;
}

bool vcGIS_SlippyTileIDsToLatLong(udDouble2 *pLongLat, udInt2 tileID, int zoomLevel)
{
  if (pLongLat == nullptr)
    return false;

  pLongLat->x = tileID.x / udPow(2.0, zoomLevel) * 360.0 - 180; // Long
  pLongLat->y = 180.0 / UD_PI * udATan(udSinh(UD_PI * (1 - 2 * tileID.y / (udPow(2.0, zoomLevel))))); // Lat

  return true;
}



//////////////////////////////// IBM IMPLEMENTATION FROM https://www.ibm.com/developerworks/library/j-coordconvert/index.html
/*
* Author: Sami Salkosuo, sami.salkosuo@fi.ibm.com
*
* (c) Copyright IBM Corp. 2007
*/
