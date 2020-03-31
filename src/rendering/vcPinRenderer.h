#ifndef vcPinRenderer_h__
#define vcPinRenderer_h__

#include "vcRender.h"

struct vcPinRenderer;

udResult vcPinRenderer_Create(vcPinRenderer **ppPinRenderer);
udResult vcPinRenderer_Destroy(vcPinRenderer **ppPinRenderer);

bool vcPinRenderer_ConditionalTurnIntoPin(vcPinRenderer *pPinRenderer, vcState *pProgramState, const char *pPinIconURL, udDouble3 position);

void vcPinRenderer_Reset(vcPinRenderer *pPinRenderer);
void vcPinRenderer_Render(vcPinRenderer *pPinRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize);

#endif//vcPinRenderer_h__
