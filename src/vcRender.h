#ifndef vcRender_h__
#define vcRender_h__

#include "vcState.h"
#include "vcModel.h"

#include "vdkRenderContext.h"
#include "vdkRenderView.h"

#include "gl/vcMesh.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcWaterRenderer.h"
#include "vcImageRenderer.h"
#include "vcCompass.h"
#include "vcPolygonModel.h"

struct vcRenderContext;
struct vcTexture;
struct vcSceneLayerRenderer;
struct udWorkerPool;

struct vcRenderPolyInstance
{
  vcPolygonModel *pModel;
  udDouble4x4 worldMat; // will be converted to eye internally

  vcSceneItem *pSceneItem;
  uint64_t sceneItemInternalId;
};

struct vcMouseData
{
  udInt2 position;
  bool clicked;
};

struct vcRenderData
{
  vcGISSpace *pGISSpace;

  double deltaTime;

  vcMouseData mouse;
  udDouble3 worldMousePos;

  udDouble3 *pWorldAnchorPos; // If this is not nullptr, this is the point to highlight
  bool pickingSuccess;
  int udModelPickedIndex;
  vcTexture *pWatermarkTexture;

  udChunkedArray<vcModel*> models;
  udChunkedArray<vcFenceRenderer*> fences;
  udChunkedArray<vcLabelInfo*> labels;
  udChunkedArray<vcRenderPolyInstance> polyModels;
  udChunkedArray<vcWaterRenderer*> waterVolumes;
  udChunkedArray<vcImageRenderInfo*> images;
  udChunkedArray<vcSceneLayerRenderer*> sceneLayers;

  vcCamera *pCamera;
  vcCameraSettings *pCameraSettings;
};

udResult vcRender_Init(vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vdkContext *pVaultContext);

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

void vcRender_BeginFrame(vcRenderContext *pRenderContext);
vcTexture* vcRender_GetSceneTexture(vcRenderContext *pRenderContext);
void vcRender_RenderScene(vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer);
void vcRender_vcRenderSceneImGui(vcRenderContext *pRenderContext, const vcRenderData &renderData);

void vcRender_ClearTiles(vcRenderContext *pRenderContext);
void vcRender_ClearPoints(vcRenderContext *pRenderContext);

struct vcRenderPickResult
{
  bool success;
  udDouble3 position;

  vcModel *pModel;
  vcRenderPolyInstance *pPolygon;
};
vcRenderPickResult vcRender_PolygonPick(vcRenderContext *pRenderContext, vcRenderData &renderData);
bool vcRender_PickTiles(vcRenderContext *pRenderContext, udDouble3 &hitPoint);

#endif//vcRender_h__
