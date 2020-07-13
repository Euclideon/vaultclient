#include "vcRender.h"

#include "vcFramebuffer.h"
#include "vcShader.h"
#include "vcGLState.h"
#include "vcFenceRenderer.h"
#include "vcWaterRenderer.h"
#include "vcTileRenderer.h"
#include "vcState.h"
#include "vcVoxelShaders.h"
#include "vcConstants.h"

#include "vcInternalModels.h"
#include "vcSceneLayerRenderer.h"
#include "vcCamera.h"
#include "vcAtmosphereRenderer.h"
#include "vcLineRenderer.h"
#include "vcPinRenderer.h"

#include "vdkStreamer.h"

#include "stb_image.h"
#include <vector>

enum
{
  // directX framebuffer can only be certain increments
  vcRender_SceneSizeIncrement = 32,

  // certain effects don't need to be at 100% resolution (e.g. outline). 0 is highest quality
  vcRender_OutlineEffectDownscale = 1,

  // number of buffers for primary rendering passes
  vcRender_RenderBufferCount = 2,
};

enum vcObjectId
{
  vcObjectId_Null = 0,
  vcObjectId_Terrain = 1,

  // This is an offset
  vcObjectId_SceneItems = 2,
  // Individual category offsets depend on active scene item counts.
  // Ordering is:
  //   1. Polygon Models
  //   2. Images
  //   3. Labels
  //   4. TODO: Pins (images)
};

// Temp hard-coded view shed properties
static const int ViewShedMapCount = 3;
static const udUInt2 ViewShedMapRes = udUInt2::create(640 * ViewShedMapCount, 1920);

static const double PinRendererUpdateRateSec = 1.0;

struct vcViewShedRenderContext
{
  // re-use this memory
  float *pDepthBuffer;
  vcTexture *pUDDepthTexture;

  vcTexture *pDepthTex;
  vcTexture *pDummyColour;
  vcTexture *pDummyNormal;
  vcFramebuffer *pFramebuffer;

  vdkRenderView *pRenderView;
};

struct vcUDRenderContext
{
  vdkRenderContext *pRenderer;
  vdkRenderView *pRenderView;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;

  vcFramebuffer *pFramebuffer;
  vcTexture *pColourTex;
  vcTexture *pDepthTex;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_depth;
  } presentShader;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_params;
    vcShaderSampler *uniform_texture;

    struct
    {
      udFloat4 id;
    } params;

  } splatIdShader;
};

struct vcRenderContext
{
  vcViewShedRenderContext viewShedRenderingContext;

  udUInt2 sceneResolution;
  udUInt2 originalSceneResolution;

  uint32_t activeRenderTarget;

  struct
  {
    vcFramebuffer *pFramebuffer;
    vcTexture *pColour; // RGBA
    vcTexture *pNormal; // Normal.xy, ID.z, DepthCopy.w
    vcTexture *pDepth;
  } gBuffer[vcRender_RenderBufferCount];

  vcFramebuffer *pFinalFramebuffer;
  vcTexture *pFinalColour;

  vcFramebuffer *pAuxiliaryFramebuffers[2];
  vcTexture *pAuxiliaryTextures[2];
  udUInt2 effectResolution;

  vcAtmosphereRenderer *pAtmosphereRenderer;
  vcLineRenderer *pLineRenderer;

  vcPinRenderer *pPinRenderer;
  double pinUpdateRateTimer;

  // cache some frame data
  udDouble3 cameraZeroAltitude;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_normal;
    vcShaderSampler *uniform_depth;
    vcShaderConstantBuffer *uniform_vertParams;
    vcShaderConstantBuffer *uniform_fragParams;

    struct
    {
      udFloat4 screenParams;  // sampleStepX, sampleStepSizeY, (unused), (unused)
      udFloat4x4 inverseViewProjection;
      udFloat4x4 inverseProjection;
      udFloat4 eyeToEarthSurfaceEyeSpace;

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
    } fragParams;

    struct
    {
      udFloat4 outlineStepSize; // outlineStepSize.xy (in uv space), (unused), (unused)
    } vertParams;

  } visualizationShader;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_colour;
    vcShaderConstantBuffer *uniform_params;

    struct
    {
      udFloat4 screenParams;  // sampleStepX, sampleStepSizeY, near plane, far plane
      udFloat4 saturation;
    } params;

  } postEffectsShader;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_depth;
    vcShaderSampler *uniform_shadowMapAtlas;
    vcShaderConstantBuffer *uniform_params;

    struct
    {
      udFloat4x4 shadowMapVP[ViewShedMapCount];
      udFloat4x4 inverseProjection;
      udFloat4 visibleColour;
      udFloat4 notVisibleColour;
      udFloat4 viewDistance; // .yzw unused
    } params;

  } shadowShader;

  vcUDRenderContext udRenderContext;

  vcTileRenderer *pTileRenderer;

  uint16_t previousPickedId;
  float previousFrameDepth;
  udFloat2 currentMouseUV;
  int asyncReadTarget;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderConstantBuffer *uniform_MatrixBlock;

    vcTexture *pSkyboxTexture;
  } skyboxShaderPanorama;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_params;
    vcShaderSampler *uniform_texture;

    vcTexture *pLogoTexture;

    struct
    {
      udFloat4 colour;
      udFloat4 imageSize;
    } params;
  } skyboxShaderTintImage;

  struct
  {
    udUInt2 location;
  } picking;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderConstantBuffer *uniform_params;

    struct
    {
      udFloat4 stepSize;
    } params;
  } blurShader;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderConstantBuffer *uniform_params;

    struct
    {
      udFloat4 stepSizeThickness;  // (stepSize.xy, outline thickness, inner overlay strength)
      udFloat4 colour;    // rgb, (unused)
    } params;
  } selectionShader;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderConstantBuffer *uniform_params;
    struct
    {
      udFloat4x4 u_worldViewProjectionMatrix;
    } params;

  } watermarkShader;
};

udResult vcRender_LoadShaders(vcRenderContext *pRenderContext, udWorkerPool *pWorkerPool);
udResult vcRender_RecreateUDView(vcState *pProgramState, vcRenderContext *pRenderContext);
udResult vcRender_RenderUD(vcState *pProgramState, vcRenderContext *pRenderContext, vdkRenderView *pRenderView, vcCamera *pCamera, vcRenderData &renderData, bool doPick);
void vcRender_RenderWatermark(vcRenderContext *pRenderContext, vcTexture *pWatermark);
void vcRender_RenderAtmosphere(vcState *pProgramState, vcRenderContext *pRenderContext);

vcFramebuffer *vcRender_GetSceneFramebuffer(vcRenderContext *pRenderContext)
{
  return pRenderContext->pFinalFramebuffer;
}

udResult vcRender_Init(vcState *pProgramState, vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, const udUInt2 &sceneResolution)
{
  udResult result;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  UD_ERROR_CHECK(vcInternalModels_Init());

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  pRenderContext->pinUpdateRateTimer = PinRendererUpdateRateSec;

  pRenderContext->viewShedRenderingContext.pDepthBuffer = udAllocType(float, ViewShedMapRes.x * ViewShedMapRes.y, udAF_Zero);
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pUDDepthTexture, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_Dynamic));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pDepthTex, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pDummyColour, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pDummyNormal, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->viewShedRenderingContext.pFramebuffer, pRenderContext->viewShedRenderingContext.pDummyColour, pRenderContext->viewShedRenderingContext.pDepthTex, 0, pRenderContext->viewShedRenderingContext.pDummyNormal), udR_InternalError);

  UD_ERROR_CHECK(vcTexture_AsyncCreateFromFilename(&pRenderContext->skyboxShaderPanorama.pSkyboxTexture, pWorkerPool, "asset://assets/skyboxes/WaterClouds.jpg", vcTFM_Linear));

  UD_ERROR_CHECK(vcAtmosphereRenderer_Create(&pRenderContext->pAtmosphereRenderer, pWorkerPool));
  UD_ERROR_CHECK(vcTileRenderer_Create(&pRenderContext->pTileRenderer, pWorkerPool, &pProgramState->settings));
  UD_ERROR_CHECK(vcLineRenderer_Create(&pRenderContext->pLineRenderer, pWorkerPool));
  UD_ERROR_CHECK(vcPinRenderer_Create(&pRenderContext->pPinRenderer));

  UD_ERROR_CHECK(vcRender_LoadShaders(pRenderContext, pProgramState->pWorkerPool));
  UD_ERROR_CHECK(vcRender_ResizeScene(pProgramState, pRenderContext, sceneResolution.x, sceneResolution.y));

  *ppRenderContext = pRenderContext;
  pRenderContext = nullptr;
  result = udR_Success;
epilogue:

  if (pRenderContext != nullptr)
    vcRender_Destroy(pProgramState, &pRenderContext);

  return result;
}

