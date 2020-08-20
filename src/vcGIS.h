#ifndef vcGIS_h__
#define vcGIS_h__

#include "udMath.h"
#include "udGeoZone.h"

bool vcGIS_AcceptableSRID(int32_t sridCode);

// Changes pSpace from its current zone to the new zone, pCameraPosition if set gets moved to the new zone
bool vcGIS_ChangeSpace(udGeoZone *pGeozone, const udGeoZone &newZone, udDouble3 *pCameraPosition = nullptr, bool changeZone = true);

bool vcGIS_LatLongToSlippy(udInt2 *pSlippyCoords, udDouble3 latLong, int zoomLevel);
bool vcGIS_LatLongToSlippy(udDouble2 *pSlippyCoords, udDouble3 latLong, int zoomLevel);
bool vcGIS_SlippyToLatLong(udDouble3 *pLatLong, udInt2 slippyCoords, int zoomLevel);

bool vcGIS_LocalToSlippy(const udGeoZone &zone, udInt2 *pSlippyCoords, const udDouble3 &localCoords, const int zoomLevel);
bool vcGIS_SlippyToLocal(const udGeoZone &zone, udDouble3 *pLocalCoords, const udInt2 &slippyCoords, const int zoomLevel);

// Helpers
void vcGIS_GetOrthonormalBasis(const udGeoZone &zone, udDouble3 localPosition, udDouble3 *pUp, udDouble3 *pNorth, udDouble3 *pEast);
udDoubleQuat vcGIS_GetQuaternion(const udGeoZone &zone, udDouble3 localPosition); // Get rotational transform from cartesian space to the world position
udDouble3 vcGIS_GetWorldLocalUp(const udGeoZone &zone, udDouble3 localCoords); // Returns which direction is "up" at a given coordinate
udDouble3 vcGIS_GetWorldLocalNorth(const udGeoZone &zone, udDouble3 localCoords); // Returns which direction is "north"
udDouble2 vcGIS_QuaternionToHeadingPitch(const udGeoZone &zone, udDouble3 localPosition, udDoubleQuat orientation); // Returns HPR from a Quaternion at a specific location
udDoubleQuat vcGIS_HeadingPitchToQuaternion(const udGeoZone &zone, udDouble3 localPosition, udDouble2 headingPitch); // Returns a Quaternion from a HPR at a specific location

// Note: Pitch is not implemented yet
udDouble2 vcGIS_GetHeadingPitchFromLatLong(const udGeoZone &zone, const udDouble3 &latLon0, const udDouble3 &latLon1);

#endif // !vcGIS_h__
