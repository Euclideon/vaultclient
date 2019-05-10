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

// TODO: (EVC-550) Split this giant file out into individual model/renderer/converter files
udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL, const vcSceneLayerRendererMode &mode = vcSLRM_Rendering);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayer);

// TODO: add a _Load() function, and use instead of vcSceneLayerRendererMode parameter
// inside _create should not load any node data, including the root IMO
// then when converting, call this function in the _Open() callback

// TODO: (EVC-550) Split this giant file out into individual model/renderer/converter files
bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution);

// TODO: (EVC-550) Split this giant file out into individual model/renderer/converter files
#include "vdkConvertCustom.h"
#include "vdkTriangleVoxelizer.h"
vdkError vcSceneLayerRenderer_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, const char *pFilename);

#endif//vcSceneLayerRenderer_h__
