#ifndef vcLineRenderer_h__
#define vcLineRenderer_h__

#include "udMath.h"

struct vcLineRenderer;

struct vcLineData
{
  udFloat4 colour;
  float thickness;
  udDouble3 *pPoints;
  int pointCount;
};

udResult vcLineRenderer_Create(vcLineRenderer **ppLineRenderer);
udResult vcLineRenderer_Destroy(vcLineRenderer **ppLineRenderer);

udResult vcLineRenderer_ReloadShaders(vcLineRenderer *pLineRenderer);

udResult vcLineRenderer_AddLine(vcLineRenderer *pLineRenderer, const vcLineData &data);
void vcLineRenderer_ClearLines(vcLineRenderer *pLineRenderer);

bool vcLineRenderer_Render(vcLineRenderer *pLineRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, double deltaTime);

#endif//vcLineRenderer_h__
