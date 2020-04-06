#ifndef vcAtmosphereRenderer_h__
#define vcAtmosphereRenderer_h__

#include "udResult.h"

struct vcAtmosphereRenderer;
struct vcState;
struct vcTexture;

udResult vcAtmosphereRenderer_Create(vcAtmosphereRenderer **ppAtmosphereRenderer);
udResult vcAtmosphereRenderer_Destroy(vcAtmosphereRenderer **ppAtmosphereRenderer);

void vcAtmosphereRenderer_DestroyShaders(vcAtmosphereRenderer *pAtmosphereRenderer);
udResult vcAtmosphereRenderer_LoadShaders(vcAtmosphereRenderer *pAtmosphereRenderer);

void vcAtmosphereRenderer_SetVisualParams(vcAtmosphereRenderer *pAtmosphereRenderer, float exposure, float timeOfDay);

bool vcAtmosphereRenderer_Render(vcAtmosphereRenderer *pAtmosphereRenderer, vcState *pProgramState, vcTexture *pSceneColour, vcTexture *pSceneDepth);

#endif//vcAtmosphereRenderer_h__
