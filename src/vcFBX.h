#ifndef vcFBX_h__
#define vcFBX_h__

#ifdef FBXSDK_ON
#include "vdkConvertCustom.h"

vdkError vcFBX_AddItem(vdkConvertContext *pConvertContext, const char *pFilename);
#endif

#endif//vcFBX_h__
