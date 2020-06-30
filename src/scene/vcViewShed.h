#ifndef vcViewShed_h__
#define vcViewShed_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"

class vcViewShed : public vcSceneItem
{
private:
  udDouble3 m_position;
  double m_distance;
  uint32_t m_visibleColour;
  uint32_t m_hiddenColour;

public:
  vcViewShed(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcViewShed() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleSceneEmbeddedUI(vcState *pProgramState);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble4x4 GetWorldSpaceMatrix();
};

#endif //vcViewShed_h__
