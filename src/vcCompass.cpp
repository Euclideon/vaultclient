#include "vcCompass.h"

#include "gl/vcLayout.h"
#include "gl/vcShader.h"
#include "gl/vcRenderShaders.h"

#include "vcInternalModels.h"

struct vcAnchor
{
  vcMesh *meshes[vcAS_Count];

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

udResult vcCompass_Create(vcAnchor **ppCompass)
{
  if (ppCompass == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  vcAnchor *pCompass = udAllocType(vcAnchor, 1, udAF_Zero);

  UD_ERROR_NULL(pCompass, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcMesh_Create(&pCompass->meshes[vcAS_Orbit], vcNormalVertexLayout, (int)udLengthOf(vcNormalVertexLayout), pOrbitVerts, (int)udLengthOf(orbitVertsFltArray), orbitFaces, (int)udLengthOf(orbitFaces), vcMF_IndexShort));
  UD_ERROR_CHECK(vcMesh_Create(&pCompass->meshes[vcAS_Compass], vcNormalVertexLayout, (int)udLengthOf(vcNormalVertexLayout), pCompassVerts, (int)udLengthOf(compassVertsFltArray), compassFaces, (int)udLengthOf(compassFaces), vcMF_IndexShort));

  UD_ERROR_IF(!vcShader_CreateFromText(&pCompass->pShader, g_CompassVertexShader, g_CompassFragmentShader, vcNormalVertexLayout), udR_InvalidConfiguration);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pCompass->pShaderConstantBuffer, pCompass->pShader, "u_EveryObject", sizeof(vcAnchor::shaderBuffer)), udR_InvalidParameter_);

  *ppCompass = pCompass;
  pCompass = nullptr;

epilogue:
  if (pCompass != nullptr)
    vcCompass_Destroy(&pCompass);

  return result;
};

udResult vcCompass_Destroy(vcAnchor **ppCompass)
{
  if (ppCompass == nullptr || *ppCompass == nullptr)
    return udR_InvalidParameter_;

  for (size_t i = 0; i < vcAS_Count; ++i)
    vcMesh_Destroy(&(*ppCompass)->meshes[i]);

  vcShader_DestroyShader(&(*ppCompass)->pShader);
  udFree((*ppCompass));

  return udR_Success;
}

udResult vcCompass_Render(vcAnchor *pCompass, vcAnchorStyle anchorStyle, const udDouble4x4 &worldViewProj, const udDouble4 &colour /*= udDouble4::create(1.0, 1.0, 1.0, 1.0)*/)
{
  if (pCompass == nullptr || pCompass->meshes[anchorStyle] == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  pCompass->shaderBuffer.u_colour = udFloat4::create(colour);
  pCompass->shaderBuffer.u_viewProjectionMatrix = udFloat4x4::create(worldViewProj);
  pCompass->shaderBuffer.u_sunDirection = udNormalize(udFloat3::create(1.0f, 0.0f, -1.0f));

  UD_ERROR_IF(!vcShader_Bind(pCompass->pShader), udR_InternalError);
  UD_ERROR_IF(!vcShader_BindConstantBuffer(pCompass->pShader, pCompass->pShaderConstantBuffer, &pCompass->shaderBuffer, sizeof(vcAnchor::shaderBuffer)), udR_InputExhausted);
  UD_ERROR_IF(!vcMesh_Render(pCompass->meshes[anchorStyle]), udR_InternalError);

  result = udR_Success;

epilogue:
  return result;
}
