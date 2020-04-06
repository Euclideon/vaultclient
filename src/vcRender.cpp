#include "vcRender.h"

#include "gl/vcFramebuffer.h"
#include "gl/vcShader.h"
#include "gl/vcGLState.h"
#include "vcFenceRenderer.h"
#include "vcWaterRenderer.h"
#include "vcTileRenderer.h"
#include "vcCompass.h"
#include "vcState.h"
#include "vcVoxelShaders.h"
#include "vcConstants.h"

#include "vcInternalModels.h"
#include "vcSceneLayerRenderer.h"
#include "vcCamera.h"
#include "vcAtmosphereRenderer.h"

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

// Temp hard-coded view shed properties
static const int ViewShedMapCount = 3;
static const udUInt2 ViewShedMapRes = udUInt2::create(640 * ViewShedMapCount, 1920);

struct vcViewShedRenderContext
{
  // re-use this memory
  float *pDepthBuffer;
  vcTexture *pUDDepthTexture;

  vcTexture *pDepthTex;
  vcTexture *pDummyColour;
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

  vcFramebuffer *pFramebuffer[vcRender_RenderBufferCount];
  vcTexture *pTexture[vcRender_RenderBufferCount];
  vcTexture *pDepthTexture[vcRender_RenderBufferCount];

  vcFramebuffer *pAuxiliaryFramebuffers[2];
  vcTexture *pAuxiliaryTextures[2];
  udUInt2 effectResolution;

  vcAtmosphereRenderer *pAtmosphereRenderer;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_depth;
    vcShaderConstantBuffer *uniform_vertParams;
    vcShaderConstantBuffer *uniform_fragParams;

    struct
    {
      udFloat4 screenParams;  // sampleStepX, sampleStepSizeY, (unused), (unused)
      udFloat4x4 inverseViewProjection;
      udFloat4x4 inverseProjection;

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
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_depth;
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
  vcFenceRenderer *pDiagnosticFences;

  vcTileRenderer *pTileRenderer;
  vcAnchor *pCompass;

  float previousFrameDepth;
  udFloat2 currentMouseUV;

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

    vcFramebuffer *pFramebuffer;
    vcTexture *pTexture;
    vcTexture *pDepth;
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

udResult vcRender_RecreateUDView(vcState *pProgramState, vcRenderContext *pRenderContext);
udResult vcRender_RenderUD(vcState *pProgramState, vcRenderContext *pRenderContext, vdkRenderView *pRenderView, vcCamera *pCamera, vcRenderData &renderData, bool doPick);
void vcRender_RenderWatermark(vcRenderContext *pRenderContext, vcTexture *pWatermark);

vcFramebuffer *vcRender_GetSceneFramebuffer(vcRenderContext *pRenderContext)
{
  return pRenderContext->pFramebuffer[0];
}

udResult vcRender_Init(vcState *pProgramState, vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, const udUInt2 &sceneResolution)
{
  udResult result;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  UD_ERROR_CHECK(vcInternalModels_Init());

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  pRenderContext->viewShedRenderingContext.pDepthBuffer = udAllocType(float, ViewShedMapRes.x * ViewShedMapRes.y, udAF_Zero);
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pUDDepthTexture, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_Dynamic));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pDepthTex, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->viewShedRenderingContext.pDummyColour, ViewShedMapRes.x, ViewShedMapRes.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->viewShedRenderingContext.pFramebuffer, pRenderContext->viewShedRenderingContext.pDummyColour, pRenderContext->viewShedRenderingContext.pDepthTex), udR_InternalError);

  UD_ERROR_CHECK(vcTexture_AsyncCreateFromFilename(&pRenderContext->skyboxShaderPanorama.pSkyboxTexture, pWorkerPool, "asset://assets/skyboxes/WaterClouds.jpg", vcTFM_Linear));
  UD_ERROR_CHECK(vcCompass_Create(&pRenderContext->pCompass));

  UD_ERROR_CHECK(vcAtmosphereRenderer_Create(&pRenderContext->pAtmosphereRenderer));

  UD_ERROR_CHECK(vcPolygonModel_CreateShaders());
  UD_ERROR_CHECK(vcImageRenderer_Init());
  UD_ERROR_CHECK(vcLabelRenderer_Init());

  UD_ERROR_CHECK(vcTileRenderer_Create(&pRenderContext->pTileRenderer, &pProgramState->settings));
  UD_ERROR_CHECK(vcFenceRenderer_Create(&pRenderContext->pDiagnosticFences));

  UD_ERROR_CHECK(vcRender_ReloadShaders(pRenderContext, pWorkerPool));
  UD_ERROR_CHECK(vcRender_ResizeScene(pProgramState, pRenderContext, sceneResolution.x, sceneResolution.y));

  *ppRenderContext = pRenderContext;
  pRenderContext = nullptr;
  result = udR_Success;
epilogue:

  if (pRenderContext != nullptr)
    vcRender_Destroy(pProgramState, &pRenderContext);

  return result;
}

