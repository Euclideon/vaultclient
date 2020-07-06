#ifndef vcScene_h__
#define vcScene_h__

#include "vcGIS.h"
#include "vcProject.h"
#include "udJSON.h"
#include "imgui_ex/ImGuizmo.h"

struct vcState;
class vcSceneItem;
struct vcRenderData;

enum vcSceneLoadStatus
{
  vcSLS_Pending,
  vcSLS_Loading,
  vcSLS_Loaded,
  vcSLS_Failed,
  vcSLS_Corrupt,
  vcSLS_OpenFailure,
  vcSLS_Unloaded,
  vcSLS_Success, // Rarely used to show something specifically succeeded

  vcSLS_Count
};

typedef void (udSceneItemImGuiCallback)(vcState *pProgramState, vcSceneItem *pBaseItem, size_t *pItemID);
typedef void (udSceneItemBasicCallback)(vcState *pProgramState, vcSceneItem *pBaseItem);

class vcSceneItem
{
public:
  vcProject *m_pProject;
  vdkProjectNode *m_pNode;

  volatile int32_t m_loadStatus;
  const char *m_pActiveWarningStatus; // This is used to give temporary alerts that the node is not working as intended (load failures, wrong projection, etc)- gets passed to vcString:Get
  bool m_visible;
  bool m_selected;
  bool m_expanded;
  bool m_editName;

  char *m_pName;
  size_t m_nameCapacity;

  double m_lastUpdateTime; // The stored time that this node was last updated (compared with the node to see if the node updated outside of udStream)

  udJSON m_metadata; // This points to a metadata (may be an empty object)
  udGeoZone *m_pPreferredProjection; // nullptr if there is no preferred zone

  vcSceneItem(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  vcSceneItem(vcState *pProgramState, const char *pType, const char *pName);
  virtual ~vcSceneItem();

  virtual bool Is3DSceneObject() const { return true; }
  virtual bool IsValid() const { return true; }

  // This lets SceneItems know that their vdkProjectNode has changed
  virtual void OnNodeUpdate(vcState *pProgramState) = 0;

  virtual void SelectSubitem(uint64_t internalId);
  virtual bool IsSubitemSelected(uint64_t internalId);

  // This is used to help with adding the item to the renderer
  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) = 0;

  // This is used to help with applying changes (e.g. Gizmo)
  virtual void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) = 0;

  // These are used to help with exposing item specific UI
  virtual void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) = 0; // Shows expanded settings in scene explorer
  virtual void HandleSceneEmbeddedUI(vcState *pProgramState); // Shows basic settings when selected in the scene
  virtual void HandleContextMenu(vcState *pProgramState); // Show expanded context menu settings in scene explorer
  virtual void HandleAttachmentUI(vcState *pProgramState); // Show additional settings in the scene when attached to this item
  virtual void HandleToolUI(vcState *pProgramState); // Show additional settings in the scene when this item is being used as a tool

  // Only calls this if its 'completed' loading and is 'vcSLS_Loaded'; note: this is called before other cleanup operations
  virtual void Cleanup(vcState *pProgramState) = 0;

  // This function handles projection changes if additional handling is required
  virtual void ChangeProjection(const udGeoZone &newZone) = 0;

  // Moves the camera to the item
  virtual void SetCameraPosition(vcState *pProgramState);

  // Gets the item's pivot point in various spaces
  virtual udDouble3 GetWorldSpacePivot();
  virtual udDouble3 GetLocalSpacePivot();

  // Gets the world space matrix (or identity if not applicable)
  virtual udDouble4x4 GetWorldSpaceMatrix();

  // Gets the allowed Gizmo controls
  virtual vcGizmoAllowedControls GetAllowedControls();

  // Other
  void UpdateNode(vcState *pProgramState);
};

#endif
