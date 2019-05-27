#ifndef vcModel_h__
#define vcModel_h__

#include "vcSceneItem.h"
#include "vdkRenderContext.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

class vcModel : public vcSceneItem
{
public:
  vdkPointCloud *m_pPointCloud;

  udDouble3 m_pivot;
  udDouble4x4 m_defaultMatrix; // This is the matrix that was originally loaded
  udDouble4x4 m_sceneMatrix; // This is the matrix used to render into the current projection

  double m_meterScale;

  bool m_hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *m_pWatermark; // If the watermark is loaded, it will be here

  vcModel(vdkProjectNode *pNode, vcState *pProgramState);
  vcModel(vcState *pProgramState, const char *pName, vdkPointCloud *pCloud, bool jumpToModelOnLoad = false);
  ~vcModel() {};

  void OnNodeUpdate() {};

  void ChangeProjection(const udGeoZone &newZone);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);

  udDouble3 GetLocalSpacePivot();
  udDouble4x4 GetWorldSpaceMatrix();
};

#endif //vcModel_h__