udResult vcRender_LoadShaders(vcRenderContext *pRenderContext, udWorkerPool *pWorkerPool)
{
  udResult result;

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->udRenderContext.presentShader.pProgram, pWorkerPool, "asset://assets/shaders/udVertexShader", "asset://assets/shaders/udFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_texture, pRenderContext->udRenderContext.presentShader.pProgram, "sceneColour");
      vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_depth, pRenderContext->udRenderContext.presentShader.pProgram, "sceneDepth");
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->visualizationShader.pProgram, pWorkerPool, "asset://assets/shaders/visualizationVertexShader", "asset://assets/shaders/visualizationFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->visualizationShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->visualizationShader.uniform_texture, pRenderContext->visualizationShader.pProgram, "sceneColour");
      vcShader_GetSamplerIndex(&pRenderContext->visualizationShader.uniform_normal, pRenderContext->visualizationShader.pProgram, "sceneNormal");
      vcShader_GetSamplerIndex(&pRenderContext->visualizationShader.uniform_depth, pRenderContext->visualizationShader.pProgram, "sceneDepth");
      vcShader_GetConstantBuffer(&pRenderContext->visualizationShader.uniform_vertParams, pRenderContext->visualizationShader.pProgram, "u_vertParams", sizeof(pRenderContext->visualizationShader.vertParams));
      vcShader_GetConstantBuffer(&pRenderContext->visualizationShader.uniform_fragParams, pRenderContext->visualizationShader.pProgram, "u_fragParams", sizeof(pRenderContext->visualizationShader.fragParams));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->shadowShader.pProgram, pWorkerPool, "asset://assets/shaders/viewShedVertexShader", "asset://assets/shaders/viewShedFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->shadowShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->shadowShader.uniform_depth, pRenderContext->shadowShader.pProgram, "sceneDepth");
      vcShader_GetSamplerIndex(&pRenderContext->shadowShader.uniform_shadowMapAtlas, pRenderContext->shadowShader.pProgram, "shadowMapAtlas");
      vcShader_GetConstantBuffer(&pRenderContext->shadowShader.uniform_params, pRenderContext->shadowShader.pProgram, "u_params", sizeof(pRenderContext->shadowShader.params));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->skyboxShaderPanorama.pProgram, pWorkerPool, "asset://assets/shaders/panoramaSkyboxVertexShader", "asset://assets/shaders/panoramaSkyboxFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->skyboxShaderPanorama.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->skyboxShaderPanorama.uniform_texture, pRenderContext->skyboxShaderPanorama.pProgram, "albedo");
      vcShader_GetConstantBuffer(&pRenderContext->skyboxShaderPanorama.uniform_MatrixBlock, pRenderContext->skyboxShaderPanorama.pProgram, "u_EveryFrame", sizeof(udFloat4x4));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->skyboxShaderTintImage.pProgram, pWorkerPool, "asset://assets/shaders/imageColourSkyboxVertexShader", "asset://assets/shaders/imageColourSkyboxFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->skyboxShaderTintImage.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->skyboxShaderTintImage.uniform_texture, pRenderContext->skyboxShaderTintImage.pProgram, "albedo");
      vcShader_GetConstantBuffer(&pRenderContext->skyboxShaderTintImage.uniform_params, pRenderContext->skyboxShaderTintImage.pProgram, "u_EveryFrame", sizeof(pRenderContext->skyboxShaderTintImage.params));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->udRenderContext.splatIdShader.pProgram, pWorkerPool, "asset://assets/shaders/udVertexShader", "asset://assets/shaders/udSplatIdFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->udRenderContext.splatIdShader.pProgram);
      vcShader_GetConstantBuffer(&pRenderContext->udRenderContext.splatIdShader.uniform_params, pRenderContext->udRenderContext.splatIdShader.pProgram, "u_params", sizeof(pRenderContext->udRenderContext.splatIdShader.params));
      vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.splatIdShader.uniform_texture, pRenderContext->udRenderContext.splatIdShader.pProgram, "sceneColour");
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->postEffectsShader.pProgram, pWorkerPool, "asset://assets/shaders/postEffectsVertexShader", "asset://assets/shaders/postEffectsFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->postEffectsShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->postEffectsShader.uniform_colour, pRenderContext->postEffectsShader.pProgram, "sceneColour");
      vcShader_GetConstantBuffer(&pRenderContext->postEffectsShader.uniform_params, pRenderContext->postEffectsShader.pProgram, "u_params", sizeof(pRenderContext->postEffectsShader.params));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->blurShader.pProgram, pWorkerPool, "asset://assets/shaders/blurVertexShader", "asset://assets/shaders/blurFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->blurShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->blurShader.uniform_texture, pRenderContext->blurShader.pProgram, "colour");
      vcShader_GetConstantBuffer(&pRenderContext->blurShader.uniform_params, pRenderContext->blurShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->blurShader.params));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->selectionShader.pProgram, pWorkerPool, "asset://assets/shaders/highlightVertexShader", "asset://assets/shaders/highlightFragmentShader", vcP3UV2VertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->selectionShader.pProgram);
      vcShader_GetSamplerIndex(&pRenderContext->selectionShader.uniform_texture, pRenderContext->selectionShader.pProgram, "colour");
      vcShader_GetConstantBuffer(&pRenderContext->selectionShader.uniform_params, pRenderContext->selectionShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->selectionShader.params));
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pRenderContext->watermarkShader.pProgram, pWorkerPool, "asset://assets/shaders/imguiVertexShader", "asset://assets/shaders/imguiFragmentShader", vcImGuiVertexLayout,
    [pRenderContext](void *)
    {
      vcShader_Bind(pRenderContext->watermarkShader.pProgram);
      vcShader_GetConstantBuffer(&pRenderContext->watermarkShader.uniform_params, pRenderContext->watermarkShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->watermarkShader.params));
      vcShader_GetSamplerIndex(&pRenderContext->watermarkShader.uniform_texture, pRenderContext->watermarkShader.pProgram, "Texture");
    }
  ), udR_InternalError);

  UD_ERROR_CHECK(vcPolygonModel_CreateShaders(pWorkerPool));
  UD_ERROR_CHECK(vcImageRenderer_Init(pWorkerPool));
  UD_ERROR_CHECK(vcLabelRenderer_Init(pWorkerPool));

  UD_ERROR_IF(!vcShader_Bind(nullptr), udR_InternalError);

  result = udR_Success;
epilogue:

  return result;
}

void vcRender_DestroyShaders(vcRenderContext *pRenderContext)
{
  vcShader_DestroyShader(&pRenderContext->udRenderContext.presentShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->visualizationShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->postEffectsShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->shadowShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->skyboxShaderPanorama.pProgram);
  vcShader_DestroyShader(&pRenderContext->skyboxShaderTintImage.pProgram);
  vcShader_DestroyShader(&pRenderContext->udRenderContext.splatIdShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->blurShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->selectionShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->watermarkShader.pProgram);

  vcPolygonModel_DestroyShaders();
  vcImageRenderer_Destroy();
  vcLabelRenderer_Destroy();
}

udResult vcRender_ReloadShaders(vcRenderContext *pRenderContext, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcRender_DestroyShaders(pRenderContext);

  UD_ERROR_CHECK(vcAtmosphereRenderer_ReloadShaders(pRenderContext->pAtmosphereRenderer, pWorkerPool));
  UD_ERROR_CHECK(vcTileRenderer_ReloadShaders(pRenderContext->pTileRenderer, pWorkerPool));
  UD_ERROR_CHECK(vcLineRenderer_ReloadShaders(pRenderContext->pLineRenderer, pWorkerPool));

  UD_ERROR_CHECK(vcRender_LoadShaders(pRenderContext, pWorkerPool));

  result = udR_Success;
epilogue:

  return result;
}

udResult vcRender_Destroy(vcState *pProgramState, vcRenderContext **ppRenderContext)
{
  if (ppRenderContext == nullptr || *ppRenderContext == nullptr)
    return udR_Success;

  udResult result;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = *ppRenderContext;
  *ppRenderContext = nullptr;

  if (pProgramState->pVDKContext != nullptr)
  {
    if (pRenderContext->viewShedRenderingContext.pRenderView != nullptr && vdkRenderView_Destroy(&pRenderContext->viewShedRenderingContext.pRenderView) != vE_Success)
      UD_ERROR_SET(udR_InternalError);

    if (pRenderContext->udRenderContext.pRenderView != nullptr && vdkRenderView_Destroy(&pRenderContext->udRenderContext.pRenderView) != vE_Success)
      UD_ERROR_SET(udR_InternalError);

    if (vdkRenderContext_Destroy(&pRenderContext->udRenderContext.pRenderer) != vE_Success)
      UD_ERROR_SET(udR_InternalError);
  }

  vcRender_DestroyShaders(pRenderContext);
  vcTexture_Destroy(&pRenderContext->skyboxShaderPanorama.pSkyboxTexture);

  UD_ERROR_CHECK(vcPinRenderer_Destroy(&pRenderContext->pPinRenderer));
  UD_ERROR_CHECK(vcAtmosphereRenderer_Destroy(&pRenderContext->pAtmosphereRenderer));

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);
  udFree(pRenderContext->viewShedRenderingContext.pDepthBuffer);

  UD_ERROR_CHECK(vcTileRenderer_Destroy(&pRenderContext->pTileRenderer));
  UD_ERROR_CHECK(vcLineRenderer_Destroy(&pRenderContext->pLineRenderer));

  UD_ERROR_CHECK(vcInternalModels_Deinit());
  result = udR_Success;

epilogue:
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pUDDepthTexture);
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pDepthTex);
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pDummyColour);
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pDummyNormal);
  vcFramebuffer_Destroy(&pRenderContext->viewShedRenderingContext.pFramebuffer);

  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcFramebuffer_Destroy(&pRenderContext->udRenderContext.pFramebuffer);
  for (int i = 0; i < vcRender_RenderBufferCount; ++i)
  {
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pColour);
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pNormal);
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pDepth);
    vcFramebuffer_Destroy(&pRenderContext->gBuffer[i].pFramebuffer);
  }

  vcTexture_Destroy(&pRenderContext->pFinalColour);
  vcFramebuffer_Destroy(&pRenderContext->pFinalFramebuffer);

  for (int i = 0; i < 2; ++i)
  {
    vcTexture_Destroy(&pRenderContext->pAuxiliaryTextures[i]);
    vcFramebuffer_Destroy(&pRenderContext->pAuxiliaryFramebuffers[i]);
  }

  udFree(pRenderContext);
  return result;
}

