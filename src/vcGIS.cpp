#include "vcGIS.h"

#include "udPlatformUtil.h"
#include "udGeoZone.h"

bool vcGIS_AcceptableSRID(vcSRID sridCode)
{
  udGeoZone zone;
  return (udGeoZone_SetFromSRID(&zone, sridCode) == udR_Success);
}

bool vcGIS_ChangeSpace(vcGISSpace *pSpace, const udGeoZone &newZone, udDouble3 *pCameraPosition /*= nullptr*/)
{
  bool currentlyProjected = pSpace->isProjected;

  pSpace->SRID = newZone.srid;
  pSpace->isProjected = false;

  if (pSpace->SRID == 0)
  {
    memset(&pSpace->zone, 0, sizeof(pSpace->zone));
    return true;
  }

  if (currentlyProjected && pCameraPosition != nullptr)
    *pCameraPosition = udGeoZone_TransformPoint(*pCameraPosition, pSpace->zone, newZone);

  pSpace->isProjected = true;
  pSpace->zone = newZone;

  return true;
}

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel)
{
  if (pSlippyCoords == nullptr)
    return false;

  pSlippyCoords->x = int((latLong.y + 180.0) / 360.0 * udPow(2.0, zoomLevel)); // Long
  pSlippyCoords->y = int((1.0 - udLogN(udTan(latLong.x * UD_PI / 180.0) + 1.0 / udCos(latLong.x * UD_PI / 180.0)) / UD_PI) / 2.0 * udPow(2.0, zoomLevel)); //Lat

  int maxx = (int)udPow(2.0, zoomLevel);

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

bool vcGIS_LocalToSlippy(const vcGISSpace *pSpace, udInt2 *pSlippyCoords, const udDouble3 &localCoords, const int zoomLevel)
{
  if (!pSpace->isProjected)
    return false;

  return vcGIS_LatLongToSlippy(pSlippyCoords, udGeoZone_CartesianToLatLong(pSpace->zone, localCoords), zoomLevel);
}

bool vcGIS_SlippyToLocal(const vcGISSpace *pSpace, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel)
{
  if (!pSpace->isProjected)
    return false;

  udDouble3 latLong;
  bool success = vcGIS_SlippyToLatLong(&latLong, slippyCoords, zoomLevel);
  *pLocalCoords = udGeoZone_LatLongToCartesian(pSpace->zone, latLong);
  return success;
}

udDouble3 vcGIS_GetWorldLocalUp(const vcGISSpace &space, udDouble3 localCoords)
{
  if (!space.isProjected || space.zone.projection >= udGZPT_TransverseMercator)
    return udDouble3::create(0, 0, 1);

  udDouble3 latLong = udGeoZone_CartesianToLatLong(space.zone, localCoords);
  latLong.z += 1.f;

  udDouble3 upVector = udGeoZone_LatLongToCartesian(space.zone, latLong);

  return udNormalize(upVector - localCoords);
}

udDouble3 vcGIS_GetWorldLocalNorth(const vcGISSpace &space, udDouble3 localCoords)
{
  if (!space.isProjected || space.zone.projection >= udGZPT_TransverseMercator)
    return udDouble3::create(0, 1, 0);

  udDouble3 northDirection = udDouble3::create(0, 1, 0); //TODO: Fix this
  udDouble3 up = vcGIS_GetWorldLocalUp(space, localCoords);

  udDouble3 currentLatLong = udGeoZone_CartesianToLatLong(space.zone, localCoords);
  currentLatLong.x = udClamp(currentLatLong.x, -90.0, 89.9);
  northDirection = udNormalize(udGeoZone_LatLongToCartesian(space.zone, udDouble3::create(currentLatLong.x + 0.1, currentLatLong.y, currentLatLong.z)) - localCoords);

  udDouble3 east = udCross(northDirection, up);
  udDouble3 northFlat = udCross(up, east);

  return northFlat;
}

udDouble2 vcGIS_QuaternionToHeadingPitch(const vcGISSpace &space, udDouble3 localPosition, udDoubleQuat orientation)
{
  if (!space.isProjected || space.zone.projection >= udGZPT_TransverseMercator)
  {
    udDouble2 headingPitch = orientation.eulerAngles().toVector2();
    headingPitch.x *= -1;
    return headingPitch;
  }

  udDouble3 up = vcGIS_GetWorldLocalUp(space, localPosition);
  udDouble3 north = vcGIS_GetWorldLocalNorth(space, localPosition);
  udDouble3 east = udCross(north, up);

  udDouble4x4 rotation = udDouble4x4::rotationQuat(orientation);
  udDouble4x4 referenceFrame = udDouble4x4::create(udDouble4::create(east, 0), udDouble4::create(north, 0), udDouble4::create(up, 0), udDouble4::identity());

  udDouble3 headingData = (referenceFrame.inverse() * rotation).extractYPR();

  if (headingData.x > UD_PI)
    headingData.x -= UD_2PI;

  if (headingData.y > UD_PI)
    headingData.y -= UD_2PI;

  headingData.x *= -1;

  return headingData.toVector2();
}

udDoubleQuat vcGIS_HeadingPitchToQuaternion(const vcGISSpace &space, udDouble3 localPosition, udDouble2 headingPitch)
{
  if (!space.isProjected || space.zone.projection >= udGZPT_TransverseMercator)
    return udDoubleQuat::create(-headingPitch.x, headingPitch.y, 0.0);

  udDouble3 up = vcGIS_GetWorldLocalUp(space, localPosition);
  udDouble3 north = vcGIS_GetWorldLocalNorth(space, localPosition);
  udDouble3 east = udCross(north, up);

  udDoubleQuat rotationHeading = udDoubleQuat::create(up, -headingPitch.x);
  udDoubleQuat rotationPitch = udDoubleQuat::create(east, headingPitch.y);
  
  udDouble4x4 mat = udDouble4x4::lookAt(localPosition, localPosition + north, up);

  return rotationHeading * rotationPitch * mat.extractQuaternion();
}
