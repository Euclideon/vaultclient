#ifndef vcI3S_h__
#define vcI3S_h__

#include "udPlatform/udMath.h"

struct vcIndexed3DSceneLayer;

udResult vcIndexed3DSceneLayer_Create(vcIndexed3DSceneLayer **ppSceneLayer, const char *pSceneLayerURL);
udResult vcIndexed3DSceneLayer_Destroy(vcIndexed3DSceneLayer **ppSceneLayer);

bool vcIndexed3DSceneLayer_Render(vcIndexed3DSceneLayer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix);

#endif//vcI3S_h__
