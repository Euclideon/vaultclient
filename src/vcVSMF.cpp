#include "vcVSMF.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udResult.h"

#include "vcCompass.h"
#include "vCore/vStringFormat.h"

enum meshFlags {
  vMS_None = 0,
  vMS_Normals = 1,
  vMS_UVs = 2,
  vMS_Tangents = 4,
  vMS_SkinWeights = 8
};

struct VSMFHeader {
  uint16_t versionID;
  uint16_t flags;
  uint16_t numMaterials;
  uint16_t numMeshes;
  uint16_t numTextures;
  uint16_t reserved;
  float minXYZ[3];
  float maxXYZ[3];
};

struct VSMFMaterial {
  uint16_t flags;
  uint32_t BGRA;
};

struct VSMFMesh {
  uint16_t flags;
  uint16_t materialID;
  uint16_t LOD;
  uint16_t numVertices;
  uint16_t numElements;
};

struct VSMFTexture {

};

struct vcMeshyModel
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

udResult vcVSMF_ReadData(void *pInputVSMF, int dataLength)
{
  if (pInputVSMF == nullptr || dataLength < sizeof(VSMFHeader))
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  char *pFilePos = (char*)pInputVSMF;

  VSMFHeader header;

  UD_ERROR_IF(!udStrBeginsWith(pFilePos, "VSMF"), udR_InvalidParameter_);
  pFilePos += 4; // "VSMF"

  // Header
  UD_ERROR_CHECK(udReadFromPointer(&header, pFilePos, &dataLength));

  VSMFMaterial *pMaterials = udAllocType(VSMFMaterial, header.numMaterials, udAF_Zero);
  VSMFMesh *pMeshes = udAllocType(VSMFMesh, header.numMeshes, udAF_Zero);
  //VSMFTexture *pTextures = udAllocType(VSMFTexture, header.numTextures, udAF_Zero);

  // Materials
  for (int i = 0; i < header.numMaterials; ++i)
  {
    UD_ERROR_CHECK(udReadFromPointer(&pMaterials[i].flags, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pMaterials[i].BGRA, pFilePos, &dataLength));
  }

  // Vertex Block
  // Mesh
  for (int i = 0; i < header.numMeshes; ++i)
  {
    UD_ERROR_CHECK(udReadFromPointer(&pMeshes[i].flags, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pMeshes[i].materialID, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pMeshes[i].LOD, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pMeshes[i].numVertices, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pMeshes[i].numElements, pFilePos, &dataLength));

    //char *pNormals, pUVs;
    uint32_t recordSize = 12;

    vcVertexLayoutTypes *pVertexLayout = udAllocType(vcVertexLayoutTypes, 4, udAF_Zero);
    vcVertexLayoutTypes *pNextVL = pVertexLayout;
    *pNextVL = vcVLT_Position3;
    ++pNextVL;

    char *pNormals, *pUVs;

    if (pMeshes[i].flags & vMS_Normals)
    {
      *pNextVL = vcVLT_Normal3;
      ++pNextVL;
      recordSize += 12;
      pNormals = "float3 normal : NORMAL;";
    }
    if (pMeshes[i].flags & vMS_UVs)
    {
      *pNextVL = vcVLT_TextureCoords2;
      ++pNextVL;
      recordSize += 8;
      pUVs = "float2 uv : TEXCOORD0;";
    }
    //if (pMeshes[i].flags & vMS_Tangents)
    //  pNextVL = vcVLT_??;

    int64_t totalTypes = pNextVL - pVertexLayout;

    void *pVerts = pFilePos;
    pFilePos += recordSize * pMeshes[i].numVertices;

    void *pIndices = pFilePos;
    pFilePos += pMeshes[i].numVertices;

    vcMeshyModel *pNewModel = udAllocType(vcMeshyModel, 1, udAF_Zero);

    UD_ERROR_CHECK(vcMesh_Create(&pNewModel->pMesh, pVertexLayout, (int)totalTypes, pVerts, pMeshes[i].numVertices, pIndices, pMeshes[i].numVertices));

    // const char *ppStrings[] = { pNormals, pUVs };
    // const char *pShaderConfig = vStringFormat("struct VS_INPUT { float3 pos : POSITION; {0} {1} };", ppStrings, 2);

    // need to build shader to suit pMeshes[i].flags?, also meshes reference materials which currently is just a (diffuse?) color: pMaterials[pMeshes[i].materialID].BGRA
    // In a platform independent way, or just statically create some shaders for each vertex layout case

    // UD_ERROR_IF(!vcShader_CreateFromText(&pNewModel->pShader, VERTEX.SHADER, FRAG.SHADER, pVertexLayout, (uint32_t)totalTypes), udR_InvalidConfiguration);
    //UD_ERROR_IF(!vcShader_GetConstantBuffer(&pNewModel->pShaderConstantBuffer, pNewModel->pShader, "u_EveryObject", sizeof(vcMeshyModel::shaderBuffer)), udR_InvalidParameter_);

    float s = 1.0f / 255.0f;
    udFloat4 color = udFloat4::create(
      ((pMaterials[pMeshes[i].materialID].BGRA >> 8) & 0xFF) * s,
      ((pMaterials[pMeshes[i].materialID].BGRA >> 16) & 0xFF) * s,
      ((pMaterials[pMeshes[i].materialID].BGRA >> 24) & 0xFF) * s,
      ((pMaterials[pMeshes[i].materialID].BGRA >> 0) & 0xFF) * s);

    pNewModel->shaderBuffer.u_colour = color;

    udFree(pVertexLayout);
  }

  result = udR_Success;

epilogue:
  return result;
}

udResult vcVSMF_Render(vcMeshyModel *pModel, const udDouble4x4 &worldViewProj)
{
  if (pModel == nullptr || pModel->pMesh == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  pModel->shaderBuffer.u_viewProjectionMatrix = udFloat4x4::create(worldViewProj);
  pModel->shaderBuffer.u_sunDirection = udNormalize(udFloat3::create(1.0f, 0.0f, -1.0f));

  UD_ERROR_IF(!vcShader_Bind(pModel->pShader), udR_InternalError);
  UD_ERROR_IF(!vcShader_BindConstantBuffer(pModel->pShader, pModel->pShaderConstantBuffer, &pModel->shaderBuffer, sizeof(vcMeshyModel::shaderBuffer)), udR_InputExhausted);
  UD_ERROR_IF(!vcMesh_Render(pModel->pMesh), udR_InternalError);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcVSMF_Destroy(vcMeshyModel **ppModel)
{
  if (ppModel == nullptr || *ppModel == nullptr)
    return udR_InvalidParameter_;

  vcMesh_Destroy(&(*ppModel)->pMesh);

  vcShader_DestroyShader(&(*ppModel)->pShader);
  udFree((*ppModel));

  return udR_Success;
}
