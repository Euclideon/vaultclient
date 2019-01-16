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

    if (expectedBufSize <= *pInfo->pBufferSize)
      return 0; // We don't need to resize right now

    size_t newSize = udMin(expectedBufSize * 2, (size_t)32); // Doubles until only adding 32 bytes

    char *pNewStr = (char*)udMemDup(*pInfo->ppBuffer, *pInfo->pBufferSize, newSize, udAF_Zero);
    udFree(*pInfo->ppBuffer);
    *pInfo->ppBuffer = pNewStr;
    *pInfo->pBufferSize = *pInfo->pBufferSize + newSize;

    pData->Buf = pNewStr;
  }

  return 0;
}

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize)
{
  vcIGSWResizeContainer info;
  info.ppBuffer = ppBuffer;
  info.pBufferSize = pBufferSize;

  if (*pBufferSize == 0)
    *pBufferSize = udStrlen(*ppBuffer) + 1; //+1 for '\0'

  return ImGui::InputText(pLabel, *ppBuffer, *pBufferSize, ImGuiInputTextFlags_CallbackResize, vcIGSW_ResizeString, &info);
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