udResult vcRender_SetVaultContext(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (vdkRenderContext_Create(pProgramState->pVDKContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  UD_ERROR_CHECK(vcRender_RecreateUDView(pProgramState, pRenderContext));

epilogue:
  return result;
}

udResult vcRender_RemoveVaultContext(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (vdkRenderView_Destroy(&pRenderContext->viewShedRenderingContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Destroy(&pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderContext_Destroy(&pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_ResizeScene(vcState *pProgramState, vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height)
{
  udResult result = udR_Success;

  uint32_t widthIncr = width + (width % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - width % vcRender_SceneSizeIncrement : 0);
  uint32_t heightIncr = height + (height % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - height % vcRender_SceneSizeIncrement : 0);

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);
  UD_ERROR_IF(width == 0, udR_InvalidParameter_);
  UD_ERROR_IF(height == 0, udR_InvalidParameter_);

  pRenderContext->sceneResolution.x = widthIncr;
  pRenderContext->sceneResolution.y = heightIncr;
  pRenderContext->originalSceneResolution.x = width;
  pRenderContext->originalSceneResolution.y = height;

  //Resize CPU Targets
  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

  pRenderContext->udRenderContext.pColorBuffer = udAllocType(uint32_t, pRenderContext->sceneResolution.x * pRenderContext->sceneResolution.y, udAF_Zero);
  UD_ERROR_NULL(pRenderContext->udRenderContext.pColorBuffer, udR_MemoryAllocationFailure);

  pRenderContext->udRenderContext.pDepthBuffer = udAllocType(float, pRenderContext->sceneResolution.x * pRenderContext->sceneResolution.y, udAF_Zero);
  UD_ERROR_NULL(pRenderContext->udRenderContext.pDepthBuffer, udR_MemoryAllocationFailure);

  //Resize GPU Targets
  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcFramebuffer_Destroy(&pRenderContext->udRenderContext.pFramebuffer);

  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->udRenderContext.pColourTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pColorBuffer, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_Dynamic));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->udRenderContext.pDepthTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pDepthBuffer, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_Dynamic));

  for (int i = 0; i < vcRender_RenderBufferCount; ++i)
  {
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pColour);
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pNormal);
    vcTexture_Destroy(&pRenderContext->gBuffer[i].pDepth);
    vcFramebuffer_Destroy(&pRenderContext->gBuffer[i].pFramebuffer);
  }

  vcTexture_Destroy(&pRenderContext->pFinalColour);
  vcFramebuffer_Destroy(&pRenderContext->pFinalFramebuffer);

  for (int i = 0; i < 2; ++i)
  {
    vcTexture_Destroy(&pRenderContext->pAuxiliaryTextures[i]);
    vcFramebuffer_Destroy(&pRenderContext->pAuxiliaryFramebuffers[i]);
  }

  for (int i = 0; i < vcRender_RenderBufferCount; ++i)
  {
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->gBuffer[i].pColour, widthIncr, heightIncr, nullptr, vcTextureFormat_RGBA16F, vcTFM_Linear, vcTCF_RenderTarget));
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->gBuffer[i].pNormal, widthIncr, heightIncr, nullptr, vcTextureFormat_RGBA16F, vcTFM_Linear, vcTCF_RenderTarget | vcTCF_AsynchronousRead));
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->gBuffer[i].pDepth, widthIncr, heightIncr, nullptr, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_RenderTarget));
    UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->gBuffer[i].pFramebuffer, pRenderContext->gBuffer[i].pColour, pRenderContext->gBuffer[i].pDepth, 0, pRenderContext->gBuffer[i].pNormal), udR_InternalError);
  }

  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->pFinalColour, widthIncr, heightIncr, nullptr, vcTextureFormat_RGBA8, vcTFM_Linear, vcTCF_RenderTarget));
  UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->pFinalFramebuffer, pRenderContext->pFinalColour), udR_InternalError);

  pRenderContext->effectResolution.x = widthIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.y = heightIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.x = pRenderContext->effectResolution.x + (pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement : 0);
  pRenderContext->effectResolution.y = pRenderContext->effectResolution.y + (pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement : 0);

  for (int i = 0; i < 2; ++i)
  {
    UD_ERROR_CHECK(vcTexture_CreateAdv(&pRenderContext->pAuxiliaryTextures[i], vcTextureType_Texture2D, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, 1, nullptr, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Clamp, vcTCF_RenderTarget));
    UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->pAuxiliaryFramebuffers[i], pRenderContext->pAuxiliaryTextures[i]), udR_InternalError);
  }

  if (pProgramState->pVDKContext)
    UD_ERROR_CHECK(vcRender_RecreateUDView(pProgramState, pRenderContext));

epilogue:
  return result;
}

// TODO: Move to udCore (AB#1573)
// Unit tests:
//float a = Float16ToFloat32(0b0000000000000001); //0.000000059605
//float b = Float16ToFloat32(0b0000001111111111); //0.000060976
//float c = Float16ToFloat32(0b0000010000000000); //0.000061035
//float d = Float16ToFloat32(0b0111101111111111); //65504
//float e = Float16ToFloat32(0b0011101111111111); //0.99951
//float f = Float16ToFloat32(0b0011110000000000); //1
//float g = Float16ToFloat32(0b0011110000000001); //1.001
//float h = Float16ToFloat32(0b0011010101010101); //0.333251953125
//float i = Float16ToFloat32(0b1100000000000000); //-2
//float j = Float16ToFloat32(0b0000000000000000); //0
//float k = Float16ToFloat32(0b1000000000000000); //-0
//float l = Float16ToFloat32(0b0111110000000000); //inf
//float m = Float16ToFloat32(0b1111110000000000); //-inf
float Float16ToFloat32(uint16_t float16)
{
  uint16_t sign_bit = (float16 & 0b1000000000000000) >> 15;
  uint16_t exponent = (float16 & 0b0111110000000000) >> 10;
  uint16_t fraction = (float16 & 0b0000001111111111) >> 0;

  float sign = (sign_bit) ? -1.0f : 1.0f;
  float m = 1.0f;
  uint16_t exponentBias = 15;

  // The exponents '00000' and '11111' are interpreted specially
  if (exponent == 31)
    return sign * INFINITY;

  if (exponent == 0)
  {
    m = 0.0f;
    exponentBias = 14;
  }

  return sign * udPow(2.0f, float(exponent - exponentBias)) * (m + (fraction / 1024.0f));
}

float vcRender_EncodeModelId(uint32_t id)
{
  return float(id & 0xffff) / 0xffff;
}

void vcRender_DecodeModelId(uint8_t pixelBytes[8], uint16_t *pId, float *pDepth)
{
  *pDepth = udAbs(Float16ToFloat32(uint16_t((pixelBytes[2] & 0xFF) | ((pixelBytes[3] & 0xFF) << 8))));
  *pId = uint16_t((Float16ToFloat32(uint16_t((pixelBytes[0] & 0xFF) | ((pixelBytes[1] & 0xFF) << 8))) * 0xffff) + 0.5f);
}

// Asychronously read a 1x1 region of last frames depth buffer 
udResult vcRender_AsyncReadFrameDepth(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  if (pRenderContext->currentMouseUV.x < 0 || pRenderContext->currentMouseUV.x > 1 || pRenderContext->currentMouseUV.y < 0 || pRenderContext->currentMouseUV.y > 1)
    return result;

  static udUInt2 lastPickLocation = udUInt2::zero();

  uint8_t pixelBytes[8] = {};

  pRenderContext->previousFrameDepth = 1.0f;
  pRenderContext->previousPickedId = vcObjectId_Null;
  if (pRenderContext->asyncReadTarget != -1)
  {
    UD_ERROR_IF(!vcTexture_EndReadPixels(pRenderContext->gBuffer[pRenderContext->asyncReadTarget].pNormal, lastPickLocation.x, lastPickLocation.y, 1, 1, pixelBytes), udR_InternalError); // read previous copy
    vcRender_DecodeModelId(pixelBytes, &pRenderContext->previousPickedId, &pRenderContext->previousFrameDepth);

    pRenderContext->asyncReadTarget = -1;
  }

  pRenderContext->asyncReadTarget = pRenderContext->activeRenderTarget;
  UD_ERROR_IF(!vcTexture_BeginReadPixels(pRenderContext->gBuffer[pRenderContext->asyncReadTarget].pNormal, pRenderContext->picking.location.x, pRenderContext->picking.location.y, 1, 1, pixelBytes, pRenderContext->gBuffer[pRenderContext->asyncReadTarget].pFramebuffer), udR_InternalError); // begin copy for next frame read

  lastPickLocation = pRenderContext->picking.location;

epilogue:
  return result;
}

udDouble3 vcRender_DepthToWorldPosition(vcState *pProgramState, vcRenderContext *pRenderContext, double depthIn)
{
  // reconstruct clip depth from log z
  float a = pProgramState->settings.camera.farPlane / (pProgramState->settings.camera.farPlane - pProgramState->settings.camera.nearPlane);
  float b = pProgramState->settings.camera.farPlane * pProgramState->settings.camera.nearPlane / (pProgramState->settings.camera.nearPlane - pProgramState->settings.camera.farPlane);
  double worldDepth = udPow(2.0, depthIn * udLog2(pProgramState->settings.camera.farPlane + 1.0)) - 1.0;
  double depth = a + b / worldDepth;

  // note: upside down (1.0 - uv.y)
  udDouble4 clipPos = udDouble4::create(pRenderContext->currentMouseUV.x * 2.0 - 1.0, (1.0 - pRenderContext->currentMouseUV.y) * 2.0 - 1.0, depth, 1.0);
#if GRAPHICS_API_OPENGL
  clipPos.z = clipPos.z * 2.0 - 1.0;
  clipPos.y = -clipPos.y;
#endif

  udDouble4 pickPosition4 = pProgramState->camera.matrices.inverseViewProjection * clipPos;
  return pickPosition4.toVector3() / pickPosition4.w;
}

void vcRenderSkybox(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  // Draw the skybox only at the far plane, where there is no geometry.
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  if (pProgramState->settings.presentation.skybox.type == vcSkyboxType_Simple)
  {
    // undo ecef orientation
    udDoubleQuat baseCameraOrientation = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->camera.position, udDouble2::zero());
    udFloat4x4 viewMatrixF = udFloat4x4::create(pProgramState->camera.matrices.view * udDouble4x4::rotationQuat(baseCameraOrientation));
    udFloat4x4 projectionMatrixF = udFloat4x4::create(pProgramState->camera.matrices.projectionNear);
    udFloat4x4 inverseViewProjMatrixF = projectionMatrixF * viewMatrixF;
    inverseViewProjMatrixF.axis.t = udFloat4::create(0, 0, 0, 1);
    inverseViewProjMatrixF.inverse();

    vcShader_Bind(pRenderContext->skyboxShaderPanorama.pProgram);
    vcShader_BindTexture(pRenderContext->skyboxShaderPanorama.pProgram, pRenderContext->skyboxShaderPanorama.pSkyboxTexture, 0, pRenderContext->skyboxShaderPanorama.uniform_texture);
    vcShader_BindConstantBuffer(pRenderContext->skyboxShaderPanorama.pProgram, pRenderContext->skyboxShaderPanorama.uniform_MatrixBlock, &inverseViewProjMatrixF, sizeof(inverseViewProjMatrixF));
  }
  else // colour
  {
    pRenderContext->skyboxShaderTintImage.params.colour = pProgramState->settings.presentation.skybox.colour;

    int x = 0;
    int y = 0;

    vcShader_Bind(pRenderContext->skyboxShaderTintImage.pProgram);

    if (vcTexture_GetSize(pRenderContext->skyboxShaderTintImage.pLogoTexture, &x, &y) == udR_Success)
    {
      pRenderContext->skyboxShaderTintImage.params.imageSize.x = (float)x / pRenderContext->sceneResolution.x;
      pRenderContext->skyboxShaderTintImage.params.imageSize.y = -(float)y / pRenderContext->sceneResolution.y;

      vcShader_BindTexture(pRenderContext->skyboxShaderTintImage.pProgram, pRenderContext->skyboxShaderTintImage.pLogoTexture, 0, pRenderContext->skyboxShaderTintImage.uniform_texture);
    }
    else
    {
      pRenderContext->skyboxShaderTintImage.params.colour.w = 0.0;
    }

    vcShader_BindConstantBuffer(pRenderContext->skyboxShaderTintImage.pProgram, pRenderContext->skyboxShaderTintImage.uniform_params, &pRenderContext->skyboxShaderTintImage.params, sizeof(pRenderContext->skyboxShaderTintImage.params));
  }

  vcGLState_SetViewportDepthRange(1.0f, 1.0f);

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);

  vcGLState_SetViewportDepthRange(0.0f, 1.0f);

  vcShader_Bind(nullptr);
}

