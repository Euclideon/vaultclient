#include "udGeoZone.h"

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat)
{
  double lat = !flipFromLongLat ? latLong.x : latLong.y;
  double lon = !flipFromLongLat ? latLong.y : latLong.x;

  int32_t zone = (uint32_t)(udFloor(lon + 186.0) / 6.0);
  if (zone < 1 || zone > 60)
    return udR_ObjectNotFound;

  int32_t sridCode = (lat >= 0) ? zone + 32600 : zone + 32700;
  if (pSRIDCode)
    *pSRIDCode = sridCode;
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
static void SetUTMZoneBounds(udGeoZone *pZone, bool northernHemisphere)
{
  pZone->latLongBoundMin.x = (northernHemisphere) ? 0 : -80;
  pZone->latLongBoundMax.x = (northernHemisphere) ? 84 : 0;
  pZone->latLongBoundMin.y = pZone->meridian - 3;
  pZone->latLongBoundMax.y = pZone->meridian + 3;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static void SetSpheroid(udGeoZone *pZone)
{
  pZone->semiMinorAxis = pZone->semiMajorAxis * (1 - pZone->flattening);
  pZone->thirdFlattening = pZone->flattening / (2 - pZone->flattening);
  pZone->eccentricitySq = pZone->flattening * (2 - pZone->flattening);
  pZone->eccentricity = udSqrt(pZone->eccentricitySq);
  pZone->n[0] = 1.0;
  pZone->n[1] = pZone->thirdFlattening;
  pZone->n[2] = pZone->thirdFlattening * pZone->n[1];
  pZone->n[3] = pZone->thirdFlattening * pZone->n[2];
  pZone->n[4] = pZone->thirdFlattening * pZone->n[3];
  pZone->n[5] = pZone->thirdFlattening * pZone->n[4];
  pZone->n[6] = pZone->thirdFlattening * pZone->n[5];
  pZone->n[7] = pZone->thirdFlattening * pZone->n[6];
  pZone->n[8] = pZone->thirdFlattening * pZone->n[7];
  pZone->n[9] = pZone->thirdFlattening * pZone->n[8];
  pZone->alpha[0] = 1.0 / 2.0 * pZone->n[1] - 2.0 / 3.0 * pZone->n[2] + 5.0 / 16.0 * pZone->n[3] + 41.0 / 80.0 * pZone->n[4] - 127.0 / 288.0 * pZone->n[5] + 7891.0 / 37800.0 * pZone->n[6] + 72161.0 / 387072.0 * pZone->n[7] - 18975107.0 / 50803200.0 * pZone->n[8] + 60193001.0 / 290304000.0 * pZone->n[9];
  pZone->alpha[1] = 13.0 / 48.0 * pZone->n[2] - 3.0 / 5.0 * pZone->n[3] + 557.0 / 1440.0 * pZone->n[4] + 281.0 / 630.0 * pZone->n[5] - 1983433.0 / 1935360.0 * pZone->n[6] + 13769.0 / 28800.0 * pZone->n[7] + 148003883.0 / 174182400.0 * pZone->n[8] - 705286231.0 / 465696000.0 * pZone->n[9];
  pZone->alpha[2] = 61.0 / 240.0 * pZone->n[3] - 103.0 / 140.0 * pZone->n[4] + 15061.0 / 26880.0 * pZone->n[5] + 167603.0 / 181440.0 * pZone->n[6] - 67102379.0 / 29030400.0 * pZone->n[7] + 79682431.0 / 79833600.0 * pZone->n[8] + 6304945039.0 / 2128896000.0 * pZone->n[9];
  pZone->alpha[3] = 49561.0 / 161280.0 * pZone->n[4] - 179.0 / 168.0 * pZone->n[5] + 6601661.0 / 7257600.0 * pZone->n[6] + 97445.0 / 49896.0 * pZone->n[7] - 40176129013.0 / 7664025600.0 * pZone->n[8] + 138471097.0 / 66528000.0 * pZone->n[9];
  pZone->alpha[4] = 34729.0 / 80640.0 * pZone->n[5] - 3418889.0 / 1995840.0 * pZone->n[6] + 14644087.0 / 9123840.0 * pZone->n[7] + 2605413599.0 / 622702080.0 * pZone->n[8] - 31015475399.0 / 2583060480.0 * pZone->n[9];
  pZone->alpha[5] = 212378941.0 / 319334400.0 * pZone->n[6] - 30705481.0 / 10378368.0 * pZone->n[7] + 175214326799.0 / 58118860800.0 * pZone->n[8] + 870492877.0 / 96096000.0 * pZone->n[9];
  pZone->alpha[6] = 1522256789.0 / 1383782400.0 * pZone->n[7] - 16759934899.0 / 3113510400.0 * pZone->n[8] + 1315149374443.0 / 221405184000.0 * pZone->n[9];
  pZone->beta[0] = -1.0 / 2.0 * pZone->n[1] + 2.0 / 3.0 * pZone->n[2] - 37.0 / 96.0 * pZone->n[3] + 1.0 / 360.0 * pZone->n[4] + 81.0 / 512.0 * pZone->n[5] - 96199.0 / 604800.0 * pZone->n[6] + 5406467.0 / 38707200.0 * pZone->n[7] - 7944359.0 / 67737600.0 * pZone->n[8] + 7378753979.0 / 97542144000.0 * pZone->n[9];
  pZone->beta[1] = -1.0 / 48.0 * pZone->n[2] - 1.0 / 15.0 * pZone->n[3] + 437.0 / 1440.0 * pZone->n[4] - 46.0 / 105.0 * pZone->n[5] + 1118711.0 / 3870720.0 * pZone->n[6] - 51841.0 / 1209600.0 * pZone->n[7] - 24749483.0 / 348364800.0 * pZone->n[8] + 115295683.0 / 1397088000.0 * pZone->n[9];
  pZone->beta[2] = -17.0 / 480.0 * pZone->n[3] + 37.0 / 840.0 * pZone->n[4] + 209.0 / 4480.0 * pZone->n[5] - 5569.0 / 90720.0 * pZone->n[6] - 9261899.0 / 58060800.0 * pZone->n[7] + 6457463.0 / 17740800.0 * pZone->n[8] - 2473691167.0 / 9289728000.0 * pZone->n[9];
  pZone->beta[3] = -4397.0 / 161280.0 * pZone->n[4] + 11.0 / 504.0 * pZone->n[5] + 830251.0 / 7257600.0 * pZone->n[6] - 466511.0 / 2494800.0 * pZone->n[7] - 324154477.0 / 7664025600.0 * pZone->n[8] + 937932223.0 / 3891888000.0 * pZone->n[9];
  pZone->beta[4] = -4583.0 / 161280.0 * pZone->n[5] + 108847.0 / 3991680.0 * pZone->n[6] + 8005831.0 / 63866880.0 * pZone->n[7] - 22894433.0 / 124540416.0 * pZone->n[8] - 112731569449.0 / 557941063680.0 * pZone->n[9];
  pZone->beta[5] = -20648693.0 / 638668800.0 * pZone->n[6] + 16363163.0 / 518918400.0 * pZone->n[7] + 2204645983.0 / 12915302400.0 * pZone->n[8] - 4543317553.0 / 18162144000.0 * pZone->n[9];
  pZone->beta[6] = -219941297.0 / 5535129600.0 * pZone->n[7] + 497323811.0 / 12454041600.0 * pZone->n[8] + 79431132943.0 / 332107776000.0 * pZone->n[9];
  pZone->radius = pZone->semiMajorAxis / (1 + pZone->n[1]) * (1.0 + 1.0 / 4.0 * pZone->n[2] + 1.0 / 64.0 * pZone->n[4] + 1.0 / 256.0 * pZone->n[6]);
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode)
{
  if (pZone == nullptr)
    return udR_InvalidParameter_;

  pZone->padding = 0;
  if (sridCode > 32600 && sridCode < 32661)
  {
    // WGS84 Northern Hemisphere
    pZone->zone = sridCode - 32600;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->firstParallel = 0.0;
    pZone->secondParallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257223563;
    pZone->semiMajorAxis = 6378137.0;
    SetSpheroid(pZone);
    SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode > 32700 && sridCode < 32761)
  {
    // WGS84 Southern Hemisphere
    pZone->zone = sridCode - 32700;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->firstParallel = 0.0;
    pZone->secondParallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257223563;
    pZone->semiMajorAxis = 6378137.0;
    SetSpheroid(pZone);
    SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode > 26900 && sridCode < 26924)
  {
    // NAD83 Northern Hemisphere
    pZone->zone = sridCode - 26900;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->firstParallel = 0.0;
    pZone->secondParallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    SetSpheroid(pZone);
    SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode > 28347 && sridCode < 28357)
  {
    // GDA94 Southern Hemisphere (for MGA)
    pZone->zone = sridCode - 28300;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->firstParallel = 0.0;
    pZone->secondParallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    SetSpheroid(pZone);
    SetUTMZoneBounds(pZone, false);
  }
  else //unordered codes
  {
    switch (sridCode)
    {
    case 2222: // TEST Clarke 1866
      pZone->zone = sridCode;
      pZone->meridian = -96.0;
      pZone->parallel = 23.0;
      pZone->firstParallel = 33;
      pZone->secondParallel = 45;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 294.9786982;
      pZone->semiMajorAxis = 6378206.4;
      SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(33.0, -98.0);
      pZone->latLongBoundMax = udDouble2::create(45.0, -94.0);
      break;
    case 2248: // NAD83 / Maryland(ftUS)
      pZone->zone = sridCode;
      pZone->meridian = -77.0;
      pZone->parallel = 37.0 + 2.0 / 3.0;
      pZone->firstParallel = 39.45;
      pZone->secondParallel = 38.3;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 500000;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(37.88, -79.49);
      pZone->latLongBoundMax = udDouble2::create(39.72, -74.98);
      break;
    case 3949: // France Conic Conformal zone 8
      pZone->zone = sridCode;
      pZone->meridian = 3.0;
      pZone->parallel = 49.0;
      pZone->firstParallel = 48.25;
      pZone->secondParallel = 49.75;
      pZone->falseNorthing = 8200000;
      pZone->falseEasting = 1700000;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(37.88, -79.49);
      pZone->latLongBoundMax = udDouble2::create(39.72, -74.98);
      break;
    case 2285: // NAD83 / Washington North (ftUS)
      pZone->zone = sridCode;
      pZone->meridian = -120 - 25.0 / 30.0;
      pZone->parallel = 47.0;
      pZone->firstParallel = 48.0 + 22.0 / 30.0;;
      pZone->secondParallel = 47.5;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 500000.0;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(47.08, -124.75);
      pZone->latLongBoundMax = udDouble2::create(49.0, -117.03);
      break;
    default:
      return udR_ObjectNotFound;
    }
  }
  return udR_Success;
}


// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double latConverge(double t, double td, double e)
{
  double s = udSinh(e * udATanh(e * t / udSqrt(1 + t * t)));
  double fn = t * udSqrt(1 + s * s) - s * udSqrt(1 + t * t) - td;
  double fd = (udSqrt(1 + s * s) * udSqrt(1 + t * t) - s * t) * ((1 - e * e) * udSqrt(1 + t * t)) / (1 + (1 - e * e) * t * t);
  return fn / fd;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double meridonial(double phi, double e)
 {
   double d = e * udSin(phi);
   return udCos(phi) / udSqrt(1 - d * d);
 }

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double conformal(double phi, double e)
 {
   double d = e * udSin(phi);
   return udTan(UD_PI / 4.0 - phi / 2.0) / udPow((1 - d) / (1 + d), e / 2.0);
 }

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_ToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat)
{
  double e = zone.eccentricity;
  double phi = UD_DEG2RAD((!flipFromLongLat) ? latLong.x : latLong.y);
  double omega = UD_DEG2RAD(((!flipFromLongLat) ? latLong.y : latLong.x) - zone.meridian);
  double X, Y;

  if (zone.secondParallel == 0.0)
  {
    double sigma = udSinh(e * udATanh(e * udTan(phi) / udSqrt(1 + udPow(udTan(phi), 2))));
    double tanConformalPhi = udTan(phi) * udSqrt(1 + udPow(sigma, 2)) - sigma * udSqrt(1 + udPow(udTan(phi), 2));

    double u = udATan2(tanConformalPhi, udCos(omega));
    double v = udASinh(udSin(omega) / udSqrt(udPow(tanConformalPhi, 2) + udPow(udCos(omega), 2)));

    X = v;
    for (int i = 0; i < 7; i++)
    {
      double j = (i + 1) * 2.0;
      X += zone.alpha[i] * udCos(j * u) * udSinh(j * v);
    }
    X = X * zone.radius;

    Y = u;
    for (int i = 0; i < 7; i++)
    {
      double j = (i + 1) * 2.0;
      Y += zone.alpha[i] * udSin(j * u) * udCosh(j * v);
    }
    Y = Y * zone.radius;

    return udDouble3::create(zone.scaleFactor * X + zone.falseEasting, zone.scaleFactor * Y + zone.falseNorthing, latLong.z);
  }
  else
  {
    double phi0 = UD_DEG2RAD(zone.parallel);
    double phi1 = UD_DEG2RAD(zone.firstParallel);
    double phi2 = UD_DEG2RAD(zone.secondParallel);
    double m1 = meridonial(phi1, e);
    double m2 = meridonial(phi2, e);
    double t = conformal(phi, e);
    double tOrigin = conformal(phi0, e);
    double t1 = conformal(phi1, e);
    double t2 = conformal(phi2, e);
    double n = (udLogN(m1) - udLogN(m2)) / (udLogN(t1) - udLogN(t2));
    double F = m1 / (n * udPow(t1, n));
    double p0 = zone.semiMajorAxis * F *  udPow(tOrigin, n);
    double p = zone.semiMajorAxis * F *  udPow(t, n);
    X = p * udSin(n * omega);
    Y = p0 - p * udCos(n * omega);
    return udDouble3::create(X + zone.falseEasting, Y + zone.falseNorthing, latLong.z);
  }
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_ToLatLong(const udGeoZone & zone, const udDouble3 &position, bool flipToLongLat)
{
  udDouble3 latLong;
  double e = zone.eccentricity;

  if (zone.secondParallel == 0.0)
  {
    double y = (position.y - zone.falseNorthing) / (zone.radius*zone.scaleFactor);
    double x = (position.x - zone.falseEasting) / (zone.radius*zone.scaleFactor);

    double u = y;
    for (int i = 0; i < 7; i++)
    {
      double j = (i + 1) * 2.0;
      u += zone.beta[i] * udSin(j * y) * udCosh(j * x);
    }

    double v = x;
    for (int i = 0; i < 7; i++)
    {
      double j = (i + 1) * 2.0;
      v += zone.beta[i] * udCos(j * y) * udSinh(j * x);
    }

    double tanConformalPhi = udSin(u) / (udSqrt(udPow(udSinh(v), 2) + udPow(udCos(u), 2)));
    double omega = udATan2(udSinh(v), udCos(u));
    double t = tanConformalPhi;

    for (int i = 0; i < 5; ++i)
      t = t - latConverge(t, tanConformalPhi, e);

    latLong.x = UD_RAD2DEG(udATan(t));
    latLong.y = (zone.meridian + UD_RAD2DEG(omega));
    latLong.z = position.z;
  }
  else
  {
    double y = position.y - zone.falseNorthing;
    double x = position.x - zone.falseEasting;
    double phi0 = UD_DEG2RAD(zone.parallel);
    double phi1 = UD_DEG2RAD(zone.firstParallel);
    double phi2 = UD_DEG2RAD(zone.secondParallel);
    double m1 = meridonial(phi1, e);
    double m2 = meridonial(phi2, e);
    double tOrigin = conformal(phi0, e);
    double t1 = conformal(phi1, e);
    double t2 = conformal(phi2, e);
    double n = (udLogN(m1) - udLogN(m2)) / (udLogN(t1) - udLogN(t2));
    double F = m1 / (n * udPow(t1, n));
    double p0 = zone.semiMajorAxis * F *  udPow(tOrigin, n);
    double p = udSqrt(x * x + (p0 - y) * (p0 - y));
    double theta = udATan2(x, p0 - y);
    double t = udPow(p / (zone.semiMajorAxis * F), 1 / n);
    double phi = UD_HALF_PI - 2.0 * udATan(t);
    for (int i = 0; i < 5; ++i)
      phi = UD_HALF_PI - 2 * udATan(t * udPow((1 - e * udSin(phi)) / (1 + e * udSin(phi)), e / 2.0));

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(theta / n) + zone.meridian;
    latLong.z = position.z;
  }
  return (!flipToLongLat) ? latLong : udDouble3::create(latLong.y, latLong.x, latLong.z);
}

