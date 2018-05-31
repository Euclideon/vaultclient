#include "vcRender.h"

#include "vcRenderShaders.h"
#include "vcTerrain.h"

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
  vaultUDRenderer *pRenderer;
  vaultUDRenderView *pRenderView;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  vcTexture tex;
  vcTexture depth;
  GLint programObject;
};

struct vcRenderContext
{
  vaultContext *pVaultContext;

  udUInt2 sceneResolution;

  vcFramebuffer framebuffer;
  vcTexture texture;
  vcTexture depthbuffer;

  vcUDRenderContext udRenderContext;

  GLint skyboxProgramObject;
  vcTexture skyboxCubeMapTexture;

  udDouble4x4 viewMatrix;
  udDouble4x4 projectionMatrix;
  udDouble4x4 skyboxProjMatrix;
  udDouble4x4 viewProjectionMatrix;

  vcTerrain *pTerrain;
  vcSettings *pSettings;
};

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext);
udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, const vcRenderData &renderData);

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, const udUInt2 &sceneResolution)
{
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  if (vcTerrain_Init(&pRenderContext->pTerrain) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

  pRenderContext->udRenderContext.programObject = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_udVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_udFragmentShader));
  if (pRenderContext->udRenderContext.programObject == -1)
    UD_ERROR_SET(udR_InternalError);

  pRenderContext->skyboxProgramObject = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_udVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_vcSkyboxShader));
  if (pRenderContext->skyboxProgramObject == -1)
    goto epilogue;

  pRenderContext->skyboxCubeMapTexture = vcTexture_LoadCubemap("CloudWater.jpg");

  glUseProgram(pRenderContext->skyboxProgramObject);
  vcSbCubemapSamplerLocation = glGetUniformLocation(pRenderContext->skyboxProgramObject, "CubemapSampler");
  vcSbMatrixLocation = glGetUniformLocation(pRenderContext->skyboxProgramObject, "invSkyboxMatrix");
  glUseProgram(0);

  //
  glUseProgram(pRenderContext->udRenderContext.programObject);
  udTextureLocation = glGetUniformLocation(pRenderContext->udRenderContext.programObject, "u_texture");
  udDepthLocation = glGetUniformLocation(pRenderContext->udRenderContext.programObject, "u_depth");
  glUniform1i(udTextureLocation, 0);
  glUniform1i(udDepthLocation, 1);

  vcCreateQuads(qrSqVertices, 4, qrIndices, 6, qrSqVboID, qrSqIboID, qrSqVaoID);
  VERIFY_GL();

  glBindVertexArray(0);
  glUseProgram(0);

  *ppRenderContext = pRenderContext;

  pRenderContext->pSettings = pSettings;
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

  if (vaultUDRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderer_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

epilogue:
  udFree(pRenderContext);
  return result;
}

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vaultContext *pVaultContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  pRenderContext->pVaultContext = pVaultContext;

  if (vaultUDRenderer_Create(pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
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
  vcDestroyTexture(&pRenderContext->udRenderContext.tex);
  vcDestroyTexture(&pRenderContext->udRenderContext.depth);
  pRenderContext->udRenderContext.tex = vcCreateTexture(pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, vcTextureFormat_RGBA8, GL_LINEAR);
  pRenderContext->udRenderContext.depth = vcCreateTexture(pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, vcTextureFormat_D32F, GL_NEAREST);

  vcDestroyTexture(&pRenderContext->texture);
  vcDestroyTexture(&pRenderContext->depthbuffer);
  vcDestroyFramebuffer(&pRenderContext->framebuffer);
  pRenderContext->texture = vcCreateTexture(width, height, vcTextureFormat_RGBA8, GL_NEAREST);
  pRenderContext->depthbuffer = vcCreateDepthTexture(width, height, vcTextureFormat_D32F, GL_NEAREST);
  pRenderContext->framebuffer = vcCreateFramebuffer(&pRenderContext->texture, &pRenderContext->depthbuffer);

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

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, const vcRenderData &renderData, GLuint defaultFramebuffer)
{
  pRenderContext->viewMatrix = renderData.cameraMatrix;
  pRenderContext->viewMatrix.inverse();
  pRenderContext->viewProjectionMatrix = pRenderContext->projectionMatrix * pRenderContext->viewMatrix;

  vcRender_RenderAndUploadUDToTexture(pRenderContext, renderData);

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);

  glClearColor(0, 0, 0, 1);
  glViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  glBindFramebuffer(GL_FRAMEBUFFER, pRenderContext->framebuffer.id);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(pRenderContext->udRenderContext.programObject);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.tex.id);
  glUniform1i(udTextureLocation, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.depth.id);
  glUniform1i(udDepthLocation, 1);

  glBindVertexArray(qrSqVaoID);
  glBindBuffer(GL_ARRAY_BUFFER, qrSqVboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrSqIboID);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  if (renderData.srid != 0)
  {
    // for now just rebuild terrain every frame
    extern float gWorldScale;
    float depthToHeightScalar = 1.0f; // temp 'zoom level' mechanic
    udFloat2 fakeLatLong = udFloat2::create(float(renderData.cameraMatrix.axis.t.x) / gWorldScale, float(renderData.cameraMatrix.axis.t.y) / gWorldScale);
    float fakeViewSize = udMax(0.0f, float(renderData.cameraMatrix.axis.t.z) / (gWorldScale * depthToHeightScalar));
    vcTerrain_BuildTerrain(pRenderContext->pTerrain, fakeLatLong, fakeViewSize);

    vcTerrain_Render(pRenderContext->pTerrain, pRenderContext->viewProjectionMatrix);
  }

  vcRenderSkybox(pRenderContext);

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

  if (pRenderContext->udRenderContext.pRenderView && vaultUDRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_Create(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pRenderer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_SetTargets(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pColorBuffer, 0, pRenderContext->udRenderContext.pDepthBuffer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vUDRVM_Projection, pRenderContext->projectionMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, const vcRenderData &renderData)
{
  udResult result = udR_Success;
  vaultUDModel **ppModels = nullptr;
  int numVisibleModels = 0;

  if (vaultUDRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vUDRVM_View, pRenderContext->viewMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (renderData.models.length > 0)
    ppModels = udAllocStack(vaultUDModel*, renderData.models.length, udAF_None);

  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->modelVisible)
    {
      ppModels[numVisibleModels] = renderData.models[i]->pVaultModel;
      ++numVisibleModels;
    }
  }

  if (vaultUDRenderer_Render(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->udRenderContext.pRenderView, ppModels, (int)numVisibleModels) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.tex.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pRenderContext->udRenderContext.pColorBuffer);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.depth.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pRenderContext->udRenderContext.pDepthBuffer);


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

  glUseProgram(pRenderContext->skyboxProgramObject);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, pRenderContext->skyboxCubeMapTexture.id);
  glUniform1i(vcSbCubemapSamplerLocation, 0);
  glUniformMatrix4fv(vcSbMatrixLocation, 1, false, viewProjMatrixF.a);

  glDepthRangef(1.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glBindVertexArray(qrSqVaoID);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glDepthRangef(0.0f, 1.0f);

  glBindVertexArray(0);
  glUseProgram(0);
}
