#include "vcLabelRenderer.h"
#include "udPlatformUtil.h"

#include "imgui.h"

bool vcLabelRenderer_Render(vcLabelInfo *pLabelRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize)
{
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  if (!drawList || !pLabelRenderer->pText)
    return false;

  // These values were picked by visual inspection
  if (pLabelRenderer->textSize == vcLFS_Large)
    ImGui::SetWindowFontScale(1.2f);
  else if (pLabelRenderer->textSize == vcLFS_Small)
    ImGui::SetWindowFontScale(0.87f);

  ImVec2 halfLabelSize = ImGui::CalcTextSize(pLabelRenderer->pText);
  halfLabelSize.x *= 0.5;
  halfLabelSize.y *= 0.5;

  udDouble4 screenPosition = viewProjectionMatrix * udDouble4::create(pLabelRenderer->worldPosition, 1.0);
  screenPosition = screenPosition / screenPosition.w;
  screenPosition.x = (0.5 * screenPosition.x + 0.5);
  screenPosition.y = 1.0 - (0.5 * screenPosition.y + 0.5);

  if (screenPosition.x > 0 && screenPosition.x < 1 && screenPosition.y > 0 && screenPosition.y < 1 && screenPosition.z > -1 && screenPosition.z < 1)
  {
    ImVec2 winMin = ImGui::GetWindowPos();
    ImVec2 windowPosition = ImVec2(float(screenPosition.x * screenSize.x) + winMin.x, float(screenPosition.y * screenSize.y) + winMin.y);

    drawList->AddQuadFilled(ImVec2(windowPosition.x - halfLabelSize.x, windowPosition.y - halfLabelSize.y), ImVec2(windowPosition.x + halfLabelSize.x, windowPosition.y - halfLabelSize.y), ImVec2(windowPosition.x + halfLabelSize.x, windowPosition.y + halfLabelSize.y), ImVec2(windowPosition.x - halfLabelSize.x, windowPosition.y + halfLabelSize.y), pLabelRenderer->backColourRGBA);
    drawList->AddText(ImVec2(windowPosition.x - halfLabelSize.x, windowPosition.y - halfLabelSize.y), pLabelRenderer->textColourRGBA, pLabelRenderer->pText);
  }

  if (pLabelRenderer->textSize != vcLFS_Medium)
    ImGui::SetWindowFontScale(1.f);

  return true;
}
