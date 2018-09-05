#include "vcCompass.h"

#include "gl/vcLayout.h"
#include "gl/vcShader.h"
#include "gl/vcRenderShaders.h"

#include "vcInternalModels.h"

struct vcCompass
{
  vcMesh *pMesh;
  vcShader *pShader;
  vcShaderConstantBuffer *pShaderConstantBuffer;

  struct
  {
    udFloat4x4 u_viewProjectionMatrix;
    udFloat4 u_colour;
    udFloat3 u_sunDirection;
    float _padding;
  } shaderBuffer;
};

udResult vcCompass_Create(vcCompass **ppCompass)
{
  if (ppCompass == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  vcCompass *pCompass = udAllocType(vcCompass, 1, udAF_Zero);

  UD_ERROR_NULL(pCompass, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(vcMesh_Create(&pCompass->pMesh, vcNormalVertexLayout, (int)udLengthOf(vcNormalVertexLayout), pCompassVerts, (int)udLengthOf(compassVertsFltArray), compassFaces, (int)udLengthOf(compassFaces), vcMF_IndexShort));
  UD_ERROR_IF(!vcShader_CreateFromText(&pCompass->pShader, g_PositionNormalVertexShader, g_PositionNormalFragmentShader, vcNormalVertexLayout, (uint32_t)UDARRAYSIZE(vcNormalVertexLayout)), udR_InvalidConfiguration);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pCompass->pShaderConstantBuffer, pCompass->pShader, "u_EveryObject", sizeof(vcCompass::shaderBuffer)), udR_InvalidParameter_);

  *ppCompass = pCompass;
  pCompass = nullptr;

epilogue:
  if (pCompass != nullptr)
    vcCompass_Destroy(&pCompass);

  return result;
};

udResult vcCompass_Destroy(vcCompass **ppCompass)
{
  if (ppCompass == nullptr || *ppCompass == nullptr)
    return udR_InvalidParameter_;

  vcMesh_Destroy(&(*ppCompass)->pMesh);
  udFree((*ppCompass));

  return udR_Success;
}

udResult vcCompass_Render(vcCompass *pCompass, const udDouble4x4 &worldViewProj, const udDouble4 &colour /*= udDouble4::create(1.0, 1.0, 1.0, 1.0)*/)
{
  if (pCompass == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  pCompass->shaderBuffer.u_colour = udFloat4::create(colour);
  pCompass->shaderBuffer.u_viewProjectionMatrix = udFloat4x4::create(worldViewProj);
  pCompass->shaderBuffer.u_sunDirection = udNormalize(udFloat3::create(1.0f, 0.0f, -1.0f));

  UD_ERROR_IF(!vcShader_Bind(pCompass->pShader), udR_InternalError);
  UD_ERROR_IF(!vcShader_BindConstantBuffer(pCompass->pShader, pCompass->pShaderConstantBuffer, &pCompass->shaderBuffer, sizeof(vcCompass::shaderBuffer)), udR_InputExhausted);
  UD_ERROR_IF(!vcMesh_RenderTriangles(pCompass->pMesh, (uint32_t)udLengthOf(compassFaces) / 3), udR_InternalError);

epilogue:
  return result;
}