udResult vcRender_ReloadShaders(vcRenderContext *pRenderContext, udWorkerPool *pWorkerPool)
{
  udResult result;

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

  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->udRenderContext.presentShader.pProgram, "asset://assets/shaders/udVertexShader", "asset://assets/shaders/udFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->visualizationShader.pProgram, "asset://assets/shaders/visualizationVertexShader", "asset://assets/shaders/visualizationFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->shadowShader.pProgram, "asset://assets/shaders/viewShedVertexShader", "asset://assets/shaders/viewShedFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->skyboxShaderPanorama.pProgram, "asset://assets/shaders/panoramaSkyboxVertexShader", "asset://assets/shaders/panoramaSkyboxFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->skyboxShaderTintImage.pProgram, "asset://assets/shaders/imageColourSkyboxVertexShader", "asset://assets/shaders/imageColourSkyboxFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->udRenderContext.splatIdShader.pProgram, "asset://assets/shaders/udVertexShader", "asset://assets/shaders/udSplatIdFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->postEffectsShader.pProgram, "asset://assets/shaders/postEffectsVertexShader", "asset://assets/shaders/postEffectsFragmentShader", vcP3UV2VertexLayout), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->visualizationShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->visualizationShader.uniform_texture, pRenderContext->visualizationShader.pProgram, "sceneColour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->visualizationShader.uniform_depth, pRenderContext->visualizationShader.pProgram, "sceneDepth"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->visualizationShader.uniform_vertParams, pRenderContext->visualizationShader.pProgram, "u_vertParams", sizeof(pRenderContext->visualizationShader.vertParams)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->visualizationShader.uniform_fragParams, pRenderContext->visualizationShader.pProgram, "u_fragParams", sizeof(pRenderContext->visualizationShader.fragParams)), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->shadowShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->shadowShader.uniform_depth, pRenderContext->shadowShader.pProgram, "sceneDepth"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->shadowShader.uniform_shadowMapAtlas, pRenderContext->shadowShader.pProgram, "shadowMapAtlas"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->shadowShader.uniform_params, pRenderContext->shadowShader.pProgram, "u_params", sizeof(pRenderContext->shadowShader.params)), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->skyboxShaderPanorama.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->skyboxShaderPanorama.uniform_texture, pRenderContext->skyboxShaderPanorama.pProgram, "albedo"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->skyboxShaderPanorama.uniform_MatrixBlock, pRenderContext->skyboxShaderPanorama.pProgram, "u_EveryFrame", sizeof(udFloat4x4)), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->skyboxShaderTintImage.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->skyboxShaderTintImage.uniform_texture, pRenderContext->skyboxShaderTintImage.pProgram, "albedo"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->skyboxShaderTintImage.uniform_params, pRenderContext->skyboxShaderTintImage.pProgram, "u_EveryFrame", sizeof(pRenderContext->skyboxShaderTintImage.params)), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_texture, pRenderContext->udRenderContext.presentShader.pProgram, "sceneColour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_depth, pRenderContext->udRenderContext.presentShader.pProgram, "sceneDepth"), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->udRenderContext.splatIdShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->udRenderContext.splatIdShader.uniform_params, pRenderContext->udRenderContext.splatIdShader.pProgram, "u_params", sizeof(pRenderContext->udRenderContext.splatIdShader.params)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.splatIdShader.uniform_texture, pRenderContext->udRenderContext.splatIdShader.pProgram, "sceneColour"), udR_InternalError);

  UD_ERROR_IF(!vcShader_Bind(pRenderContext->postEffectsShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->postEffectsShader.uniform_texture, pRenderContext->postEffectsShader.pProgram, "sceneColour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->postEffectsShader.uniform_depth, pRenderContext->postEffectsShader.pProgram, "sceneDepth"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->postEffectsShader.uniform_params, pRenderContext->postEffectsShader.pProgram, "u_params", sizeof(pRenderContext->postEffectsShader.params)), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->blurShader.pProgram, "asset://assets/shaders/blurVertexShader", "asset://assets/shaders/blurFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pRenderContext->blurShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->blurShader.uniform_texture, pRenderContext->blurShader.pProgram, "colour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->blurShader.uniform_params, pRenderContext->blurShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->blurShader.params)), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->selectionShader.pProgram, "asset://assets/shaders/highlightVertexShader", "asset://assets/shaders/highlightFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pRenderContext->selectionShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->selectionShader.uniform_texture, pRenderContext->selectionShader.pProgram, "colour"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->selectionShader.uniform_params, pRenderContext->selectionShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->selectionShader.params)), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFile(&pRenderContext->watermarkShader.pProgram, "asset://assets/shaders/imguiVertexShader", "asset://assets/shaders/imguiFragmentShader", vcImGuiVertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pRenderContext->watermarkShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pRenderContext->watermarkShader.uniform_params, pRenderContext->watermarkShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->watermarkShader.params)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pRenderContext->watermarkShader.uniform_texture, pRenderContext->watermarkShader.pProgram, "Texture"), udR_InternalError);

  UD_ERROR_CHECK(vcAtmosphereRenderer_ReloadShaders(pRenderContext->pAtmosphereRenderer));

  UD_ERROR_IF(!vcShader_Bind(nullptr), udR_InternalError);

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

  vcTexture_Destroy(&pRenderContext->skyboxShaderPanorama.pSkyboxTexture);
  UD_ERROR_CHECK(vcCompass_Destroy(&pRenderContext->pCompass));

  UD_ERROR_CHECK(vcAtmosphereRenderer_Destroy(&pRenderContext->pAtmosphereRenderer));

  UD_ERROR_CHECK(vcPolygonModel_DestroyShaders());
  UD_ERROR_CHECK(vcImageRenderer_Destroy());
  UD_ERROR_CHECK(vcLabelRenderer_Destroy());

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);
  udFree(pRenderContext->viewShedRenderingContext.pDepthBuffer);

  UD_ERROR_CHECK(vcTileRenderer_Destroy(&pRenderContext->pTileRenderer));
  UD_ERROR_CHECK(vcFenceRenderer_Destroy(&pRenderContext->pDiagnosticFences));

  UD_ERROR_CHECK(vcInternalModels_Deinit());
  result = udR_Success;

epilogue:
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pUDDepthTexture);
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pDepthTex);
  vcTexture_Destroy(&pRenderContext->viewShedRenderingContext.pDummyColour);
  vcFramebuffer_Destroy(&pRenderContext->viewShedRenderingContext.pFramebuffer);

  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcFramebuffer_Destroy(&pRenderContext->udRenderContext.pFramebuffer);
  for (int i = 0; i < vcRender_RenderBufferCount; ++i)
  {
    vcTexture_Destroy(&pRenderContext->pTexture[i]);
    vcTexture_Destroy(&pRenderContext->pDepthTexture[i]);
    vcFramebuffer_Destroy(&pRenderContext->pFramebuffer[i]);
  }

  vcTexture_Destroy(&pRenderContext->picking.pTexture);
  vcTexture_Destroy(&pRenderContext->picking.pDepth);
  vcFramebuffer_Destroy(&pRenderContext->picking.pFramebuffer);

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
    vcTexture_Destroy(&pRenderContext->pTexture[i]);
    vcTexture_Destroy(&pRenderContext->pDepthTexture[i]);
    vcFramebuffer_Destroy(&pRenderContext->pFramebuffer[i]);
  }

  vcTexture_Destroy(&pRenderContext->picking.pTexture);
  vcTexture_Destroy(&pRenderContext->picking.pDepth);
  vcFramebuffer_Destroy(&pRenderContext->picking.pFramebuffer);

  for (int i = 0; i < 2; ++i)
  {
    vcTexture_Destroy(&pRenderContext->pAuxiliaryTextures[i]);
    vcFramebuffer_Destroy(&pRenderContext->pAuxiliaryFramebuffers[i]);
  }

  for (int i = 0; i < vcRender_RenderBufferCount; ++i)
  {
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->pTexture[i], widthIncr, heightIncr, nullptr, vcTextureFormat_RGBA8, vcTFM_Linear, vcTCF_RenderTarget));
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->pDepthTexture[i], widthIncr, heightIncr, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, vcTCF_RenderTarget | vcTCF_AsynchronousRead));
    UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->pFramebuffer[i], pRenderContext->pTexture[i], pRenderContext->pDepthTexture[i]), udR_InternalError);
  }

  pRenderContext->effectResolution.x = widthIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.y = heightIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.x = pRenderContext->effectResolution.x + (pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement : 0);
  pRenderContext->effectResolution.y = pRenderContext->effectResolution.y + (pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement : 0);

  for (int i = 0; i < 2; ++i)
  {
    UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->pAuxiliaryTextures[i], pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Linear, vcTCF_RenderTarget));
    UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->pAuxiliaryFramebuffers[i], pRenderContext->pAuxiliaryTextures[i]), udR_InternalError);
  }

  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->picking.pTexture, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_CHECK(vcTexture_Create(&pRenderContext->picking.pDepth, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, vcTCF_RenderTarget));
  UD_ERROR_IF(!vcFramebuffer_Create(&pRenderContext->picking.pFramebuffer, pRenderContext->picking.pTexture, pRenderContext->picking.pDepth), udR_InternalError);

  if (pProgramState->pVDKContext)
    UD_ERROR_CHECK(vcRender_RecreateUDView(pProgramState, pRenderContext));

