#ifndef vcLineRenderer_h__
#define vcLineRenderer_h__

#include "udMath.h"

struct vcLineRenderer;
struct vcLineInstance;

udResult vcLineRenderer_Create(vcLineRenderer **ppLineRenderer);
udResult vcLineRenderer_Destroy(vcLineRenderer **ppLineRenderer);

udResult vcLineRenderer_ReloadShaders(vcLineRenderer *pLineRenderer);

bool vcLineRenderer_Render(vcLineRenderer *pLineRenderer, const vcLineInstance *pLine, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize);

udResult vcLineRenderer_CreateLine(vcLineInstance **ppLine);
udResult vcLineRenderer_UpdatePoints(vcLineInstance *pLine, const udDouble3 *pPoints, size_t pointCount, const udFloat4 &colour, float width, bool closed);
udResult vcLineRenderer_DestroyLine(vcLineInstance **ppLine);

#endif//vcLineRenderer_h__
