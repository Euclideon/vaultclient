#include "vcImageRenderer.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"

// These values were picked by visual inspection
static const float vcISToPixelSize[] = { 150.0, 350.0 };
UDCOMPILEASSERT(udLengthOf(vcISToPixelSize) == vcIS_Count, "ImageSize not equal size");

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udDouble4x4 &viewMatrix, const udUInt2 &screenSize)
{
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  if (!drawList || !pImageInfo->pTexture)
    return false;

  udDouble4 viewPosition = viewMatrix * udDouble4::create(pImageInfo->worldPosition, 1.0);
  double zScale = 1.0 - (viewPosition.y / vcISToPixelSize[pImageInfo->size]);
  if (zScale <= 0.0)
    return false;

  udDouble4 screenPosition = viewProjectionMatrix * udDouble4::create(pImageInfo->worldPosition, 1.0);
  screenPosition = screenPosition / screenPosition.w;
  screenPosition.x = (0.5 * screenPosition.x + 0.5);
  screenPosition.y = 1.0 - (0.5 * screenPosition.y + 0.5);

  // TODO: Does this need to be set?
  // ImGui::SetNextWindowSize(ImVec2(size.x, size.y));

  if (screenPosition.z > -1 && screenPosition.z < 1)
  {
    ImVec2 winMin = ImGui::GetWindowPos();
    ImVec2 windowPosition = ImVec2(float(screenPosition.x * screenSize.x) + winMin.x, float(screenPosition.y * screenSize.y) + winMin.y);

    float halfSize = float(vcISToPixelSize[pImageInfo->size] * zScale * 0.5);
    drawList->AddImage(pImageInfo->pTexture, ImVec2(windowPosition.x - halfSize, windowPosition.y - halfSize), ImVec2(windowPosition.x + halfSize, windowPosition.y + halfSize), ImVec2(0, 0), ImVec2(1, 1), pImageInfo->colourRGBA);
  }

  return true;
}
