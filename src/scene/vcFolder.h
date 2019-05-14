#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcScene.h"
#include <vector>

struct vcState;

class vcFolder : public vcSceneItem
{
public:
  std::vector<vcSceneItem*> m_children;

  vcFolder(const char *pName);

  void ChangeProjection(vcState *pProgramState, const udGeoZone &newZone);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

#endif //vcFolder_h__
