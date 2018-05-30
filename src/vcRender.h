#ifndef vcRender_h__
#define vcRender_h__

#include "vcRenderUtils.h"
#include "vaultUDRenderer.h"
#include "vaultUDRenderView.h"
#include "udPlatform/udChunkedArray.h"
#include "vcSettings.h"
#include "udPlatform/udValue.h"

struct vcRenderContext;
struct vaultContext;

struct vcModel
{
  char modelPath[1024];
  bool modelLoaded;
  vaultUDModel *pVaultModel;
  udValue metadata;
};

struct vcRenderData
{
  uint16_t srid;

  udChunkedArray<vcModel*> models;
  udDouble4x4 cameraMatrix;
  GLuint skyboxCubemap;
};

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vaultContext *pVaultContext);

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, const vcRenderData &renderData, GLuint defaultFramebuffer);

void vcRenderSkybox(vcRenderContext *pRenderContext);

#endif//vcRender_h__
