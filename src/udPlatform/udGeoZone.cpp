#include "udGeoZone.h"

// Stored as g_udGZ_StdTransforms FROM WGS84 to the given datum
struct udGeoZoneGeodeticTransform
{
  udGeoZoneEllipsoid ellipsoid;
  double paramsHelmert7[7]; //TO-WGS84 as { Tx, Ty, Tz, Rx, Ry, Rz, DS }
};

struct udGeoZoneEllipsoidInfo
{
  double semiMajorAxis;
  double flattening;
};

const udGeoZoneEllipsoidInfo g_udGZ_StdEllipsoids[] = {
  { 6378137.000, 1.0 / 298.257223563 }, // udGZE_WGS84
  { 6377563.396, 1.0 / 299.3249646 }, // udGZE_Airy1830
  { 6377340.189, 1.0 / 299.3249646 }, // udGZE_AiryModified
  { 6377397.155, 1.0 / 299.1528128 }, // udGZE_Bessel1841
  { 6378206.400, 1.0 / 294.978698214 }, // udGZE_Clarke1866
  { 6378249.200, 1.0 / 293.466021294 }, // udGZE_Clarke1880IGN
  { 6378137.000, 1.0 / 298.257222101 }, // udGZE_GRS80
  { 6378388.000, 1.0 / 297.00 }, // udGZE_Intl1924
  { 6378135.000, 1.0 / 298.26 }, // udGZE_WGS72
};

