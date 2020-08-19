#ifndef vcMeasureHeight_h__
#define vcMeasureHeight_h__

#include "vcSceneTool.h"

struct vcState;

class vcMeasureHeight : public vcSceneTool
{
public:
  vcMeasureHeight() : vcSceneTool(vcActiveTool_MeasureHeight) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcMeasureHeight m_instance;
};

#endif //vcMeasureHeight_h__
