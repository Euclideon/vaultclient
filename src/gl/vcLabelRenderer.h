#ifndef vcLabelRenderer_h__
#define vcLabelRenderer_h__

#include "udPlatform/udMath.h"

struct vcLabelRenderer;

enum vcLabelFontSize
{
  vcLFS_Small,
  vcLFS_Medium,
  vcLFS_Large,

  vcLFS_Count,
};

struct vcLabelRendererConfig
{
  udDouble3 worldPosition;

  char *pText;
  vcLabelFontSize textSize;
  uint32_t textColour;
};

udResult vcLabelRenderer_Create(vcLabelRenderer **ppLabelRenderer);
udResult vcLabelRenderer_Destroy(vcLabelRenderer **ppLabelRenderer);

bool vcLabelRenderer_Render(vcLabelRenderer *pLabelRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize);

udResult vcLabelRenderer_SetConfig(vcLabelRenderer *pLabelRenderer, const vcLabelRendererConfig &config);

#endif//vcLabelRenderer_h__
