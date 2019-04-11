#ifndef vcImageRenderer_h__
#define vcImageRenderer_h__

#include "udPlatform/udMath.h"
#include "vcTexture.h"

enum vcImageSize
{
  vcIS_Small,
  vcIS_Large,

  vcIS_Count,
};

struct vcImageRenderInfo
{
  udDouble3 worldPosition;

  vcTexture *pTexture;
  vcImageSize size;
  uint32_t colourRGBA;
};

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udDouble4x4 &viewMatrix, const udUInt2 &screenSize);

#endif//vcImageRenderer_h__
