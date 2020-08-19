#ifndef vcSceneTool_h__
#define vcSceneTool_h__

#include "vcState.h"
#include "vcRender.h"

class vcSceneTool
{
protected:
  vcSceneTool(vcActiveTool tool);
public:
  virtual void SceneUI(vcState *pProgramState);
  virtual void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult);
  virtual void PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult);

  static vcSceneTool *tools[vcActiveTool_Count];
};

inline void vcSceneTool::SceneUI(vcState *) {};
inline void vcSceneTool::HandlePicking(vcState *, vcRenderData &, const vcRenderPickResult &) {};
inline void vcSceneTool::PreviewPicking(vcState *, vcRenderData &, const vcRenderPickResult &) {};

#endif //vcSceneTool_h__
