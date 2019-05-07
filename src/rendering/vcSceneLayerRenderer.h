#ifndef vcSceneLayerRenderer_h__
#define vcSceneLayerRenderer_h__

// I3D Layer specifications:
// https://github.com/Esri/i3s-spec/blob/master/format/Indexed%203d%20Scene%20Layer%20Format%20Specification.md
// https://docs.opengeospatial.org/cs/17-014r5/17-014r5.html

#include "udPlatform/udMath.h"

struct vcSceneLayerRenderer;

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, const char *pSceneLayerURL);
udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayer);

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix);

#endif//vcSceneLayerRenderer_h__
