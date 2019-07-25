#include "vcRender.h"

#include "gl/vcRenderShaders.h"
#include "gl/vcFramebuffer.h"
#include "gl/vcShader.h"
#include "gl/vcGLState.h"
#include "vcFenceRenderer.h"
#include "vcWaterRenderer.h"
#include "vcTileRenderer.h"
#include "vcCompass.h"

#include "vcInternalModels.h"
#include "vcSceneLayerRenderer.h"
#include "vcGPURenderer.h"

#include "stb_image.h"
#include <vector>

enum
{
  vcRender_SceneSizeIncrement = 32, // directX framebuffer can only be certain increments

  // certain effects don't need to be at 100% resolution (e.g. outline). 0 is highest quality
  vcRender_OutlineEffectDownscale = 1
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

#if ALLOW_EXPERIMENT_GPURENDER
  vcGPURenderer *pGPURenderer;
  bool usingGPURenderer;
#endif

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_depth;
    vcShaderConstantBuffer *uniform_params;

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
      udFloat4 contourParams; // contour distance, contour band height, (unused), (unused)
    } params;

  } presentShader;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_params;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_depth;

    struct
    {
      udFloat4 id;
    } params;

  } splatIdShader;
};

struct vcRenderContext
{
  vdkContext *pVaultContext;
  vcSettings *pSettings;
  vcCamera *pCamera;

  udUInt2 sceneResolution;
  udUInt2 originalSceneResolution;

  vcFramebuffer *pFramebuffer;
  vcTexture *pTexture;
  vcTexture *pDepthTexture;

  vcFramebuffer *pAuxiliaryFramebuffers[2];
  vcTexture *pAuxiliaryTextures[2];
  udUInt2 effectResolution;

  vcUDRenderContext udRenderContext;
  vcFenceRenderer *pDiagnosticFences;

  vcTileRenderer *pTileRenderer;
  vcAnchor *pCompass;

  struct
  {
    vcShader *pProgram;
    vcShaderSampler *uniform_texture;
    vcShaderConstantBuffer *uniform_MatrixBlock;
  } skyboxShader;

  vcMesh *pScreenQuadMesh;
  vcMesh *pFlippedScreenQuadMesh;
  vcTexture *pSkyboxTexture;

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
};

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext);
udResult vcRender_RenderUD(vcRenderContext *pRenderContext, vcRenderData &renderData);

udResult vcRender_Init(vcRenderContext **ppRenderContext, udWorkerPool *pWorkerPool, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &sceneResolution)
{
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  const int maxPointCount = 5 * 1000000; // TODO: calculate this from GPU information

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

#if ALLOW_EXPERIMENT_GPURENDER
  UD_ERROR_CHECK(vcGPURenderer_Create(&pRenderContext->udRenderContext.pGPURenderer, vcBRPRM_GeometryShader, maxPointCount));
#else
  udUnused(maxPointCount);
#endif

  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->udRenderContext.presentShader.pProgram, g_udVertexShader, g_udFragmentShader, vcSimpleVertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->skyboxShader.pProgram, g_vcSkyboxVertexShader, g_vcSkyboxFragmentShader, vcSimpleVertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->udRenderContext.splatIdShader.pProgram, g_udVertexShader, g_udSplatIdFragmentShader, vcSimpleVertexLayout), udR_InternalError);

  vcMesh_Create(&pRenderContext->pScreenQuadMesh, vcSimpleVertexLayout, int(udLengthOf(vcSimpleVertexLayout)), screenQuadVertices, 4, screenQuadIndices, 6, vcMF_Dynamic);
  vcMesh_Create(&pRenderContext->pFlippedScreenQuadMesh, vcSimpleVertexLayout, int(udLengthOf(vcSimpleVertexLayout)), flippedScreenQuadVertices, 4, flippedScreenQuadIndices, 6, vcMF_Dynamic);

  vcTexture_AsyncCreateFromFilename(&pRenderContext->pSkyboxTexture, pWorkerPool, "asset://assets/skyboxes/WaterClouds.jpg", vcTFM_Linear);
  UD_ERROR_CHECK(vcCompass_Create(&pRenderContext->pCompass));

  vcShader_Bind(pRenderContext->skyboxShader.pProgram);
  vcShader_GetSamplerIndex(&pRenderContext->skyboxShader.uniform_texture, pRenderContext->skyboxShader.pProgram, "u_texture");
  vcShader_GetConstantBuffer(&pRenderContext->skyboxShader.uniform_MatrixBlock, pRenderContext->skyboxShader.pProgram, "u_EveryFrame", sizeof(udFloat4x4));

  vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);
  vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_texture, pRenderContext->udRenderContext.presentShader.pProgram, "u_texture");
  vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.presentShader.uniform_depth, pRenderContext->udRenderContext.presentShader.pProgram, "u_depth");
  vcShader_GetConstantBuffer(&pRenderContext->udRenderContext.presentShader.uniform_params, pRenderContext->udRenderContext.presentShader.pProgram, "u_params", sizeof(pRenderContext->udRenderContext.presentShader.params));

  vcShader_Bind(pRenderContext->udRenderContext.splatIdShader.pProgram);
  vcShader_GetConstantBuffer(&pRenderContext->udRenderContext.splatIdShader.uniform_params, pRenderContext->udRenderContext.splatIdShader.pProgram, "u_params", sizeof(pRenderContext->udRenderContext.splatIdShader.params));
  vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.splatIdShader.uniform_depth, pRenderContext->udRenderContext.splatIdShader.pProgram, "u_depth");
  vcShader_GetSamplerIndex(&pRenderContext->udRenderContext.splatIdShader.uniform_texture, pRenderContext->udRenderContext.splatIdShader.pProgram, "u_texture");

  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->blurShader.pProgram, g_BlurVertexShader, g_BlurFragmentShader, vcSimpleVertexLayout), udR_InternalError);
  vcShader_Bind(pRenderContext->blurShader.pProgram);
  vcShader_GetSamplerIndex(&pRenderContext->blurShader.uniform_texture, pRenderContext->blurShader.pProgram, "u_texture");
  vcShader_GetConstantBuffer(&pRenderContext->blurShader.uniform_params, pRenderContext->blurShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->blurShader.params));

  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->selectionShader.pProgram, g_HighlightVertexShader, g_HighlightFragmentShader, vcSimpleVertexLayout), udR_InternalError);
  vcShader_Bind(pRenderContext->selectionShader.pProgram);
  vcShader_GetSamplerIndex(&pRenderContext->selectionShader.uniform_texture, pRenderContext->selectionShader.pProgram, "u_texture");
  vcShader_GetConstantBuffer(&pRenderContext->selectionShader.uniform_params, pRenderContext->selectionShader.pProgram, "u_EveryFrame", sizeof(pRenderContext->selectionShader.params));

  vcPolygonModel_CreateShaders();
  vcImageRenderer_Init();

  vcShader_Bind(nullptr);

  UD_ERROR_CHECK(vcTileRenderer_Create(&pRenderContext->pTileRenderer, pSettings));
  UD_ERROR_CHECK(vcFenceRenderer_Create(&pRenderContext->pDiagnosticFences));

  *ppRenderContext = pRenderContext;

  pRenderContext->pSettings = pSettings;
  pRenderContext->pCamera = pCamera;
  result = vcRender_ResizeScene(pRenderContext, sceneResolution.x, sceneResolution.y);
  if (result != udR_Success)
    goto epilogue;

