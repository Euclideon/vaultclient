#include "vcGIS.h"

bool vcGIS_LocalZoneToLatLong(uint16_t epsgCode, udDouble3 localSpace, udDouble3 *pLatLong)
{
  udUnused(epsgCode);
  udUnused(localSpace);
  udUnused(pLatLong);

  return false;
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
