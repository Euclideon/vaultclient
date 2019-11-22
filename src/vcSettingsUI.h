#ifndef vcSettingsUI_h__
#define vcSettingsUI_h__

struct vcState;

static const char* ScreenshotExportFormats[4] = { "PNG", "BMP", "TGA", "JPG" };

void vcSettingsUI_Show(vcState *pProgramState);

#endif
