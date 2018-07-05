#include "vcGIS.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udGeoZone.h"

bool vcGIS_AcceptableSRID(uint16_t sridCode)
{
  udGeoZone zone;
  return (udGeoZone_SetFromSRID(&zone, sridCode) == udR_Success);
}

bool vcGIS_LocalToLatLong(uint16_t sridCode, udDouble3 localCoords, udDouble3 *pLatLong)
{
  udGeoZone zone;

  if (udGeoZone_SetFromSRID(&zone, sridCode) != udR_Success)
    return false;

  *pLatLong = udGeoZone_ToLatLong(zone, localCoords);
  return true;
}

bool vcGIS_LatLongToLocal(uint16_t sridCode, udDouble3 latLong, udDouble3 *pLocalCoords)
{
  udGeoZone zone;

  if (udGeoZone_SetFromSRID(&zone, sridCode) != udR_Success)
    return false;

  *pLocalCoords = udGeoZone_ToCartesian(zone, latLong);
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

bool vcGIS_LocalToSlippy(int16_t sridCode, udInt2 *pSlippyCoords, udDouble3 localCoords, int zoomLevel)
{
  udDouble3 latLong;
  bool success = true;

  success &= vcGIS_LocalToLatLong(sridCode, localCoords, &latLong);
  success &= vcGIS_LatLongToSlippy(pSlippyCoords, latLong, zoomLevel);

  return success;
}

bool vcGIS_SlippyToLocal(int16_t sridCode, udDouble3 *pLocalCoords, udInt2 slippyCoords, int zoomLevel)
{
  udDouble3 latLong;
  bool success = true;

  success &= vcGIS_SlippyToLatLong(&latLong, slippyCoords, zoomLevel);
  success &= vcGIS_LatLongToLocal(sridCode, latLong, pLocalCoords);

  return success;
}
