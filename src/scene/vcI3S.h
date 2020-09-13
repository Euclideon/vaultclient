#ifndef vcI3S_h__
#define vcI3S_h__

#include "vcSceneItem.h"
#include "vcSceneLayerRenderer.h"

struct vcState;
struct vcRenderData;

class vcI3S : public vcSceneItem
{
public:
  vcI3S(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcI3S();

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleContextMenu(vcState *pProgramState) override;

  void Cleanup(vcState *pProgramState) override;

  void ChangeProjection(const udGeoZone &newZone) override;
  udDouble3 GetLocalSpacePivot() override;
  udDouble4x4 GetWorldSpaceMatrix() override;

  udDouble4x4 m_sceneMatrix; // This is the matrix used to render into the current projection
private:
  vcSceneLayerRenderer *m_pSceneRenderer;
};

#endif //vcI3S_h__
