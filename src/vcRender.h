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

#include "gl/vcRenderShaders.h"
#include "gl/vcFramebuffer.h"
#include "gl/vcShader.h"
#include "gl/vcGLState.h"
#include "vcTileRenderer.h"
#include "vcCompass.h"
#include "vcState.h"
#include "vcVoxelShaders.h"

#include "vcInternalModels.h"
#include "vcSceneLayerRenderer.h"
#include "vcCamera.h"

#include "udFile.h"

struct vcRenderContext;
struct vcTexture;
struct vcSceneLayerRenderer;
struct udWorkerPool;

// Temp hard-coded view shed properties
static const int ViewShedMapCount = 3;
static const udUInt2 ViewShedMapRes = udUInt2::create(640 * ViewShedMapCount, 1920);

enum
{
  // directX framebuffer can only be certain increments
  vcRender_SceneSizeIncrement = 32,

  // certain effects don't need to be at 100% resolution (e.g. outline). 0 is highest quality
  vcRender_OutlineEffectDownscale = 1,

  // number of buffers for primary rendering passes
  vcRender_RenderBufferCount = 2,
};


struct vcViewShedRenderContext
{
  // re-use this memory
  float* pDepthBuffer;
  vcTexture* pUDDepthTexture;

  vcTexture* pDepthTex;
  vcTexture* pDummyColour;
  vcFramebuffer* pFramebuffer;

  vdkRenderView* pRenderView;
};

struct vcUDRenderContext
{
  vdkRenderContext* pRenderer;
  vdkRenderView* pRenderView;
  uint32_t* pColorBuffer;
  float* pDepthBuffer;

  vcFramebuffer* pFramebuffer;
  vcTexture* pColourTex;
  vcTexture* pDepthTex;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_texture;
    vcShaderSampler* uniform_depth;
  } presentShader;

  struct
  {
    vcShader* pProgram;
    vcShaderConstantBuffer* uniform_params;
    vcShaderSampler* uniform_texture;
    vcShaderSampler* uniform_depth;

    struct
    {
      udFloat4 id;
    } params;

  } splatIdShader;
};

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

  bool affectsViewShed;
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

struct vcRenderContext
{
  vcViewShedRenderContext viewShedRenderingContext;

  udUInt2 sceneResolution;
  udUInt2 originalSceneResolution;

  vcFramebuffer* pFramebuffer[vcRender_RenderBufferCount];
  vcTexture* pTexture[vcRender_RenderBufferCount];
  vcTexture* pDepthTexture[vcRender_RenderBufferCount];

  vcFramebuffer* pAuxiliaryFramebuffers[2];
  vcTexture* pAuxiliaryTextures[2];
  udUInt2 effectResolution;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_texture;
    vcShaderSampler* uniform_depth;
    vcShaderConstantBuffer* uniform_params;

    struct
    {
      udFloat4 screenParams;  // sampleStepX, sampleStepSizeY, near plane, far plane
      udFloat4x4 inverseViewProjection;

      // outlining
      udFloat4 outlineColour;
      udFloat4 outlineParams;   // outlineWidth, threshold, (unused), (unused)

      // colour by height
      udFloat4 colourizeHeightColourMin;
      udFloat4 colourizeHeightColourMax;
      udFloat4 colourizeHeightParams; // min world height, max world height, (unused), (unused)

      // colour by depth
      udFloat4 colourizeDepthColour;
      udFloat4 colourizeDepthParams; // min depth, max depth, (unused), (unused)

      // contours
      udFloat4 contourColour;
      udFloat4 contourParams; // contour distance, contour band height, contour rainbow repeat rate, contour rainbow factoring
    } params;

  } visualizationShader;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_depth;
    vcShaderSampler* uniform_shadowMapAtlas;
    vcShaderConstantBuffer* uniform_params;

    struct
    {
      udFloat4x4 shadowMapVP[ViewShedMapCount];
      udFloat4x4 inverseProjection;
      udFloat4 visibleColour;
      udFloat4 notVisibleColour;
      udFloat4 nearFarPlane; // .zw unused
    } params;

  } shadowShader;

  vcUDRenderContext udRenderContext;
  vcFenceRenderer* pDiagnosticFences;

  vcTileRenderer* pTileRenderer;
  vcAnchor* pCompass;

  float previousFrameDepth;
  udFloat2 currentMouseUV;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_texture;
    vcShaderConstantBuffer* uniform_MatrixBlock;

    vcTexture* pSkyboxTexture;
  } skyboxShaderPanorama;

  struct
  {
    vcShader* pProgram;
    vcShaderConstantBuffer* uniform_params;
    vcShaderSampler* uniform_texture;

    vcTexture* pLogoTexture;

    struct
    {
      udFloat4 colour;
      udFloat4 imageSize;
    } params;
  } skyboxShaderTintImage;

  struct
  {
    udUInt2 location;

    vcFramebuffer* pFramebuffer;
    vcTexture* pTexture;
    vcTexture* pDepth;
  } picking;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_texture;
    vcShaderConstantBuffer* uniform_params;

    struct
    {
      udFloat4 stepSize;
    } params;
  } blurShader;

  struct
  {
    vcShader* pProgram;
    vcShaderSampler* uniform_texture;
    vcShaderConstantBuffer* uniform_params;

    struct
    {
      udFloat4 stepSizeThickness;  // (stepSize.xy, outline thickness, inner overlay strength)
      udFloat4 colour;    // rgb, (unused)
    } params;
  } selectionShader;
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