void vcRender_RenderAtmosphere(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  if (pProgramState->settings.presentation.skybox.type == vcSkyboxType_None)
    return;

  if (pProgramState->camera.cameraIsUnderSurface || pProgramState->settings.presentation.skybox.type == vcSkyboxType_Colour || pProgramState->settings.presentation.skybox.type == vcSkyboxType_Simple)
  {
    // Render simple skybox instead
    vcRenderSkybox(pProgramState, pRenderContext);
    return;
  }

  vcAtmosphereRenderer_SetVisualParams(pProgramState, pRenderContext->pAtmosphereRenderer);

  pRenderContext->activeRenderTarget = 1 - pRenderContext->activeRenderTarget;
  vcFramebuffer_Bind(pRenderContext->gBuffer[pRenderContext->activeRenderTarget].pFramebuffer, vcFramebufferClearOperation_All, 0xff000000);

  vcAtmosphereRenderer_Render(pRenderContext->pAtmosphereRenderer, pProgramState, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pColour, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pNormal, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pDepth);
}

void vcRender_SplatUDWithId(vcState *pProgramState, vcRenderContext *pRenderContext, float id)
{
  udUnused(pProgramState); // Some configurations no longer use this parameter

  vcShader_Bind(pRenderContext->udRenderContext.splatIdShader.pProgram);

  vcShader_BindTexture(pRenderContext->udRenderContext.splatIdShader.pProgram, pRenderContext->udRenderContext.pColourTex, 0, pRenderContext->udRenderContext.splatIdShader.uniform_texture);

  pRenderContext->udRenderContext.splatIdShader.params.id = udFloat4::create(0.0f, 0.0f, 0.0f, id);
  vcShader_BindConstantBuffer(pRenderContext->udRenderContext.splatIdShader.pProgram, pRenderContext->udRenderContext.splatIdShader.uniform_params, &pRenderContext->udRenderContext.splatIdShader.params, sizeof(pRenderContext->udRenderContext.splatIdShader.params));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

void vcRender_ConditionalSplatUD(vcState *pProgramState, vcRenderContext *pRenderContext, vcTexture *pColour, vcTexture *pDepth, const vcRenderData &renderData)
{
  if (renderData.models.length == 0)
    return;

  udUnused(pProgramState);

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);

  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pColour, 0, pRenderContext->udRenderContext.presentShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pDepth, 1, pRenderContext->udRenderContext.presentShader.uniform_depth);

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

void vcRenderTerrain(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  if (pProgramState->geozone.projection != udGZPT_Unknown && pProgramState->settings.maptiles.mapEnabled)
  {
    udDouble4x4 cameraMatrix = pProgramState->camera.matrices.camera;
    udDouble4x4 viewProjection = pProgramState->camera.matrices.viewProjection;

#ifndef GIT_BUILD
    static bool debugDetachCamera = false;
    static udDouble4x4 gRealCameraMatrix = udDouble4x4::identity();
    if (!debugDetachCamera)
      gRealCameraMatrix = pProgramState->camera.matrices.camera;

    cameraMatrix = gRealCameraMatrix;
    viewProjection = pProgramState->camera.matrices.projection * udInverse(cameraMatrix);
#endif
    udDouble3 localCamPos = cameraMatrix.axis.t.toVector3();

    double cameraHeightAboveEarthSurface = -pProgramState->settings.maptiles.mapHeight;
    udDouble3 earthSurfaceToCamera = localCamPos - pRenderContext->cameraZeroAltitude;
    if (udMagSq3(earthSurfaceToCamera) > 0)
      cameraHeightAboveEarthSurface += udMag3(earthSurfaceToCamera);

    int currentZoom = 21;

    // These values were trial and errored.
    const double BaseViewDistance = 10000.0;
    const double HeightViewDistanceScale = 35.0;
    double visibleFarPlane = udMin((double)s_CameraFarPlane, BaseViewDistance + cameraHeightAboveEarthSurface * HeightViewDistanceScale);

    // Cube Corners
    udDouble3 localCorners[8];
    udInt2 slippyCorners[8];

    // Cardinal Limits
    localCorners[0] = localCamPos + udDouble3::create(-visibleFarPlane, +visibleFarPlane, +visibleFarPlane);
    localCorners[1] = localCamPos + udDouble3::create(+visibleFarPlane, +visibleFarPlane, +visibleFarPlane);
    localCorners[2] = localCamPos + udDouble3::create(-visibleFarPlane, -visibleFarPlane, +visibleFarPlane);
    localCorners[3] = localCamPos + udDouble3::create(+visibleFarPlane, -visibleFarPlane, +visibleFarPlane);
    localCorners[4] = localCamPos + udDouble3::create(-visibleFarPlane, +visibleFarPlane, -visibleFarPlane);
    localCorners[5] = localCamPos + udDouble3::create(+visibleFarPlane, +visibleFarPlane, -visibleFarPlane);
    localCorners[6] = localCamPos + udDouble3::create(-visibleFarPlane, -visibleFarPlane, -visibleFarPlane);
    localCorners[7] = localCamPos + udDouble3::create(+visibleFarPlane, -visibleFarPlane, -visibleFarPlane);

    for (size_t i = 0; i < udLengthOf(localCorners); ++i)
      vcGIS_LocalToSlippy(pProgramState->geozone, &slippyCorners[i], localCorners[i], currentZoom);

    while (currentZoom > 0)
    {
      bool matching = true;
      for (size_t i = 1; i < udLengthOf(localCorners) && matching; ++i)
        matching = (slippyCorners[i] == slippyCorners[i - 1]);

      if (matching)
        break;

      --currentZoom;

      for (size_t i = 0; i < udLengthOf(localCorners); ++i)
        slippyCorners[i] /= 2;
    }

    vcTileRenderer_Update(pRenderContext->pTileRenderer, pProgramState->deltaTime, &pProgramState->geozone, udInt3::create(slippyCorners[0], currentZoom), localCamPos, pProgramState->camera.cameraIsUnderSurface, pRenderContext->cameraZeroAltitude, viewProjection, &pProgramState->isStreaming);

    float terrainId = vcRender_EncodeModelId(vcObjectId_Terrain);
    vcTileRenderer_Render(pRenderContext->pTileRenderer, pProgramState->camera.matrices.view, pProgramState->camera.matrices.projection, pProgramState->camera.cameraIsUnderSurface, terrainId);
  }
}

