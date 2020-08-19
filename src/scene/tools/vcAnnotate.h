#ifndef vcAnnotate_h__
#define vcAnnotate_h__

#include "vcSceneTool.h"

struct vcState;

class vcAnnotate : public vcSceneTool
{
public:
  vcAnnotate() : vcSceneTool(vcActiveTool_Annotate) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcAnnotate m_instance;
};

#endif //vcAnnotate_h__
