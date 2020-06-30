#ifndef vcUnsupportedNode_h__
#define vcUnsupportedNode_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

class vcUnsupportedNode : public vcSceneItem
{
public:
  vcUnsupportedNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcUnsupportedNode() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();
};

#endif //vcUnsupportedNode_h__