epilogue:
  return result;
}

// Asychronously read a 1x1 region of last frames depth buffer 
udResult vcRender_AsyncReadFrameDepth(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  if (pRenderContext->currentMouseUV.x < 0 || pRenderContext->currentMouseUV.x > 1 || pRenderContext->currentMouseUV.y < 0 || pRenderContext->currentMouseUV.y > 1)
    return result;

  uint8_t depthBytes[4] = {};
  udUInt2 pickLocation = { (uint32_t)(pRenderContext->currentMouseUV.x * pRenderContext->sceneResolution.x), (uint32_t)(pRenderContext->currentMouseUV.y * pRenderContext->sceneResolution.y) };

  static const int readBufferIndex = 0;
  UD_ERROR_IF(!vcTexture_EndReadPixels(pRenderContext->pDepthTexture[readBufferIndex], pickLocation.x, pickLocation.y, 1, 1, depthBytes), udR_InternalError); // read previous copy
  UD_ERROR_IF(!vcTexture_BeginReadPixels(pRenderContext->pDepthTexture[readBufferIndex], pickLocation.x, pickLocation.y, 1, 1, depthBytes, pRenderContext->pFramebuffer[readBufferIndex]), udR_InternalError); // begin copy for next frame read

  // 24 bit unsigned int -> float
#if GRAPHICS_API_OPENGL || GRAPHICS_API_METAL
  pRenderContext->previousFrameDepth = uint32_t((depthBytes[3] << 16) | (depthBytes[2] << 8) | (depthBytes[1] << 0)) / ((1 << 24) - 1.0f);
  //uint8_t stencil = depthBytes[0];
#else
  pRenderContext->previousFrameDepth = uint32_t((depthBytes[2] << 16) | (depthBytes[1] << 8) | (depthBytes[0] << 0)) / ((1 << 24) - 1.0f);
  //uint8_t stencil = depthBytes[3];
#endif

  // fbo state may not be valid (e.g. first read back will be '0')
  if (pRenderContext->previousFrameDepth == 0.0f)
    pRenderContext->previousFrameDepth = 1.0f;

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
  if (pProgramState->settings.presentation.skybox.type != vcSkyboxType_Simple && pProgramState->settings.presentation.skybox.type != vcSkyboxType_Colour)
    return;

  // Draw the skybox only at the far plane, where there is no geometry.
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  if (pProgramState->settings.presentation.skybox.type == vcSkyboxType_Simple)
  {
    udFloat4x4 viewMatrixF = udFloat4x4::create(pProgramState->camera.matrices.view);
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
  if (pProgramState->settings.presentation.skybox.type != vcSkyboxType_Atmosphere)
    return;

  vcAtmosphereRenderer_SetVisualParams(pRenderContext->pAtmosphereRenderer, pProgramState->settings.presentation.skybox.exposure, pProgramState->settings.presentation.skybox.timeOfDay);

  pRenderContext->activeRenderTarget = 1 - pRenderContext->activeRenderTarget;
  vcFramebuffer_Bind(pRenderContext->pFramebuffer[pRenderContext->activeRenderTarget], vcFramebufferClearOperation_All, 0xff000000);

  vcAtmosphereRenderer_Render(pRenderContext->pAtmosphereRenderer, pProgramState, pRenderContext->pTexture[1 - pRenderContext->activeRenderTarget], pRenderContext->pDepthTexture[1 - pRenderContext->activeRenderTarget]);
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
  if (pProgramState->gis.isProjected && pProgramState->settings.maptiles.mapEnabled)
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

    // Corners [nw, ne, sw, se]
    udDouble3 localCorners[4];
    udInt2 slippyCorners[4];

    int currentZoom = 21;

    // project camera position to base altitude
    udDouble3 cameraPositionInLongLat = udGeoZone_CartesianToLatLong(pProgramState->gis.zone, pProgramState->camera.position);
    cameraPositionInLongLat.z = 0.0;
    udDouble3 cameraZeroAltitude = udGeoZone_LatLongToCartesian(pProgramState->gis.zone, cameraPositionInLongLat);
    udDouble3 cameraToZeroAltitude = localCamPos - cameraZeroAltitude;
    double cameraDistanceToAltitudeZero = udMag3(cameraToZeroAltitude);

    // TODO: Fix this
    // determine if camera is 'inside' the ground
    //udDouble3 zoneRoot = udGeoZone_LatLongToCartesian(pProgramState->gis.zone, udDouble3::zero());
    //udDouble3 surfaceNormal = udNormalize3(cameraZeroAltitude - zoneRoot);
    //if (udAbs(surfaceNormal.z) <= UD_EPSILON) // can this be assumed?
    //  surfaceNormal = udDouble3::create(0.0, 0.0, 1.0);
    bool cameraInsideGround = false;//udDot3(cameraToZeroAltitude, surfaceNormal) < 0;

    // These values were trial and errored.
    const double BaseViewDistance = 20000.0;
    const double HeightViewDistanceScale = 40.0;
    double visibleFarPlane = udMin((double)s_CameraFarPlane, BaseViewDistance + cameraDistanceToAltitudeZero * HeightViewDistanceScale);

    // Cardinal Limits
    localCorners[0] = localCamPos + udDouble3::create(-visibleFarPlane, +visibleFarPlane, 0);
    localCorners[1] = localCamPos + udDouble3::create(+visibleFarPlane, +visibleFarPlane, 0);
    localCorners[2] = localCamPos + udDouble3::create(-visibleFarPlane, -visibleFarPlane, 0);
    localCorners[3] = localCamPos + udDouble3::create(+visibleFarPlane, -visibleFarPlane, 0);

    for (int i = 0; i < 4; ++i)
      vcGIS_LocalToSlippy(&pProgramState->gis, &slippyCorners[i], localCorners[i], currentZoom);

    while (currentZoom > 0 && (slippyCorners[0] != slippyCorners[1] || slippyCorners[1] != slippyCorners[2] || slippyCorners[2] != slippyCorners[3]))
    {
      --currentZoom;

      for (int i = 0; i < 4; ++i)
        slippyCorners[i] /= 2;
    }

    for (int i = 0; i < 4; ++i)
      vcGIS_SlippyToLocal(&pProgramState->gis, &localCorners[i], slippyCorners[0] + udInt2::create(i & 1, i / 2), currentZoom);

    vcTileRenderer_Update(pRenderContext->pTileRenderer, pProgramState->deltaTime, &pProgramState->gis, udInt3::create(slippyCorners[0], currentZoom), localCamPos, cameraZeroAltitude, viewProjection);
    vcTileRenderer_Render(pRenderContext->pTileRenderer, pProgramState->camera.matrices.view, pProgramState->camera.matrices.projection, cameraInsideGround);
  }
}

