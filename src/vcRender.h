#ifndef vcRender_h__
#define vcRender_h__

#include "vcState.h"
#include "vcModel.h"

#include "vdkRenderContext.h"
#include "vdkRenderView.h"
#include "vdkQuery.h"

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
  enum RenderType
  {
    RenderType_Polygon,
    RenderType_SceneLayer
  } renderType;

  union
  {
    vcPolygonModel *pModel;
    vcSceneLayerRenderer *pSceneLayer;
  };

  udDouble4x4 worldMat; // will be converted to eye internally
  vcTexture *pDiffuseOverride; // optionally override diffuse texture. Only available on RenderType_Polygon

  vcGLStateCullMode cullFace;

  bool transparent; // TODO: turn this into appropriate flags
  vcSceneItem *pSceneItem;
  uint64_t sceneItemInternalId; // 0 is entire model; for most systems this will be +1 compared to internal arrays
};

struct vcMouseData
{
  udInt2 position;
  bool clicked;
};

struct vcViewShedData
{
  //udUInt2 resolution; // TODO
  udDouble3 position;
  float fieldOfView;
  udFloat2 nearFarPlane;
  udFloat4 visibleColour;
  udFloat4 notVisibleColour;
};

struct vcRenderData
{
  vcMouseData mouse;
  vdkQueryFilter *pQueryFilter;

  udChunkedArray<vcModel*> models;
  udChunkedArray<vcFenceRenderer*> fences;
  udChunkedArray<vcLabelInfo*> labels;
  udChunkedArray<vcRenderPolyInstance> polyModels;
  udChunkedArray<vcWaterRenderer*> waterVolumes;
  udChunkedArray<vcImageRenderInfo*> images;

  udChunkedArray<vcViewShedData> viewSheds;

  vcTexture *pSceneTexture;
  udFloat2 sceneScaling;
};

udResult vcRender_Init(vcState *pProgramState, vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcState *pProgramState, vcRenderContext **pRenderContext);

udResult vcRender_SetVaultContext(vcState *pProgramState, vcRenderContext *pRenderContext);

udResult vcRender_ResizeScene(vcState *pProgramState, vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

void vcRender_BeginFrame(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData);

void vcRender_RenderScene(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer);
void vcRender_SceneImGui(vcState *pProgramState, vcRenderContext *pRenderContext, const vcRenderData &renderData);

void vcRender_ClearTiles(vcRenderContext *pRenderContext);
void vcRender_ClearPoints(vcRenderContext *pRenderContext);

struct vcRenderPickResult
{
  bool success;
  udDouble3 position;

  vcModel *pModel;
  vcRenderPolyInstance *pPolygon;
};
vcRenderPickResult vcRender_PolygonPick(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, bool doSelectRender);

#endif//vcRender_h__
