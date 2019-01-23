#ifndef vcScene_h__
#define vcScene_h__

#include "vcGIS.h"

class udJSON;
struct vcState;
struct vcSceneItem;
struct vcFolder;
struct vcRenderData;

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
  vcSOT_Folder, // "Folder"

  vcSOT_Custom, // Need to check the type string manually

  vcSOT_Count
};

typedef void (udSceneItemImGuiCallback)(vcState *pProgramState, vcSceneItem *pBaseItem, size_t *pItemID);
typedef void (udSceneItemBasicCallback)(vcState *pProgramState, vcSceneItem *pBaseItem);

struct vcSceneItem
{
  volatile int32_t loadStatus;
  bool visible;
  bool selected;
  bool expanded;
  bool editName;

  vcSceneItemType type;
  char typeStr[8];

  udDouble4x4 defaultMatrix; // This is the matrix that was originally loaded
  udDouble4x4 sceneMatrix; // This is the matrix used to render into the current projection
  udDouble3 pivot; // The point in local space that the object is anchored around (for scaling and rotating)

  udJSON *pMetadata; // This points to a metadata (if it exists)
  udGeoZone *pZone; // nullptr if not geolocated

  const char *pPath;

  char *pName;
  size_t nameBufferLength;

  // This is used to help with adding the item to current folder
  virtual void AddItem(vcState *pProgramState);

  // This is used to help with adding the item to the renderer
  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) = 0;

  // This is used to help with applying changes (e.g. Gizmo)
  virtual void ApplyDelta(vcState *pProgramState) = 0;

  // This is used to help with exposing item specific UI
  virtual void HandleImGui(vcState *pProgramState, size_t *pItemID) = 0;

  // Only calls this if its 'completed' loading and is 'vcSLS_Loaded'; note: this is called before other cleanup operations
  virtual void Cleanup(vcState *pProgramState) = 0;
};

void vcScene_RemoveItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_RemoveAll(vcState *pProgramState);
void vcScene_RemoveSelected(vcState *pProgramState);

bool vcScene_ContainsItem(vcFolder *pParent, vcSceneItem *pItem);

void vcScene_SelectItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_UnselectItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_ClearSelection(vcState *pProgramState);

void vcScene_UpdateItemToCurrentProjection(vcState *pProgramState, vcSceneItem *pModel); // If pModel is nullptr, everything in the scene is moved to the current space
bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel);

udDouble3 vcScene_GetItemWorldSpacePivotPoint(vcSceneItem *pModel);


#endif
