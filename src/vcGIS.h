#ifndef vcGIS_h__
#define vcGIS_h__

#include "udPlatform/udMath.h"
#include "udPlatform/udGeoZone.h"

typedef uint16_t vcSRID;

struct vcGISSpace
{
  bool isProjected; // Is the camera currently in a GIS projected space
  vcSRID SRID;
  udGeoZone zone;
};

bool vcGIS_AcceptableSRID(vcSRID sridCode);

// Changes pSpace from its current zone to the new zone, pCameraPosition if set gets moved to the new zone
bool vcGIS_ChangeSpace(vcGISSpace *pSpace, vcSRID newSRID, udDouble3 *pCameraPosition = nullptr);

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel);
bool vcGIS_SlippyToLatLong(udDouble3 *pLatLong, udInt2 slippyCoords, int zoomLevel);

bool vcGIS_LocalToSlippy(vcGISSpace *pSpace, udInt2 *pSlippyCoords, udDouble3 localCoords, int zoomLevel);
bool vcGIS_SlippyToLocal(vcGISSpace *pSpace, udDouble3 *pLocalCoords, udInt2 slippyCoords, int zoomLevel);

#endif // !vcGIS_h__
