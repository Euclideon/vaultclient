#ifndef vcAddCylinderFilter_h__
#define vcAddCylinderFilter_h__

#include "vcSceneTool.h"

struct vcState;

class vcAddCylinderFilter : public vcSceneTool
{
public:
  vcAddCylinderFilter() : vcSceneTool(vcActiveTool_AddCylinderFilter) {};
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState* pProgramState, vcRenderData& renderData, const vcRenderPickResult& pickResult) override;

private:
  static vcAddCylinderFilter m_instance;
};

#endif //vcAddCylinderFilter_h__
