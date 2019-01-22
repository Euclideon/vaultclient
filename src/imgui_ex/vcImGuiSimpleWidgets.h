#ifndef vcImGuiSimpleWidgets_h__
#define vcImGuiSimpleWidgets_h__

#include "vcMath.h"

typedef int ImGuiColorEditFlags;
typedef int ImGuiInputTextFlags;

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize, ImGuiInputTextFlags flags = 0);
bool vcIGSW_ColorPickerU32(const char *pLabel, uint32_t *pColor, ImGuiColorEditFlags flags);

#endif // vcImGuiSimpleWidgets_h__
