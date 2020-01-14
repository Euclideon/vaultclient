#ifndef vcI3S_h__
#define vcI3S_h__

#include "vcSceneItem.h"
#include "vcSceneLayerRenderer.h"

struct vcState;
struct vcRenderData;

class vcI3S : public vcSceneItem
{
public:
  vcI3S(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcI3S();

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);

  void ChangeProjection(const udGeoZone &newZone);
  udDouble3 GetLocalSpacePivot();
  udDouble4x4 GetWorldSpaceMatrix();
private:
  vcSceneLayerRenderer *m_pSceneRenderer;
  udDouble4x4 m_sceneMatrix; // This is the matrix used to render into the current projection
};

#endif //vcI3S_h__
