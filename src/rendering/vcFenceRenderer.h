#ifndef vcFenceRenderer_h__
#define vcFenceRenderer_h__

#include "udMath.h"

struct vcFenceRenderer;

enum vcFenceRendererCurveType
{
  vcRRCT_Simple,
};

enum vcFenceRendererVisualMode
{
  vcRRVM_Fence,
  vcRRVM_Flat,

  vcRRVM_Count
};

enum vcFenceRendererImageMode
{
  vcRRIM_Arrow,
  vcRRIM_Glow,
  vcRRIM_Solid,
  vcRRIM_Diagonal,

  vcRRIM_Count
};

struct vcFenceRendererConfig
{
  vcFenceRendererVisualMode visualMode;
  vcFenceRendererImageMode imageMode;
  bool isDualColour;
  udFloat4 primaryColour;
  udFloat4 secondaryColour;
  float ribbonWidth;
  float textureRepeatScale;
  float textureScrollSpeed;
};

udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer);
udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer);

udResult vcFenceRenderer_AddPoints(vcFenceRenderer *pFenceRenderer, udDouble3 *pPoints, size_t pointCount, bool closed = false);
void vcFenceRenderer_ClearPoints(vcFenceRenderer *pFenceRenderer);

udResult vcFenceRenderer_SetConfig(vcFenceRenderer *pFenceRenderer, const vcFenceRendererConfig &config);

bool vcFenceRenderer_Render(vcFenceRenderer *pFenceRenderer, const udDouble4x4 &viewProjectionMatrix, double deltaTime);

#endif//vcFenceRenderer_h__