void vcRender_PostProcessPass(vcState *pProgramState, vcRenderContext *pRenderContext)
{
  udUnused(pProgramState);

  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  pRenderContext->activeRenderTarget = 1 - pRenderContext->activeRenderTarget;
  vcFramebuffer_Bind(pRenderContext->pFramebuffer[pRenderContext->activeRenderTarget], vcFramebufferClearOperation_All, 0xff000000);

  vcShader_Bind(pRenderContext->postEffectsShader.pProgram);
  vcShader_BindTexture(pRenderContext->postEffectsShader.pProgram, pRenderContext->pTexture[1 - pRenderContext->activeRenderTarget], 0, pRenderContext->postEffectsShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->postEffectsShader.pProgram, pRenderContext->pDepthTexture[1 - pRenderContext->activeRenderTarget], 1, pRenderContext->postEffectsShader.uniform_depth);

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
  vcFramebuffer_Bind(pRenderContext->pFramebuffer[pRenderContext->activeRenderTarget], vcFramebufferClearOperation_All, 0xff000000);

  vcShader_Bind(pRenderContext->visualizationShader.pProgram);
  vcShader_BindTexture(pRenderContext->visualizationShader.pProgram, pRenderContext->pTexture[1 - pRenderContext->activeRenderTarget], 0, pRenderContext->visualizationShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->visualizationShader.pProgram, pRenderContext->pDepthTexture[1 - pRenderContext->activeRenderTarget], 1, pRenderContext->visualizationShader.uniform_depth);

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
  vcShader_BindTexture(pRenderContext->shadowShader.pProgram, pRenderContext->pDepthTexture[0], 0, pRenderContext->shadowShader.uniform_depth);
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
      vcCamera_UpdateMatrices(pProgramState->gis, &shadowRenderCameras[r], cameraSettings, atlasSize, nullptr);

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
            vcPolygonModel_Render(pInstance->pModel, pInstance->worldMat, viewProjection, vcPMP_Shadows);
          else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
            vcSceneLayerRenderer_Render(pInstance->pSceneLayer, pInstance->worldMat, viewProjection, shadowRenderCameras[r].position, ViewShedMapRes, nullptr, true);
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

    vcFramebuffer_Bind(pRenderContext->pFramebuffer[1]); // assumed this is the working target
    vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
    vcRender_ApplyViewShed(pRenderContext);
  }

  vcGLState_ResetState();
}

