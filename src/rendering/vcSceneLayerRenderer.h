#ifndef vcSceneLayerRenderer_h__
#define vcSceneLayerRenderer_h__

#include "vcSceneLayer.h"

struct vcSceneLayerRenderer;

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayerRenderer);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayerRenderer);

// TODO: (EVC-550) Split this giant file out into individual model/renderer/converter files
bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution);

#endif//vcSceneLayerRenderer_h__
