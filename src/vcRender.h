#ifndef vcRender_h__
#define vcRender_h__

#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udValue.h"

#include "vdkRenderContext.h"
#include "vdkRenderView.h"

#include "vcRenderUtils.h"
#include "vcSettings.h"

struct vcRenderContext;

struct vcModel
{
  char modelPath[1024];
  bool modelLoaded;
  bool modelVisible;
  bool modelSelected;
  vdkModel *pVaultModel;
  udValue *pMetadata;
};

struct vcRenderData
{
  uint16_t srid;

  udChunkedArray<vcModel*> models;
  udDouble4x4 cameraMatrix;
  GLuint skyboxCubemap;
};

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vdkContext *pVaultContext);

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, const vcRenderData &renderData, GLuint defaultFramebuffer);

void vcRenderSkybox(vcRenderContext *pRenderContext);

#endif//vcRender_h__
