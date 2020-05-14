#ifndef vcModel_h__
#define vcModel_h__

#include "vcSceneItem.h"
#include "vcSettings.h"
#include "vdkRenderContext.h"
#include "vdkPointCloud.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

class vcModel : public vcSceneItem
{
public:
  vdkPointCloud *m_pPointCloud;
  vdkPointCloudHeader m_pointCloudHeader;

  udDouble3 m_pivot; // The models pivot in local space
  udDouble4x4 m_defaultMatrix; // This is the matrix from the model header- in m_pPreferredZone space

  udGeoZone *m_pCurrentZone; // The current zone that this model is in (used to fast transform)

  udDouble3 m_positionCurrent;
  udDouble3 m_scaleCurrent;
  udDoubleQuat m_orientationCurrent;

  bool m_changeZones; // If true, this model needs to have its zone recalculated
  double m_meterScale;

  bool m_hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *m_pWatermark; // If the watermark is loaded, it will be here

  vcVisualizationSettings m_visualization; // Overrides global visualization settings

  vcModel(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  vcModel(vcState *pProgramState, const char *pName, vdkPointCloud *pCloud);
  ~vcModel() {};

  void OnNodeUpdate(vcState *pProgramState);

  void ChangeProjection(const udGeoZone &newZone);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void HandleContextMenu(vcState *pProgramState);

  void Cleanup(vcState *pProgramState);

  udDouble3 GetLocalSpacePivot();
  udDouble4x4 GetWorldSpaceMatrix();
  vcGizmoAllowedControls GetAllowedControls();

  void ContextMenuListModels(vcState *pProgramState, vdkProjectNode *pParentNode, vcSceneItem **ppCurrentSelectedModel, const char *pProjectNodeType, bool allowEmpty);
  void SetTransforms(const udDouble4x4 &matrix);
private:
  void SaveTransformsToNode(vcProject * pProject, const udGeoZone &defaultZone);
};

#endif //vcModel_h__
