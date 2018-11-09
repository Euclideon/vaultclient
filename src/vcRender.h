#ifndef vcRender_h__
#define vcRender_h__

#include "vcState.h"
#include "vcModel.h"
#include "vcCompass.h"

#include "vdkRenderContext.h"
#include "vdkRenderView.h"

#include "gl/vcMesh.h"

struct vcRenderContext;
struct vcTexture;

struct vcRenderData
{
  vcGISSpace *pGISSpace;

  udInt2 mouse;
  udDouble3 worldMousePos;
  udRay<double> worldMouseRay; // Gets calcuated during the render

  udDouble3 *pWorldAnchorPos; // If this is not nullptr, this is the point to highlight
  bool pickingSuccess;
  vcTexture *pWatermarkTexture;

  udChunkedArray<vcModel*> models;
  udDouble4x4 cameraMatrix;
  vcCameraSettings *pCameraSettings;
  vcTexture *pSkyboxCubemap;
};

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vdkContext *pVaultContext);

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

vcTexture* vcRender_RenderScene(vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer);

udResult vcRender_CreateTerrain(vcRenderContext *pRenderContext, vcSettings *pSettings);
udResult vcRender_DestroyTerrain(vcRenderContext *pRenderContext);

#endif//vcRender_h__
