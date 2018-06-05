#include "vcRender.h"

#include "vcRenderShaders.h"
#include "vcTerrain.h"
#include "vcGIS.h"

int qrIndices[6] = { 0, 1, 2, 0, 2, 3 };
vcSimpleVertex qrSqVertices[4]{ { { -1.f, 1.f, 0.f },{ 0, 0 } },{ { -1.f, -1.f, 0.f },{ 0, 1 } },{ { 1.f, -1.f, 0.f },{ 1, 1 } },{ { 1.f, 1.f, 0.f },{ 1, 0 } } };
GLuint qrSqVboID = GL_INVALID_INDEX;
GLuint qrSqVaoID = GL_INVALID_INDEX;
GLuint qrSqIboID = GL_INVALID_INDEX;

GLuint vcSbVboID = GL_INVALID_INDEX;
GLuint vcSbVaoID = GL_INVALID_INDEX;

GLint udTextureLocation = -1;
GLint udDepthLocation = -1;

GLint vcSbCubemapSamplerLocation = -1;
GLint vcSbMatrixLocation = -1;

struct vcUDRenderContext
{
  vdkRenderContext *pRenderer;
  vdkRenderView *pRenderView;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  vcTexture colour;
  vcTexture depth;

  struct
  {
    GLuint program;
    GLint uniform_texture;
    GLint uniform_depth;
  } presentShader;
};

struct vcRenderContext
{
  vdkContext *pVaultContext;
  vcSettings *pSettings;
  vcCamera *pCamera;

  udUInt2 sceneResolution;

  vcFramebuffer framebuffer;
  vcTexture texture;
  vcTexture depthbuffer;

  vcUDRenderContext udRenderContext;

  udDouble4x4 viewMatrix;
  udDouble4x4 projectionMatrix;
  udDouble4x4 skyboxProjMatrix;
  udDouble4x4 viewProjectionMatrix;

  vcTerrain *pTerrain;

