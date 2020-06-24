#ifndef vcPinRenderer_h__
#define vcPinRenderer_h__

#include "vcRender.h"

struct vcPinRenderer;

udResult vcPinRenderer_Create(vcPinRenderer **ppPinRenderer);
udResult vcPinRenderer_Destroy(vcPinRenderer **ppPinRenderer);

bool vcPinRenderer_AddPin(vcPinRenderer *pPinRenderer, vcState *pProgramState, vcSceneItem *pSceneItem, const char *pPinIconURL, udDouble3 position, int count);

void vcPinRenderer_Reset(vcPinRenderer *pPinRenderer);
void vcPinRenderer_Render(vcPinRenderer *pPinRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, udChunkedArray<vcLabelInfo*> &labels);

#endif//vcPinRenderer_h__