void vcRender_PostProcessPass(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  udUnused(pProgramState);

  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  vcFramebuffer_Bind(pRenderContext->pFinalFramebuffer, vcFramebufferClearOperation_All, 0xff000000);

  vcShader_Bind(pRenderContext->postEffectsShader.pProgram);
  vcShader_BindTexture(pRenderContext->postEffectsShader.pProgram, pRenderContext->gBuffer[pRenderContext->activeRenderTarget].pColour, 0, pRenderContext->postEffectsShader.uniform_colour);

  pRenderContext->postEffectsShader.params.screenParams.x = (1.0f / pRenderContext->sceneResolution.x);
  pRenderContext->postEffectsShader.params.screenParams.y = (1.0f / pRenderContext->sceneResolution.y);
  pRenderContext->postEffectsShader.params.saturation.x = pProgramState->settings.presentation.saturation;

  vcShader_BindConstantBuffer(pRenderContext->postEffectsShader.pProgram, pRenderContext->postEffectsShader.uniform_params, &pRenderContext->postEffectsShader.params, sizeof(pRenderContext->postEffectsShader.params));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

void vcRender_VisualizationPass(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, true);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  pRenderContext->activeRenderTarget = 1 - pRenderContext->activeRenderTarget;
  vcFramebuffer_Bind(pRenderContext->gBuffer[pRenderContext->activeRenderTarget].pFramebuffer, vcFramebufferClearOperation_All, 0xff000000);

  vcShader_Bind(pRenderContext->visualizationShader.pProgram);
  vcShader_BindTexture(pRenderContext->visualizationShader.pProgram, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pColour, 0, pRenderContext->visualizationShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->visualizationShader.pProgram, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pNormal, 1, pRenderContext->visualizationShader.uniform_normal);
  vcShader_BindTexture(pRenderContext->visualizationShader.pProgram, pRenderContext->gBuffer[1 - pRenderContext->activeRenderTarget].pDepth, 2, pRenderContext->visualizationShader.uniform_depth);

  // edge outlines
  int outlineWidth = pProgramState->settings.postVisualization.edgeOutlines.width;
  float outlineEdgeThreshold = pProgramState->settings.postVisualization.edgeOutlines.threshold;
  udFloat4 outlineColour = pProgramState->settings.postVisualization.edgeOutlines.colour;
  if (!pProgramState->settings.postVisualization.edgeOutlines.enable)
    outlineColour.w = 0.0f;

  // colour by height
  udFloat4 colourByHeightMinColour = pProgramState->settings.postVisualization.colourByHeight.minColour;
  if (!pProgramState->settings.postVisualization.colourByHeight.enable)
    colourByHeightMinColour.w = 0.f;
  udFloat4 colourByHeightMaxColour = pProgramState->settings.postVisualization.colourByHeight.maxColour;
  if (!pProgramState->settings.postVisualization.colourByHeight.enable)
    colourByHeightMaxColour.w = 0.f;
  float colourByHeightStartHeight = pProgramState->settings.postVisualization.colourByHeight.startHeight;
  float colourByHeightEndHeight = pProgramState->settings.postVisualization.colourByHeight.endHeight;

  // colour by depth
  udFloat4 colourByDepthColour = pProgramState->settings.postVisualization.colourByDepth.colour;
  if (!pProgramState->settings.postVisualization.colourByDepth.enable)
    colourByDepthColour.w = 0.f;
  float colourByDepthStart = pProgramState->settings.postVisualization.colourByDepth.startDepth;
  float colourByDepthEnd = pProgramState->settings.postVisualization.colourByDepth.endDepth;

  // contours
  udFloat4 contourColour = pProgramState->settings.postVisualization.contours.colour;
  float contourDistances = pProgramState->settings.postVisualization.contours.distances;
  float contourBandHeight = pProgramState->settings.postVisualization.contours.bandHeight;
  float contourRainboxRepeatRate = pProgramState->settings.postVisualization.contours.rainbowRepeat;
  float contourRainboxIntensity = pProgramState->settings.postVisualization.contours.rainbowIntensity;

  if (!pProgramState->settings.postVisualization.contours.enable)
  {
    contourColour.w = 0.f;
    contourRainboxIntensity = 0.f;
  }

  udDouble4 eyeToEarthSurfaceEyeSpace = pProgramState->camera.matrices.view * udDouble4::create(pRenderContext->cameraZeroAltitude, 1.0);

  pRenderContext->visualizationShader.fragParams.eyeToEarthSurfaceEyeSpace = udFloat4::create((float)eyeToEarthSurfaceEyeSpace.x, (float)eyeToEarthSurfaceEyeSpace.y, (float)eyeToEarthSurfaceEyeSpace.z, 1.0f);
  pRenderContext->visualizationShader.fragParams.inverseViewProjection = udFloat4x4::create(pProgramState->camera.matrices.inverseViewProjection);
  pRenderContext->visualizationShader.fragParams.inverseProjection = udFloat4x4::create(udInverse(pProgramState->camera.matrices.projection));
  pRenderContext->visualizationShader.fragParams.screenParams.x = (1.0f / pRenderContext->sceneResolution.x);
  pRenderContext->visualizationShader.fragParams.screenParams.y = (1.0f / pRenderContext->sceneResolution.y);
  pRenderContext->visualizationShader.fragParams.outlineColour = outlineColour;
  pRenderContext->visualizationShader.fragParams.outlineParams.x = (float)outlineWidth;
  pRenderContext->visualizationShader.fragParams.outlineParams.y = outlineEdgeThreshold;
  pRenderContext->visualizationShader.fragParams.colourizeHeightColourMin = colourByHeightMinColour;
  pRenderContext->visualizationShader.fragParams.colourizeHeightColourMax = colourByHeightMaxColour;
  pRenderContext->visualizationShader.fragParams.colourizeHeightParams.x = colourByHeightStartHeight;
  pRenderContext->visualizationShader.fragParams.colourizeHeightParams.y = colourByHeightEndHeight;
  pRenderContext->visualizationShader.fragParams.colourizeDepthColour = colourByDepthColour;
  pRenderContext->visualizationShader.fragParams.colourizeDepthParams.x = colourByDepthStart;
  pRenderContext->visualizationShader.fragParams.colourizeDepthParams.y = colourByDepthEnd;
  pRenderContext->visualizationShader.fragParams.contourColour = contourColour;
  pRenderContext->visualizationShader.fragParams.contourParams.x = contourDistances;
  pRenderContext->visualizationShader.fragParams.contourParams.y = contourBandHeight;
  pRenderContext->visualizationShader.fragParams.contourParams.z = contourRainboxRepeatRate;
  pRenderContext->visualizationShader.fragParams.contourParams.w = contourRainboxIntensity;

  pRenderContext->visualizationShader.vertParams.outlineStepSize.x = outlineWidth * (1.0f / pRenderContext->sceneResolution.x);
  pRenderContext->visualizationShader.vertParams.outlineStepSize.y = outlineWidth * (1.0f / pRenderContext->sceneResolution.y);

  vcShader_BindConstantBuffer(pRenderContext->visualizationShader.pProgram, pRenderContext->visualizationShader.uniform_vertParams, &pRenderContext->visualizationShader.vertParams, sizeof(pRenderContext->visualizationShader.vertParams));
  vcShader_BindConstantBuffer(pRenderContext->visualizationShader.pProgram, pRenderContext->visualizationShader.uniform_fragParams, &pRenderContext->visualizationShader.fragParams, sizeof(pRenderContext->visualizationShader.fragParams));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

void vcRender_ApplyViewShed(vcRenderContext *pRenderContext)
{
  vcShader_Bind(pRenderContext->shadowShader.pProgram);
  vcShader_BindTexture(pRenderContext->shadowShader.pProgram, pRenderContext->gBuffer[0].pDepth, 0, pRenderContext->shadowShader.uniform_depth);
  vcShader_BindTexture(pRenderContext->shadowShader.pProgram, pRenderContext->viewShedRenderingContext.pDepthTex, 1, pRenderContext->shadowShader.uniform_shadowMapAtlas);

  vcShader_BindConstantBuffer(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->shadowShader.uniform_params, &pRenderContext->shadowShader.params, sizeof(pRenderContext->shadowShader.params));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

void vcRender_RenderAndApplyViewSheds(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (renderData.viewSheds.length == 0)
    return;

  udUInt2 singleRenderSize = udUInt2::create(ViewShedMapRes.x / ViewShedMapCount, ViewShedMapRes.y);
  udFloat2 atlasSize = udFloat2::create((float)ViewShedMapRes.x, (float)ViewShedMapRes.y);

  // TODO: aabb vs. point test to render only affecting geometry
  bool doUDRender = renderData.models.length > 0;
  bool doPolygonRender = false;
  for (size_t p = 0; p < renderData.polyModels.length && !doPolygonRender; ++p)
    doPolygonRender = !renderData.polyModels[p].HasFlag(vcRenderPolyInstance::RenderFlags_Transparent);

  if (!doUDRender && !doPolygonRender)
    return;

  if (pRenderContext->viewShedRenderingContext.pRenderView == nullptr)
    vdkRenderView_Create(pProgramState->pVDKContext, &pRenderContext->viewShedRenderingContext.pRenderView, pRenderContext->udRenderContext.pRenderer, singleRenderSize.x, singleRenderSize.y);

  for (size_t v = 0; v < renderData.viewSheds.length; ++v)
  {
    vcViewShedData *pViewShedData = &renderData.viewSheds[v];

    vcCameraSettings cameraSettings = {};
    cameraSettings.nearPlane = s_CameraNearPlane;
    cameraSettings.farPlane = s_CameraFarPlane;
    cameraSettings.fieldOfView = pViewShedData->fieldOfView;

    // set up cameras for these renders
    vcCamera shadowRenderCameras[ViewShedMapCount] = {};
    for (int r = 0; r < ViewShedMapCount; ++r)
    {
      shadowRenderCameras[r].position = pViewShedData->position;

      double rot = (UD_DEG2RAD(360.0) / ViewShedMapCount) * r;
      shadowRenderCameras[r].headingPitch = udDouble2::create(-rot, 0);
      vcCamera_UpdateMatrices(pProgramState->geozone, &shadowRenderCameras[r], cameraSettings, atlasSize, nullptr);

      pRenderContext->shadowShader.params.shadowMapVP[r] = udFloat4x4::create(shadowRenderCameras[r].matrices.projectionUD * (shadowRenderCameras[r].matrices.view * udInverse(pProgramState->camera.matrices.view)));
    }

    // Texture uploads first (Unlimited Detail)
    if (doUDRender)
    {
      for (int r = 0; r < ViewShedMapCount; ++r)
      {
        // configure UD render to only render into portion of buffer
        vdkRenderView_SetTargetsWithPitch(pRenderContext->viewShedRenderingContext.pRenderView, nullptr, 0, pRenderContext->viewShedRenderingContext.pDepthBuffer + r * singleRenderSize.x, 0, ViewShedMapRes.x * 4);
        vdkRenderView_SetMatrix(pRenderContext->viewShedRenderingContext.pRenderView, vdkRVM_Projection, shadowRenderCameras[r].matrices.projectionUD.a);

        // render UD
        vcRender_RenderUD(pProgramState, pRenderContext, pRenderContext->viewShedRenderingContext.pRenderView, &shadowRenderCameras[r], renderData, false);
      }

      vcTexture_UploadPixels(pRenderContext->viewShedRenderingContext.pUDDepthTexture, pRenderContext->viewShedRenderingContext.pDepthBuffer, ViewShedMapRes.x, ViewShedMapRes.y);
    }

    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
    
    vcGLState_SetViewport(0, 0, ViewShedMapRes.x, ViewShedMapRes.y);
    vcFramebuffer_Bind(pRenderContext->viewShedRenderingContext.pFramebuffer, vcFramebufferClearOperation_All);

    if (doUDRender)
      vcRender_ConditionalSplatUD(pProgramState, pRenderContext, pRenderContext->viewShedRenderingContext.pDummyColour, pRenderContext->viewShedRenderingContext.pUDDepthTexture, renderData);
    
    if (doPolygonRender)
    {
      for (int r = 0; r < ViewShedMapCount; ++r)
      {
        udDouble4x4 viewProjection = shadowRenderCameras[r].matrices.projection * shadowRenderCameras[r].matrices.view;
        vcGLState_SetViewport(r * singleRenderSize.x, 0, singleRenderSize.x, ViewShedMapRes.y);
    
        for (size_t p = 0; p < renderData.polyModels.length; ++p)
        {
          vcRenderPolyInstance *pInstance = &renderData.polyModels[p];
          if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_Transparent))
            continue;
    
          if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
            vcPolygonModel_Render(pInstance->pModel, 0.0f, pInstance->worldMat, viewProjection, vcPMP_Shadows);
          else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
            vcSceneLayerRenderer_Render(pInstance->pSceneLayer, 0.0f, pInstance->worldMat, viewProjection, shadowRenderCameras[r].position, ViewShedMapRes, nullptr, vcPMP_Shadows);
        }
      }
    }
    
    pRenderContext->shadowShader.params.inverseProjection = udFloat4x4::create(udInverse(pProgramState->camera.matrices.projection));
    pRenderContext->shadowShader.params.viewDistance = udFloat4::create(pViewShedData->viewDistance, 0.0f, 0.0f, 0.0f);
    pRenderContext->shadowShader.params.visibleColour = pViewShedData->visibleColour;
    pRenderContext->shadowShader.params.notVisibleColour = pViewShedData->notVisibleColour;
    
    vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcGLState_SetBlendMode(vcGLSBM_Additive);

    vcFramebuffer_Bind(pRenderContext->pFinalFramebuffer); // assumed this is the working target
    // Buffer restore
    vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
    vcRender_ApplyViewShed(pRenderContext);
  }

  vcGLState_ResetState();
}