void vcRender_OpaquePass(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcFramebuffer_Bind(pRenderContext->pFramebuffer[pRenderContext->activeRenderTarget], vcFramebufferClearOperation_All, 0xff000000);

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
    for (size_t i = 0; i < renderData.polyModels.length; ++i)
    {
      vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
      if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_Transparent))
        continue;

      udFloat4 *pTintOverride = nullptr;
      if (pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_IgnoreTint))
        pTintOverride = &whiteColour;

      vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

      if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
        vcPolygonModel_Render(pInstance->pModel, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_Standard, pInstance->pDiffuseOverride, pTintOverride);
      else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
        vcSceneLayerRenderer_Render(pInstance->pSceneLayer, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution);
    }

    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcSceneLayer_EndFrame();

    for (size_t i = 0; i < renderData.waterVolumes.length; ++i)
      vcWaterRenderer_Render(renderData.waterVolumes[i], pProgramState->camera.matrices.view, pProgramState->camera.matrices.viewProjection, pRenderContext->skyboxShaderPanorama.pSkyboxTexture, pProgramState->deltaTime);
  }

  vcRenderTerrain(pProgramState, pRenderContext);

  vcRender_AsyncReadFrameDepth(pRenderContext); // note: one frame behind
}

void vcRender_RenderUI(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  // Labels
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  for (size_t i = 0; i < renderData.labels.length; ++i)
    vcLabelRenderer_Render(drawList, renderData.labels[i], pProgramState->camera.matrices.viewProjection, pProgramState->sceneResolution);

  // Watermark
  if (pProgramState->settings.presentation.showEuclideonLogo || pProgramState->pSceneWatermark != nullptr)
    vcRender_RenderWatermark(pRenderContext, pProgramState->settings.presentation.showEuclideonLogo ? pProgramState->pCompanyWatermark : pProgramState->pSceneWatermark);

  vcGLState_ResetState();
}

