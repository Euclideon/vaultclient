#ifndef vcSceneLayerRenderer_h__
#define vcSceneLayerRenderer_h__

// I3D Layer specifications:
// https://github.com/Esri/i3s-spec/blob/master/format/Indexed%203d%20Scene%20Layer%20Format%20Specification.md
// https://docs.opengeospatial.org/cs/17-014r5/17-014r5.html

#include "udPlatform/udMath.h"

enum vcSceneLayerRendererMode
{
  vcSLRM_Convert,   // loads entire model to memory on create
  vcSLRM_Rendering  // load model nodes as needed, uploads to gpu and throws out memory
};

struct vcSceneLayerRenderer;
struct vcSettings;

struct vWorkerThreadPool;

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, const vcSettings *pSettings, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL, const vcSceneLayerRendererMode &mode = vcSLRM_Rendering);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayer);

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution);

#endif//vcSceneLayerRenderer_h__
