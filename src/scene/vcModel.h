#ifndef vcModel_h__
#define vcModel_h__

#include "vcSceneItem.h"
#include "vcSettings.h"
#include "udRenderContext.h"
#include "udPointCloud.h"

struct udPointCloud;
struct vcTexture;
struct vcState;

class vcModel : public vcSceneItem
{
public:
  udPointCloud *m_pPointCloud;
  udPointCloudHeader m_pointCloudHeader;

  udDouble3 m_pivot; // The models pivot in local space
  udDouble4x4 m_defaultMatrix; // This is the matrix from the model header- in m_pPreferredZone space

  udGeoZone *m_pCurrentZone; // The current zone that this model is in (used to fast transform)

  udDouble4x4 m_sceneMatrix; // This is the matrix used to render in m_pCurrentZone (if specified) space
  udDouble4x4 m_baseMatrix; // This is the scene matrix in m_pPreferredZone space

  bool m_changeZones; // If true, this model needs to have its zone recalculated
  double m_meterScale;

  bool m_hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *m_pWatermark; // If the watermark is loaded, it will be here

  vcVisualizationSettings m_visualization; // Overrides global visualization settings

  vcModel(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  vcModel(vcState *pProgramState, const char *pName, udPointCloud *pCloud);
  ~vcModel() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void ChangeProjection(const udGeoZone &newZone) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  void HandleContextMenu(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void Cleanup(vcState *pProgramState) override;

  udDouble3 GetLocalSpacePivot() override;
  udDouble4x4 GetWorldSpaceMatrix() override;
  vcGizmoAllowedControls GetAllowedControls() override;

  void ContextMenuListModels(vcState *pProgramState, udProjectNode *pParentNode, vcSceneItem **ppCurrentSelectedModel, const char *pProjectNodeType, bool allowEmpty);

private:
  void Init(vcState *pProgramState);
};

#endif //vcModel_h__