void vcRender_TransparentPass(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);

  // Images
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Front);

    for (size_t i = 0; i < renderData.images.length; ++i)
    {
      double zScale = 1.0;
      zScale -= udMag3(pProgramState->camera.position - renderData.images[i]->position) / pProgramState->settings.presentation.imageRescaleDistance;

      if (zScale < 0) // too far
        continue;

      vcImageRenderer_Render(renderData.images[i], pProgramState->camera.matrices.viewProjection, pRenderContext->sceneResolution, zScale);
    }
  }

  // Fences
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
    if (pProgramState->settings.presentation.showDiagnosticInfo)
      vcFenceRenderer_Render(pRenderContext->pDiagnosticFences, pProgramState->camera.matrices.viewProjection, pProgramState->deltaTime);

    for (size_t i = 0; i < renderData.fences.length; ++i)
      vcFenceRenderer_Render(renderData.fences[i], pProgramState->camera.matrices.viewProjection, pProgramState->deltaTime);
  }

  udFloat4 transparentColour = udFloat4::create(1, 1, 1, 0.65f);
  for (size_t i = 0; i < renderData.polyModels.length; ++i)
  {
    vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
    if (!pInstance->HasFlag(vcRenderPolyInstance::RenderFlags_Transparent))
      continue;

    vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

    if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
      vcPolygonModel_Render(pInstance->pModel, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_Standard, pInstance->pDiffuseOverride, &transparentColour);
    else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
      vcSceneLayerRenderer_Render(pInstance->pSceneLayer, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution);
  }

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
}

void vcRender_BeginFrame(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  udUnused(pProgramState);

  // Would be nice to use 'pRenderContext->activeRenderTarget' here, but this causes
  // a single frame 'flicker' if options are changed at run time...just manually set
  renderData.pSceneTexture = pRenderContext->pTexture[1];
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

udFloat4 vcRender_EncodeIdAsColour(uint32_t id)
{
  return udFloat4::create(0.0f, ((id & 0xff) / 255.0f), ((id & 0xff00) >> 8) / 255.0f, 1.0f);// ((id & 0xff0000) >> 16) / 255.0f);// ((id & 0xff000000) >> 24) / 255.0f);
}

bool vcRender_DrawSelectedGeometry(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  bool active = false;

  // check UD first
  uint32_t modelIndex = 0; // index is based on certain models
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
      ++modelIndex;
    }
  }

  udFloat4 selectionMask = udFloat4::create(1.0f); // mask selected object
  for (size_t i = 0; i < renderData.polyModels.length; ++i)
  {
    vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
    if ((pInstance->sceneItemInternalId == 0 && pInstance->pSceneItem->m_selected) || (pInstance->sceneItemInternalId != 0 && pInstance->pSceneItem->IsSubitemSelected(pInstance->sceneItemInternalId)))
    {
      vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

      if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
        vcPolygonModel_Render(pInstance->pModel, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_ColourOnly, nullptr, &selectionMask);
      else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
        vcSceneLayerRenderer_Render(pInstance->pSceneLayer, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution, &selectionMask);

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

      vcShader_BindTexture(pRenderContext->blurShader.pProgram, nullptr, 0, pRenderContext->blurShader.uniform_texture);
    }
  }
  return true;
}

void vcRender_RenderWatermark(vcRenderContext *pRenderContext, vcTexture *pWatermark)
{
  if (pWatermark == nullptr)
    return;

  udInt2 imageSize = udInt2::zero();
  vcTexture_GetSize(pWatermark, &imageSize.x, &imageSize.y);

  udFloat3 position = udFloat3::create(float(imageSize.x) / pRenderContext->sceneResolution.x - 1, float(imageSize.y) / pRenderContext->sceneResolution.y - 1, 0);
#if GRAPHICS_API_OPENGL
  position.y = -position.y;
#endif
  udFloat3 scale = udFloat3::create(2.0f * float(imageSize.x) / pRenderContext->sceneResolution.x, -2.0f * float(imageSize.y) / pRenderContext->sceneResolution.y, 1.0);
  pRenderContext->watermarkShader.params.u_worldViewProjectionMatrix = udFloat4x4::scaleNonUniform(scale, position);

  vcShader_Bind(pRenderContext->watermarkShader.pProgram);
  vcShader_BindConstantBuffer(pRenderContext->watermarkShader.pProgram, pRenderContext->watermarkShader.uniform_params, &pRenderContext->watermarkShader.params, sizeof(pRenderContext->watermarkShader.params));
  vcShader_BindTexture(pRenderContext->watermarkShader.pProgram, pWatermark, 0, pRenderContext->watermarkShader.uniform_texture);
  vcMesh_Render(gInternalMeshes[vcInternalMeshType_ImGuiQuad]);
}

