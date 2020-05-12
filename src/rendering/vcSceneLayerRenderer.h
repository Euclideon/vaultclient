#ifndef vcSceneLayerRenderer_h__
#define vcSceneLayerRenderer_h__

#include "vcSceneLayer.h"

struct vcTexture;

struct vcSceneLayerRenderer
{
  vcSceneLayer *pSceneLayer;
};

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayerRenderer, udWorkerPool *pWorkerThreadPool, const char *pSceneLayerURL);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayerRenderer);

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, const udDouble4x4 &worldMatrix, const udDouble4x4 &viewProjectionMatrix, const udDouble3 &cameraPosition, const udUInt2 &screenResolution, const udFloat4 *pColourOverride = nullptr, bool shadowsPass = false);

#endif//vcSceneLayerRenderer_h__
