#include "vcImGuiSimpleWidgets.h"

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h" // Required for button hover state
#include "imgui_markdown.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcHotkey.h"
#include "vcWebFile.h"
#include "gl/vcTextureCache.h"

struct vcIGSWResizeContainer
{
  char **ppBuffer;
  size_t *pBufferSize;
};

struct vcIGSWMarkdownContext
{
  ImVec2 areaSize;
  const char *pPath;
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

bool vcIGSW_InputText(const char *pLabel, char *pBuffer, size_t bufferSize, ImGuiInputTextFlags flags /*= ImGuiInputTextFlags_None*/)
{
  bool retVal = ImGui::InputText(pLabel, pBuffer, bufferSize, flags);

  if (ImGui::BeginPopupContextItem())
  {
    if (ImGui::MenuItem(vcString::Get("popupMenuCopy"), nullptr, nullptr, (flags & ImGuiInputTextFlags_Password) == 0))
      ImGui::SetClipboardText(pBuffer);

    if (ImGui::MenuItem(vcString::Get("popupMenuPaste")))
      udStrcpy(pBuffer, bufferSize, ImGui::GetClipboardText());

    ImGui::EndPopup();
  }

  return retVal;
}

bool vcIGSW_InputTextWithResize(const char *pLabel, char **ppBuffer, size_t *pBufferSize, ImGuiInputTextFlags flags /*= ImGuiInputTextFlags_None*/)
{
  vcIGSWResizeContainer info;
  info.ppBuffer = ppBuffer;
  info.pBufferSize = pBufferSize;

  if (*pBufferSize == 0)
    *pBufferSize = udStrlen(*ppBuffer) + 1; //+1 for '\0'

  bool retVal = ImGui::InputText(pLabel, *ppBuffer, *pBufferSize, ImGuiInputTextFlags_CallbackResize | flags, vcIGSW_ResizeString, &info);

  if (ImGui::BeginPopupContextItem())
  {
    if (ImGui::MenuItem(vcString::Get("popupMenuCopy"), nullptr, nullptr, (flags & ImGuiInputTextFlags_Password) == 0))
      ImGui::SetClipboardText(*ppBuffer);

    if (ImGui::MenuItem(vcString::Get("popupMenuPaste")))
      udStrcpy(*ppBuffer, *pBufferSize, ImGui::GetClipboardText());

    ImGui::EndPopup();
  }

  return retVal;
}

void vcIGSW_FilePicker(vcState *pProgramState, const char *pLabel, char *pBuffer, size_t bufferSize, const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback onChange)
{
  const float ButtonWidth = 20.f;

  float itemSize = ImGui::CalcItemWidth();
  ImGui::SetNextItemWidth(itemSize - ButtonWidth + 3.f);

  ImGui::InputText(udTempStr("##%s_fpicker", pLabel), pBuffer, bufferSize);

  if (ImGui::IsItemDeactivatedAfterEdit() && onChange != nullptr)
    onChange();

  if (ImGui::BeginPopupContextItem())
  {
    if (ImGui::MenuItem(vcString::Get("popupMenuCopy")))
      ImGui::SetClipboardText(pBuffer);

    if (ImGui::MenuItem(vcString::Get("popupMenuPaste")))
    {
      udStrcpy(pBuffer, bufferSize, ImGui::GetClipboardText());

      if (onChange != nullptr)
        onChange();
    }

    ImGui::EndPopup();
  }

  ImGui::SameLine(0, 0);

#if UDPLATFORM_EMSCRIPTEN
  udUnused(pProgramState);
  udUnused(ppExtensions);
  udUnused(numExtensions);
  udUnused(dialogType);
#else
  if (ImGui::Button(udTempStr("...##%s_fpicker_openmore", pLabel)))
    vcFileDialog_Open(pProgramState, pLabel, pBuffer, bufferSize, ppExtensions, numExtensions, dialogType, onChange);

  vcFileDialog_ShowModal(pProgramState);
#endif

  ImGui::SameLine();

  ImGui::TextUnformatted(pLabel);
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

    val |= ((int)(ImSaturate(colors[0]) * 255) << 16); // Blue
    val |= ((int)(ImSaturate(colors[1]) * 255) << 8); // Green
    val |= ((int)(ImSaturate(colors[2]) * 255) << 0); // Red
    val |= ((int)(ImSaturate(colors[3]) * 255) << 24); // Alpha

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

 uint32_t vcIGSW_ImGuiToBGRA(udFloat4 const & vec)
{
  uint32_t val = 0;

  udFloat4 clamped = udClamp(vec, udFloat4::create(0.f, 0.f, 0.f, 0.f), udFloat4::create(1.f, 1.f, 1.f, 1.f));

  val |= ((uint32_t)(clamped[0] * 255) << 16); // Blue
  val |= ((uint32_t)(clamped[1] * 255) << 8); // Green
  val |= ((uint32_t)(clamped[2] * 255) << 0); // Red
  val |= ((uint32_t)(clamped[3] * 255) << 24); // Alpha

  return val;
}

uint32_t vcIGSW_BGRAToRGBAUInt32(uint32_t lineColour)
{
  // BGRA to RGBA
  return ((lineColour & 0xff) << 16) | (lineColour & 0x0000ff00) | (((lineColour >> 16) & 0xff) << 0) | (lineColour & 0xff000000);
}

bool vcIGSW_IsItemHovered(ImGuiHoveredFlags flags, float /*timer*/ /*= 0.5f*/)
{
  return ImGui::IsItemHovered(flags);
}

void vcIGSW_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, const char *pToolTip /*= nullptr*/)
{
  const char *loadingChars[] = { "\xE2\x96\xB2", "\xE2\x96\xB6", "\xE2\x96\xBC", "\xE2\x97\x80" };
  int64_t currentLoadingChar = (int64_t)(10 * udGetEpochSecsUTCf());

  // Load Status (if any)
  if (loadStatus == vcSLS_Pending)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Yellow Exclamation in Triangle
    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get(pToolTip == nullptr ? "sceneExplorerPending" : pToolTip));
  }
  else if (loadStatus == vcSLS_Loading)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", loadingChars[currentLoadingChar % udLengthOf(loadingChars)]); // Yellow Spinning clock
    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get(pToolTip == nullptr ? "sceneExplorerLoading" : pToolTip));
  }
  else if (loadStatus == vcSLS_Failed || loadStatus == vcSLS_OpenFailure || loadStatus == vcSLS_Corrupt)
  {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    if (vcIGSW_IsItemHovered())
    {
      if (loadStatus == vcSLS_OpenFailure)
        ImGui::SetTooltip("%s", vcString::Get(pToolTip == nullptr ? "sceneExplorerErrorOpen" : pToolTip));
      else if (loadStatus == vcSLS_Corrupt)
        ImGui::SetTooltip("%s", vcString::Get(pToolTip == nullptr ? "sceneExplorerErrorCorrupt" : pToolTip));
      else
        ImGui::SetTooltip("%s", vcString::Get(pToolTip == nullptr ? "sceneExplorerErrorLoad" : pToolTip));
    }
  }
  else if (loadStatus == vcSLS_Success)
  {
    ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "\xE2\x9C\x93"); // Green Checkmark
    if (pToolTip != nullptr && vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get(pToolTip));
  }

  ImGui::SameLine();
}

