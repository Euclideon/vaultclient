#ifndef vcSettingsUI_h__
#define vcSettingsUI_h__

#include "imgui.h"

struct vcState;

void vcSettingsUI_Show(vcState *pProgramState);
bool vcSettingsUI_LangCombo(vcState *pProgramState);

#endif
