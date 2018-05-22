#ifndef vcRender_h__
#define vcRender_h__

#include "vcRenderUtils.h"
#include "vaultUDRenderer.h"
#include "vaultUDRenderView.h"
#include "udPlatform/udChunkedArray.h"

struct vcRenderContext;
struct vaultContext;

struct vcModel
{
  char modelPath[1024];
  bool modelLoaded;
  vaultUDModel *pVaultModel;
};

struct vcRenderData
{
  udChunkedArray<vcModel*> models;
  udDouble4x4 cameraMatrix;
};

udResult vcRender_Init(vcRenderContext **pRenderContext, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vaultContext *pVaultContext);

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, const vcRenderData &renderData);

#endif//vcRender_h__
