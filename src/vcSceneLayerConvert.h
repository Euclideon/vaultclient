#ifndef vcSceneLayerConvert_h__
#define vcSceneLayerConvert_h__

#include "vcSceneLayer.h"

#include "udError.h"

struct vcSceneLayerConvert;

struct udConvertContext;
struct vWorkerThreadPool;

udError vcSceneLayerConvert_AddItem(udConvertContext *pConvertContext, const char *pSceneLayerURL);

#endif//vcSceneLayerConvert_h__

