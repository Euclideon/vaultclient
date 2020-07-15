#ifndef vcSettingsUI_h__
#define vcSettingsUI_h__

#include "vcMath.h"
#include "imgui.h"

struct vcState;
struct vcVisualizationSettings;
struct udAttributeSet;

void vcSettingsUI_Show(vcState *pProgramState);
bool vcSettingsUI_LangCombo(vcState *pProgramState);
bool vcSettingsUI_VisualizationSettings(vcVisualizationSettings *pVisualizationSettings, bool isGlobal = true, udAttributeSet *pAttributes = nullptr);
void vcSettingsUI_BasicMapSettings(vcState *pProgramState, bool alwaysShowOptions = false);
void vcSettingsUI_SceneVisualizationSettings(vcState *pProgramState);
void vcSettingsUI_AdvancedGISSettings(vcState *pProgramState);

const char *vcSettingsUI_GetClassificationName(vcState *pProgramState, uint8_t classification);

void vcSettingsUI_Cleanup(vcState *pProgramState);

#endif
