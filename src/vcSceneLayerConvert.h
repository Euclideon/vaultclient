#ifndef vcSceneLayerConvert_h__
#define vcSceneLayerConvert_h__

#include "vcSceneLayer.h"

#include "vdkError.h"

struct vcSceneLayerConvert;

struct vdkConvertContext;
struct vWorkerThreadPool;

vdkError vcSceneLayerConvert_AddItem(vdkConvertContext *pConvertContext, const char *pSceneLayerURL);

#endif//vcSceneLayerConvert_h__