  struct
  {
    GLuint program;
    GLint uniform_texture;
    GLint uniform_inverseViewProjection;
  } skyboxShader;
  vcTexture skyboxCubeMapTexture;
};

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext);
udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, vcRenderData &renderData);

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &sceneResolution)
{
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  if (vcTerrain_Init(&pRenderContext->pTerrain, pSettings) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

  pRenderContext->udRenderContext.presentShader.program = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_udVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_udFragmentShader));
  if (pRenderContext->udRenderContext.presentShader.program == GL_INVALID_INDEX)
    UD_ERROR_SET(udR_InternalError);

  pRenderContext->skyboxShader.program = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_udVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_vcSkyboxShader));
  if (pRenderContext->skyboxShader.program == GL_INVALID_INDEX)
    goto epilogue;
  glUniform1i(udDepthLocation, 1);

  pRenderContext->skyboxCubeMapTexture = vcTexture_LoadCubemap("CloudWater.jpg");

  glUseProgram(pRenderContext->skyboxShader.program);
  pRenderContext->skyboxShader.uniform_texture = glGetUniformLocation(pRenderContext->skyboxShader.program, "u_texture");
  pRenderContext->skyboxShader.uniform_inverseViewProjection = glGetUniformLocation(pRenderContext->skyboxShader.program, "u_inverseViewProjection");

  glUseProgram(pRenderContext->udRenderContext.presentShader.program);
  pRenderContext->udRenderContext.presentShader.uniform_texture = glGetUniformLocation(pRenderContext->udRenderContext.presentShader.program, "u_texture");
  pRenderContext->udRenderContext.presentShader.uniform_depth = glGetUniformLocation(pRenderContext->udRenderContext.presentShader.program, "u_depth");

  vcCreateQuads(qrSqVertices, 4, qrIndices, 6, qrSqVboID, qrSqIboID, qrSqVaoID);
  VERIFY_GL();

  glBindVertexArray(0);
  glUseProgram(0);

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
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = *ppRenderContext;
  *ppRenderContext = nullptr;

  if (vcTerrain_Destroy(&pRenderContext->pTerrain) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderContext_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

epilogue:
  vcTextureDestroy(&pRenderContext->udRenderContext.colour);
  vcTextureDestroy(&pRenderContext->udRenderContext.depth);
  vcTextureDestroy(&pRenderContext->texture);
  vcTextureDestroy(&pRenderContext->depthbuffer);
  vcFramebufferDestroy(&pRenderContext->framebuffer);

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
  float fov = pRenderContext->pSettings->camera.fieldOfView;
  float aspect = width / (float)height;
  float zNear = pRenderContext->pSettings->camera.nearPlane;
  float zFar = pRenderContext->pSettings->camera.farPlane;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);
  UD_ERROR_IF(width == 0, udR_InvalidParameter_);
  UD_ERROR_IF(height == 0, udR_InvalidParameter_);

  pRenderContext->sceneResolution.x = width;
  pRenderContext->sceneResolution.y = height;
  pRenderContext->projectionMatrix = udDouble4x4::perspective(fov, aspect, zNear, zFar);
  pRenderContext->skyboxProjMatrix = udDouble4x4::perspective(fov, aspect, 0.5f, 10000.f);

  //Resize GPU Targets
  vcTextureDestroy(&pRenderContext->udRenderContext.colour);
  vcTextureDestroy(&pRenderContext->udRenderContext.depth);
  pRenderContext->udRenderContext.colour = vcTextureCreate(pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, vcTextureFormat_RGBA8, GL_NEAREST);
  pRenderContext->udRenderContext.depth = vcTextureCreateDepth(pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, vcTextureFormat_D32F, GL_NEAREST);

  vcTextureDestroy(&pRenderContext->texture);
  vcTextureDestroy(&pRenderContext->depthbuffer);
  vcFramebufferDestroy(&pRenderContext->framebuffer);
  pRenderContext->texture = vcTextureCreate(width, height, vcTextureFormat_RGBA8, GL_NEAREST);
  pRenderContext->depthbuffer = vcTextureCreateDepth(width, height, vcTextureFormat_D32F, GL_NEAREST);
  pRenderContext->framebuffer = vcFramebufferCreate(&pRenderContext->texture, &pRenderContext->depthbuffer);

  //Resize CPU Targets
  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

  pRenderContext->udRenderContext.pColorBuffer = udAllocType(uint32_t, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);
  pRenderContext->udRenderContext.pDepthBuffer = udAllocType(float, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);

  if (pRenderContext->pVaultContext)
    vcRender_RecreateUDView(pRenderContext);

