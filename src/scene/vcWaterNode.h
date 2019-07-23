#ifndef vcWater_h__
#define vcWater_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

class vcWater : public vcSceneItem
{
public:
  vcWater(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcWater() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();
};

#endif //vcUnsupportedNode_h__
