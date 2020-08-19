#ifndef vcMeasureArea_h__
#define vcMeasureArea_h__

#include "vcSceneTool.h"

struct vcState;

class vcMeasureArea : public vcSceneTool
{
public:
  vcMeasureArea() : vcSceneTool(vcActiveTool_MeasureArea) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcMeasureArea m_instance;
};

#endif //vcMeasureArea_h__