void vcRender_RenderScene(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer)
{
  udUnused(pDefaultFramebuffer);

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

  vcRender_OpaquePass(pProgramState, pRenderContext, renderData); // first pass
  vcRender_VisualizationPass(pProgramState, pRenderContext);

  vcRender_RenderAndApplyViewSheds(pProgramState, pRenderContext, renderData);

  // Drawing skybox after opaque geometry saves a bit on fill rate.
  vcRenderSkybox(pProgramState, pRenderContext);

  vcRender_RenderAtmosphere(pProgramState, pRenderContext);

  vcRender_TransparentPass(pProgramState, pRenderContext, renderData);

  if (selectionBufferActive)
    vcRender_ApplySelectionBuffer(pProgramState, pRenderContext);

  // Draw Compass
  if (pProgramState->settings.presentation.mouseAnchor != vcAS_None && (pProgramState->pickingSuccess || pProgramState->isUsingAnchorPoint))
  {
    // resolve (last frame) polygon vs. (current frame) UD here
    udDouble3 pickPosition = vcRender_DepthToWorldPosition(pProgramState, pRenderContext, pRenderContext->previousFrameDepth);
    if (udMagSq(pickPosition - pProgramState->camera.position) < udMagSq(pProgramState->worldMousePosCartesian - pProgramState->camera.position))
      pProgramState->worldMousePosCartesian = pickPosition;

    udDouble4x4 mvp = pProgramState->camera.matrices.viewProjection * udDouble4x4::translation(pProgramState->isUsingAnchorPoint ? pProgramState->worldAnchorPoint : pProgramState->worldMousePosCartesian);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);

    // Render highlighting any occlusion
    vcGLState_SetBlendMode(vcGLSBM_Additive);
    vcGLState_SetDepthStencilMode(vcGLSDM_Greater, false);
    vcCompass_Render(pRenderContext->pCompass, pProgramState->settings.presentation.mouseAnchor, mvp, udDouble4::create(0.0, 0.15, 1.0, 0.5));

    // Render non-occluded
    vcGLState_SetBlendMode(vcGLSBM_Interpolative);
    vcGLState_SetDepthStencilMode(vcGLSDM_Less, true);
    vcCompass_Render(pRenderContext->pCompass, pProgramState->settings.presentation.mouseAnchor, mvp);

    vcGLState_ResetState();
  }

  vcRender_PostProcessPass(pProgramState, pRenderContext);
  vcRender_RenderUI(pProgramState, pProgramState->pRenderContext, renderData);

  vcGLState_ResetState();
  vcShader_Bind(nullptr);
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
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
  for (int offsety = udMax(0, renderData.mouse.position.y-pProgramState->settings.mouseSnap.range); offsety < udMin(renderData.mouse.position.y + pProgramState->settings.mouseSnap.range, (int)pRenderContext->originalSceneResolution.y-1); offsety++)
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
  pProgramState->pSceneWatermark = nullptr;

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
      else if ((pVisSettings->mode == vcVM_Default || pVisSettings->mode == vcVM_Displacement) && vdkAttributeSet_GetOffsetOfNamedAttribute(&renderData.models[i]->m_pointCloudHeader.attributes, "udDisplacement", &pVoxelShaderData[numVisibleModels].attributeOffset) == vE_Success)
      {
        pModels[numVisibleModels].pVoxelShader = vcVoxelShader_Displacement;

        pVoxelShaderData[numVisibleModels].data.displacement.minThreshold = pVisSettings->displacement.x;
        pVoxelShaderData[numVisibleModels].data.displacement.maxThreshold = pVisSettings->displacement.y;
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

          pProgramState->pSceneWatermark = renderData.models[i]->m_pWatermark;
        }
      }
    }
  }

  if (pProgramState->settings.presentation.showDiagnosticInfo && pProgramState->gis.isProjected)
  {
    vcFenceRenderer_ClearPoints(pRenderContext->pDiagnosticFences);

    float z = 0;
    if (pProgramState->settings.maptiles.mapEnabled)
      z = pProgramState->settings.maptiles.mapHeight;

    udInt2 slippyCurrent;
    udDouble3 localCurrent;

    double xRange = pProgramState->gis.zone.latLongBoundMax.x - pProgramState->gis.zone.latLongBoundMin.x;
    double yRange = pProgramState->gis.zone.latLongBoundMax.y - pProgramState->gis.zone.latLongBoundMin.y;

    if (xRange > 0 || yRange > 0)
    {
      std::vector<udDouble3> corners;

      for (int i = 0; i < yRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(pProgramState->gis.zone.latLongBoundMin.x, pProgramState->gis.zone.latLongBoundMin.y + i, 0), 21);
        vcGIS_SlippyToLocal(&pProgramState->gis, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < xRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(pProgramState->gis.zone.latLongBoundMin.x + i, pProgramState->gis.zone.latLongBoundMax.y, 0), 21);
        vcGIS_SlippyToLocal(&pProgramState->gis, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < yRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(pProgramState->gis.zone.latLongBoundMax.x, pProgramState->gis.zone.latLongBoundMax.y - i, 0), 21);
        vcGIS_SlippyToLocal(&pProgramState->gis, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < xRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(pProgramState->gis.zone.latLongBoundMax.x - i, pProgramState->gis.zone.latLongBoundMin.y, 0), 21);
        vcGIS_SlippyToLocal(&pProgramState->gis, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      corners.push_back(udDouble3::create(corners[0]));

      vcFenceRenderer_AddPoints(pRenderContext->pDiagnosticFences, &corners[0], corners.size());
    }
  }

  vdkRenderPicking picking = {};
  picking.x = (uint32_t)((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x * (float)pRenderContext->sceneResolution.x);
  picking.y = (uint32_t)((float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y * (float)pRenderContext->sceneResolution.y);

  vdkRenderOptions renderOptions;
  memset(&renderOptions, 0, sizeof(vdkRenderOptions));

  if (doPick)
  {
    pProgramState->udModelPickedIndex = -1;
    renderOptions.pPick = &picking;
  }

  renderOptions.pFilter = renderData.pQueryFilter;
  renderOptions.pointMode = (vdkRenderContextPointMode)pProgramState->settings.presentation.pointMode;
  renderOptions.flags = vdkRF_LogarithmicDepth;

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

void vcRender_ClearPoints(vcRenderContext *pRenderContext)
{
  if (pRenderContext == nullptr)
    return;

  vcFenceRenderer_ClearPoints(pRenderContext->pDiagnosticFences);
}

vcRenderPickResult vcRender_PolygonPick(vcState *pProgramState, vcRenderContext *pRenderContext, vcRenderData &renderData, bool doSelectRender)
{
  vcRenderPickResult result = {};

  pRenderContext->currentMouseUV = udFloat2::create((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x, (float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y);
  if (pRenderContext->currentMouseUV.x < 0 || pRenderContext->currentMouseUV.x > 1 || pRenderContext->currentMouseUV.y < 0 || pRenderContext->currentMouseUV.y > 1)
    return result;

  pRenderContext->picking.location.x = (uint32_t)(pRenderContext->currentMouseUV.x * pRenderContext->effectResolution.x);
  pRenderContext->picking.location.y = (uint32_t)(pRenderContext->currentMouseUV.y * pRenderContext->effectResolution.y);

#if GRAPHICS_API_OPENGL
  // note: we render upside-down
  pRenderContext->picking.location.y = (pRenderContext->effectResolution.y - pRenderContext->picking.location.y - 1);
#endif

  float pickDepth = 1.0f;

  if (doSelectRender && (renderData.models.length > 0 || renderData.polyModels.length > 0))
  {
    // render pickable geometry with id encoded in colour
    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);

    vcFramebuffer_Bind(pRenderContext->picking.pFramebuffer, vcFramebufferClearOperation_All);

    vcGLState_SetViewport(0, 0, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y);
    vcGLState_Scissor(pRenderContext->picking.location.x, pRenderContext->picking.location.y, pRenderContext->picking.location.x + 1, pRenderContext->picking.location.y + 1);

    {
      uint32_t modelId = 1; // note: start at 1, because 0 is 'null'

      // Polygon Models
      for (size_t i = 0; i < renderData.polyModels.length; ++i)
      {
        vcRenderPolyInstance *pInstance = &renderData.polyModels[i];
        udFloat4 idAsColour = vcRender_EncodeIdAsColour((uint32_t)(modelId++));

        vcGLState_SetFaceMode(vcGLSFM_Solid, pInstance->cullFace);

        if (pInstance->renderType == vcRenderPolyInstance::RenderType_Polygon)
          vcPolygonModel_Render(pInstance->pModel, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, vcPMP_ColourOnly, nullptr, &idAsColour);
        else if (pInstance->renderType == vcRenderPolyInstance::RenderType_SceneLayer)
          vcSceneLayerRenderer_Render(pInstance->pSceneLayer, pInstance->worldMat, pProgramState->camera.matrices.viewProjection, pProgramState->camera.position, pRenderContext->sceneResolution, &idAsColour);

      }

      vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    }

    udUInt2 readLocation = { pRenderContext->picking.location.x, pRenderContext->picking.location.y };
    uint8_t colourBytes[4] = {};
    uint8_t depthBytes[4] = {};

#if GRAPHICS_API_OPENGL
    // read upside down
    readLocation.y = (pRenderContext->effectResolution.y - readLocation.y - 1);
#endif

    // Synchronously read back data
    vcTexture_BeginReadPixels(pRenderContext->picking.pTexture, readLocation.x, readLocation.y, 1, 1, colourBytes, pRenderContext->picking.pFramebuffer);
    vcTexture_BeginReadPixels(pRenderContext->picking.pDepth, readLocation.x, readLocation.y, 1, 1, depthBytes, pRenderContext->picking.pFramebuffer);

    vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

    // 24 bit unsigned int -> float
#if GRAPHICS_API_OPENGL || GRAPHICS_API_METAL
    pickDepth = uint32_t((depthBytes[3] << 16) | (depthBytes[2] << 8) | (depthBytes[1] << 0)) / ((1 << 24) - 1.0f);
    //uint8_t stencil = depthBytes[0];
#else
    pickDepth = uint32_t((depthBytes[2] << 16) | (depthBytes[1] << 8) | (depthBytes[0] << 0)) / ((1 << 24) - 1.0f);
    //uint8_t stencil = depthBytes[3];
#endif

    // note `-1`, and BGRA format
    int pickedPolygonId = (int)((colourBytes[1] << 0) | (colourBytes[0] << 8)) - 1;
    if (pickedPolygonId != -1)
    {
      result.success = true;
      result.pPolygon = &renderData.polyModels[pickedPolygonId];
    }
  }
  else
  {
    result.success = true;
    pickDepth = pRenderContext->previousFrameDepth;
  }

  if (result.success)
    result.position = vcRender_DepthToWorldPosition(pProgramState, pRenderContext, pickDepth);

  return result;
}
