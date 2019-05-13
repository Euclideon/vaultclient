#ifndef vcSceneLayerConvert_h__
#define vcSceneLayerConvert_h__

#include "vcSceneLayer.h"

#include "vdkError.h"

struct vcSceneLayerConvert;

struct vdkContext;
struct vdkConvertContext;
struct vWorkerThreadPool;

vdkError vcSceneLayerConvert_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL);

#endif//vcSceneLayerConvert_h__