void vcRender_OpaquePass(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcFramebuffer_Bind(pRenderContext->gBuffer[pRenderContext->activeRenderTarget].pFramebuffer, vcFramebufferClearOperation_All, 0xff000000);

  vcGLState_ResetState();
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, true);

  // UD
  vcRender_ConditionalSplatUD(pProgramState, pRenderContext, pRenderContext->udRenderContext.pColourTex, pRenderContext->udRenderContext.pDepthTex, renderData);

  // Polygon Models
  {
    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);

    vcSceneLayer_BeginFrame();

    udFloat4 whiteColour = udFloat4::one();
    uint32_t polygonIds = vcObjectId_SceneItems;
    for (uint32_t i = 0; i < renderData.polyModels.length; ++i)
    {
      vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
      if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_Transparent))
        continue;

      udFloat4 *pColourOverride = nullptr;
      if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_IgnoreTint))
        pColourOverride = &whiteColour;

      vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

      float objectId = vcRender_EncodeModelId(polygonIds + i);
      if (!pInstance->selectable)
        objectId = 0.0f; // sentinel for 'not selectable'

      if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
        vcPolygonModel_Render(pInstance->pModel, objectId, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_Standard, pInstance->tint, pInstance->pDiffuseOverride, pColourOverride);
      else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
        vcSceneLayerRenderer_Render(pInstance->pSceneLayer, objectId, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution);
    }

    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcSceneLayer_EndFrame();

    for (size_t i = 0; i < renderData.waterVolumes.length; ++i)
      vcWaterRenderer_Render(renderData.waterVolumes[i], pProgramState->camera.matrices.view, pProgramState->camera.matrices.viewProjection, pRenderContext->skyboxShaderPanorama.pSkyboxTexture, pProgramState->deltaTime);
  }

  // Terrain rendering is weird - this should be last in this pass because:
  // a) It has depth test optimizations enabled
  // b) It could be transparent, depending on settings
  vcRenderTerrain(pProgramState, pRenderContext);
}

void vcRender_RenderUI(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  // screen images
  uint32_t imageIds = vcObjectId_SceneItems + (uint32_t)renderData.polyModels.length;
  for (uint32_t i = 0; i < renderData.images.length; ++i)
  {
    vcImageRenderInfo *pImage = renderData.images[i];
    if (pImage->type != vcIT_ScreenPhoto) // too far
      continue;

    float encodedImageId = vcRender_EncodeModelId(imageIds + i);
    vcImageRenderer_Render(pImage, encodedImageId, pProgramState->camera.matrices.viewProjection, pRenderContext->sceneResolution, 1.0f);
  }

  // Pins
  vcPinRenderer_Render(pRenderContext->pPinRenderer, pProgramState->camera.matrices.viewProjection, pProgramState->sceneResolution, renderData.labels);

  // Labels
  uint32_t labelIds = vcObjectId_SceneItems + (uint32_t)(renderData.polyModels.length + renderData.images.length);
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  for (uint32_t i = 0; i < renderData.labels.length; ++i)
  {
    float encodedLabelId = vcRender_EncodeModelId(labelIds + i);
    vcLabelRenderer_Render(drawList, renderData.labels[i], encodedLabelId, pProgramState->camera.matrices.viewProjection, pProgramState->sceneResolution);
  }

  vcGLState_ResetState();
}

void vcRender_TransparentPass(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);

  // lines
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  udDouble4 nearPlane = vcCamera_GetNearPlane(pProgramState->camera, pProgramState->settings.camera);
  for (size_t i = 0; i < renderData.lines.length; ++i)
    vcLineRenderer_Render(pRenderContext->pLineRenderer, renderData.lines[i], pProgramState->camera.matrices.viewProjection, pProgramState->sceneResolution, nearPlane);

  // Images
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Front);

    uint32_t imageIds = vcObjectId_SceneItems + (uint32_t)(renderData.polyModels.length);
    for (uint32_t i = 0; i < renderData.images.length; ++i)
    {
      vcImageRenderInfo *pImage = renderData.images[i];
      double distToCamera = udMag3(pProgramState->camera.position - renderData.images[i]->position);
      double zScale = 1.0 - distToCamera / pProgramState->settings.presentation.imageRescaleDistance;

      if (pImage->type == vcIT_ScreenPhoto || zScale < 0) // too far
        continue;

      float encodedImageId = vcRender_EncodeModelId(imageIds + i);
      vcImageRenderer_Render(pImage, encodedImageId, pProgramState->camera.matrices.viewProjection, pRenderContext->sceneResolution, zScale);
    }
  }

  // Fences
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

    for (size_t i = 0; i < renderData.fences.length; ++i)
      vcFenceRenderer_Render(renderData.fences[i], pProgramState->camera.matrices.viewProjection, pProgramState->deltaTime);
  }

  // Transparent polygons
  udFloat4 whiteColour = udFloat4::one();
  uint32_t polygonIds = vcObjectId_SceneItems;
  for (uint32_t i = 0; i < renderData.polyModels.length; ++i)
  {
    vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
    if (!pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_Transparent))
      continue;

    udFloat4 *pColourOverride = nullptr;
    if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_IgnoreTint))
      pColourOverride = &whiteColour;

    vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

    float objectId = vcRender_EncodeModelId(polygonIds + i);
    if (!pInstance->selectable)
      objectId = 0.0f; // sentinel for 'not selectable'

    if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
      vcPolygonModel_Render(pInstance->pModel, objectId, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_Standard, pInstance->tint, pInstance->pDiffuseOverride, pColourOverride);
    else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
      vcSceneLayerRenderer_Render(pInstance->pSceneLayer, objectId, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution);
  }

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
}

void vcRender_BeginFrame(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  udUnused(pProgramState);

  pRenderContext->currentMouseUV = udFloat2::create((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x, (float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y);
  pRenderContext->picking.location.x = (uint32_t)(pRenderContext->currentMouseUV.x * pRenderContext->sceneResolution.x);
  pRenderContext->picking.location.y = (uint32_t)(pRenderContext->currentMouseUV.y * pRenderContext->sceneResolution.y);

  renderData.pSceneTexture = pRenderContext->pFinalColour;
  renderData.sceneScaling = udFloat2::one();

  pRenderContext->activeRenderTarget = 0;

  // TODO (EVC-835): fix scene scaling
  // udFloat2::create(float(pRenderContext->originalSceneResolution.x) / pRenderContext->sceneResolution.x, float(pRenderContext->originalSceneResolution.y) / pRenderContext->sceneResolution.y);
}

void vcRender_ApplySelectionBuffer(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  udFloat2 sampleStepSize = udFloat2::create(1.0f / pRenderContext->effectResolution.x, 1.0f / pRenderContext->effectResolution.y);

  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  pRenderContext->selectionShader.params.stepSizeThickness.x = sampleStepSize.x;
  pRenderContext->selectionShader.params.stepSizeThickness.y = sampleStepSize.y;
  pRenderContext->selectionShader.params.stepSizeThickness.z = (float)(2 << vcRender_OutlineEffectDownscale) * (pProgramState->settings.objectHighlighting.thickness - 1.0f); // roughly
  pRenderContext->selectionShader.params.stepSizeThickness.w = 0.2f;
  pRenderContext->selectionShader.params.colour = pProgramState->settings.objectHighlighting.colour;

  vcShader_Bind(pRenderContext->selectionShader.pProgram);

  vcShader_BindTexture(pRenderContext->selectionShader.pProgram, pRenderContext->pAuxiliaryTextures[0], 0, pRenderContext->selectionShader.uniform_texture);
  vcShader_BindConstantBuffer(pRenderContext->selectionShader.pProgram, pRenderContext->selectionShader.uniform_params, &pRenderContext->selectionShader.params, sizeof(pRenderContext->selectionShader.params));

  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
}

bool vcRender_DrawSelectedGeometry(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  bool active = false;

  // check UD first
  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->m_visible && renderData.models[i]->m_loadStatus == vcSLS_Loaded)
    {
      if (renderData.models[i]->IsSubitemSelected(0))
      {
        float splatId = 1.0f / 255.0f;

        vcRender_SplatUDWithId(pProgramState, pRenderContext, splatId);
        active = true;
      }
    }
  }

  float selectionMask = 1.0f; // mask selected object
  for (size_t i = 0; i < renderData.polyModels.length; ++i)
  {
    vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
    if ((pInstance->sceneItemInternalId == 0 && pInstance->pSceneItem->m_selected) || (pInstance->sceneItemInternalId != 0 && pInstance->pSceneItem->IsSubitemSelected(pInstance->sceneItemInternalId)))
    {
      vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

      if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
        vcPolygonModel_Render(pInstance->pModel, selectionMask, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_ColourOnly);
      else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
        vcSceneLayerRenderer_Render(pInstance->pSceneLayer, selectionMask, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution, nullptr, vcPMP_ColourOnly);

      active = true;
    }
  }

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  // screen images
  for (uint32_t i = 0; i < renderData.images.length; ++i)
  {
    vcImageRenderInfo *pImage = renderData.images[i];
    if ((pImage->sceneItemInternalId == 0 && pImage->pSceneItem->m_selected) || (pImage->sceneItemInternalId != 0 && pImage->pSceneItem->IsSubitemSelected(pImage->sceneItemInternalId)))
    {
      double zScale = 1.0f;
      if (pImage->type != vcIT_ScreenPhoto)
      {
        // code duplication
        double distToCamera = udMag3(pProgramState->camera.position - renderData.images[i]->position);
        zScale = 1.0 - distToCamera / pProgramState->settings.presentation.imageRescaleDistance;
        if (zScale < 0) // too far
          continue;
      }

      vcImageRenderer_Render(pImage, selectionMask, pProgramState->camera.matrices.viewProjection, pRenderContext->sceneResolution, zScale, pProgramState->pWhiteTexture);

      active = true;
    }
  }

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);

  return active;
}

bool vcRender_CreateSelectionBuffer(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (pProgramState->settings.objectHighlighting.colour.w == 0.0f || !pProgramState->settings.objectHighlighting.enable) // disabled
    return false;

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetViewport(0, 0, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y);

  vcFramebuffer_Bind(pRenderContext->pAuxiliaryFramebuffers[0], vcFramebufferClearOperation_All);

  if (!vcRender_DrawSelectedGeometry(pProgramState, pRenderContext, renderData))
    return false;

  // blur disabled at thickness value of 1.0
  if (pProgramState->settings.objectHighlighting.thickness > 1.0f)
  {
    udFloat2 sampleStepSize = udFloat2::create(1.0f / pRenderContext->effectResolution.x, 1.0f / pRenderContext->effectResolution.y);

    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

    for (int i = 0; i < 2; ++i)
    {
      vcFramebuffer_Bind(pRenderContext->pAuxiliaryFramebuffers[1 - i], vcFramebufferClearOperation_All);

      pRenderContext->blurShader.params.stepSize.x = i == 0 ? sampleStepSize.x : 0.0f;
      pRenderContext->blurShader.params.stepSize.y = i == 0 ? 0.0f : sampleStepSize.y;

      vcShader_Bind(pRenderContext->blurShader.pProgram);

      vcShader_BindTexture(pRenderContext->blurShader.pProgram, pRenderContext->pAuxiliaryTextures[i], 0, pRenderContext->blurShader.uniform_texture);
      vcShader_BindConstantBuffer(pRenderContext->blurShader.pProgram, pRenderContext->blurShader.uniform_params, &pRenderContext->blurShader.params, sizeof(pRenderContext->blurShader.params));

      vcMesh_Render(gInternalMeshes[vcInternalMeshType_ScreenQuad]);
    }
  }
  return true;
}

