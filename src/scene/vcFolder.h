#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcSceneItem.h"

struct vcState;

class vcFolder : public vcSceneItem
{
public:
  vcFolder(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcFolder() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void ChangeProjection(const udGeoZone &newZone) override;

  bool Is3DSceneObject() const override { return false; }
  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void Cleanup(vcState *pProgramState) override;
};

#endif //vcFolder_h__
