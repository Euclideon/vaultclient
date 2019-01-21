#ifndef vcFolder_h__
#define vcFolder_h__

#include "vcScene.h"
#include <vector>

struct vcState;

struct vcFolder : public vcSceneItem
{
  std::vector<vcSceneItem*> children;
};

void vcFolder_AddToList(vcState *pProgramState, const char *pName);

#endif //vcFolder_h__
