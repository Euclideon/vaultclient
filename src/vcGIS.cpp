#include "vcGIS.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udGeoZone.h"

#include <unordered_map>

struct SlippyGISResult
{
  udDouble3 result;
  bool calculated;
};
static std::unordered_map<uint64_t, SlippyGISResult> gSlippyGISCache;

static inline bool vcGIS_GetCachedResult(const vcGISSpace *pSpace, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel)
{
  uint64_t hashKey = uint64_t(zoomLevel) << 50 | uint64_t(slippyCoords.x) << 25 | uint64_t(slippyCoords.y) << 0;
  SlippyGISResult *pResult = &gSlippyGISCache[hashKey];

  bool success = true;
  if (!pResult->calculated)
  {
    udDouble3 latLong;
    success = vcGIS_SlippyToLatLong(&latLong, slippyCoords, zoomLevel);
    pResult->result = udGeoZone_ToCartesian(pSpace->zone, latLong);
    pResult->calculated = true;
  }

  *pLocalCoords = pResult->result;
  return success;
}

void vcGIS_ClearCache()
{
  gSlippyGISCache.clear();
}

bool vcGIS_AcceptableSRID(vcSRID sridCode)
{
  udGeoZone zone;
  return (udGeoZone_SetFromSRID(&zone, sridCode) == udR_Success);
}

bool vcGIS_ChangeSpace(vcGISSpace *pSpace, vcSRID newSRID, udDouble3 *pCameraPosition /*= nullptr*/)
{
  udGeoZone newZone;
  bool currentlyProjected = pSpace->isProjected;

  pSpace->SRID = newSRID;
  pSpace->isProjected = false;

  if (newSRID == 0)
    return true;

  if (udGeoZone_SetFromSRID(&newZone, newSRID) != udR_Success)
    return false;

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

  pSlippyCoords->x = (int)udFloor((latLong.y + 180.0) / 360.0 * udPow(2.0, zoomLevel)); // Long
  pSlippyCoords->y = (int)(udFloor((1.0 - udLogN(udTan(latLong.x * UD_PI / 180.0) + 1.0 / udCos(latLong.x * UD_PI / 180.0)) / UD_PI) / 2.0 * udPow(2.0, zoomLevel))); //Lat

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

  return vcGIS_LatLongToSlippy(pSlippyCoords, udGeoZone_ToLatLong(pSpace->zone, localCoords), zoomLevel);
}

bool vcGIS_SlippyToLocal(const vcGISSpace *pSpace, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel)
{
  if (!pSpace->isProjected)
    return false;

  return vcGIS_GetCachedResult(pSpace, pLocalCoords, slippyCoords, zoomLevel);
}
