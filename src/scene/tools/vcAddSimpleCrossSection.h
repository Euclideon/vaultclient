#ifndef vcAddSimpleCrossSection_h__
#define vcAddSimpleCrossSection_h__

#include "vcSceneTool.h"

struct vcState;

class vcAddSimpleCrossSection : public vcSceneTool
{
public:
  vcAddSimpleCrossSection() : vcSceneTool(vcActiveTool_AddSimpleCrossSection) {};
  void SceneUI(vcState *pProgramState) override;

  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void OnCancel(vcState *pProgramState) override;

private:
  static vcAddSimpleCrossSection m_instance;
};

#endif //vcAddSimpleCrossSection_h__
