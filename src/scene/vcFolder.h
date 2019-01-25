#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcScene.h"
#include <vector>

struct vcState;

struct vcFolder : public vcSceneItem
{
  std::vector<vcSceneItem*> children;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

void vcFolder_AddToList(vcState *pProgramState, const char *pName);
void vcFolder_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, bool sameLine = true);

#endif //vcFolder_h__
