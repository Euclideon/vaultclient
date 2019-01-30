#include "vcLabelRenderer.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"

struct vcLabelRenderer
{
  vcLabelRendererConfig config;

  // cached
  udInt2 labelSizePixels;
};

static const int vcLFSToPixelSize[] = { 6, 8, 10 };
UDCOMPILEASSERT(udLengthOf(vcLFSToPixelSize) == vcLFS_Count, "LabelFontSize not equal size");

udResult vcLabelRenderer_Create(vcLabelRenderer **ppLabelRenderer)
{
  udResult result = udR_Success;

  vcLabelRenderer *pLabelRenderer = udAllocType(vcLabelRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pLabelRenderer, udR_MemoryAllocationFailure);

  *ppLabelRenderer = pLabelRenderer;
epilogue:
  return result;
}

udResult vcLabelRenderer_Destroy(vcLabelRenderer **ppLabelRenderer)
{
  if (!ppLabelRenderer || !(*ppLabelRenderer))
    return udR_Success;

  vcLabelRenderer *pLabelRenderer = (*ppLabelRenderer);

  udFree(pLabelRenderer->config.pText);
  udFree(pLabelRenderer);

  return udR_Success;
}

udResult vcLabelRenderer_SetConfig(vcLabelRenderer *pLabelRenderer, const vcLabelRendererConfig &config)
{
  if (udStrcmp(config.pText, pLabelRenderer->config.pText) != 0)
  {
    udFree(pLabelRenderer->config.pText);
    pLabelRenderer->config.pText = udStrdup(config.pText);
  }

  pLabelRenderer->config.worldPosition = config.worldPosition;
  pLabelRenderer->config.textColour = config.textColour;
  pLabelRenderer->config.textSize = config.textSize;

  // TODO: This is just a guess for now
  pLabelRenderer->labelSizePixels = udInt2::create((int)udStrlen(pLabelRenderer->config.pText) * vcLFSToPixelSize[pLabelRenderer->config.textSize], vcLFSToPixelSize[pLabelRenderer->config.textSize] * 2);

  return udR_Success;
}

bool vcLabelRenderer_Render(vcLabelRenderer *pLabelRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize)
{
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  if (!drawList || !pLabelRenderer->config.pText)
    return false;

  udDouble4 screenPosition = viewProjectionMatrix * udDouble4::create(pLabelRenderer->config.worldPosition, 1.0);
  screenPosition = screenPosition / screenPosition.w;
  screenPosition.x = (0.5 * screenPosition.x + 0.5);
  screenPosition.y = 1.0 - (0.5 * screenPosition.y + 0.5);

  if (screenPosition.x < 0 || screenPosition.x > 1 || screenPosition.y < 0 || screenPosition.y > 1 || screenPosition.z < -1 || screenPosition.z > 1)
    return true;

  ImVec2 windowPosition = ImVec2(float(screenPosition.x * screenSize.x), float(screenPosition.y * screenSize.y));

  drawList->AddQuadFilled(ImVec2(windowPosition.x, windowPosition.y), ImVec2(windowPosition.x + pLabelRenderer->labelSizePixels.x, windowPosition.y), ImVec2(windowPosition.x + pLabelRenderer->labelSizePixels.x, windowPosition.y + pLabelRenderer->labelSizePixels.y), ImVec2(windowPosition.x, windowPosition.y + pLabelRenderer->labelSizePixels.y), 0x7F000000);
  drawList->AddText(windowPosition, pLabelRenderer->config.textColour, pLabelRenderer->config.pText);

  return true;
}
