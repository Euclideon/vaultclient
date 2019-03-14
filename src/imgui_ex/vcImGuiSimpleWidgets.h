#ifndef vcImGuiSimpleWidgets_h__
#define vcImGuiSimpleWidgets_h__

#include "vcMath.h"

typedef int ImGuiColorEditFlags;
typedef int ImGuiInputTextFlags;
typedef int ImGuiHoveredFlags;

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize, ImGuiInputTextFlags flags = 0);
bool vcIGSW_ColorPickerU32(const char *pLabel, uint32_t *pColor, ImGuiColorEditFlags flags);

udFloat4 vcIGSW_BGRAToImGui(uint32_t lineColour);
uint32_t vcIGSW_BGRAToRGBAUInt32(uint32_t lineColour);

bool vcIGSW_IsItemHovered(ImGuiHoveredFlags flags = 0, float timer = 0.5f);

#endif // vcImGuiSimpleWidgets_h__