epilogue:
  return result;
}

udResult vcRender_Destroy(vcRenderContext **ppRenderContext)
{
  if (ppRenderContext == nullptr || *ppRenderContext == nullptr)
    return udR_Success;

  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = *ppRenderContext;
  *ppRenderContext = nullptr;

  if (pRenderContext->pVaultContext != nullptr)
  {
    if (pRenderContext->udRenderContext.pRenderView != nullptr && vdkRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
      UD_ERROR_SET(udR_InternalError);

    if (vdkRenderContext_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
      UD_ERROR_SET(udR_InternalError);
  }

#if ALLOW_EXPERIMENT_GPURENDER
  UD_ERROR_CHECK(vcGPURenderer_Destroy(&pRenderContext->udRenderContext.pGPURenderer));
#endif

  vcShader_DestroyShader(&pRenderContext->udRenderContext.presentShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->skyboxShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->blurShader.pProgram);
  vcShader_DestroyShader(&pRenderContext->selectionShader.pProgram);

  vcMesh_Destroy(&pRenderContext->pScreenQuadMesh);

  vcTexture_Destroy(&pRenderContext->pSkyboxTexture);
  UD_ERROR_CHECK(vcCompass_Destroy(&pRenderContext->pCompass));

  vcPolygonModel_DestroyShaders();
  vcImageRenderer_Destroy();

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

  UD_ERROR_CHECK(vcTileRenderer_Destroy(&pRenderContext->pTileRenderer));
  UD_ERROR_CHECK(vcFenceRenderer_Destroy(&pRenderContext->pDiagnosticFences));

epilogue:
  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcFramebuffer_Destroy(&pRenderContext->udRenderContext.pFramebuffer);
  vcTexture_Destroy(&pRenderContext->pTexture);
  vcTexture_Destroy(&pRenderContext->pDepthTexture);
  vcFramebuffer_Destroy(&pRenderContext->pFramebuffer);

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

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vdkContext *pVaultContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  pRenderContext->pVaultContext = pVaultContext;

  if (vdkRenderContext_Create(pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height)
{
  udResult result = udR_Success;

  uint32_t widthIncr = width +(width % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - width % vcRender_SceneSizeIncrement : 0);
  uint32_t heightIncr = height +(height % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - height % vcRender_SceneSizeIncrement : 0);

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

  pRenderContext->udRenderContext.pColorBuffer = udAllocType(uint32_t, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);
  pRenderContext->udRenderContext.pDepthBuffer = udAllocType(float, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);

  //Resize GPU Targets
  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcFramebuffer_Destroy(&pRenderContext->udRenderContext.pFramebuffer);

#if ALLOW_EXPERIMENT_GPURENDER
  if (pRenderContext->udRenderContext.usingGPURenderer)
  {
    vcTexture_Create(&pRenderContext->udRenderContext.pColourTex, widthIncr, heightIncr, nullptr, vcTextureFormat_BGRA8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
    vcTexture_Create(&pRenderContext->udRenderContext.pDepthTex, widthIncr, heightIncr, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
    vcFramebuffer_Create(&pRenderContext->udRenderContext.pFramebuffer, pRenderContext->udRenderContext.pColourTex, pRenderContext->udRenderContext.pDepthTex);
  }
  else
#endif //ALLOW_EXPERIMENT_GPURENDER
  {
    vcTexture_Create(&pRenderContext->udRenderContext.pColourTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pColorBuffer, vcTextureFormat_BGRA8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_Dynamic);
    vcTexture_Create(&pRenderContext->udRenderContext.pDepthTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pDepthBuffer, vcTextureFormat_D32F, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_Dynamic);
  }

  vcTexture_Destroy(&pRenderContext->pTexture);
  vcTexture_Destroy(&pRenderContext->pDepthTexture);
  vcFramebuffer_Destroy(&pRenderContext->pFramebuffer);

  vcTexture_Destroy(&pRenderContext->picking.pTexture);
  vcTexture_Destroy(&pRenderContext->picking.pDepth);
  vcFramebuffer_Destroy(&pRenderContext->picking.pFramebuffer);

  for (int i = 0; i < 2; ++i)
  {
    vcTexture_Destroy(&pRenderContext->pAuxiliaryTextures[i]);
    vcFramebuffer_Destroy(&pRenderContext->pAuxiliaryFramebuffers[i]);
  }

  vcTexture_Create(&pRenderContext->pTexture, widthIncr, heightIncr, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  vcTexture_Create(&pRenderContext->pDepthTexture, widthIncr, heightIncr, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  vcFramebuffer_Create(&pRenderContext->pFramebuffer, pRenderContext->pTexture, pRenderContext->pDepthTexture);

  pRenderContext->effectResolution.x = widthIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.y = heightIncr >> vcRender_OutlineEffectDownscale;
  pRenderContext->effectResolution.x = pRenderContext->effectResolution.x + (pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.x % vcRender_SceneSizeIncrement : 0);
  pRenderContext->effectResolution.y = pRenderContext->effectResolution.y + (pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement != 0 ? vcRender_SceneSizeIncrement - pRenderContext->effectResolution.y % vcRender_SceneSizeIncrement : 0);

  for (int i = 0; i < 2; ++i)
  {
    vcTexture_Create(&pRenderContext->pAuxiliaryTextures[i], pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Clamp, vcTCF_RenderTarget);
    vcFramebuffer_Create(&pRenderContext->pAuxiliaryFramebuffers[i], pRenderContext->pAuxiliaryTextures[i]);
  }

  vcTexture_Create(&pRenderContext->picking.pTexture, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_BGRA8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  vcTexture_Create(&pRenderContext->picking.pDepth, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  vcFramebuffer_Create(&pRenderContext->picking.pFramebuffer, pRenderContext->picking.pTexture, pRenderContext->picking.pDepth);

  if (pRenderContext->pVaultContext)
    vcRender_RecreateUDView(pRenderContext);

epilogue:
  return result;
}

void vcRenderSkybox(vcRenderContext *pRenderContext)
{
  // Draw the skybox only at the far plane, where there is no geometry.
  // Drawing skybox here (after 'opaque' geometry) saves a bit on fill rate.

  udFloat4x4 viewMatrixF = udFloat4x4::create(pRenderContext->pCamera->matrices.view);
  udFloat4x4 projectionMatrixF = udFloat4x4::create(pRenderContext->pCamera->matrices.projectionNear);
  udFloat4x4 inverseViewProjMatrixF = projectionMatrixF * viewMatrixF;
  inverseViewProjMatrixF.axis.t = udFloat4::create(0, 0, 0, 1);
  inverseViewProjMatrixF.inverse();

  vcGLState_SetViewportDepthRange(1.0f, 1.0f);

  vcShader_Bind(pRenderContext->skyboxShader.pProgram);

  vcShader_BindTexture(pRenderContext->skyboxShader.pProgram, pRenderContext->pSkyboxTexture, 0, pRenderContext->skyboxShader.uniform_texture);
  vcShader_BindConstantBuffer(pRenderContext->skyboxShader.pProgram, pRenderContext->skyboxShader.uniform_MatrixBlock, &inverseViewProjMatrixF, sizeof(inverseViewProjMatrixF));

  vcMesh_Render(pRenderContext->pScreenQuadMesh, 2);

  vcGLState_SetViewportDepthRange(0.0f, 1.0f);

  vcShader_Bind(nullptr);
}

void vcRender_SplatUDWithId(vcRenderContext *pRenderContext, float id)
{
  vcShader_Bind(pRenderContext->udRenderContext.splatIdShader.pProgram);

  vcShader_BindTexture(pRenderContext->udRenderContext.splatIdShader.pProgram, pRenderContext->udRenderContext.pColourTex, 0, pRenderContext->udRenderContext.splatIdShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->udRenderContext.splatIdShader.pProgram, pRenderContext->udRenderContext.pDepthTex, 1, pRenderContext->udRenderContext.splatIdShader.uniform_depth);

  pRenderContext->udRenderContext.splatIdShader.params.id = udFloat4::create(0.0f, 0.0f, 0.0f, id);
  vcShader_BindConstantBuffer(pRenderContext->udRenderContext.splatIdShader.pProgram, pRenderContext->udRenderContext.splatIdShader.uniform_params, &pRenderContext->udRenderContext.splatIdShader.params, sizeof(pRenderContext->udRenderContext.splatIdShader.params));

#if ALLOW_EXPERIMENT_GPURENDER && GRAPHICS_API_OPENGL
  if (pRenderContext->pSettings->experimental.useGPURenderer)
    vcMesh_Render(pRenderContext->pFlippedScreenQuadMesh, 2);
  else
#endif
    vcMesh_Render(pRenderContext->pScreenQuadMesh, 2);
}

void vcRender_PresentUD(vcRenderContext *pRenderContext)
{
  float nearPlane = pRenderContext->pSettings->camera.nearPlane;
  float farPlane = pRenderContext->pSettings->camera.farPlane;

  // edge outlines
  int outlineWidth = pRenderContext->pSettings->postVisualization.edgeOutlines.width;
  float outlineEdgeThreshold = pRenderContext->pSettings->postVisualization.edgeOutlines.threshold;
  udFloat4 outlineColour = pRenderContext->pSettings->postVisualization.edgeOutlines.colour;
  if (!pRenderContext->pSettings->postVisualization.edgeOutlines.enable)
    outlineColour.w = 0.0f;

  if (pRenderContext->pSettings->camera.cameraMode == vcCM_OrthoMap)
  {
    // adjust some visuals in map mode
    nearPlane = float(vcSL_CameraOrthoNearFarPlane.x);
    farPlane = float(vcSL_CameraOrthoNearFarPlane.y);
    outlineEdgeThreshold /= float(vcSL_CameraOrthoNearFarPlane.y * 0.15);
  }

  // colour by height
  udFloat4 colourByHeightMinColour = pRenderContext->pSettings->postVisualization.colourByHeight.minColour;
  if (!pRenderContext->pSettings->postVisualization.colourByHeight.enable)
    colourByHeightMinColour.w = 0.f;
  udFloat4 colourByHeightMaxColour = pRenderContext->pSettings->postVisualization.colourByHeight.maxColour;
  if (!pRenderContext->pSettings->postVisualization.colourByHeight.enable)
    colourByHeightMaxColour.w = 0.f;
  float colourByHeightStartHeight = pRenderContext->pSettings->postVisualization.colourByHeight.startHeight;
  float colourByHeightEndHeight = pRenderContext->pSettings->postVisualization.colourByHeight.endHeight;

  // colour by depth
  udFloat4 colourByDepthColour = pRenderContext->pSettings->postVisualization.colourByDepth.colour;
  if (!pRenderContext->pSettings->postVisualization.colourByDepth.enable)
    colourByDepthColour.w = 0.f;
  float colourByDepthStart = pRenderContext->pSettings->postVisualization.colourByDepth.startDepth;
  float colourByDepthEnd = pRenderContext->pSettings->postVisualization.colourByDepth.endDepth;

  // contours
  udFloat4 contourColour = pRenderContext->pSettings->postVisualization.contours.colour;
  if (!pRenderContext->pSettings->postVisualization.contours.enable)
    contourColour.w = 0.f;
  float contourDistances = pRenderContext->pSettings->postVisualization.contours.distances;
  float contourBandHeight = pRenderContext->pSettings->postVisualization.contours.bandHeight;

  pRenderContext->udRenderContext.presentShader.params.inverseViewProjection = udFloat4x4::create(pRenderContext->pCamera->matrices.inverseViewProjection);
  pRenderContext->udRenderContext.presentShader.params.screenParams.x = (1.0f / pRenderContext->sceneResolution.x);
  pRenderContext->udRenderContext.presentShader.params.screenParams.y = (1.0f / pRenderContext->sceneResolution.y);
  pRenderContext->udRenderContext.presentShader.params.screenParams.z = nearPlane;
  pRenderContext->udRenderContext.presentShader.params.screenParams.w = farPlane;
  pRenderContext->udRenderContext.presentShader.params.outlineColour = outlineColour;
  pRenderContext->udRenderContext.presentShader.params.outlineParams.x = (float)outlineWidth;
  pRenderContext->udRenderContext.presentShader.params.outlineParams.y = outlineEdgeThreshold;
  pRenderContext->udRenderContext.presentShader.params.colourizeHeightColourMin = colourByHeightMinColour;
  pRenderContext->udRenderContext.presentShader.params.colourizeHeightColourMax = colourByHeightMaxColour;
  pRenderContext->udRenderContext.presentShader.params.colourizeHeightParams.x = colourByHeightStartHeight;
  pRenderContext->udRenderContext.presentShader.params.colourizeHeightParams.y = colourByHeightEndHeight;
  pRenderContext->udRenderContext.presentShader.params.colourizeDepthColour = colourByDepthColour;
  pRenderContext->udRenderContext.presentShader.params.colourizeDepthParams.x = colourByDepthStart;
  pRenderContext->udRenderContext.presentShader.params.colourizeDepthParams.y = colourByDepthEnd;
  pRenderContext->udRenderContext.presentShader.params.contourColour = contourColour;
  pRenderContext->udRenderContext.presentShader.params.contourParams.x = contourDistances;
  pRenderContext->udRenderContext.presentShader.params.contourParams.y = contourBandHeight;

  vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);

  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->udRenderContext.pColourTex, 0, pRenderContext->udRenderContext.presentShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->udRenderContext.pDepthTex, 1, pRenderContext->udRenderContext.presentShader.uniform_depth);
  vcShader_BindConstantBuffer(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->udRenderContext.presentShader.uniform_params, &pRenderContext->udRenderContext.presentShader.params, sizeof(pRenderContext->udRenderContext.presentShader.params));

#if ALLOW_EXPERIMENT_GPURENDER && GRAPHICS_API_OPENGL
  if (pRenderContext->pSettings->experimental.useGPURenderer)
    vcMesh_Render(pRenderContext->pFlippedScreenQuadMesh, 2);
  else
#endif
    vcMesh_Render(pRenderContext->pScreenQuadMesh, 2);
}

void vcRenderTerrain(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (renderData.pGISSpace->isProjected && pRenderContext->pSettings->maptiles.mapEnabled)
  {
    udDouble4x4 cameraMatrix = pRenderContext->pCamera->matrices.camera;
    udDouble4x4 viewProjection = pRenderContext->pCamera->matrices.viewProjection;

#ifndef GIT_BUILD
    static bool debugDetachCamera = false;
    static udDouble4x4 gRealCameraMatrix = udDouble4x4::identity();
    if (!debugDetachCamera)
      gRealCameraMatrix = pRenderContext->pCamera->matrices.camera;

    cameraMatrix = gRealCameraMatrix;
    viewProjection = pRenderContext->pCamera->matrices.projection * udInverse(cameraMatrix);
#endif
    udDouble3 localCamPos = cameraMatrix.axis.t.toVector3();

    // Corners [nw, ne, sw, se]
    udDouble3 localCorners[4];
    udInt2 slippyCorners[4];

    int currentZoom = 21;

    double farPlane = pRenderContext->pSettings->camera.farPlane;
    if (pRenderContext->pSettings->camera.cameraMode == vcCM_OrthoMap)
      farPlane = udMax(farPlane, pRenderContext->pSettings->camera.orthographicSize * 2.0);

    // Cardinal Limits
    localCorners[0] = localCamPos + udDouble3::create(-farPlane, +farPlane, 0);
    localCorners[1] = localCamPos + udDouble3::create(+farPlane, +farPlane, 0);
    localCorners[2] = localCamPos + udDouble3::create(-farPlane, -farPlane, 0);
    localCorners[3] = localCamPos + udDouble3::create(+farPlane, -farPlane, 0);

    for (int i = 0; i < 4; ++i)
      vcGIS_LocalToSlippy(renderData.pGISSpace, &slippyCorners[i], localCorners[i], currentZoom);

    while (currentZoom > 0 && (slippyCorners[0] != slippyCorners[1] || slippyCorners[1] != slippyCorners[2] || slippyCorners[2] != slippyCorners[3]))
    {
      --currentZoom;

      for (int i = 0; i < 4; ++i)
        slippyCorners[i] /= 2;
    }

    for (int i = 0; i < 4; ++i)
      vcGIS_SlippyToLocal(renderData.pGISSpace, &localCorners[i], slippyCorners[0] + udInt2::create(i & 1, i / 2), currentZoom);

    vcTileRenderer_Update(pRenderContext->pTileRenderer, renderData.deltaTime, renderData.pGISSpace, localCorners, udInt3::create(slippyCorners[0], currentZoom), localCamPos, viewProjection);
    vcTileRenderer_Render(pRenderContext->pTileRenderer, pRenderContext->pCamera->matrices.view, pRenderContext->pCamera->matrices.projection);
  }
}

void vcRenderOpaqueGeometry(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_ResetState();

  // Polygon Models
  {
    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);

    for (size_t i = 0; i < renderData.polyModels.length; ++i)
      vcPolygonModel_Render(renderData.polyModels[i].pModel, renderData.polyModels[i].worldMat, pRenderContext->pCamera->matrices.viewProjection);

    // TODO: (EVC-553) This is temporary
    gpuBytesUploadedThisFrame = 0;
    for (size_t i = 0; i < renderData.sceneLayers.length; ++i)
      vcSceneLayerRenderer_Render(renderData.sceneLayers[i], pRenderContext->pCamera->matrices.viewProjection, pRenderContext->sceneResolution);

    for (size_t i = 0; i < renderData.waterVolumes.length; ++i)
      vcWaterRenderer_Render(renderData.waterVolumes[i], pRenderContext->pCamera->matrices.view, pRenderContext->pCamera->matrices.viewProjection, pRenderContext->pSkyboxTexture, renderData.deltaTime);
  }
}

void vcRenderTransparentGeometry(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);

  // Images
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Front);

    for (size_t i = 0; i < renderData.images.length; ++i)
    {
      static const double distScalar = 600.0;

      double zScale = 1.0;
      if (pRenderContext->pSettings->camera.cameraMode == vcCM_FreeRoam)
        zScale -= udMag3(pRenderContext->pCamera->position - renderData.images[i]->position) / distScalar;
      else // map mode
        zScale -= pRenderContext->pSettings->camera.orthographicSize / distScalar;

      if (zScale < 0) // too far
        continue;

      vcImageRenderer_Render(renderData.images[i], pRenderContext->pCamera->matrices.viewProjection, pRenderContext->sceneResolution, zScale);
    }
  }

  // Fences
  {
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
    if (pRenderContext->pSettings->presentation.showDiagnosticInfo)
      vcFenceRenderer_Render(pRenderContext->pDiagnosticFences, pRenderContext->pCamera->matrices.viewProjection, renderData.deltaTime);

    for (size_t i = 0; i < renderData.fences.length; ++i)
      vcFenceRenderer_Render(renderData.fences[i], pRenderContext->pCamera->matrices.viewProjection, renderData.deltaTime);
  }
}

void vcRender_BeginFrame(vcRenderContext *pRenderContext)
{
#if ALLOW_EXPERIMENT_GPURENDER
  if (pRenderContext->pSettings->experimental.useGPURenderer != pRenderContext->udRenderContext.usingGPURenderer)
  {
    pRenderContext->udRenderContext.usingGPURenderer = pRenderContext->pSettings->experimental.useGPURenderer;
    vcRender_ResizeScene(pRenderContext, pRenderContext->originalSceneResolution.x, pRenderContext->originalSceneResolution.y);
    printf("Recreating context\n");
  }
#else
  udUnused(pRenderContext);
#endif
}

vcTexture* vcRender_GetSceneTexture(vcRenderContext *pRenderContext)
{
  return pRenderContext->pTexture;
}

void vcRender_ApplySelectionBuffer(vcRenderContext *pRenderContext)
{
  udFloat2 sampleStepSize = udFloat2::create(1.0f / pRenderContext->effectResolution.x, 1.0f / pRenderContext->effectResolution.y);

  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  pRenderContext->selectionShader.params.stepSizeThickness.x = sampleStepSize.x;
  pRenderContext->selectionShader.params.stepSizeThickness.y = sampleStepSize.y;
  pRenderContext->selectionShader.params.stepSizeThickness.z = (float)(2 << vcRender_OutlineEffectDownscale) * (pRenderContext->pSettings->objectHighlighting.thickness - 1.0f); // roughly
  pRenderContext->selectionShader.params.stepSizeThickness.w = 0.2f;
  pRenderContext->selectionShader.params.colour = pRenderContext->pSettings->objectHighlighting.colour;

  vcShader_Bind(pRenderContext->selectionShader.pProgram);

  vcShader_BindTexture(pRenderContext->selectionShader.pProgram, pRenderContext->pAuxiliaryTextures[0], 0, pRenderContext->selectionShader.uniform_texture);
  vcShader_BindConstantBuffer(pRenderContext->selectionShader.pProgram, pRenderContext->selectionShader.uniform_params, &pRenderContext->selectionShader.params, sizeof(pRenderContext->selectionShader.params));

  vcMesh_Render(pRenderContext->pScreenQuadMesh, 2);
}

udFloat4 vcRender_EncodeIdAsColour(uint32_t id)
{
  return udFloat4::create(0.0f, ((id & 0xff) / 255.0f), ((id & 0xff00) >> 8) / 255.0f, 1.0f);// ((id & 0xff0000) >> 16) / 255.0f);// ((id & 0xff000000) >> 24) / 255.0f);
}

bool vcRender_DrawSelectedGeometry(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  // check UD first
  uint32_t modelIndex = 0; // index is based on certain models
  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->m_visible && renderData.models[i]->m_loadStatus == vcSLS_Loaded)
    {
      if (renderData.models[i]->IsSceneSelected(0))
      {
        vcRender_SplatUDWithId(pRenderContext, (modelIndex + 1) / 255.0f);
        return true; // assuming only a single selection
      }
      ++modelIndex;
    }
  }

  udFloat4 selectionMask = udFloat4::create(1.0f); // mask selected object
  for (size_t i = 0; i < renderData.polyModels.length; ++i)
  {
    vcRenderPolyInstance *pPolygon = &renderData.polyModels[i];

    if (pPolygon->pSceneItem->IsSceneSelected(pPolygon->sceneItemInternalId))
    {
      vcPolygonModel_Render(pPolygon->pModel, pPolygon->worldMat, pRenderContext->pCamera->matrices.viewProjection, vcPMP_ColourOnly, nullptr, &selectionMask);
      return true; // assuming only a single selection
    }
  }

  for (size_t i = 0; i < renderData.sceneLayers.length; ++i)
  {
    //vcSceneLayerRenderer *pSceneLayerRenderer = renderData.sceneLayers[i];
    //if (pSceneLayerRenderer->)
    //vcSceneLayerRenderer_Render(renderData.sceneLayers[i], pRenderContext->pCamera->matrices.viewProjection, pRenderContext->sceneResolution);
  }

  return false;
}

bool vcRender_CreateSelectionBuffer(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (pRenderContext->pSettings->objectHighlighting.colour.w == 0.0f) // disabled
    return false;

  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetViewport(0, 0, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y);

  vcFramebuffer_Bind(pRenderContext->pAuxiliaryFramebuffers[0]);
  vcFramebuffer_Clear(pRenderContext->pAuxiliaryFramebuffers[0], 0x00000000);

  if (!vcRender_DrawSelectedGeometry(pRenderContext, renderData))
    return false;

  // blur disabled at thickness value of 1.0
  if (pRenderContext->pSettings->objectHighlighting.thickness > 1.0f)
  {
    udFloat2 sampleStepSize = udFloat2::create(1.0f / pRenderContext->effectResolution.x, 1.0f / pRenderContext->effectResolution.y);

    vcGLState_SetBlendMode(vcGLSBM_None);
    for (int i = 0; i < 2; ++i)
    {
      vcFramebuffer_Bind(pRenderContext->pAuxiliaryFramebuffers[1 - i]);
      vcFramebuffer_Clear(pRenderContext->pAuxiliaryFramebuffers[1 - i], 0x00000000);

      pRenderContext->blurShader.params.stepSize.x = i == 0 ? sampleStepSize.x : 0.0f;
      pRenderContext->blurShader.params.stepSize.y = i == 0 ? 0.0f : sampleStepSize.y;

      vcShader_Bind(pRenderContext->blurShader.pProgram);

      vcShader_BindTexture(pRenderContext->blurShader.pProgram, pRenderContext->pAuxiliaryTextures[i], 0, pRenderContext->blurShader.uniform_texture);
      vcShader_BindConstantBuffer(pRenderContext->blurShader.pProgram, pRenderContext->blurShader.uniform_params, &pRenderContext->blurShader.params, sizeof(pRenderContext->blurShader.params));

      vcMesh_Render(pRenderContext->pScreenQuadMesh, 2);

      vcShader_BindTexture(pRenderContext->blurShader.pProgram, nullptr, 0, pRenderContext->blurShader.uniform_texture);
    }
  }
  return true;
}

void vcRender_RenderScene(vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer)
{
  udUnused(pDefaultFramebuffer);

  float aspect = pRenderContext->sceneResolution.x / (float)pRenderContext->sceneResolution.y;

  vcRender_RenderUD(pRenderContext, renderData);

  bool selectionBufferActive = vcRender_CreateSelectionBuffer(pRenderContext, renderData);

  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  vcFramebuffer_Bind(pRenderContext->pFramebuffer);
  vcFramebuffer_Clear(pRenderContext->pFramebuffer, 0x00FF8080);

  vcRender_PresentUD(pRenderContext);

  //vcGLState_SetBlendMode(vcGLSBM_None);
  vcRenderOpaqueGeometry(pRenderContext, renderData);
  vcRenderSkybox(pRenderContext);
  vcRenderTerrain(pRenderContext, renderData);
  vcRenderTransparentGeometry(pRenderContext, renderData);

  if (selectionBufferActive)
    vcRender_ApplySelectionBuffer(pRenderContext);

  if (pRenderContext->pSettings->presentation.mouseAnchor != vcAS_None && (renderData.pickingSuccess || (renderData.pWorldAnchorPos != nullptr)))
  {
    udDouble4x4 mvp = pRenderContext->pCamera->matrices.viewProjection * udDouble4x4::translation(renderData.pWorldAnchorPos ? *renderData.pWorldAnchorPos : renderData.worldMousePos);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);

    // Render highlighting any occlusion
    vcGLState_SetBlendMode(vcGLSBM_Additive);
    vcGLState_SetDepthStencilMode(vcGLSDM_Greater, false);
    vcCompass_Render(pRenderContext->pCompass, pRenderContext->pSettings->presentation.mouseAnchor, mvp, udDouble4::create(0.0, 0.15, 1.0, 0.5));

    // Render non-occluded
    vcGLState_SetBlendMode(vcGLSBM_Interpolative);
    vcGLState_SetDepthStencilMode(vcGLSDM_Less, true);
    vcCompass_Render(pRenderContext->pCompass, pRenderContext->pSettings->presentation.mouseAnchor, mvp);

    vcGLState_ResetState();
  }

  if (pRenderContext->pSettings->presentation.showCompass)
  {
    udDouble4x4 cameraRotation = udDouble4x4::rotationYPR(pRenderContext->pCamera->matrices.camera.extractYPR());
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
    vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

    if (!renderData.pGISSpace->isProjected)
    {
      vcCompass_Render(pRenderContext->pCompass, vcAS_Compass, udDouble4x4::perspectiveZO(vcLens30mm, aspect, 0.01, 2.0) * udDouble4x4::translation(vcLens30mm * 0.45 * aspect, 1.0, -vcLens30mm * 0.45) * udDouble4x4::scaleUniform(vcLens30mm / 20.0) * udInverse(cameraRotation));
    }
    else
    {
      udDouble3 currentLatLong = udGeoZone_ToLatLong(renderData.pGISSpace->zone, pRenderContext->pCamera->position);
      currentLatLong.x = udClamp(currentLatLong.x, -90.0, 89.0);
      udDouble3 norther = udGeoZone_ToCartesian(renderData.pGISSpace->zone, udDouble3::create(currentLatLong.x + 1.0, currentLatLong.y, currentLatLong.z));
      udDouble4x4 north = udDouble4x4::lookAt(pRenderContext->pCamera->position, norther);
      vcCompass_Render(pRenderContext->pCompass, vcAS_Compass, udDouble4x4::perspectiveZO(vcLens30mm, aspect, 0.01, 2.0) * udDouble4x4::translation(vcLens30mm * 0.45 * aspect, 1.0, -vcLens30mm * 0.45) * udDouble4x4::scaleUniform(vcLens30mm / 20.0) * udDouble4x4::rotationYPR(north.extractYPR()) * udInverse(cameraRotation));
    }

    vcGLState_ResetState();
  }

  vcShader_Bind(nullptr);
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
}

void vcRender_vcRenderSceneImGui(vcRenderContext *pRenderContext, const vcRenderData &renderData)
{
  // Labels
  for (size_t i = 0; i < renderData.labels.length; ++i)
    vcLabelRenderer_Render(renderData.labels[i], pRenderContext->pCamera->matrices.viewProjection, pRenderContext->sceneResolution);
}

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (pRenderContext->udRenderContext.pRenderView && vdkRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Create(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pRenderer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetTargets(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pColorBuffer, 0, pRenderContext->udRenderContext.pDepthBuffer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pRenderContext->pCamera->matrices.projectionUD.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_RenderUD(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (pRenderContext == nullptr)
    return udR_InvalidParameter_;

  vdkRenderInstance *pModels = nullptr;
  int numVisibleModels = 0;

  double *pProjectionMatrix = pRenderContext->pCamera->matrices.projectionUD.a;
#if ALLOW_EXPERIMENT_GPURENDER
  if (pRenderContext->pSettings->experimental.useGPURenderer)
    pProjectionMatrix = pRenderContext->pCamera->matrices.projection.a; // native render api space
#endif

  vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pProjectionMatrix);
  vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_View, pRenderContext->pCamera->matrices.view.a);

  switch (pRenderContext->pSettings->visualization.mode)
  {
  case vcVM_Intensity:
    vdkRenderContext_ShowIntensity(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->pSettings->visualization.minIntensity, pRenderContext->pSettings->visualization.maxIntensity);
    break;
  case vcVM_Classification:
    vdkRenderContext_ShowClassification(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, (int*)pRenderContext->pSettings->visualization.customClassificationColors, (int)udLengthOf(pRenderContext->pSettings->visualization.customClassificationColors));
    break;
  default: //Includes vcVM_Colour
    vdkRenderContext_ShowColor(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer);
    break;
  }

  vdkRenderContext_SetPointMode(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, (vdkRenderContextPointMode)pRenderContext->pSettings->presentation.pointMode);

  if (renderData.models.length > 0)
    pModels = udAllocStack(vdkRenderInstance, renderData.models.length, udAF_None);

  double maxDistSqr = pRenderContext->pSettings->camera.farPlane * pRenderContext->pSettings->camera.farPlane;
  renderData.pWatermarkTexture = nullptr;

  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->m_visible && renderData.models[i]->m_loadStatus == vcSLS_Loaded)
    {
      // Copy to the contiguous array
      pModels[numVisibleModels].pPointCloud = renderData.models[i]->m_pPointCloud;
      memcpy(&pModels[numVisibleModels].matrix, renderData.models[i]->m_sceneMatrix.a, sizeof(pModels[numVisibleModels].matrix));
      ++numVisibleModels;

      if (renderData.models[i]->m_hasWatermark)
      {
        udDouble3 distVector = pRenderContext->pCamera->position - renderData.models[i]->GetWorldSpacePivot();
        if (pRenderContext->pSettings->camera.cameraMode == vcCM_OrthoMap)
          distVector.z = 0.0;

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
                vcTexture_Create(&renderData.models[i]->m_pWatermark, imageWidth, imageHeight, pImageData, vcTextureFormat_RGBA8, vcTFM_Nearest, false);
                free(pImageData);
              }

              udFree(pImage);
            }
          }

          renderData.pWatermarkTexture = renderData.models[i]->m_pWatermark;
        }
      }
    }
  }


  if (pRenderContext->pSettings->presentation.showDiagnosticInfo && renderData.pGISSpace != nullptr)
  {
    vcFenceRenderer_ClearPoints(pRenderContext->pDiagnosticFences);

    float z = 0;
    if (pRenderContext->pSettings->maptiles.mapEnabled)
      z = pRenderContext->pSettings->maptiles.mapHeight;

    udInt2 slippyCurrent;
    udDouble3 localCurrent;

    double xRange = renderData.pGISSpace->zone.latLongBoundMax.x - renderData.pGISSpace->zone.latLongBoundMin.x;
    double yRange = renderData.pGISSpace->zone.latLongBoundMax.y - renderData.pGISSpace->zone.latLongBoundMin.y;

    if (xRange > 0 || yRange > 0)
    {
      std::vector<udDouble3> corners;

      for (int i = 0; i < yRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(renderData.pGISSpace->zone.latLongBoundMin.x, renderData.pGISSpace->zone.latLongBoundMin.y + i, 0), 21);
        vcGIS_SlippyToLocal(renderData.pGISSpace, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < xRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(renderData.pGISSpace->zone.latLongBoundMin.x + i, renderData.pGISSpace->zone.latLongBoundMax.y, 0), 21);
        vcGIS_SlippyToLocal(renderData.pGISSpace, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < yRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(renderData.pGISSpace->zone.latLongBoundMax.x, renderData.pGISSpace->zone.latLongBoundMax.y - i, 0), 21);
        vcGIS_SlippyToLocal(renderData.pGISSpace, &localCurrent, slippyCurrent, 21);
        corners.push_back(udDouble3::create(localCurrent.x, localCurrent.y, z));
      }
      for (int i = 0; i < xRange; ++i)
      {
        vcGIS_LatLongToSlippy(&slippyCurrent, udDouble3::create(renderData.pGISSpace->zone.latLongBoundMax.x - i, renderData.pGISSpace->zone.latLongBoundMin.y, 0), 21);
        vcGIS_SlippyToLocal(renderData.pGISSpace, &localCurrent, slippyCurrent, 21);
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
  renderOptions.pPick = &picking;

#if ALLOW_EXPERIMENT_GPURENDER
  if (pRenderContext->pSettings->experimental.useGPURenderer)
  {
    renderOptions.flags = vdkRF_GPURender;

    vcFramebuffer_Bind(pRenderContext->udRenderContext.pFramebuffer);
    vcFramebuffer_Clear(pRenderContext->udRenderContext.pFramebuffer, 0x00000000);

    vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
  }
#endif

  vdkError result = vdkRenderContext_Render(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->udRenderContext.pRenderView, pModels, numVisibleModels, &renderOptions);

  if (result == vE_Success)
  {
    if (picking.hit != 0)
    {
      // More to be done here
      renderData.worldMousePos = udDouble3::create(picking.pointCenter[0], picking.pointCenter[1], picking.pointCenter[2]);
      renderData.pickingSuccess = true;
      renderData.udModelPickedIndex = picking.modelIndex;
    }
  }
  else
  {
    //TODO: Clear the buffers
  }

#if ALLOW_EXPERIMENT_GPURENDER
  if (!pRenderContext->pSettings->experimental.useGPURenderer)
#endif
  {
    vcTexture_UploadPixels(pRenderContext->udRenderContext.pColourTex, pRenderContext->udRenderContext.pColorBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
    vcTexture_UploadPixels(pRenderContext->udRenderContext.pDepthTex, pRenderContext->udRenderContext.pDepthBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
  }


  udFreeStack(pModels);
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

vcRenderPickResult vcRender_PolygonPick(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  vcRenderPickResult result = {};

  udFloat2 pickUV = udFloat2::create((float)renderData.mouse.position.x / (float)pRenderContext->originalSceneResolution.x, (float)renderData.mouse.position.y / (float)pRenderContext->originalSceneResolution.y);

  if (pickUV.x < 0 || pickUV.x > 1 || pickUV.y < 0 || pickUV.y > 1)
    return result;

  pRenderContext->picking.location.x = (uint32_t)(pickUV.x * pRenderContext->effectResolution.x);
  pRenderContext->picking.location.y = (uint32_t)(pickUV.y * pRenderContext->effectResolution.y);

  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);

  vcFramebuffer_Bind(pRenderContext->picking.pFramebuffer);
  vcFramebuffer_Clear(pRenderContext->picking.pFramebuffer, 0x0);

  vcGLState_SetViewport(0, 0, pRenderContext->effectResolution.x, pRenderContext->effectResolution.y);
  vcGLState_Scissor(pRenderContext->picking.location.x, pRenderContext->picking.location.y, pRenderContext->picking.location.x + 1, pRenderContext->picking.location.y + 1);

  // render pickable geometry
  {
    uint32_t modelId = 1; // note: start at 1, because 0 is 'null'

#if ALLOW_EXPERIMENT_GPURENDER
    if (pRenderContext->udRenderContext.usingGPURenderer)
      vcRender_SplatUDWithId(pRenderContext, 0.0f); // `0.0` is a sentinel to tell it to use the id encoded in the alpha channel
#endif

    // Polygon Models
    {
      for (size_t i = 0; i < renderData.polyModels.length; ++i)
      {
        udFloat4 idAsColour = vcRender_EncodeIdAsColour((uint32_t)(modelId++));
        vcPolygonModel_Render(renderData.polyModels[i].pModel, renderData.polyModels[i].worldMat, pRenderContext->pCamera->matrices.viewProjection, vcPMP_ColourOnly, nullptr, &idAsColour);
      }

      /*
      for (size_t i = 0; i < renderData.sceneLayers.length; ++i)
      {
        udFloat4 idAsColour = vcRender_EncodeIdAsColour((uint32_t)(modelId++));
        //vcSceneLayerRenderer_Render(renderData.sceneLayers[i], pRenderContext->pCamera->matrices.viewProjection, pRenderContext->sceneResolution);
      }
      */
    }
  }

  uint8_t pickColourBGRA[4] = {};
  float pickDepth = 0.0f;

#if GRAPHICS_API_OPENGL
  // note: we render upside-down
  vcFramebuffer_ReadPixels(pRenderContext->picking.pFramebuffer, pRenderContext->picking.pTexture, pickColourBGRA, pRenderContext->picking.location.x, pRenderContext->effectResolution.y - pRenderContext->picking.location.y - 1, 1, 1);
  vcFramebuffer_ReadPixels(pRenderContext->picking.pFramebuffer, pRenderContext->picking.pDepth, &pickDepth, pRenderContext->picking.location.x, pRenderContext->effectResolution.y - pRenderContext->picking.location.y - 1, 1, 1);
#else // All others are the same direction
  vcFramebuffer_ReadPixels(pRenderContext->picking.pFramebuffer, pRenderContext->picking.pTexture, pickColourBGRA, pRenderContext->picking.location.x, pRenderContext->picking.location.y, 1, 1);
  vcFramebuffer_ReadPixels(pRenderContext->picking.pFramebuffer, pRenderContext->picking.pDepth, &pickDepth, pRenderContext->picking.location.x, pRenderContext->picking.location.y, 1, 1);
#endif
  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  // note `-1`, and BGRA format
  int udPickedId = -1;
#ifdef ALLOW_EXPERIMENT_GPURENDER
  if (pRenderContext->pSettings->experimental.useGPURenderer)
    udPickedId = (pickColourBGRA[2] << 0) - 1;
#endif

  int pickedPolygonId = (int)((pickColourBGRA[1] << 0) | (pickColourBGRA[0] << 8)) - 1;
  if (pickedPolygonId != -1 || udPickedId != -1)
  {
    result.success = true;

    // note: upside down (1.0 - uv.y)
#if GRAPHICS_API_OPENGL
    udDouble4 clipPos = udDouble4::create(pickUV.x * 2.0 - 1.0, (1.0 - pickUV.y) * 2.0 - 1.0, pickDepth * 2.0 - 1.0, 1.0);
#else // All others are the same direction
    // note: direct x clip depth is [0, 1]
    udDouble4 clipPos = udDouble4::create(pickUV.x * 2.0 - 1.0, (1.0 - pickUV.y) * 2.0 - 1.0, pickDepth, 1.0);
#endif
    udDouble4 pickPosition = pRenderContext->pCamera->matrices.inverseViewProjection * clipPos;
    pickPosition = pickPosition / pickPosition.w;
    result.success = true;
    result.position = pickPosition.toVector3();

    if (pickedPolygonId != -1)
    {
      result.pPolygon = &renderData.polyModels[pickedPolygonId];
    }
    else
    {
      result.pModel = renderData.models[udPickedId];
    }
  }

  return result;
}

void vcRender_PickTiles(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  if (pRenderContext->pSettings->maptiles.mapEnabled && pRenderContext->pSettings->maptiles.mouseInteracts)// check map tiles
  {
    udDouble4x4 cameraMatrix = pRenderContext->pCamera->matrices.camera;
    udDouble3 localCamPos = cameraMatrix.axis.t.toVector3();

    udPlane<double> mapPlane = udPlane<double>::create({ 0, 0, pRenderContext->pSettings->maptiles.mapHeight }, { 0, 0, 1 });

    udDouble3 hitPoint;
    double hitDistance;
    if (mapPlane.intersects(pRenderContext->pCamera->worldMouseRay, &hitPoint, &hitDistance))
    {
      if (hitDistance < pRenderContext->pSettings->camera.farPlane && (!renderData.pickingSuccess || hitDistance < udMag3(renderData.worldMousePos - localCamPos) - pRenderContext->pSettings->camera.nearPlane))
      {
        renderData.pickingSuccess = true;
        renderData.worldMousePos = hitPoint;
      }
    }
  }
}
