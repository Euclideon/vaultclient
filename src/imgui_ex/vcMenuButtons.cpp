#include "vcMenuButtons.h"
#include "vcImGuiSimpleWidgets.h"
#include "vcTexture.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include "imgui.h"

static const float buttonSize = 24.f;
static const float textureRelativeButtonSize = 240.f;
static const float buttonUVSize = buttonSize / textureRelativeButtonSize;

udFloat4 vcGetIconUV(vcMenuBarButtonIcon iconIndex)
{
  float buttonX = (iconIndex % (int)(textureRelativeButtonSize / buttonSize)) * buttonUVSize;
  float buttonY = (iconIndex / (int)(textureRelativeButtonSize / buttonSize)) * buttonUVSize;

  return udFloat4::create(buttonX, buttonY, buttonX + buttonUVSize, buttonY + buttonUVSize);
}

bool vcMenuBarButton(vcTexture *pUITexture, const char *pButtonName, const char *pKeyCode, const vcMenuBarButtonIcon buttonIndex, vcMenuBarButtonGap gap, bool selected /*= false*/, float scale /*= 1.f*/)
{
  const ImVec4 DefaultBGColor = ImVec4(0, 0, 0, 0);
  const ImVec4 EnabledColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
  const float MBtn_Gap = 10.f;
  const float MBtn_Padding = 2.f;

  udFloat4 iconUV = vcGetIconUV(buttonIndex);

  bool retVal = false;

  if (gap == vcMBBG_SameGroup)
    ImGui::SameLine(0.f, MBtn_Padding);
  else if (gap == vcMBBG_NewGroup)
    ImGui::SameLine(0.f, MBtn_Gap);

  ImGui::PushID(pButtonName);
  if (pUITexture != nullptr)
    retVal = ImGui::ImageButton(pUITexture, ImVec2(buttonSize * scale, buttonSize * scale), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w), 2, selected ? EnabledColor : DefaultBGColor);
  else
    retVal = ImGui::Button(udTempStr("?###%s", pButtonName), ImVec2(buttonSize * scale, buttonSize * scale));

  if (vcIGSW_IsItemHovered())
  {
    if (pKeyCode == nullptr)
      ImGui::SetTooltip("%s", pButtonName);
    else
      ImGui::SetTooltip("%s [%s]", pButtonName, pKeyCode);
  }
  ImGui::PopID();

  return retVal;
}
