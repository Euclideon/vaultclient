#ifndef vcRender_h__
#define vcRender_h__

#include "vcState.h"
#include "vcModel.h"

#include "vdkRenderContext.h"
#include "vdkRenderView.h"
#include "vdkQuery.h"

#include "vcMesh.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcWaterRenderer.h"
#include "vcImageRenderer.h"
#include "vcPolygonModel.h"
#include "vcLineRenderer.h"

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

  enum RenderFlags
  {
    RenderFlags_None        = 0,

    RenderFlags_IgnoreTint  = 1 << 0,
    RenderFlags_Transparent = 1 << 1
  } renderFlags;

  union
  {
    vcPolygonModel *pModel;
    vcSceneLayerRenderer *pSceneLayer;
  };

  udDouble4x4 worldMat; // will be converted to eye internally
  vcTexture *pDiffuseOverride; // optionally override diffuse texture. Only available on RenderType_Polygon
  udFloat4 tint; // RGB. Alpha will control transparency for transparent instances 

  vcGLStateCullMode cullFace;

  bool selectable;
  vcSceneItem *pSceneItem;
  uint64_t sceneItemInternalId; // 0 is entire model; for most systems this will be +1 compared to internal arrays

  // Helper to determine if flag is set
  bool HasFlag(const enum RenderFlags flag) { return (renderFlags & flag) == flag; }
};

inline enum vcRenderPolyInstance::RenderFlags operator|(const enum vcRenderPolyInstance::RenderFlags a, const enum vcRenderPolyInstance::RenderFlags b) { return (enum vcRenderPolyInstance::RenderFlags)(int(a) | int(b)); }

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
  float viewDistance;
  udFloat4 visibleColour;
  udFloat4 notVisibleColour;
};

struct vcPins
{
  udDouble3 position;
  const char *pPinAddress;
  int count;

  vcSceneItem *pSceneItem;
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
  udChunkedArray<vcLineInstance*> lines;

  udChunkedArray<vcViewShedData> viewSheds;
  udChunkedArray<vcPins> pins;

  vcTexture *pSceneTexture;
  udFloat2 sceneScaling;
};

vcFramebuffer *vcRender_GetSceneFramebuffer(vcRenderContext *pRenderContext);

udResult vcRender_Init(vcState *pProgramState, vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, const udUInt2 &windowResolution);
udResult vcRender_Destroy(vcState *pProgramState, vcRenderContext **pRenderContext);

udResult vcRender_ReloadShaders(vcRenderContext *pRenderContext, udWorkerPool *pWorkerPool);

udResult vcRender_SetVaultContext(vcState *pProgramState, vcRenderContext *pRenderContext);
udResult vcRender_RemoveVaultContext(vcRenderContext *pRenderContext);

udResult vcRender_ResizeScene(vcState *pProgramState, vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height);

void vcRender_BeginFrame(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData);

void vcRender_RenderScene(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer);

void vcRender_ClearTiles(vcRenderContext *pRenderContext);

// optionally pass in a pre-calculated world up.
udDouble3 vcRender_QueryMapAtCartesian(vcRenderContext *pRenderContext, const udDouble3 &point, const udDouble3 *pWorldUp = nullptr, udDouble3 *pNormal = nullptr);

struct vcRenderPickResult
{
  bool success;
  udDouble3 position;

  vcSceneItem *pSceneItem;
  uint64_t sceneItemInternalId;
};
vcRenderPickResult vcRender_PolygonPick(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, bool doSelectRender);

#endif//vcRender_h__
