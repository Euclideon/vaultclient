#ifndef vcI3S_h__
#define vcI3S_h__

#include "vcSceneItem.h"
#include "vcSceneLayerRenderer.h"

struct vcState;
struct vcRenderData;

class vcI3S : public vcSceneItem
{
public:
  vcI3S(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcI3S();

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleContextMenu(vcState *pProgramState);

  void Cleanup(vcState *pProgramState);

  void ChangeProjection(const udGeoZone &newZone);
  udDouble3 GetLocalSpacePivot();
  udDouble4x4 GetWorldSpaceMatrix();

  udDouble4x4 m_sceneMatrix; // This is the matrix used to render into the current projection
private:
  vcSceneLayerRenderer *m_pSceneRenderer;
};

#endif //vcI3S_h__