void vcRender_RenderWatermark(vcRenderContext *pRenderContext, vcTexture *pWatermark)
{
  if (pWatermark == nullptr)
    return;

  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  udInt2 imageSize = udInt2::zero();
  vcTexture_GetSize(pWatermark, &imageSize.x, &imageSize.y);

  // Scale the logo with scene resolution, maintaining aspect ratio
  static const float ImageToSceneRatio = 3.0f;
  udFloat2 logoScaleDimensions = udFloat2::create(pRenderContext->sceneResolution.x / (imageSize.x * ImageToSceneRatio), pRenderContext->sceneResolution.y / (imageSize.y * ImageToSceneRatio));
  float logoScale = udMin(1.0f, udMin(logoScaleDimensions.x, logoScaleDimensions.y));

  udFloat3 position = udFloat3::create(logoScale * float(imageSize.x + 10) / pRenderContext->sceneResolution.x - 1, logoScale * float(imageSize.y + 50) / pRenderContext->sceneResolution.y - 1, 0);
  udFloat3 scale = udFloat3::create(logoScale * 2.0f * float(imageSize.x) / pRenderContext->sceneResolution.x, logoScale * -2.0f * float(imageSize.y) / pRenderContext->sceneResolution.y, 1.0);

#if GRAPHICS_API_OPENGL
  position.y = -position.y;
  scale.y = -scale.y;
#endif

  pRenderContext->watermarkShader.params.u_worldViewProjectionMatrix = udFloat4x4::scaleNonUniform(scale, position);

  vcShader_Bind(pRenderContext->watermarkShader.pProgram);
  vcShader_BindConstantBuffer(pRenderContext->watermarkShader.pProgram, pRenderContext->watermarkShader.uniform_params, &pRenderContext->watermarkShader.params, sizeof(pRenderContext->watermarkShader.params));
  vcShader_BindTexture(pRenderContext->watermarkShader.pProgram, pWatermark, 0, pRenderContext->watermarkShader.uniform_texture);
  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ImGuiQuad]);
}

