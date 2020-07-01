#ifndef vcImGuiSimpleWidgets_h__
#define vcImGuiSimpleWidgets_h__

#include "vcMath.h"
#include "vcSceneItem.h"
#include "imgui.h"
#include "vcFileDialog.h"

bool vcIGSW_InputText(const char *pLabel, char *pBuffer, size_t bufferSize, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
template <size_t N> inline bool vcIGSW_InputText(const char *pLabel, char(&buffer)[N], ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
  return vcIGSW_InputText(pLabel, buffer, N, flags);
}

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

void vcIGSW_FilePicker(vcState *pProgramState, const char *pLabel, char *pBuffer, size_t bufferSize, const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback onChange);
template <size_t N, size_t M> inline void vcIGSW_FilePicker(vcState *pProgramState, const char *pLabel, char(&path)[N], const char *(&extensions)[M], vcFileDialogType dialogType, vcFileDialogCallback onChange)
{
  vcIGSW_FilePicker(pProgramState, pLabel, path, N, extensions, M, dialogType, onChange);
}

bool vcIGSW_ColorPickerU32(const char *pLabel, uint32_t *pColor, ImGuiColorEditFlags flags);
bool vcIGSW_StickyIntSlider(const char* label, int* v, int v_min, int v_max, int sticky);

udFloat4 vcIGSW_BGRAToImGui(uint32_t lineColour);
uint32_t vcIGSW_ImGuiToBGRA(const udFloat4 &vec);
uint32_t vcIGSW_BGRAToRGBAUInt32(uint32_t lineColour);

bool vcIGSW_IsItemHovered(ImGuiHoveredFlags flags = 0, float timer = 0.5f); // Timer not currently working

void vcIGSW_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, const char *pToolTip = nullptr);

#endif // vcImGuiSimpleWidgets_h__