bool vcIGSW_StickyIntSlider(const char* label, int* v, int v_min, int v_max, int sticky)
{
  int stickThreshold = v_max / 500;

  if (*v > (v_max - stickThreshold))
    *v = v_max;
  else if (*v < stickThreshold)
    *v = v_min;
  else if (*v >(sticky - stickThreshold) && *v < (sticky + stickThreshold))
    *v = sticky;

  if (ImGui::SliderInt(label, v, v_min, v_max, "%d"))
  {
    *v = udClamp(*v, v_min, v_max);
    return true;
  }
  return false;
}

void vcIGSW_verticalSplitter(const char* label, const udFloat2& size, const int currentColumnIndex, float currentColumnWidth, float leftColumnWidth)
{
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::SameLine();
  ImGui::InvisibleButton(label, ImVec2(size.x, size.y));
  if (ImGui::IsItemHovered())
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  }
  if (ImGui::IsItemActive())
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    float _currentColumnWidth = (float)udMax(10.0, currentColumnWidth - ImGui::GetIO().MouseDelta.x);
    float _leftColumnWidth = (float)udMax(5.0, leftColumnWidth + ImGui::GetIO().MouseDelta.x);
    ImGui::SetColumnWidth(currentColumnIndex - 1, _leftColumnWidth);
    ImGui::SetColumnWidth(currentColumnIndex, _currentColumnWidth);
  }
  ImGui::SameLine();
  ImGui::PopStyleVar();
}

