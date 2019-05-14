#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcSceneItem.h"

struct vcState;

class vcFolder : public vcSceneItem
{
public:
  vcFolder(vdkProjectNode *pNode);

  void ChangeProjection(vcState *pProgramState, const udGeoZone &newZone);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

#endif //vcFolder_h__
