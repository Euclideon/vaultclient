#ifndef vcScene_h__
#define vcScene_h__

#include "vcGIS.h"

class udJSON;
struct vcState;
struct vcSceneItem;

enum vcSceneLoadStatus
{
  vcSLS_Pending,
  vcSLS_Loading,
  vcSLS_Loaded,
  vcSLS_Failed,
  vcSLS_OpenFailure,
  vcSLS_Unloaded,

  vcSLS_Count
};

enum vcSceneItemType
{
  vcSOT_Unknown, // default is no type string (shouldn't stay in this state for long)

  vcSOT_PointCloud, // "UDS"
  vcSOT_PointOfInterest, // "POI"

  vcSOT_Custom, // Need to check the type string manually

  vcSOT_Count
};

typedef void (udSceneItem_CleanupFunc)(vcState *pProgramState, vcSceneItem *pBaseItem);

struct vcSceneItem
{
  volatile int32_t loadStatus;
  bool visible;
  bool selected;

  vcSceneItemType type;
  char typeStr[8];

  udDouble4x4 sceneMatrix; // This is the matrix used to render into the current projection
  udDouble3 pivot; // The point in local space that the object is anchored around (for scaling and rotating)

  udJSON *pMetadata; // This points to a metadata (if it exists)
  udGeoZone *pZone; // nullptr if not geolocated

  const char *pName;
  const char *pPath;

  udSceneItem_CleanupFunc *pCleanupFunc; // Only calls this is its 'completed' loading and is 'vcSLS_Loaded'; this is called before other cleanup operations
};

void vcScene_RemoveItem(vcState *pProgramState, size_t index);
void vcScene_RemoveAll(vcState *pProgramState);

void vcScene_UpdateItemToCurrentProjection(vcState *pProgramState, vcSceneItem *pModel); // If pModel is nullptr, everything in the scene is moved to the current space
bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel);

udDouble3 vcScene_GetItemWorldSpacePivotPoint(vcSceneItem *pModel);


#endif
