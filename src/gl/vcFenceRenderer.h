#ifndef vcFenceRenderer_h__
#define vcFenceRenderer_h__

#include "udPlatform/udMath.h"

struct vcFenceRenderer;

enum vcFenceRendererCurveType
{
  vcRRCT_Simple,
};

enum vcFenceRendererVisualMode
{
  vcRRVM_Fence,
  vcRRVM_Flat,
};

enum vcFenceRendererImageMode
{
  vcRRIM_Arrow,
  vcRRIM_Glow,
  vcRRIM_Solid,
};

udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer);
udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer);

udResult vcFenceRenderer_SetPoints(vcFenceRenderer *pFenceRenderer, udFloat3 *pPoints, int pointCount);
void vcFenceRenderer_SetVisualMode(vcFenceRenderer *pFenceRenderer, vcFenceRendererVisualMode visualMode);
void vcFenceRenderer_SetImageMode(vcFenceRenderer *pFenceRenderer, vcFenceRendererImageMode imageMode);
void vcFenceRenderer_SetColours(vcFenceRenderer *pFenceRenderer, const udFloat4 &bottomColour, const udFloat4 &topColour);
void vcFenceRenderer_SetWidth(vcFenceRenderer *pFenceRenderer, float ribbonWidth);
void vcFenceRenderer_SetTextureRepeatScale(vcFenceRenderer *pFenceRenderer, float textureRepeatScale);
void vcFenceRenderer_SetTextureScrollSpeed(vcFenceRenderer *pFenceRenderer, float textureScrollSpeed);

bool vcFenceRenderer_Render(vcFenceRenderer *pFenceRenderer, const udDouble4x4 &viewProjectionMatrix, double deltaTime);

#endif//vcFenceRenderer_h__
