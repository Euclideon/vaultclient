#ifndef vcSHPNode_h__
#define vcSHPNode_h__

#include "vcSceneItem.h"
#include "vcSceneLayerRenderer.h"
#include "vcSHP.h"

struct vcState;
struct vcRenderData;

class vcSHPNode : public vcSceneItem
{
public:
  vcSHPNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcSHPNode();

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
  vcSHP m_SHP;
};

#endif //vcSHPNode_h__
