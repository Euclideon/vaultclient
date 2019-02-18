#ifndef vcScene_h__
#define vcScene_h__

#include "vcGIS.h"

class udJSON;
struct vcState;
class vcSceneItem;
class vcFolder;
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
  vcSOT_LiveFeed, // "IOT"

  vcSOT_Custom, // Need to check the type string manually

  vcSOT_Count
};

typedef void (udSceneItemImGuiCallback)(vcState *pProgramState, vcSceneItem *pBaseItem, size_t *pItemID);
typedef void (udSceneItemBasicCallback)(vcState *pProgramState, vcSceneItem *pBaseItem);

class vcSceneItem
{
public:
  volatile int32_t m_loadStatus;
  bool m_visible;
  bool m_selected;
  bool m_expanded;
  bool m_editName;

  vcSceneItemType m_type;
  char m_typeStr[8];

  udJSON *m_pMetadata; // This points to a metadata (if it exists)
  udGeoZone *m_pOriginalZone; // nullptr if not geolocated
  udGeoZone *m_pZone; // nullptr if not geolocated

  const char *m_pPath;

  char *m_pName;
  size_t m_nameBufferLength;

  // This is used to help with adding the item to current folder
  virtual void AddItem(vcState *pProgramState);

  // This is used to help with adding the item to the renderer
  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) = 0;

  // This is used to help with applying changes (e.g. Gizmo)
  virtual void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) = 0;

  // This is used to help with exposing item specific UI
  virtual void HandleImGui(vcState *pProgramState, size_t *pItemID) = 0;

  // Only calls this if its 'completed' loading and is 'vcSLS_Loaded'; note: this is called before other cleanup operations
  virtual void Cleanup(vcState *pProgramState) = 0;

  // This function handles projection changes if additional handling is required
  virtual void ChangeProjection(vcState *pProgramState, const udGeoZone &newZone);

  // Gets the item's pivot point in various spaces
  virtual udDouble3 GetWorldSpacePivot();
  virtual udDouble3 GetLocalSpacePivot();

  // Gets the world space matrix (or identity if not applicable)
  virtual udDouble4x4 GetWorldSpaceMatrix();
};

void vcScene_RemoveItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_RemoveAll(vcState *pProgramState);
void vcScene_RemoveSelected(vcState *pProgramState);

bool vcScene_ContainsItem(vcFolder *pParent, vcSceneItem *pItem);

void vcScene_SelectItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_UnselectItem(vcState *pProgramState, vcFolder *pParent, size_t index);
void vcScene_ClearSelection(vcState *pProgramState);

bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel);

#endif
