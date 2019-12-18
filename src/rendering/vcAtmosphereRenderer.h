#ifndef vcAtmosphereRenderer_h__
#define vcAtmosphereRenderer_h__

#include "udResult.h"

struct vcAtmosphereRenderer;
struct vcState;
struct vcTexture;

udResult vcAtmosphereRenderer_Create(vcAtmosphereRenderer **ppAtmosphereRenderer);
udResult vcAtmosphereRenderer_Destroy(vcAtmosphereRenderer **ppAtmosphereRenderer);

bool vcAtmosphereRenderer_Render(vcAtmosphereRenderer *pAtmosphereRenderer, vcState *pProgramState, vcTexture *pSceneColour, vcTexture *pSceneDepth);

#endif//vcAtmosphereRenderer_h__
