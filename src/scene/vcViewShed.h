#ifndef vcViewShed_h__
#define vcViewShed_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"

class vcViewShed : public vcSceneItem
{
private:
  udDouble3 m_position;
  double m_distance;
  uint32_t m_visibleColour;
  uint32_t m_hiddenColour;

public:
  vcViewShed(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcViewShed() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  udDouble4x4 GetWorldSpaceMatrix() override;
};

#endif //vcViewShed_h__
