#include "vcGIS.h"

#include "udPlatformUtil.h"
#include "udGeoZone.h"

bool vcGIS_AcceptableSRID(int32_t sridCode)
{
  udGeoZone zone;

  return (udGeoZone_SetFromSRID(&zone, sridCode) == udR_Success);
}

bool vcGIS_ChangeSpace(udGeoZone *pZone, const udGeoZone &newZone, udDouble3 *pCameraPosition /*= nullptr*/)
{
  if (pCameraPosition != nullptr)
    *pCameraPosition = udGeoZone_TransformPoint(*pCameraPosition, *pZone, newZone);

  *pZone = newZone;

  return true;
}

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel)
{
  if (pSlippyCoords == nullptr)
    return false;

  udDouble2 slippyCoords = {};
  vcGIS_LatLongToSlippy(&slippyCoords, latLong, zoomLevel);

  pSlippyCoords->x = (int)slippyCoords.x;
  pSlippyCoords->y = (int)slippyCoords.y;
  return true;
}

bool vcGIS_LatLongToSlippy(udDouble2 *pSlippyCoords, udDouble3 latLong, int zoomLevel)
{
  if (pSlippyCoords == nullptr)
    return false;

  pSlippyCoords->x = (latLong.y + 180.0) / 360.0 * udPow(2.0, zoomLevel); // Long
  pSlippyCoords->y = (1.0 - udLogN(udTan(latLong.x * UD_PI / 180.0) + 1.0 / udCos(latLong.x * UD_PI / 180.0)) / UD_PI) / 2.0 * udPow(2.0, zoomLevel); //Lat

  double maxx = udPow(2.0, zoomLevel);

  while (pSlippyCoords->x < 0)
    pSlippyCoords->x += maxx;

  while (pSlippyCoords->x > maxx - 1)
    pSlippyCoords->x -= maxx;

  return true;
}


bool vcGIS_SlippyToLatLong(udDouble3 *pLatLong, udInt2 slippyCoords, int zoomLevel)
{
  if (pLatLong == nullptr)
    return false;

  pLatLong->x = 180.0 / UD_PI * udATan(udSinh(UD_PI * (1 - 2 * slippyCoords.y / (udPow(2.0, zoomLevel))))); // Lat
  pLatLong->y = slippyCoords.x / udPow(2.0, zoomLevel) * 360.0 - 180; // Long
  pLatLong->z = 0;

  return true;
}

bool vcGIS_LocalToSlippy(const udGeoZone &zone, udInt2 *pSlippyCoords, const udDouble3 &localCoords, const int zoomLevel)
{
  if (zone.projection == udGZPT_Unknown)
    return false;

  return vcGIS_LatLongToSlippy(pSlippyCoords, udGeoZone_CartesianToLatLong(zone, localCoords), zoomLevel);
}

bool vcGIS_SlippyToLocal(const udGeoZone &zone, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel)
{
  if (zone.projection == udGZPT_Unknown)
    return false;

  udDouble3 latLong;
  bool success = vcGIS_SlippyToLatLong(&latLong, slippyCoords, zoomLevel);
  *pLocalCoords = udGeoZone_LatLongToCartesian(zone, latLong);

  return success;
}

void vcGIS_GetOrthonormalBasis(const udGeoZone &zone, udDouble3 localPosition, udDouble3 *pUp, udDouble3 *pNorth, udDouble3 *pEast)
{
  *pUp = vcGIS_GetWorldLocalUp(zone, localPosition);
  *pNorth = vcGIS_GetWorldLocalNorth(zone, localPosition);
  *pEast = udCross(*pNorth, *pUp);

  *pEast = udNormalize3(*pEast);
  *pNorth = udCross(*pUp, *pEast);
  //Shouldn't need to normalise north
}

