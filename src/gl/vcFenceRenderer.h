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

struct vcFenceRendererConfig
{
  vcFenceRendererVisualMode visualMode;
  vcFenceRendererImageMode imageMode;
  udFloat4 bottomColour;
  udFloat4 topColour;
  float ribbonWidth;
  float textureRepeatScale;
  float textureScrollSpeed;
};

udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer);
udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer);

udResult vcFenceRenderer_SetPoints(vcFenceRenderer *pFenceRenderer, udDouble3 *pPoints, size_t pointCount, bool recalculateCachedPoints = true);
udResult vcFenceRenderer_SetConfig(vcFenceRenderer *pFenceRenderer, const vcFenceRendererConfig &config);

bool vcFenceRenderer_Render(vcFenceRenderer *pFenceRenderer, const udDouble4x4 &viewProjectionMatrix, double deltaTime);

#endif//vcFenceRenderer_h__
