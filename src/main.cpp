#include "vaultContext.h"
#include "vaultUDRenderer.h"
#include "vaultUDRenderView.h"
#include "vaultUDModel.h"

#include <stdlib.h>
#include <stdio.h>

int main(int /*argc*/, char ** /*argv*/)
{
  const uint32_t width = 1280;
  const uint32_t height = 720;
  uint32_t *pColorBuffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
  float *pDepthBuffer = (float*)malloc(width * height * sizeof(float));

  vaultContext *pContext = nullptr;
  vaultUDRenderer *pRenderer = nullptr;
  vaultUDRenderView *pRenderView = nullptr;
  vaultUDModel *pModel = nullptr;

  const char *plocalHost = "http://127.0.0.1:80/api/v1/";

  vaultError error = vaultContext_Connect(&pContext, plocalHost, "ClientSDKTest");
  if (error != vE_Success)
  {
    printf("vaultContext_Connect Error: %d;  Is the server running?", error);
    goto epilogue;
  }

  error = vaultContext_Login(pContext, "Name", "Pass");
  if (error != vE_Success)
  {
    printf("vaultContext_Login Error: %d", error);
    goto epilogue;
  }

  error = vaultContext_GetLicense(pContext, vLF_Basic);
  if (error != vE_Success)
  {
    printf("vaultContext_GetLicense Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderer_Create(pContext, &pRenderer);
  if (error != vE_Success)
  {
    printf("vaultUDRenderer_Create Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderView_Create(pContext, &pRenderView, pRenderer, width, height);
  if (error != vE_Success)
  {
    printf("vaultUDRenderView_Create Error: %d", error);
    goto epilogue;
  }

  error = vaultUDModel_Load(pContext, &pModel, "R:/ConvertedModels/Apple(UDS4).uds");
  if (error != vE_Success)
  {
    printf("vaultUDModel_Load Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderView_SetTargets(pContext, pRenderView, pColorBuffer, 0, pDepthBuffer);
  if (error != vE_Success)
  {
    printf("vaultUDRenderView_SetTarget Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderer_Render(pContext, pRenderer, pRenderView, &pModel, 1);
  if (error != vE_Success)
  {
    printf("vaultUDRenderer_Render Error: %d", error);
    goto epilogue;
  }

  error = vaultUDModel_Unload(pContext, &pModel);
  if (error != vE_Success)
  {
    printf("vaultUDModel_Unload Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderView_Destroy(pContext, &pRenderView);
  if (error != vE_Success)
  {
    printf("vaultUDRenderView_Destroy Error: %d", error);
    goto epilogue;
  }

  error = vaultUDRenderer_Destroy(pContext, &pRenderer);
  if (error != vE_Success)
  {
    printf("vaultUDRenderer_Destroy Error: %d", error);
    goto epilogue;
  }

  error = vaultContext_Disconnect(&pContext);
  if (error != vE_Success)
  {
    printf("vaultContext_Disconnect Error: %d", error);
    goto epilogue;
  }

epilogue:
  free(pColorBuffer);
  free(pDepthBuffer);

  return 0;
}