udDouble3 vcGIS_GetWorldLocalUp(const udGeoZone &zone, udDouble3 localCoords)
{
  if (zone.projection == udGZPT_Unknown || zone.projection >= udGZPT_TransverseMercator)
    return udDouble3::create(0, 0, 1);

  udDouble3 latLong = udGeoZone_CartesianToLatLong(zone, localCoords);
  latLong.z += 1.f;

  udDouble3 upVector = udGeoZone_LatLongToCartesian(zone, latLong);

  if (udEqualApprox(upVector, localCoords))
    return udDouble3::create(0, 0, 1);
  else
    return udNormalize(upVector - localCoords);
}

udDouble3 vcGIS_GetWorldLocalNorth(const udGeoZone &zone, udDouble3 localCoords)
{
  if (zone.projection == udGZPT_Unknown || zone.projection >= udGZPT_TransverseMercator)
    return udDouble3::create(0, 1, 0);

  udDouble3 northDirection = udDouble3::create(0, 1, 0); //TODO: Fix this
  udDouble3 up = vcGIS_GetWorldLocalUp(zone, localCoords);

  udDouble3 currentLatLong = udGeoZone_CartesianToLatLong(zone, localCoords);
  currentLatLong.x = udClamp(currentLatLong.x, -90.0, 89.9);
  northDirection = udNormalize(udGeoZone_LatLongToCartesian(zone, udDouble3::create(currentLatLong.x + 0.1, currentLatLong.y, currentLatLong.z)) - localCoords);

  if (udEqualApprox(northDirection, up))
    northDirection = udDouble3::create(up.x, up.z, -up.y);

  udDouble3 east = udCross(northDirection, up);
  udDouble3 northFlat = udCross(up, east);

  return northFlat;
}

udDouble2 vcGIS_QuaternionToHeadingPitch(const udGeoZone &zone, udDouble3 localPosition, udDoubleQuat orientation)
{
  if (zone.projection == udGZPT_Unknown || zone.projection >= udGZPT_TransverseMercator)
  {
    udDouble2 headingPitch = orientation.eulerAngles().toVector2();
    headingPitch.x *= -1;

    while (headingPitch.x > UD_PI)
      headingPitch.x -= UD_2PI;
    while (headingPitch.x < -UD_PI)
      headingPitch.x += UD_2PI;

    while (headingPitch.y > UD_PI)
      headingPitch.y -= UD_2PI;
    while (headingPitch.y < -UD_PI)
      headingPitch.y += UD_2PI;

    return headingPitch;
  }

  udDouble3 up, north, east;
  vcGIS_GetOrthonormalBasis(zone, localPosition, &up, &north, &east);

  udDouble4x4 rotation = udDouble4x4::rotationQuat(orientation);
  udDouble4x4 referenceFrame = udDouble4x4::create(udDouble4::create(east, 0), udDouble4::create(north, 0), udDouble4::create(up, 0), udDouble4::identity());

  udDouble3 headingData = (referenceFrame.inverse() * rotation).extractYPR();
  headingData.x *= -1;

  while (headingData.x > UD_PI)
    headingData.x -= UD_2PI;
  while (headingData.x < -UD_PI)
    headingData.x += UD_2PI;

  while (headingData.y > UD_PI)
    headingData.y -= UD_2PI;
  while (headingData.y < -UD_PI)
    headingData.y += UD_2PI;

  return headingData.toVector2();
}

udDoubleQuat vcGIS_HeadingPitchToQuaternion(const udGeoZone &zone, udDouble3 localPosition, udDouble2 headingPitch)
{
  if (zone.projection == udGZPT_Unknown || zone.projection >= udGZPT_TransverseMercator)
    return udDoubleQuat::create(-headingPitch.x, headingPitch.y, 0.0);

  udDouble3 up, north, east;
  vcGIS_GetOrthonormalBasis(zone, localPosition, &up, &north, &east);

  udDoubleQuat rotationHeading = udDoubleQuat::create(up, -headingPitch.x);
  udDoubleQuat rotationPitch = udDoubleQuat::create(east, headingPitch.y);

  udDouble4x4 mat = udDouble4x4::lookAt(localPosition, localPosition + north, up);

  return rotationHeading * rotationPitch * mat.extractQuaternion();
}