// Table from https://github.com/chrisveness/geodesy/blob/master/latlon-ellipsoidal.js
const udGeoZoneGeodeticTransform g_udGZ_StdTransforms[] = {
  { udGZE_WGS84, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_WGS84
  { udGZE_Intl1924, { -87.0, -98.0, -121.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_ED50
  { udGZE_GRS80, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZDD_ETRS89
  { udGZE_AiryModified, { 482.5, -130.6, 564.6, -1.042, -0.214, -0.631, 8.15 } }, // udGZGD_Ireland1975
  { udGZE_Clarke1866, { -8.0, 160.0, 176.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_NAD27
  { udGZE_GRS80, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_NAD83
  { udGZE_Clarke1880IGN, { -168.0, -60.0, 320.0, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_NTF
  { udGZE_Airy1830, { 446.448, -125.157, 542.06, 0.1502, 0.247, 0.8421, -20.4894 } }, // udGZGD_OSGB36
  { udGZE_Bessel1841, { 582.0, 105.0, 414.0, -1.04, -0.35, 3.08, 8.3 } }, // udGZGD_Potsdam
  { udGZE_Bessel1841, { -146.414, 507.337, 680.507, 0.0, 0.0, 0.0, 0.0 } }, // udGZGD_TokyoJapan
  { udGZE_WGS72, { 0.0, 0.0, 4.5, 0.0, 0.0, 0.554, 0.2263 } }, // udGZGD_WGS72
};

udDouble3 udGeoZone_LatLongToGeocentric(udDouble3 latLong, const udGeoZoneEllipsoidInfo &ellipsoid)
{
  double lat = UD_DEG2RAD(latLong.x);
  double lon = UD_DEG2RAD(latLong.y);
  double h = latLong.z;

  double eSq = ellipsoid.flattening * (2 - ellipsoid.flattening);
  double v = ellipsoid.semiMajorAxis / udSqrt(1 - eSq * udSin(lat) * udSin(lat));

  double x = (v + h) * udCos(lat) * udCos(lon);
  double y = (v + h) * udCos(lat) * udSin(lon);
  double z = (v * (1 - eSq) + h) * udSin(lat);

  return udDouble3::create(x, y, z);
}

udDouble3 udGeoZone_LatLongFromGeocentric(udDouble3 geoCentric, const udGeoZoneEllipsoidInfo &ellipsoid)
{
  double semiMinorAxis = ellipsoid.semiMajorAxis * (1 - ellipsoid.flattening);

  double eSq = ellipsoid.flattening * (2 - ellipsoid.flattening);
  double e3 = eSq / (1 - eSq);
  double p = udSqrt(geoCentric.x * geoCentric.x + geoCentric.y * geoCentric.y);
  double q = udATan2(geoCentric.z * ellipsoid.semiMajorAxis, p * semiMinorAxis);

  double lat = udATan2(geoCentric.z + e3 * semiMinorAxis * udSin(q) * udSin(q) * udSin(q), p - eSq * ellipsoid.semiMajorAxis * udCos(q) * udCos(q) * udCos(q));
  double lon = udATan2(geoCentric.y, geoCentric.x);

  double v = ellipsoid.semiMajorAxis / udSqrt(1 - eSq * udSin(lat) * udSin(lat)); // length of the normal terminated by the minor axis
  double h = (p / udCos(lat)) - v;

  // This is an alternative method to generate the lat- don't merge until we confirm which one is 'correct'
  double lat2 = 0;
  double lat2Temp = 1;
  while (lat2 != lat2Temp)
  {
    lat2 = lat2Temp;
    lat2Temp = udATan((geoCentric.z + eSq*v*udSin(lat2)) / udSqrt(geoCentric.x * geoCentric.x + geoCentric.y * geoCentric.y));
  }

  return udDouble3::create(UD_RAD2DEG(lat2), UD_RAD2DEG(lon), h);
}

udDouble3 udGeoZone_ApplyTransform(udDouble3 geoCentric, const udGeoZoneGeodeticTransform &transform)
{
  // transform parameters
  double rx = UD_DEG2RAD(transform.paramsHelmert7[3] / 3600.0); // x-rotation: normalise arcseconds to radians
  double ry = UD_DEG2RAD(transform.paramsHelmert7[4] / 3600.0); // y-rotation: normalise arcseconds to radians
  double rz = UD_DEG2RAD(transform.paramsHelmert7[5] / 3600.0); // z-rotation: normalise arcseconds to radians
  double ds = transform.paramsHelmert7[6] / 1000000.0 + 1.0; // scale: normalise parts-per-million to (s+1)

  // apply transform
  double x2 = transform.paramsHelmert7[0] + ds * (geoCentric.x - geoCentric.y*rz + geoCentric.z*ry);
  double y2 = transform.paramsHelmert7[1] + ds * (geoCentric.x*rz + geoCentric.y - geoCentric.z*rx);
  double z2 = transform.paramsHelmert7[2] + ds * (-geoCentric.x*ry + geoCentric.y*rx + geoCentric.z);

  return udDouble3::create(x2, y2, z2);
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, August 2018
udDouble3 udGeoZone_ConvertDatum(udDouble3 latLong, udGeoZoneGeodeticDatum currentDatum, udGeoZoneGeodeticDatum newDatum, bool flipToLongLat /*= false*/)
{
  udDouble3 oldLatLon = latLong;
  udGeoZoneGeodeticDatum oldDatum = currentDatum;

  if (flipToLongLat)
  {
    oldLatLon.x = latLong.y;
    oldLatLon.y = latLong.x;
  }

  const udGeoZoneGeodeticTransform *pTransform = nullptr;
  udGeoZoneGeodeticTransform transform;

  if (currentDatum != udGZGD_WGS84 && newDatum != udGZGD_WGS84)
  {
    oldLatLon = udGeoZone_ConvertDatum(oldLatLon, currentDatum, udGZGD_WGS84);
    oldDatum = udGZGD_WGS84;
  }

  if (newDatum == udGZGD_WGS84) // converting to WGS84; use inverse transform
  {
    pTransform = &g_udGZ_StdTransforms[oldDatum];
  }
  else // converting from WGS84
  {
    transform = g_udGZ_StdTransforms[newDatum];
    for (int i = 0; i < 7; i++)
      transform.paramsHelmert7[i] = -transform.paramsHelmert7[i];
    pTransform = &transform;
  }

  // Chain functions and get result
  udDouble3 geocentric = udGeoZone_LatLongToGeocentric(oldLatLon, g_udGZ_StdEllipsoids[pTransform->ellipsoid]);
  udDouble3 transformed = udGeoZone_ApplyTransform(geocentric, *pTransform);
  udDouble3 newLatLong = udGeoZone_LatLongFromGeocentric(transformed, g_udGZ_StdEllipsoids[pTransform->ellipsoid]);

  if (flipToLongLat)
    return udDouble3::create(newLatLong.y, newLatLong.x, newLatLong.z);
  else
    return newLatLong;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  double lat = !flipFromLongLat ? latLong.x : latLong.y;
  double lon = !flipFromLongLat ? latLong.y : latLong.x;

  if (datum != udGZGD_WGS84)
  {
    udDouble3 fixedLatLon = udGeoZone_ConvertDatum(udDouble3::create(lat, lon, 0.f), datum, udGZGD_WGS84);
    lat = fixedLatLon.x;
    lon = fixedLatLon.y;
  }

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
static void udGeoZone_SetUTMZoneBounds(udGeoZone *pZone, bool northernHemisphere)
{
  pZone->latLongBoundMin.x = (northernHemisphere) ? 0 : -80;
  pZone->latLongBoundMax.x = (northernHemisphere) ? 84 : 0;
  pZone->latLongBoundMin.y = pZone->meridian - 3;
  pZone->latLongBoundMax.y = pZone->meridian + 3;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static void udGeoZone_SetSpheroid(udGeoZone *pZone)
{
  if (pZone->semiMajorAxis == 0.0 && pZone->flattening == 0.0 && pZone->datum < udGZGD_Count)
  {
    udGeoZoneEllipsoid ellipsoidID = g_udGZ_StdTransforms[pZone->datum].ellipsoid;
    if (ellipsoidID < udGZE_Count)
    {
      const udGeoZoneEllipsoidInfo &ellipsoidInfo = g_udGZ_StdEllipsoids[ellipsoidID];
      pZone->semiMajorAxis = ellipsoidInfo.semiMajorAxis;
      pZone->flattening = ellipsoidInfo.flattening;
    }
  }

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

  // The alpha and beta constant below can be cross-referenced with https://geographiclib.sourceforge.io/html/transversemercator.html
  pZone->alpha[0] = 1.0 / 2.0 * pZone->n[1] - 2.0 / 3.0 * pZone->n[2] + 5.0 / 16.0 * pZone->n[3] + 41.0 / 180.0 * pZone->n[4] - 127.0 / 288.0 * pZone->n[5] + 7891.0 / 37800.0 * pZone->n[6] + 72161.0 / 387072.0 * pZone->n[7] - 18975107.0 / 50803200.0 * pZone->n[8] + 60193001.0 / 290304000.0 * pZone->n[9];
  pZone->alpha[1] = 13.0 / 48.0 * pZone->n[2] - 3.0 / 5.0 * pZone->n[3] + 557.0 / 1440.0 *pZone->n[4] + 281.0 / 630.0 * pZone->n[5] - 1983433.0 / 1935360.0 * pZone->n[6] + 13769.0 / 28800.0 * pZone->n[7] + 148003883.0 / 174182400.0 * pZone->n[8] - 705286231.0 / 465696000.0 * pZone->n[9];
  pZone->alpha[2] = 61.0 / 240.0 * pZone->n[3] - 103.0 / 140.0 * pZone->n[4] + 15061.0 / 26880.0 * pZone->n[5] + 167603.0 / 181440.0 * pZone->n[6] - 67102379.0 / 29030400.0 * pZone->n[7] + 79682431.0 / 79833600.0 * pZone->n[8] + 6304945039.0 / 2128896000.0 * pZone->n[9];
  pZone->alpha[3] = 49561.0 / 161280.0 * pZone->n[4] - 179.0 / 168.0 * pZone->n[5] + 6601661.0 / 7257600.0 * pZone->n[6] + 97445.0 / 49896.0 * pZone->n[7] - 40176129013.0 / 7664025600.0 * pZone->n[8] + 138471097.0 / 66528000.0 * pZone->n[9];
  pZone->alpha[4] = 34729.0 / 80640.0 * pZone->n[5] - 3418889.0 / 1995840.0 * pZone->n[6] + 14644087.0 / 9123840.0 * pZone->n[7] + 2605413599.0 / 622702080.0 * pZone->n[8] - 31015475399.0 / 2583060480.0 * pZone->n[9];
  pZone->alpha[5] = 212378941.0 / 319334400.0 * pZone->n[6] - 30705481.0 / 10378368.0 * pZone->n[7] + 175214326799.0 / 58118860800.0 * pZone->n[8] + 870492877.0 / 96096000.0 * pZone->n[9];
  pZone->alpha[6] = 1522256789.0 / 1383782400.0 * pZone->n[7] - 16759934899.0 / 3113510400.0 * pZone->n[8] + 1315149374443.0 / 221405184000.0 * pZone->n[9];
  pZone->alpha[7] = 1424729850961.0 / 743921418240.0 * pZone->n[8] - 256783708069.0 / 25204608000.0 * pZone->n[9];
  pZone->alpha[8] = 21091646195357.0 / 6080126976000.0 * pZone->n[9];

  pZone->beta[0] = -1.0 / 2.0 * pZone->n[1] + 2.0 / 3.0 * pZone->n[2] - 37.0 / 96.0 * pZone->n[3] + 1.0 / 360.0 * pZone->n[4] + 81.0 / 512.0 * pZone->n[5] - 96199.0 / 604800.0 * pZone->n[6] + 5406467.0 / 38707200.0 * pZone->n[7] - 7944359.0 / 67737600.0 * pZone->n[8] + 7378753979.0 / 97542144000.0 * pZone->n[9];
  pZone->beta[1] = -1.0 / 48.0 * pZone->n[2] - 1.0 / 15.0 * pZone->n[3] + 437.0 / 1440.0 * pZone->n[4] - 46.0 / 105.0 * pZone->n[5] + 1118711.0 / 3870720.0 * pZone->n[6] - 51841.0 / 1209600.0 * pZone->n[7] - 24749483.0 / 348364800.0 * pZone->n[8] + 115295683.0 / 1397088000.0 * pZone->n[9];
  pZone->beta[2] = -17.0 / 480.0 * pZone->n[3] + 37.0 / 840.0 * pZone->n[4] + 209.0 / 4480.0 * pZone->n[5] - 5569.0 / 90720.0 * pZone->n[6] - 9261899.0 / 58060800.0 * pZone->n[7] + 6457463.0 / 17740800.0 * pZone->n[8] - 2473691167.0 / 9289728000.0 * pZone->n[9];
  pZone->beta[3] = -4397.0 / 161280.0 * pZone->n[4] + 11.0 / 504.0 * pZone->n[5] + 830251.0 / 7257600.0 * pZone->n[6] - 466511.0 / 2494800.0 * pZone->n[7] - 324154477.0 / 7664025600.0 * pZone->n[8] + 937932223.0 / 3891888000.0 * pZone->n[9];
  pZone->beta[4] = -4583.0 / 161280.0 * pZone->n[5] + 108847.0 / 3991680.0 * pZone->n[6] + 8005831.0 / 63866880.0 * pZone->n[7] - 22894433.0 / 124540416.0 * pZone->n[8] - 112731569449.0 / 557941063680.0 * pZone->n[9];
  pZone->beta[5] = -20648693.0 / 638668800.0 * pZone->n[6] + 16363163.0 / 518918400.0 * pZone->n[7] + 2204645983.0 / 12915302400.0 * pZone->n[8] - 4543317553.0 / 18162144000.0 * pZone->n[9];
  pZone->beta[6] = -219941297.0 / 5535129600.0 * pZone->n[7] + 497323811.0 / 12454041600.0 * pZone->n[8] + 79431132943.0 / 332107776000.0 * pZone->n[9];
  pZone->beta[7] = -191773887257.0 / 3719607091200.0 * pZone->n[8] + 17822319343.0 / 336825216000.0 * pZone->n[9];
  pZone->beta[8] = -11025641854267.0 / 158083301376000.0 * pZone->n[9];

  pZone->radius = pZone->semiMajorAxis / (1 + pZone->n[1]) * (1.0 + 1.0 / 4.0 * pZone->n[2] + 1.0 / 64.0 *pZone->n[4] + 1.0 / 256.0 * pZone->n[6] + 25.0 / 16384.0 * pZone->n[8]);

  if (pZone->firstParallel == 0.0 && pZone->secondParallel == 0.0 && pZone->parallel != 0) // Latitude of origin for Transverse Mercator
  {
    double l0 = UD_DEG2RAD(pZone->parallel);
    double q0 = udASinh(udTan(l0)) - (pZone->eccentricity * udATanh(pZone->eccentricity * udSin(l0)));
    double b0 = udATan(udSinh(q0));

    double u = b0;
    for (size_t i = 0; i < UDARRAYSIZE(pZone->alpha); i++)
    {
      double j = (i + 1) * 2.0;
      u += pZone->alpha[i] * udSin(j * b0);
    }

    pZone->firstParallel = u * pZone->radius;
  }
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode)
{
  if (pZone == nullptr)
    return udR_InvalidParameter_;

  if (sridCode == -1) // Special case to help with unit tests
    udGeoZone_SetSpheroid(pZone);
  else
    memset(pZone, 0, sizeof(udGeoZone));

  if (sridCode > 32600 && sridCode < 32661)
  {
    // WGS84 Northern Hemisphere
    pZone->datum = udGZGD_WGS84;
    pZone->zone = sridCode - 32600;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode > 32700 && sridCode < 32761)
  {
    // WGS84 Southern Hemisphere
    pZone->zone = sridCode - 32700;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257223563;
    pZone->semiMajorAxis = 6378137.0;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode > 26900 && sridCode < 26924)
  {
    // NAD83 Northern Hemisphere
    pZone->zone = sridCode - 26900;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode > 28347 && sridCode < 28357)
  {
    // GDA94 Southern Hemisphere (for MGA)
    pZone->zone = sridCode - 28300;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode >= 2443 && sridCode <= 2461)
  {
    // JGD2000 / Japan Plane Rectangular CS I through XIX

    const udDouble2 jprcsRegions[] = { // Meridian, Latitude Of Origin
      { 129.5, 33.0 },
      { 131.0, 33.0 },
      { 132.0 + 1.0 / 6.0, 36.0 },
      { 133.5, 33.0 },
      { 134.0 + 1.0 / 3.0, 36.0 },
      { 136.0, 36.0 },
      { 137.0 + 1.0 / 6.0, 36.0 },
      { 138.5, 36.0 },
      { 139.0 + 5.0 / 6.0, 36.0 },
      { 140.0 + 5.0 / 6.0, 40.0 },
      { 140.25, 44.0 },
      { 142.25, 44.0 },
      { 144.25, 44.0 },
      { 142.0, 26.0 },
      { 127.5, 26.0 },
      { 124.0, 26.0 },
      { 131.0, 26.0 },
      { 136.0, 20.0 },
      { 154.0, 26.0 }
    };

    pZone->zone = sridCode - 2443;
    pZone->meridian = jprcsRegions[pZone->zone].x;
    pZone->parallel = jprcsRegions[pZone->zone].y;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 0;
    pZone->scaleFactor = 0.9999;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode >= 3942 && sridCode <= 3950)
  {
    // France Conic Conformal zones
    pZone->zone = sridCode - 3942;
    pZone->meridian = 3.0;
    pZone->parallel = 42.0 + pZone->zone;
    pZone->firstParallel = pZone->parallel - 0.75;
    pZone->secondParallel = pZone->parallel + 0.75;
    pZone->falseNorthing = 1200000 + 1000000 * pZone->zone;
    pZone->falseEasting = 1700000;
    pZone->scaleFactor = 1.0;
    pZone->flattening = 1 / 298.257222101;
    pZone->semiMajorAxis = 6378137.0;
    udGeoZone_SetSpheroid(pZone);
    pZone->latLongBoundMin = udDouble2::create(pZone->parallel - 1.0, -2.0); // Longitude here not perfect, differs greatly in different zones
    pZone->latLongBoundMax = udDouble2::create(pZone->parallel + 1.0, 10.00);
  }
  else //unordered codes
  {
    switch (sridCode)
    {
    case 2238: // NAD83 / Florida North (ftUS)
      pZone->zone = sridCode;
      pZone->meridian = -84.5;
      pZone->parallel = 29.0;
      pZone->firstParallel = 30.75;
      pZone->secondParallel = 29.0 + 175.0 / 300.0;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1968500.0 * 0.3048006096012192;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(29.28, -87.64);
      pZone->latLongBoundMax = udDouble2::create(31.0, -82.05);
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
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(37.88, -79.49);
      pZone->latLongBoundMax = udDouble2::create(39.72, -74.98);
      break;
    case 2250: // NAD83 / Maryland(ftUS)
      pZone->zone = sridCode;
      pZone->meridian = -70.5;
      pZone->parallel = 41.0;
      pZone->firstParallel = 41.0 + 145.0 / 300.0;
      pZone->secondParallel = 41.0 + 85.0 / 300.0;;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 500000;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(41.2, -70.87);
      pZone->latLongBoundMax = udDouble2::create(41.51, -69.9);
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
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(47.08, -124.75);
      pZone->latLongBoundMax = udDouble2::create(49.0, -117.03);
      break;
    case 2771: // NAD83 / California zone 6 (ftUS)
      pZone->zone = sridCode;
      pZone->meridian = -116.25;
      pZone->parallel = 32.0 + 1.0 / 6.0;
      pZone->firstParallel = 33.0 + 265.0 / 300.0;
      pZone->secondParallel = 32.0 + 235.0 / 300.0;
      pZone->falseNorthing = 500000.0;
      pZone->falseEasting = 2000000.0;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(32.51, -118.14);
      pZone->latLongBoundMax = udDouble2::create(34.08, -114.43);
      break;
    case 3112: // GDA94 / Geoscience Australia Lambert
      pZone->zone = sridCode;
      pZone->meridian = 134;
      pZone->parallel = 0;
      pZone->firstParallel = -18;
      pZone->secondParallel = -36;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 1.0;
      pZone->flattening = 1 / 298.257222101;
      pZone->semiMajorAxis = 6378137.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-45, 108.0000);
      pZone->latLongBoundMax = udDouble2::create(-10, 155.0000);
      break;
    case 27700: // OSGB 1936 / British National Grid
      pZone->datum = udGZGD_OSGB36;
      pZone->zone = sridCode;
      pZone->meridian = -2;
      pZone->parallel = 49.0;
      pZone->falseNorthing = -100000;
      pZone->falseEasting = 400000;
      pZone->scaleFactor = 0.9996012717;
      pZone->flattening = 1 / 299.3249646;
      pZone->semiMajorAxis = 6377563.396;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-7.5600, 49.9600);
      pZone->latLongBoundMax = udDouble2::create(1.7800, 60.8400);
      break;
    default:
      return udR_ObjectNotFound;
    }
  }
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
// Root finding with Newton's method to calculate conformal latitude.
static double udGeoZone_LCCLatConverge(double t, double td, double e)
{
  double s = udSinh(e * udATanh(e * t / udSqrt(1 + t * t)));
  double fn = t * udSqrt(1 + s * s) - s * udSqrt(1 + t * t) - td;
  double fd = (udSqrt(1 + s * s) * udSqrt(1 + t * t) - s * t) * ((1 - e * e) * udSqrt(1 + t * t)) / (1 + (1 - e * e) * t * t);
  return fn / fd;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double udGeoZone_LCCMeridonial(double phi, double e)
 {
   double d = e * udSin(phi);
   return udCos(phi) / udSqrt(1 - d * d);
 }

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double udGeoZone_LCCConformal(double phi, double e)
 {
   double d = e * udSin(phi);
   return udTan(UD_PI / 4.0 - phi / 2.0) / udPow((1 - d) / (1 + d), e / 2.0);
 }

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_ToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  double e = zone.eccentricity;
  double phi = ((!flipFromLongLat) ? latLong.x : latLong.y);
  double omega = ((!flipFromLongLat) ? latLong.y : latLong.x);
  double X, Y;

  if (datum != zone.datum)
  {
    udDouble3 convertedLatLong = udGeoZone_ConvertDatum(udDouble3::create(phi, omega, latLong.z), datum, zone.datum);
    phi = convertedLatLong.x;
    omega = convertedLatLong.y;
  }

  phi = UD_DEG2RAD(phi);
  omega = UD_DEG2RAD(omega - zone.meridian);

  if (zone.secondParallel == 0.0)
  {
    // UTM rather than Lambert CC which requires two parallels for calculation

    double sigma = udSinh(e * udATanh(e * udTan(phi) / udSqrt(1 + udPow(udTan(phi), 2))));
    double tanConformalPhi = udTan(phi) * udSqrt(1 + udPow(sigma, 2)) - sigma * udSqrt(1 + udPow(udTan(phi), 2));

    double v = udASinh(udSin(omega) / udSqrt(udPow(tanConformalPhi, 2) + udPow(udCos(omega), 2)));
    double u = udATan2(tanConformalPhi, udCos(omega));

    X = v;
    for (size_t i = 0; i < UDARRAYSIZE(zone.alpha); i++)
    {
      double j = (i + 1) * 2.0;
      X += zone.alpha[i] * udCos(j * u) * udSinh(j * v);
    }
    X = X * zone.radius;

    Y = u;
    for (size_t i = 0; i < UDARRAYSIZE(zone.alpha); i++)
    {
      double j = (i + 1) * 2.0;
      Y += zone.alpha[i] * udSin(j * u) * udCosh(j * v);
    }
    Y = Y * zone.radius;

    return udDouble3::create(zone.scaleFactor * X + zone.falseEasting, zone.scaleFactor * (Y - zone.firstParallel) + zone.falseNorthing, latLong.z);
  }
  else
  {
    // If two standard parallels, project onto Lambert Conformal Conic
    double phi0 = UD_DEG2RAD(zone.parallel);
    double phi1 = UD_DEG2RAD(zone.firstParallel);
    double phi2 = UD_DEG2RAD(zone.secondParallel);
    double m1 = udGeoZone_LCCMeridonial(phi1, e);
    double m2 = udGeoZone_LCCMeridonial(phi2, e);
    double t = udGeoZone_LCCConformal(phi, e);
    double tOrigin = udGeoZone_LCCConformal(phi0, e);
    double t1 = udGeoZone_LCCConformal(phi1, e);
    double t2 = udGeoZone_LCCConformal(phi2, e);
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
udDouble3 udGeoZone_ToLatLong(const udGeoZone &zone, const udDouble3 &position, bool flipToLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  udDouble3 latLong;
  double e = zone.eccentricity;

  if (zone.secondParallel == 0.0)
  {
    double y = (zone.firstParallel * zone.scaleFactor + position.y - zone.falseNorthing) / (zone.radius * zone.scaleFactor);
    double x = (position.x - zone.falseEasting) / (zone.radius*zone.scaleFactor);

    double u = y;
    for (size_t i = 0; i < UDARRAYSIZE(zone.beta); i++)
    {
      double j = (i + 1) * 2.0;
      u += zone.beta[i] * udSin(j * y) * udCosh(j * x);
    }

    double v = x;
    for (size_t i = 0; i < UDARRAYSIZE(zone.beta); i++)
    {
      double j = (i + 1) * 2.0;
      v += zone.beta[i] * udCos(j * y) * udSinh(j * x);
    }

    double tanConformalPhi = udSin(u) / (udSqrt(udPow(udSinh(v), 2) + udPow(udCos(u), 2)));
    double omega = udATan2(udSinh(v), udCos(u));
    double t = tanConformalPhi;

    for (int i = 0; i < 5; ++i)
      t = t - udGeoZone_LCCLatConverge(t, tanConformalPhi, e);

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
    double m1 = udGeoZone_LCCMeridonial(phi1, e);
    double m2 = udGeoZone_LCCMeridonial(phi2, e);
    double tOrigin = udGeoZone_LCCConformal(phi0, e);
    double t1 = udGeoZone_LCCConformal(phi1, e);
    double t2 = udGeoZone_LCCConformal(phi2, e);
    double n = (udLogN(m1) - udLogN(m2)) / (udLogN(t1) - udLogN(t2));
    double F = m1 / (n * udPow(t1, n));
    double p0 = zone.semiMajorAxis * F *  udPow(tOrigin, n);
    double p = udSqrt(x * x + (p0 - y) * (p0 - y)); // This is r' in the EPSG specs- it must have the same sign as n
    if (n < 0)
      p = -p;

    double theta = udATan(x / (p0 - y));
    double t = udPow(p / (zone.semiMajorAxis * F), 1 / n);
    double phi = UD_HALF_PI - 2.0 * udATan(t);
    for (int i = 0; i < 5; ++i)
      phi = UD_HALF_PI - 2 * udATan(t * udPow((1 - e * udSin(phi)) / (1 + e * udSin(phi)), e / 2.0));

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(theta / n) + zone.meridian;
    latLong.z = position.z;
  }

  if (datum != zone.datum)
    latLong = udGeoZone_ConvertDatum(latLong, zone.datum, datum);

  return (!flipToLongLat) ? latLong : udDouble3::create(latLong.y, latLong.x, latLong.z);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
udDouble3 udGeoZone_TransformPoint(const udDouble3 &point, const udGeoZone &sourceZone, const udGeoZone &destZone)
{
  if (sourceZone.zone == destZone.zone)
    return point;

  udDouble3 latlon = udGeoZone_ToLatLong(sourceZone, point);
  return udGeoZone_ToCartesian(destZone, latlon);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
udDouble4x4 udGeoZone_TransformMatrix(const udDouble4x4 &matrix, const udGeoZone &sourceZone, const udGeoZone &destZone)
{
  if (sourceZone.zone == destZone.zone)
    return matrix;

  // A very large model will lead to inaccuracies, so in this case, scale it down
  double accuracyScale = (matrix.a[0] > 1000) ? matrix.a[0] / 1000.0 : 1.0;

  udDouble3 llO = udGeoZone_ToLatLong(sourceZone, matrix.axis.t.toVector3());
  udDouble3 llX = udGeoZone_ToLatLong(sourceZone, matrix.axis.t.toVector3() + (matrix.axis.x.toVector3() * accuracyScale));
  udDouble3 llY = udGeoZone_ToLatLong(sourceZone, matrix.axis.t.toVector3() + (matrix.axis.y.toVector3() * accuracyScale));
  udDouble3 llZ = udGeoZone_ToLatLong(sourceZone, matrix.axis.t.toVector3() + (matrix.axis.z.toVector3() * accuracyScale));

  accuracyScale = 1.0 / accuracyScale;
  udDouble3 czO = udGeoZone_ToCartesian(destZone, llO);
  udDouble3 czX = (udGeoZone_ToCartesian(destZone, llX) - czO) * accuracyScale;
  udDouble3 czY = (udGeoZone_ToCartesian(destZone, llY) - czO) * accuracyScale;
  udDouble3 czZ = (udGeoZone_ToCartesian(destZone, llZ) - czO) * accuracyScale;

  // TODO: Get scale from matrix, ortho-normalise, then reset scale

  udDouble4x4 m;
  m.axis.x = udDouble4::create(czX, 0);
  m.axis.y = udDouble4::create(czY, 0);
  m.axis.z = udDouble4::create(czZ, 0);
  m.axis.t = udDouble4::create(czO, 1);

  return m;
}
