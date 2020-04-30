#ifndef vcGIS_h__
#define vcGIS_h__

#include "udMath.h"
#include "udGeoZone.h"

typedef int32_t vcSRID;

struct vcGISSpace
{
  bool isProjected; // Is the camera currently in a GIS projected space
  vcSRID SRID;
  udGeoZone zone;
};

bool vcGIS_AcceptableSRID(vcSRID sridCode);

// Changes pSpace from its current zone to the new zone, pCameraPosition if set gets moved to the new zone
bool vcGIS_ChangeSpace(vcGISSpace *pSpace, const udGeoZone &newZone, udDouble3 *pCameraPosition = nullptr);

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel);
bool vcGIS_SlippyToLatLong(udDouble3 *pLatLong, udInt2 slippyCoords, int zoomLevel);

bool vcGIS_LocalToSlippy(const vcGISSpace *pSpace, udInt2 *pSlippyCoords, const udDouble3 &localCoords, const int zoomLevel);
bool vcGIS_SlippyToLocal(const vcGISSpace *pSpace, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel);

// Helpers
void vcGIS_GetOrthonormalBasis(const vcGISSpace &space, udDouble3 localPosition, udDouble3 *pUp, udDouble3 *pNorth, udDouble3 *pEast);
udDouble3 vcGIS_GetWorldLocalUp(const vcGISSpace &space, udDouble3 localCoords); // Returns which direction is "up" at a given coordinate
udDouble3 vcGIS_GetWorldLocalNorth(const vcGISSpace &space, udDouble3 localCoords); // Returns which direction is "north"
udDouble2 vcGIS_QuaternionToHeadingPitch(const vcGISSpace &space, udDouble3 localPosition, udDoubleQuat orientation); // Returns HPR from a Quaternion at a specific location
udDoubleQuat vcGIS_HeadingPitchToQuaternion(const vcGISSpace &space, udDouble3 localPosition, udDouble2 headingPitch); // Returns a Quaternion from a HPR at a specific location

#endif // !vcGIS_h__
