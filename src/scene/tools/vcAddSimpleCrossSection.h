#ifndef vcAddCylinderFilter_h__
#define vcAddCylinderFilter_h__

#include "vcSceneTool.h"

struct vcState;

class vcAddSimpleCrossSection : public vcSceneTool
{
public:
  vcAddSimpleCrossSection() : vcSceneTool(vcActiveTool_AddSimpleCrossSection) {};
  void SceneUI(vcState *pProgramState) override;

  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcAddSimpleCrossSection m_instance;
};

#endif //vcAddCylinderFilter_h__
