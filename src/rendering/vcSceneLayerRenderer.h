#ifndef vcSceneLayerRenderer_h__
#define vcSceneLayerRenderer_h__

#include "vcSceneLayer.h"

struct vcSceneLayerRenderer
{
  vcSceneLayer *pSceneLayer;
  udDouble4 frustumPlanes[6];

  udDouble4x4 sceneMatrix;
};

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayerRenderer, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayerRenderer);

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution);

#endif//vcSceneLayerRenderer_h__
