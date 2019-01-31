#include "vcImGuiSimpleWidgets.h"

#include "udPlatform/udPlatform.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"

struct vcIGSWResizeContainer
{
  char **ppBuffer;
  size_t *pBufferSize;
};

int vcIGSW_ResizeString(ImGuiInputTextCallbackData *pData)
{
  if (pData->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    vcIGSWResizeContainer *pInfo = (vcIGSWResizeContainer*)pData->UserData;

    size_t expectedBufSize = (size_t)pData->BufSize;
    size_t currentBufSize = *pInfo->pBufferSize;

    if (expectedBufSize <= currentBufSize)
      return 0; // We don't need to resize right now

    size_t additionalBytes = expectedBufSize - currentBufSize;
    additionalBytes = udMin(additionalBytes * 2, additionalBytes + 64); // Gives double the amount requested until the amount requested is more than 32

    char *pNewStr = (char*)udMemDup(*pInfo->ppBuffer, currentBufSize, additionalBytes, udAF_Zero);
    udFree(*pInfo->ppBuffer);
    *pInfo->ppBuffer = pNewStr;
    *pInfo->pBufferSize = currentBufSize + additionalBytes;

    pData->Buf = pNewStr;
  }

  return 0;
}

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize, ImGuiInputTextFlags flags /*= ImGuiInputTextFlags_None*/)
{
  vcIGSWResizeContainer info;
  info.ppBuffer = ppBuffer;
  info.pBufferSize = pBufferSize;

  if (*pBufferSize == 0)
    *pBufferSize = udStrlen(*ppBuffer) + 1; //+1 for '\0'

  return ImGui::InputText(pLabel, *ppBuffer, *pBufferSize, ImGuiInputTextFlags_CallbackResize | flags, vcIGSW_ResizeString, &info);
}

bool vcIGSW_ColorPickerU32(const char *pLabel, uint32_t *pColor, ImGuiColorEditFlags flags)
{
  float colors[4];

  colors[0] = ((((*pColor) >> 16) & 0xFF) / 255.f); // Blue
  colors[1] = ((((*pColor) >> 8) & 0xFF) / 255.f); // Green
  colors[2] = ((((*pColor) >> 0) & 0xFF) / 255.f); // Red
  colors[3] = ((((*pColor) >> 24) & 0xFF) / 255.f); // Alpha

  if (ImGui::ColorEdit4(pLabel, colors, flags))
  {
    uint32_t val = 0;

    val |= ((int)(colors[0] * 255) << 16); // Blue
    val |= ((int)(colors[1] * 255) << 8); // Green
    val |= ((int)(colors[2] * 255) << 0); // Red
    val |= ((int)(colors[3] * 255) << 24); // Alpha

    *pColor = val;

    return true;
  }

  return false;
}

udFloat4 vcIGSW_BGRAToImGui(uint32_t lineColour)
{
  //TODO: Find or add a math helper for this
  udFloat4 colours; // RGBA
  colours.x = ((((lineColour) >> 16) & 0xFF) / 255.f); // Red
  colours.y = ((((lineColour) >> 8) & 0xFF) / 255.f); // Green
  colours.z = ((((lineColour) >> 0) & 0xFF) / 255.f); // Blue
  colours.w = ((((lineColour) >> 24) & 0xFF) / 255.f); // Alpha

  return colours;
}

uint32_t vcIGSW_BGRAToRGBAUInt32(uint32_t lineColour)
{
  // BGRA to RGBA
  return ((lineColour & 0xff) << 16) | (lineColour & 0x0000ff00) | (((lineColour >> 16) & 0xff) << 0) | (lineColour & 0xff000000);
}
