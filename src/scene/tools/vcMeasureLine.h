#ifndef vcMeasureLine_h__
#define vcMeasureLine_h__

#include "vcSceneTool.h"

struct vcState;

class vcMeasureLine : public vcSceneTool
{
public:
  vcMeasureLine() : vcSceneTool(vcActiveTool_MeasureLine) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcMeasureLine m_instance;
};

#endif //vcMeasureLine_h__
