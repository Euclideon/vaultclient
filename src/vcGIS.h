#ifndef vcGIS_h__
#define vcGIS_h__

#include "udPlatform/udMath.h"

bool vcGIS_LocalToLatLong(uint16_t sridCode, udDouble3 localCoords, udDouble3 *pLatLong);
bool vcGIS_LatLongToLocal(uint16_t sridCode, udDouble3 latLong, udDouble3 *pLocalCoords);

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel);
bool vcGIS_SlippyToLatLong(udDouble3 *pLatLong, udInt2 slippyCoords, int zoomLevel);

bool vcGIS_LocalToSlippy(int16_t sridCode, udInt2 *pSlippyCoords, udDouble3 localCoords, int zoomLevel);
bool vcGIS_SlippyToLocal(int16_t sridCode, udDouble3 *pLocalCoords, udInt2 slippyCoords, int zoomLevel);

#endif // !vcGIS_h__
