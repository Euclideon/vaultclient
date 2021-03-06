#ifndef vcAddSphereFilter_h__
#define vcAddSphereFilter_h__

#include "vcSceneTool.h"

struct vcState;

class vcAddSphereFilter : public vcSceneTool
{
public:
  vcAddSphereFilter() : vcSceneTool(vcActiveTool_AddSphereFilter) {};
  void SceneUI(vcState *pProgramState) override;

  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;
  void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcAddSphereFilter m_instance;
};

#endif //vcAddSphereFilter_h__
