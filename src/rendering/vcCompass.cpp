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
    udFloat4x4 u_worldViewProjectionMatrix;
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

bool vcCompass_Render(vcAnchor *pCompass, vcAnchorStyle anchorStyle, const udDouble4x4 &worldViewProj, const udFloat4 &colour)
{
  if (pCompass == nullptr || anchorStyle >= vcAS_Count || vcASToMeshType[anchorStyle] == vcInternalMeshType_Count)
    return false;

  pCompass->shaderBuffer.u_colour = colour;
  pCompass->shaderBuffer.u_worldViewProjectionMatrix = udFloat4x4::create(worldViewProj);
  pCompass->shaderBuffer.u_sunDirection = udNormalize(udFloat3::create(1.0f, 0.0f, -1.0f));

  vcShader_Bind(pCompass->pShader);
  vcShader_BindConstantBuffer(pCompass->pShader, pCompass->pShaderConstantBuffer, &pCompass->shaderBuffer, sizeof(vcAnchor::shaderBuffer));
  vcMesh_Render(gInternalMeshes[vcASToMeshType[anchorStyle]]);

  return true;
}
