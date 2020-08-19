#ifndef vcInspect_h__
#define vcInspect_h__

#include "vcSceneTool.h"

struct vcState;

class vcInspect : public vcSceneTool
{
public:
  vcInspect() : vcSceneTool(vcActiveTool_Inspect) {};
  void SceneUI(vcState *pProgramState) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcInspect m_instance;
};

#endif //vcInspect_h__
