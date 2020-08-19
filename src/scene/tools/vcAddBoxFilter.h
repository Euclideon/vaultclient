#ifndef vcAddBoxFilter_h__
#define vcAddBoxFilter_h__

#include "vcSceneTool.h"

struct vcState;

class vcAddBoxFilter : public vcSceneTool
{
public:
  vcAddBoxFilter() : vcSceneTool(vcActiveTool_AddBoxFilter) {};
  void SceneUI(vcState *pProgramState) override;

private:
  static vcAddBoxFilter m_instance;
};

#endif //vcAddBoxFilter_h__
