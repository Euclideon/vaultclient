#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcSceneItem.h"

struct vcState;

class vcFolder : public vcSceneItem
{
public:
  vcFolder(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcFolder() {};

  void OnNodeUpdate(vcState *pProgramState);

  void ChangeProjection(const udGeoZone &newZone);

  bool Is3DSceneObject() const override { return false; }
  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

#endif //vcFolder_h__
