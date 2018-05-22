
#include "vcRender.h"
#include "vcRenderShaders.h"

int qrIndices[6] = { 0, 1, 2, 0, 2, 3 };
vcSimpleVertex qrSqVertices[4]{ { { -1.f, 1.f, 0.f },{ 0, 0 } },{ { -1.f, -1.f, 0.f },{ 0, 1 } },{ { 1.f, -1.f, 0.f },{ 1, 1 } },{ { 1.f, 1.f, 0.f },{ 1, 0 } } };
GLuint qrSqVboID = GL_INVALID_INDEX;
GLuint qrSqVaoID = GL_INVALID_INDEX;
GLuint qrSqIboID = GL_INVALID_INDEX;

struct vcUDRenderContext
{
  vaultUDRenderer *pRenderer;
  vaultUDRenderView *pRenderView;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  vcTexture tex;
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

  udDouble4x4 viewMatrix;
};

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext);
udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext);

udResult vcRender_Init(vcRenderContext **ppRenderContext, const udUInt2 &sceneResolution)
{
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;
  GLint udTextureLocation = -1;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  pRenderContext->udRenderContext.programObject = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_udVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_udFragmentShader));
  if (pRenderContext->udRenderContext.programObject == -1)
    goto epilogue;

  glUseProgram(pRenderContext->udRenderContext.programObject);
  udTextureLocation = glGetUniformLocation(pRenderContext->udRenderContext.programObject, "u_texture");
  glUniform1i(udTextureLocation, 0);

  vcCreateQuads(qrSqVertices, 4, qrIndices, 6, qrSqVboID, qrSqIboID, qrSqVaoID);
  VERIFY_GL();

  glBindVertexArray(0);
  glUseProgram(0);

  *ppRenderContext = pRenderContext;

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

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);
  UD_ERROR_IF(width == 0, udR_InvalidParameter_);
  UD_ERROR_IF(height == 0, udR_InvalidParameter_);

  pRenderContext->sceneResolution.x = width;
  pRenderContext->sceneResolution.y = height;

  //Resize GPU Targets
  vcDestroyTexture(&pRenderContext->udRenderContext.tex);
  pRenderContext->udRenderContext.tex = pRenderContext->udRenderContext.tex = vcCreateTexture(pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, vcTextureFormat_RGBA8, GL_LINEAR);

  vcDestroyTexture(&pRenderContext->texture);
  vcDestroyTexture(&pRenderContext->depthbuffer);
  vcDestroyFramebuffer(&pRenderContext->framebuffer);
  pRenderContext->texture = vcCreateTexture(width, height, vcTextureFormat_RGBA8, GL_NEAREST);
  pRenderContext->depthbuffer = vcCreateDepthTexture(width, height, vcTextureFormat_D24, GL_NEAREST);
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

vcTexture vcRender_RenderScene(vcRenderContext *pRenderContext, const vcRenderData &renderData)
{
  pRenderContext->viewMatrix = renderData.cameraMatrix;

  vcRender_RenderAndUploadUDToTexture(pRenderContext);

  glClearColor(0, 0, 0, 1);
  glViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  glBindFramebuffer(GL_FRAMEBUFFER, pRenderContext->framebuffer.id);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(pRenderContext->udRenderContext.programObject);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.tex.id);

  glBindVertexArray(qrSqVaoID);
  glBindBuffer(GL_ARRAY_BUFFER, qrSqVboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrSqIboID);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  VERIFY_GL();

  return pRenderContext->texture;
}

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;
  float rad = UD_PIf / 3.f;
  float aspect = pRenderContext->sceneResolution.x / (float)pRenderContext->sceneResolution.y;
  float zNear = 0.5f;
  float zFar = 10000.f;
  udDouble4x4 projMat = udDouble4x4::perspective(rad, aspect, zNear, zFar);

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (pRenderContext->udRenderContext.pRenderView && vaultUDRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_Create(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pRenderer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_SetTargets(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pColorBuffer, 0, pRenderContext->udRenderContext.pDepthBuffer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vaultUDRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vUDRVM_Projection, projMat.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

// Temporary while setting up initially.
// References the model loaded in main.cpp
extern vaultUDModel *pModel;

udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  if (vaultUDRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vUDRVM_Camera, pRenderContext->viewMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (pModel)
  {
    if (vaultUDRenderer_Render(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->udRenderContext.pRenderView, &pModel, 1) != vE_Success)
      UD_ERROR_SET(udR_InternalError);
  }

  //if (wipeUDBuffers)
  //  memset(pRenderContext->udRenderContext.pColorBuffer, 0, sizeof(uint32_t) * pRenderContext->sceneResolution.x * pRenderContext->sceneResolution.y);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pRenderContext->udRenderContext.tex.id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pRenderContext->udRenderContext.pColorBuffer);

epilogue:
  return result;
}