void vcIGSW_MarkDownLinkCallback(ImGui::MarkdownLinkCallbackData data)
{
  char buffer[256] = {};
  udStrncpy(buffer, data.link, data.linkLength);

  if (!data.isImage)
  {
    if (data.link[0] == '#')
    {
      // Needs to navigate to
    }
    else
    {
      vcWebFile_OpenBrowser(buffer);
    }
  }
}

inline ImGui::MarkdownImageData vcIGSW_MarkDownImageCallback(ImGui::MarkdownLinkCallbackData data)
{
  vcIGSWMarkdownContext *pContext = (vcIGSWMarkdownContext *)data.userData;
  ImGui::MarkdownImageData imageData = {};
  char buffer[256] = "\0";

  if (pContext->pPath == nullptr)
    udStrncpy(buffer, data.link, data.linkLength);
  else
    udSprintf(buffer, "%s/%.*s", pContext->pPath, data.linkLength, data.link);

  vcTexture *pImage = vcTextureCache_Get(buffer, vcTFM_Linear);

  if (pImage != nullptr)
  {
    float scalar = 1.f;
    int width = 0;
    int height = 0;
    vcTexture_GetSize(pImage, &width, &height);

    if (width > pContext->areaSize.x)
      scalar = pContext->areaSize.x / width;

    imageData.isValid = true;
    imageData.size = ImVec2(scalar * width, scalar * height);
    imageData.user_texture_id = pImage;

    vcTextureCache_Release(&pImage);
  }
  else
  {
    imageData.isValid = false;
  }

  return imageData;
}

void vcIGSW_MarkDownToolTip(ImGui::MarkdownTooltipCallbackData data)
{
  if (data.linkData.isImage)
  {
    ImGui::SetTooltip("%.*s", data.linkData.textLength, data.linkData.text);
  }
  else
  {
    const char *pString = nullptr;
    udSprintf(&pString, "%s\n%.*s", vcString::Get("linkConfirmBrowserTooltip"), data.linkData.linkLength, data.linkData.link);
    if (pString != nullptr)
      ImGui::SetTooltip("%s", pString);
    udFree(pString);
  }
}

void vcIGSW_Markdown(vcState *pProgramState, const char *pMarkdownText, const char *pPath /*= nullptr*/)
{
  vcIGSWMarkdownContext context = {};
  context.areaSize = ImGui::GetContentRegionAvail();
  context.pPath = pPath;

  ImGui::MarkdownConfig config = {};

  config.userData = &context;

  config.linkCallback = vcIGSW_MarkDownLinkCallback;
  config.imageCallback = vcIGSW_MarkDownImageCallback;
  config.tooltipCallback = vcIGSW_MarkDownToolTip;

  config.headingFormats[0].font = pProgramState->pBigFont;
  config.headingFormats[1].font = pProgramState->pMidFont;

  config.boldFont = pProgramState->pBoldFont;
  config.italicFont = pProgramState->pItalicFont;
  config.boldItalicFont = pProgramState->pBoldItalicFont;

  ImGui::Markdown(pMarkdownText, udStrlen(pMarkdownText), config);
}

void vcIGSW_URLText(const char *pMessage, const char *pURL, bool *pHoveredStatus)
{
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[(pHoveredStatus != nullptr && *pHoveredStatus) ? ImGuiCol_ButtonHovered : ImGuiCol_ButtonActive]);
  ImGui::TextUnformatted(pMessage);
  ImGui::PopStyleColor();

  bool hovered = ImGui::IsItemHovered();
  if (hovered)
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(pURL);
    ImGui::EndTooltip();
  }

  if (pHoveredStatus != nullptr)
  {
    *pHoveredStatus = hovered;
    if (*pHoveredStatus && ImGui::IsMouseClicked(0))
      vcWebFile_OpenBrowser(pURL);
  }
}

bool vcIGSW_DegreesScalar(const char *pLabel, udDouble2 *pData)
{
  udDouble2 tmp = UD_RAD2DEG(*pData);
  if (ImGui::InputScalarN(pLabel, ImGuiDataType_Double, &tmp, 2))
  {
    *pData = UD_DEG2RAD(tmp);
    return true;
  }
  return false;
}

bool vcIGSW_DegreesScalar(const char *pLabel, udDouble3 *pData)
{
  udDouble3 tmp = UD_RAD2DEG(*pData);
  if (ImGui::InputScalarN(pLabel, ImGuiDataType_Double, &tmp, 3))
  {
    *pData = UD_DEG2RAD(tmp);
    return true;
  }
  return false;
}
