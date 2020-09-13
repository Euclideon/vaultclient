#ifndef vcUnsupportedNode_h__
#define vcUnsupportedNode_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

class vcUnsupportedNode : public vcSceneItem
{
public:
  vcUnsupportedNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcUnsupportedNode() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  udDouble3 GetLocalSpacePivot() override;
};

#endif //vcUnsupportedNode_h__
