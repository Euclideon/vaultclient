#ifndef vcGIS_h__
#define vcGIS_h__

#include "udPlatform/udMath.h"

bool vcGIS_LocalZoneToLatLong(uint16_t sridCode, udDouble3 localSpace, udDouble3 *pLatLong);
bool vcGIS_LatLongToLocalZone(uint16_t sridCode, udDouble3 latLong, udDouble3 *pLocalSpace);

bool vcGIS_LatLongToSlippyTileIDs(udInt2 *pTileIDs, udDouble2 latLong, int zoomLevel);
bool vcGIS_SlippyTileIDsToLatLong(udDouble2 *pLatLong, udInt2 tileID, int zoomLevel);

#endif // !vcGIS_h__
