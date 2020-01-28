#ifndef vcSettingsUI_h__
#define vcSettingsUI_h__

#include "imgui.h"

struct vcState;

enum vcSettingsErrors
{
  vcSE_Bound = 1 << 0,
  vcSE_Select = 1 << 1,

  vcSE_Count = 2
};

void vcSettingsUI_SetError(vcSettingsErrors error);
void vcSettingsUI_UnsetError(vcSettingsErrors error);
bool vcSettingsUI_CheckError(vcSettingsErrors error);
ImVec4 vcSettingsUI_GetErrorColour(vcSettingsErrors error);

void vcSettingsUI_Show(vcState *pProgramState);
bool vcSettingsUI_LangCombo(vcState *pProgramState);

#endif
