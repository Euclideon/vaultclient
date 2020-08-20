#ifndef vcAnnotate_h__
#define vcAnnotate_h__

#include "vcSceneTool.h"

struct vcState;

class vcAnnotate : public vcSceneTool
{
public:
  vcAnnotate();
  void SceneUI(vcState *pProgramState) override;
  void HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult) override;

private:
  static vcAnnotate m_instance;
  static const size_t m_bufferSize = 64;
  char m_buffer[m_bufferSize];
};

#endif //vcAnnotate_h__