void vcRender_RenderScene(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer)
{
  udUnused(pDefaultFramebuffer);

  // project camera position to base altitude
  udDouble3 cameraPositionInLongLat = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->camera.position);
  cameraPositionInLongLat.z = 0.0;
  pRenderContext->cameraZeroAltitude = udGeoZone_LatLongToCartesian(pProgramState->geozone, cameraPositionInLongLat);

  pRenderContext->pinUpdateRateTimer += pProgramState->deltaTime;
  if (pRenderContext->pinUpdateRateTimer >= PinRendererUpdateRateSec)
  {
    pRenderContext->pinUpdateRateTimer = 0.0;

    vcPinRenderer_Reset(pRenderContext->pPinRenderer);
    for (size_t i = 0; i < renderData.pins.length; ++i)
      vcPinRenderer_AddPin(pRenderContext->pPinRenderer, pProgramState, renderData.pins[i].pSceneItem, renderData.pins[i].pPinAddress, renderData.pins[i].position, renderData.pins[i].count);
  }

  // Render and upload UD buffers
  if (renderData.models.length > 0)
  {
    vcRender_RenderUD(pProgramState, pRenderContext, pRenderContext->udRenderContext.pRenderView, &pProgramState->camera, renderData, true);
    vcTexture_UploadPixels(pRenderContext->udRenderContext.pColourTex, pRenderContext->udRenderContext.pColorBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
    vcTexture_UploadPixels(pRenderContext->udRenderContext.pDepthTex, pRenderContext->udRenderContext.pDepthBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
  }

  bool selectionBufferActive = vcRender_CreateSelectionBuffer(pProgramState, pRenderContext, renderData);

  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  vcRender_OpaquePass(pProgramState, pRenderContext, renderData);

  vcRender_VisualizationPass(pProgramState, pRenderContext);
  vcRender_RenderAtmosphere(pProgramState, pRenderContext);

  vcRender_TransparentPass(pProgramState, pRenderContext, renderData);

  vcRender_RenderUI(pProgramState, pProgramState->pRenderContext, renderData);

  vcRender_AsyncReadFrameDepth(pRenderContext);
  vcRender_PostProcessPass(pProgramState, pRenderContext);

  vcRender_RenderAndApplyViewSheds(pProgramState, pRenderContext, renderData);

  if (selectionBufferActive)
    vcRender_ApplySelectionBuffer(pProgramState, pRenderContext);

  // Watermark
  if (pProgramState->settings.presentation.showEuclideonLogo)
    vcRender_RenderWatermark(pRenderContext, pProgramState->pCompanyWatermark);

  vcGLState_ResetState();
  vcShader_Bind(nullptr);
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  vdkStreamerStatus streamingStatus = {};
  vdkStreamer_Update(&streamingStatus);
  pProgramState->isStreaming |= streamingStatus.active;
  pProgramState->streamingMemory = streamingStatus.memoryInUse;
}

udResult vcRender_RecreateUDView(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (pRenderContext->udRenderContext.pRenderView && vdkRenderView_Destroy(&pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Create(pProgramState->pVDKContext, &pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pRenderer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetTargets(pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pColorBuffer, 0, pRenderContext->udRenderContext.pDepthBuffer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetMatrix(pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pProgramState->camera.matrices.projectionUD.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

bool vcRender_FindSnapPoint(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, udInt2 &snapPoint)
{
  if (!pProgramState->settings.mouseSnap.enable)
    return false;

  bool bFind = false;
  int lastOffset = 2 * pProgramState->settings.mouseSnap.range * pProgramState->settings.mouseSnap.range;
  for (int offsety = udMax(0, renderData.mouse.position.y - pProgramState->settings.mouseSnap.range); offsety < udMin(renderData.mouse.position.y + pProgramState->settings.mouseSnap.range, (int)pRenderContext->originalSceneResolution.y - 1); offsety++)
    for (int offsetx = udMax(0, renderData.mouse.position.x - pProgramState->settings.mouseSnap.range); offsetx < udMin(renderData.mouse.position.x + pProgramState->settings.mouseSnap.range, (int)pRenderContext->originalSceneResolution.x - 1); offsetx++)
    {
      uint32_t pickingX = (uint32_t)((float)offsetx / (float)pRenderContext->originalSceneResolution.x * (float)pRenderContext->sceneResolution.x);
      uint32_t pickingY = (uint32_t)((float)offsety / (float)pRenderContext->originalSceneResolution.y * (float)pRenderContext->sceneResolution.y);
      float depth = pRenderContext->udRenderContext.pDepthBuffer[pickingY * pRenderContext->sceneResolution.x + pickingX];
      if (depth == 1.0f) continue;

      int offset = (offsetx - renderData.mouse.position.x) * (offsetx - renderData.mouse.position.x) + (offsety - renderData.mouse.position.y) * (offsety - renderData.mouse.position.y);
      if (offset < lastOffset)
      {
        lastOffset = offset;
        bFind = true;
        snapPoint.x = offsetx;
        snapPoint.y = offsety;
      }
    }

  return bFind;
}

udResult vcRender_RenderUD(vcState *pProgramState, vcRenderContext *pRenderContext, vdkRenderView *pRenderView, vcCamera *pCamera, vcRenderData &renderData, bool doPick)
{
  if (pRenderContext == nullptr)
    return udR_InvalidParameter_;

  if (pRenderView == nullptr && pProgramState->pVDKContext)
    vcRender_RecreateUDView(pProgramState, pRenderContext);

  vdkRenderInstance *pModels = nullptr;
  vcUDRSData *pVoxelShaderData = nullptr;

  int numVisibleModels = 0;

  vdkRenderView_SetMatrix(pRenderView, vdkRVM_Projection, pCamera->matrices.projectionUD.a);
  vdkRenderView_SetMatrix(pRenderView, vdkRVM_View, pCamera->matrices.view.a);

  if (renderData.models.length > 0)
  {
    pModels = udAllocType(vdkRenderInstance, renderData.models.length, udAF_None);
    pVoxelShaderData = udAllocType(vcUDRSData, renderData.models.length, udAF_None);
  }

  double maxDistSqr = pProgramState->settings.camera.farPlane * pProgramState->settings.camera.farPlane;
  if (doPick)
    pProgramState->udModelPickedIndex = -1;

  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->m_visible && renderData.models[i]->m_loadStatus == vcSLS_Loaded)
    {
      // Copy to the contiguous array
      pModels[numVisibleModels].pPointCloud = renderData.models[i]->m_pPointCloud;
      memcpy(&pModels[numVisibleModels].matrix, renderData.models[i]->m_sceneMatrix.a, sizeof(pModels[numVisibleModels].matrix));
      pModels[numVisibleModels].modelFlags = vdkRMF_None;

      pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Black;
      pModels[numVisibleModels].pVoxelUserData = &pVoxelShaderData[numVisibleModels];

      pVoxelShaderData[numVisibleModels].pModel = renderData.models[i];
      pVoxelShaderData[numVisibleModels].pProgramData = pProgramState;

      // Fallback to global settings when model is default
      const vcVisualizationSettings *pVisSettings = (renderData.models[i]->m_visualization.mode != vcVM_Default ? &renderData.models[i]->m_visualization : &pProgramState->settings.visualization);

      // Fallback to the first available option when default, if all else fails, render black.
      if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_Colour) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_ARGB, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Colour;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_Intensity) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_Intensity, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Intensity;

        pVoxelShaderData[numVisibleModels].data.intensity.maxIntensity = (uint16_t)pVisSettings->maxIntensity;
        pVoxelShaderData[numVisibleModels].data.intensity.minIntensity = (uint16_t)pVisSettings->minIntensity;
        pVoxelShaderData[numVisibleModels].data.intensity.intensityRange = (float)(pVisSettings->maxIntensity - pVisSettings->minIntensity);
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_Classification) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_Classification, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Classification;

        pVoxelShaderData[numVisibleModels].data.classification.pCustomClassificationColors = pVisSettings->customClassificationColors;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_DisplacementDistance) && vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacement", &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_DisplacementDistance;

        pVoxelShaderData[numVisibleModels].data.displacementAmount.minThreshold = pVisSettings->displacement.bounds.x;
        pVoxelShaderData[numVisibleModels].data.displacementAmount.maxThreshold = pVisSettings->displacement.bounds.y;

        pVoxelShaderData[numVisibleModels].data.displacementAmount.errorColour = pVisSettings->displacement.error;

        pVoxelShaderData[numVisibleModels].data.displacementAmount.maxColour = pVisSettings->displacement.max;
        pVoxelShaderData[numVisibleModels].data.displacementAmount.minColour = pVisSettings->displacement.min;
        pVoxelShaderData[numVisibleModels].data.displacementAmount.midColour = pVisSettings->displacement.mid;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_DisplacementDirection) && vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacement", &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_DisplacementDirection;
        vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacementDirectionX", &pVoxelShaderData[numVisibleModels].data.displacementDirection.attributeOffsets[0]);
        vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacementDirectionY", &pVoxelShaderData[numVisibleModels].data.displacementDirection.attributeOffsets[1]);
        vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacementDirectionZ", &pVoxelShaderData[numVisibleModels].data.displacementDirection.attributeOffsets[2]);

        pVoxelShaderData[numVisibleModels].data.displacementDirection.cameraDirection = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->camera.position, pProgramState->camera.headingPitch).apply({ 0, 1, 0 });
        pVoxelShaderData[numVisibleModels].data.displacementDirection.posColour = pVisSettings->displacement.max;
        pVoxelShaderData[numVisibleModels].data.displacementDirection.negColour = pVisSettings->displacement.min;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_GPSTime) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_GPSTime, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        double range = pVisSettings->GPSTime.maxTime - pVisSettings->GPSTime.minTime;
        if (range < 0.0001)
        {
          pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Black;
        }
        else
        {
          pModels[numVisibleModels].pVoxelShader = vcVoxelShader_GPSTime;
          pVoxelShaderData[numVisibleModels].data.GPSTime.mult = 1.0 / range;
          pVoxelShaderData[numVisibleModels].data.GPSTime.minTime = pVisSettings->GPSTime.minTime;
          pVoxelShaderData[numVisibleModels].data.GPSTime.maxTime = pVisSettings->GPSTime.maxTime;
        }
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_ScanAngle) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_ScanAngle, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        double minAngleNorm = (pVisSettings->scanAngle.minAngle + 180.0) / 360.0;
        double maxAngleNorm = (pVisSettings->scanAngle.maxAngle + 180.0) / 360.0;

        pVoxelShaderData[numVisibleModels].data.scanAngle.minAngle = int16_t(minAngleNorm * vcVisualizationSettings::s_scanAngleRange + vcVisualizationSettings::s_scanAngleMin);
        pVoxelShaderData[numVisibleModels].data.scanAngle.maxAngle = int16_t(maxAngleNorm * vcVisualizationSettings::s_scanAngleRange + vcVisualizationSettings::s_scanAngleMin);
        pVoxelShaderData[numVisibleModels].data.scanAngle.range = pVoxelShaderData[numVisibleModels].data.scanAngle.maxAngle - pVoxelShaderData[numVisibleModels].data.scanAngle.minAngle;

        if (pVoxelShaderData[numVisibleModels].data.scanAngle.range == 0)
          pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Black;
        else
          pModels[numVisibleModels].pVoxelShader = vcVoxelShader_ScanAngle;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_PointSourceID) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_PointSourceID, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_PointSourceID;
        pVoxelShaderData[numVisibleModels].data.pointSourceID.defaultColour = pVisSettings->pointSourceID.defaultColour;
        pVoxelShaderData[numVisibleModels].data.pointSourceID.pColourMap = &pVisSettings->pointSourceID.colourMap;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_ReturnNumber) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_ReturnNumber, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_ReturnNumber;
        pVoxelShaderData[numVisibleModels].data.returnNumber.pColours = pVisSettings->returnNumberColours;
      }
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_NumberOfReturns) && vdkAttributeSet_GetOffsetOfStandardAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, vdkSA_NumberOfReturns, &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_NumberOfReturns;
        pVoxelShaderData[numVisibleModels].data.numberOfReturns.pColours = pVisSettings->numberOfReturnsColours;
      }

      ++numVisibleModels;

      if (renderData.models[i]->m_hasWatermark && pProgramState->showWatermark)
      {
        udDouble3 distVector = pCamera->position - renderData.models[i]->GetWorldSpacePivot();

        double cameraDistSqr = udMagSq(distVector);
        if (cameraDistSqr < maxDistSqr)
        {
          maxDistSqr = cameraDistSqr;

          if (renderData.models[i]->m_pWatermark == nullptr) // Load the watermark
          {
            const char *pWatermarkStr = renderData.models[i]->m_metadata.Get("Watermark").AsString();
            if (pWatermarkStr)
            {
              uint8_t *pImage = nullptr;
              size_t imageLen = 0;
              if (udBase64Decode(&pImage, &imageLen, pWatermarkStr) == udR_Success)
              {
                int imageWidth, imageHeight, imageChannels;
                unsigned char *pImageData = stbi_load_from_memory(pImage, (int)imageLen, &imageWidth, &imageHeight, &imageChannels, 4);
                vcTexture_Create(&renderData.models[i]->m_pWatermark, imageWidth, imageHeight, pImageData);
                free(pImageData);
              }

              udFree(pImage);
            }
          }
        }
      }
    }
  }

  vdkRenderPicking picking = {};
  picking.x = (uint32_t)((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x * (float)pRenderContext->sceneResolution.x);
  picking.y = (uint32_t)((float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y * (float)pRenderContext->sceneResolution.y);

  vdkRenderOptions renderOptions;
  memset(&renderOptions, 0, sizeof(vdkRenderOptions));

  if (doPick)
    renderOptions.pPick = &picking;

  renderOptions.pFilter = renderData.pQueryFilter;
  renderOptions.pointMode = (vdkRenderContextPointMode)pProgramState->settings.presentation.pointMode;
  renderOptions.flags = (vdkRenderFlags)(vdkRF_LogarithmicDepth | vdkRF_ManualStreamerUpdate);

  vdkError result = vdkRenderContext_Render(pRenderContext->udRenderContext.pRenderer, pRenderView, pModels, numVisibleModels, &renderOptions);

  if (result == vE_Success && doPick && !picking.hit && pProgramState->settings.mouseSnap.enable)
  {
    udInt2 snapPoint = udInt2::zero();
    if (vcRender_FindSnapPoint(pProgramState, pRenderContext, renderData, snapPoint))
    {
      renderData.mouse.position = snapPoint;
      picking.x = (uint32_t)((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x * (float)pRenderContext->sceneResolution.x);
      picking.y = (uint32_t)((float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y * (float)pRenderContext->sceneResolution.y);
      vdkRenderContext_Render(pRenderContext->udRenderContext.pRenderer, pRenderView, pModels, numVisibleModels, &renderOptions);
    }
  }

  if (result == vE_Success)
  {
    if (doPick && picking.hit)
    {
      // More to be done here
      pProgramState->pickingSuccess = true;
      pProgramState->worldMousePosCartesian = udDouble3::create(picking.pointCenter[0], picking.pointCenter[1], picking.pointCenter[2]);

      uint32_t j = 0;
      for (size_t i = 0; i < renderData.models.length; ++i)
      {
        if (renderData.models[i]->m_visible && renderData.models[i]->m_loadStatus == vcSLS_Loaded)
        {
          if (j == picking.modelIndex)
          {
            pProgramState->udModelPickedIndex = (int)i;
            pProgramState->udModelPickedNode = picking.voxelID;
            break;
          }
          ++j;
        }
      }
    }
  }
  else
  {
    //TODO: Clear the buffers
  }

  udFree(pModels);
  udFree(pVoxelShaderData);
  return udR_Success;
}

void vcRender_ClearTiles(vcRenderContext *pRenderContext)
{
  if (pRenderContext == nullptr)
    return;

  vcTileRenderer_ClearTiles(pRenderContext->pTileRenderer);
}

vcRenderPickResult vcRender_PolygonPick(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, bool doSelectRender)
{
  vcRenderPickResult result = {};

  if (pRenderContext->currentMouseUV.x < 0 || pRenderContext->currentMouseUV.x > 1 || pRenderContext->currentMouseUV.y < 0 || pRenderContext->currentMouseUV.y > 1)
    return result;

  float pickDepth = 1.0f;
  if (doSelectRender)
  {
    uint8_t pixelBytes[8] = {};
    vcTexture_EndReadPixels(pRenderContext->gBuffer[pRenderContext->asyncReadTarget].pNormal, pRenderContext->picking.location.x, pRenderContext->picking.location.y, 1, 1, pixelBytes);
    pRenderContext->asyncReadTarget = -1;

    uint16_t pickId = vcObjectId_Null;
    vcRender_DecodeModelId(pixelBytes, &pickId, &pickDepth);

    result.success = (pickId != vcObjectId_Null);
    if (pickId == vcObjectId_Terrain)
    {
      // Terrain    
    }
    else if (pickId >= vcObjectId_SceneItems)
    {
      pickId -= vcObjectId_SceneItems; // remove offset

      // determine selected item type, based on index
      if (pickId < renderData.polyModels.length)
      {
        // polygon
        result.pSceneItem = renderData.polyModels[pickId].pSceneItem;
        result.sceneItemInternalId = renderData.polyModels[pickId].sceneItemInternalId;
      }
      else if (pickId < renderData.polyModels.length + renderData.images.length)
      {
        // image
        result.pSceneItem = renderData.images[pickId - renderData.polyModels.length]->pSceneItem;
      }
      else if (pickId < renderData.polyModels.length + renderData.images.length + renderData.labels.length)
      {
        // label
        result.pSceneItem = renderData.labels[pickId - renderData.images.length - renderData.polyModels.length]->pSceneItem;
        result.sceneItemInternalId = renderData.labels[pickId - renderData.images.length - renderData.polyModels.length]->sceneItemInternalId;
      }
      else
      {
        // TODO: pins
      }
    }
  }
  else
  {
    if (pRenderContext->previousPickedId != vcObjectId_Null)
    {
      result.success = true;
      pickDepth = pRenderContext->previousFrameDepth;
    }
  }

  if (result.success)
    result.position = vcRender_DepthToWorldPosition(pProgramState, pRenderContext, pickDepth);

  return result;
}

udDouble3 vcRender_QueryMapAtCartesian(vcRenderContext *pRenderContext, const udDouble3 &point, const udDouble3 *pWorldUp /*= nullptr*/, udDouble3 *pNormal /*= nullptr*/)
{
  return vcTileRenderer_QueryMapAtCartesian(pRenderContext->pTileRenderer, point, pWorldUp, pNormal);
}
