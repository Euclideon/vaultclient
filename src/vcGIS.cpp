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