epilogue:
  return result;
}

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, vcRenderData &renderData, GLuint defaultFramebuffer)
{
  pRenderContext->viewMatrix = renderData.cameraMatrix;
  pRenderContext->viewMatrix.inverse();
  pRenderContext->viewProjectionMatrix = pRenderContext->projectionMatrix * pRenderContext->viewMatrix;

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  vcRender_RenderAndUploadUDToTexture(pRenderContext, renderData);

  glClearColor(0, 0, 0, 1);
  glViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  glBindFramebuffer(GL_FRAMEBUFFER, pRenderContext->framebuffer.id);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(pRenderContext->udRenderContext.presentShader.program);
  glUniform1i(udTextureLocation, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.colour.id);
  glUniform1i(pRenderContext->udRenderContext.presentShader.uniform_texture, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.depth.id);
  glUniform1i(pRenderContext->udRenderContext.presentShader.uniform_depth, 1);

  glBindVertexArray(qrSqVaoID);
  glBindBuffer(GL_ARRAY_BUFFER, qrSqVboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrSqIboID);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  vcRenderSkybox(pRenderContext);

  if (renderData.srid != 0 && pRenderContext->pSettings->maptiles.mapEnabled)
  {
    udDouble3 localCamPos = renderData.cameraMatrix.axis.t.toVector3();

    // Corners [nw, ne, sw, se]
    udDouble3 localCorners[4];
    udInt2 slippyCorners[4];

    int currentZoom = 21;

    // Cardinal Limits
    localCorners[0] = localCamPos + udDouble3::create(-pRenderContext->pSettings->camera.farPlane, +pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[1] = localCamPos + udDouble3::create(+pRenderContext->pSettings->camera.farPlane, +pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[2] = localCamPos + udDouble3::create(-pRenderContext->pSettings->camera.farPlane, -pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[3] = localCamPos + udDouble3::create(+pRenderContext->pSettings->camera.farPlane, -pRenderContext->pSettings->camera.farPlane, 0);

    for (int i = 0; i < 4; ++i)
      vcGIS_LocalToSlippy(renderData.srid, &slippyCorners[i], localCorners[i], currentZoom);

    while (currentZoom > 0 && (slippyCorners[0] != slippyCorners[1] || slippyCorners[1] != slippyCorners[2] || slippyCorners[2] != slippyCorners[3]))
    {
      --currentZoom;

      for (int i = 0; i < 4; ++i)
        slippyCorners[i] /= 2;
    }

    for (int i = 0; i < 4; ++i)
      vcGIS_SlippyToLocal(renderData.srid, &localCorners[i], slippyCorners[0] + udInt2::create(i & 1, i / 2), currentZoom);

    udDouble3 localViewPos = renderData.cameraMatrix.axis.t.toVector3();
    double localViewSize = (1.0 / (1 << 19)) + udAbs(renderData.cameraMatrix.axis.t.z - pRenderContext->pSettings->maptiles.mapHeight) / 70000.0;

    // for now just rebuild terrain every frame
    vcTerrain_BuildTerrain(pRenderContext->pTerrain, renderData.srid, localCorners, udInt3::create(slippyCorners[0], currentZoom), localViewPos, localViewSize);
    vcTerrain_Render(pRenderContext->pTerrain, pRenderContext->viewProjectionMatrix);
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
  VERIFY_GL();

  return pRenderContext->texture;
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

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pRenderContext->projectionMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  udResult result = udR_Success;
  vdkModel **ppModels = nullptr;
  int numVisibleModels = 0;

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_View, pRenderContext->viewMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (renderData.models.length > 0)
    ppModels = udAllocStack(vdkModel*, renderData.models.length, udAF_None);

  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->modelVisible)
    {
      ppModels[numVisibleModels] = renderData.models[i]->pVaultModel;
      ++numVisibleModels;
    }
  }

  vdkRenderPicking picking;
  picking.x = renderData.mouse.x;
  picking.y = renderData.mouse.y;

  if (vdkRenderContext_RenderAdv(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->udRenderContext.pRenderView, ppModels, (int)numVisibleModels, &picking) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (picking.hit != 0)
  {
    // More to be done here
    renderData.worldMousePos = udDouble3::create(picking.pointCenter[0], picking.pointCenter[1], picking.pointCenter[2]);
  }

  vcTextureUploadPixels(&pRenderContext->udRenderContext.colour, pRenderContext->udRenderContext.pColorBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
  vcTextureUploadPixels(&pRenderContext->udRenderContext.depth, pRenderContext->udRenderContext.pDepthBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

epilogue:
  udFreeStack(pModels);
  return result;
}

void vcRenderSkybox(vcRenderContext *pRenderContext)
{
  udFloat4x4 viewMatrixF = udFloat4x4::create(pRenderContext->viewMatrix);
  udFloat4x4 projectionMatrixF = udFloat4x4::create(pRenderContext->skyboxProjMatrix);
  udFloat4x4 viewProjMatrixF = projectionMatrixF * viewMatrixF;
  viewProjMatrixF.axis.t = udFloat4::create(0, 0, 0, 1);
  viewProjMatrixF.inverse();

  glUseProgram(pRenderContext->skyboxShader.program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, pRenderContext->skyboxCubeMapTexture.id);
  glUniform1i(pRenderContext->skyboxShader.uniform_texture, 0);
  glUniformMatrix4fv(pRenderContext->skyboxShader.uniform_inverseViewProjection, 1, false, viewProjMatrixF.a);

  // Draw the skybox only at the far plane, where there is no geometry.
  // Drawing skybox here (after 'opaque' geometry) saves a bit on fill rate.
  glDepthRangef(1.0f, 1.0f);
  glDepthFunc(GL_LEQUAL);

  glBindVertexArray(qrSqVaoID);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glDepthFunc(GL_LESS);
  glDepthRangef(0.0f, 1.0f);

  glBindVertexArray(0);
  glUseProgram(0);
}

bool vcRender_ClearCache(vcRenderContext *pRenderContext)
{
  return vcTerrain_ClearCache(pRenderContext->pTerrain);
}
