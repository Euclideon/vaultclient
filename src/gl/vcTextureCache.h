#ifndef vcTextureCache_h__
#define vcTextureCache_h__

#include "vcTexture.h"
#include "udWorkerPool.h"

void vcTextureCache_InitGlobal(udWorkerPool *pWorkerPool);
void vcTextureCache_DeinitGlobal();

void vcTextureCache_ConditionallyPruneGlobal();

vcTexture* vcTextureCache_Get(const char *pAssetName, vcTextureFilterMode filterMode = vcTFM_Nearest, bool hasMipmaps = false, vcTextureWrapMode wrapMode = vcTWM_Repeat);
void vcTextureCache_Release(vcTexture **ppTexture);

#endif
