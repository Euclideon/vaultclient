#include "vcLabelRenderer.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"

static const int vcLFSToPixelSize[] = { 6, 8, 10 };
UDCOMPILEASSERT(udLengthOf(vcLFSToPixelSize) == vcLFS_Count, "LabelFontSize not equal size");

bool vcLabelRenderer_Render(vcLabelInfo *pLabelRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize)
{
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  if (!drawList || !pLabelRenderer->pText)
    return false;

  ImVec2 labelSize = ImGui::CalcTextSize(pLabelRenderer->pText);

  udDouble4 screenPosition = viewProjectionMatrix * udDouble4::create(pLabelRenderer->worldPosition, 1.0);
  screenPosition = screenPosition / screenPosition.w;
  screenPosition.x = (0.5 * screenPosition.x + 0.5);
  screenPosition.y = 1.0 - (0.5 * screenPosition.y + 0.5);

  if (screenPosition.x < 0 || screenPosition.x > 1 || screenPosition.y < 0 || screenPosition.y > 1 || screenPosition.z < -1 || screenPosition.z > 1)
    return true;

  ImVec2 winMin = ImGui::GetWindowPos();
  ImVec2 windowPosition = ImVec2(float(screenPosition.x * screenSize.x) + winMin.x, float(screenPosition.y * screenSize.y) + winMin.y);

  drawList->AddQuadFilled(ImVec2(windowPosition.x, windowPosition.y), ImVec2(windowPosition.x + labelSize.x, windowPosition.y), ImVec2(windowPosition.x + labelSize.x, windowPosition.y + labelSize.y), ImVec2(windowPosition.x, windowPosition.y + labelSize.y), pLabelRenderer->backColourRGBA);
  drawList->AddText(windowPosition, pLabelRenderer->textColourRGBA, pLabelRenderer->pText);

  return true;
}
