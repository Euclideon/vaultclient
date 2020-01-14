#include "vcCompass.h"

#include "gl/vcLayout.h"
#include "gl/vcShader.h"
#include "gl/vcRenderShaders.h"

#include "gl/vcMesh.h"
#include "vcInternalModels.h"

struct vcAnchor
{
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

static vcInternalMeshType vcASToMeshType[] = { vcInternalMeshType_Count, vcInternalMeshType_Orbit, vcInternalMeshType_Compass };
UDCOMPILEASSERT(udLengthOf(vcASToMeshType) == vcAS_Count, "AnchorStyle does not equal size");

udResult vcCompass_Create(vcAnchor **ppCompass)
{
  udResult result;
  vcAnchor *pCompass = nullptr;

  UD_ERROR_NULL(ppCompass, udR_InvalidParameter_);

  pCompass = udAllocType(vcAnchor, 1, udAF_Zero);
  UD_ERROR_NULL(pCompass, udR_MemoryAllocationFailure);

  UD_ERROR_IF(!vcShader_CreateFromText(&pCompass->pShader, g_CompassVertexShader, g_CompassFragmentShader, vcP3N3VertexLayout), udR_InvalidConfiguration);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pCompass->pShaderConstantBuffer, pCompass->pShader, "u_EveryObject", sizeof(vcAnchor::shaderBuffer)), udR_InvalidParameter_);

  *ppCompass = pCompass;
  pCompass = nullptr;
  result = udR_Success;

epilogue:
  if (pCompass != nullptr)
    vcCompass_Destroy(&pCompass);

  return result;
};

udResult vcCompass_Destroy(vcAnchor **ppCompass)
{
  if (ppCompass == nullptr || *ppCompass == nullptr)
    return udR_InvalidParameter_;

  vcShader_DestroyShader(&(*ppCompass)->pShader);
  udFree((*ppCompass));

  return udR_Success;
}

udResult vcCompass_Render(vcAnchor *pCompass, vcAnchorStyle anchorStyle, const udDouble4x4 &worldViewProj, const udDouble4 &colour /*= udDouble4::create(1.0, 1.0, 1.0, 1.0)*/)
{
  if (pCompass == nullptr || anchorStyle >= vcAS_Count || vcASToMeshType[anchorStyle] == vcInternalMeshType_Count)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  pCompass->shaderBuffer.u_colour = udFloat4::create(colour);
  pCompass->shaderBuffer.u_viewProjectionMatrix = udFloat4x4::create(worldViewProj);
  pCompass->shaderBuffer.u_sunDirection = udNormalize(udFloat3::create(1.0f, 0.0f, -1.0f));

  UD_ERROR_IF(!vcShader_Bind(pCompass->pShader), udR_InternalError);
  UD_ERROR_IF(!vcShader_BindConstantBuffer(pCompass->pShader, pCompass->pShaderConstantBuffer, &pCompass->shaderBuffer, sizeof(vcAnchor::shaderBuffer)), udR_InputExhausted);
  UD_ERROR_IF(!vcMesh_Render(gInternalMeshes[vcASToMeshType[anchorStyle]]), udR_InternalError);

  result = udR_Success;

epilogue:
  return result;
}
