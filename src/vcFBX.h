#ifndef vcFBX_h__
#define vcFBX_h__

#ifdef FBXSDK_ON
#include "udConvertCustom.h"

udError vcFBX_AddItem(udConvertContext *pConvertContext, const char *pFilename);
#endif

#endif//vcFBX_h__
