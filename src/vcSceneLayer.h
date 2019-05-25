#ifndef vcSceneLayer_h__
#define vcSceneLayer_h__

// I3S Layer specifications:
// https://github.com/Esri/i3s-spec/blob/master/format/Indexed%203d%20Scene%20Layer%20Format%20Specification.md
// https://docs.opengeospatial.org/cs/17-014r5/17-014r5.html

#include "udMath.h"

struct vcSceneLayer;
struct vWorkerThreadPool;

// `vcSceneLayer_Create()` will load the root node
udResult vcSceneLayer_Create(vcSceneLayer **ppSceneLayer, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL);
udResult vcSceneLayer_Destroy(vcSceneLayer **ppSceneLayer);

#endif//vcSceneLayer_h__
