#ifndef vcSelect_h__
#define vcSelect_h__

#include "vcSceneTool.h"

struct vcState;

class vcSelect : public vcSceneTool
{
public:
  vcSelect() : vcSceneTool(vcActiveTool_Select) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcSelect m_instance;
};

#endif //vcSelect_h__
