#ifndef vcWater_h__
#define vcWater_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

class vcWater : public vcSceneItem
{
public:
  vcWater(vdkProjectNode *pNode);
  ~vcWater() {};

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  udDouble3 GetLocalSpacePivot();
};

#endif //vcUnsupportedNode_h__
