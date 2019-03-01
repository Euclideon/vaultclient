#include "vcPolygonModel.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udFile.h"

#include "gl/vcRenderShaders.h"
#include "gl/vcMesh.h"
#include "gl/vcTexture.h"

static int gPolygonShaderRefCount = 0;

enum vcPolygonModelShaderType
{
  vcPMST_P1N1UV1,

  vcPMST_Count
};

struct vcPolygonModelShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryFrameConstantBuffer;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_viewProjectionMatrix;
  } everyFrame;

  struct
  {
    udFloat4x4 u_world;
    udFloat4 u_colour;
  } everyObject;

} gShaders[vcPMST_Count];

// TODO: do these need to be public to be accessible to the importer?
enum vcPolygonModelVertexFlags
{
  vcPMVF_None = 0,
  vcPMVF_Normals = 1,
  vcPMVF_UVs = 2,
  //vcPMVF_Tangents = 4,
  //vcPMVF_SkinWeights = 8
};

struct VSMFHeader
{
  uint16_t versionID;
  uint16_t flags;
  uint16_t numMaterials;
  uint16_t numMeshes;
  uint16_t numTextures;
  uint16_t reserved;
  float minXYZ[3];
  float maxXYZ[3];
};

struct vcPolygonModelMaterial
{
  uint16_t flags;
  uint32_t colour; // bgra

  vcTexture *pTexture;
};

struct vcPolygonModelMesh
{
  uint16_t flags;
  uint16_t materialID;
  uint16_t LOD;
  uint16_t numVertices;
  uint16_t numElements;

  vcPolygonModelMaterial material; // TODO: materialID should reference a container of there. These should be shared between meshes, and rendering should be organized by material.
  vcMesh *pMesh;
};

struct vcPolygonModel
{
  int meshCount;
  vcPolygonModelMesh *pMeshes;
};

struct vcPolygonModelVertex
{
  udFloat3 pos;
  udFloat3 normal;
  udFloat2 uv;
};
const vcVertexLayoutTypes vcPolygonModelVertexLayout[] = { vcVLT_Position3, vcVLT_Normal3, vcVLT_TextureCoords2 };

udResult vcPolygonModel_CreateFromMemory(vcPolygonModel **ppModel, char *pData, int dataLength)
{
  if (pData == nullptr || (size_t)dataLength < sizeof(VSMFHeader))
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  vcPolygonModel *pNewModel = nullptr;
  char *pFilePos = pData;

  VSMFHeader header = {};

  UD_ERROR_IF(!udStrBeginsWith(pFilePos, "VSMF"), udR_InvalidParameter_);
  pFilePos += 4; // "VSMF"

  // Header
  UD_ERROR_CHECK(udReadFromPointer(&header, pFilePos, &dataLength));

  pNewModel = udAllocType(vcPolygonModel, 1, udAF_Zero);
  pNewModel->pMeshes = udAllocType(vcPolygonModelMesh, header.numMeshes, udAF_Zero);
  pNewModel->meshCount = header.numMeshes;

  // Materials
  for (int i = 0; i < header.numMaterials; ++i)
  {
    if (i >= header.numMeshes) // TODO: material count should not exceed mesh count?
    {
      pFilePos += 6;
      continue;
    }

    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].material.flags, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].material.colour, pFilePos, &dataLength));
  }

  // Vertex Block
  // Mesh
  for (int i = 0; i < header.numMeshes; ++i)
  {
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].flags, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].materialID, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].LOD, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].numVertices, pFilePos, &dataLength));
    UD_ERROR_CHECK(udReadFromPointer(&pNewModel->pMeshes[i].numElements, pFilePos, &dataLength));

    // override material id for now
    pNewModel->pMeshes[i].materialID = vcPMST_P1N1UV1;

    vcPolygonModelVertex *pVerts = (vcPolygonModelVertex*)pFilePos;
    pFilePos += sizeof(*pVerts) * pNewModel->pMeshes[i].numVertices;

    uint16_t *pIndices = (uint16_t*)pFilePos;
    pFilePos += sizeof(*pIndices) * pNewModel->pMeshes[i].numElements;

    for (int t = 0; t < header.numTextures; ++t)
    {
      uint32_t textureFileSize = 0;
      UD_ERROR_CHECK(udReadFromPointer(&textureFileSize, pFilePos, &dataLength));

      // TODO: only handle 1 texture at the moment
      if (t == 0)
      {
        void *pTextureData = pFilePos;
        vcTexture_CreateFromMemory(&pNewModel->pMeshes[i].material.pTexture, pTextureData, textureFileSize);
      }
      pFilePos += textureFileSize;
    }

    UD_ERROR_CHECK(vcMesh_Create(&pNewModel->pMeshes[i].pMesh, vcPolygonModelVertexLayout, (int)udLengthOf(vcPolygonModelVertexLayout), pVerts, pNewModel->pMeshes[i].numVertices, pIndices, pNewModel->pMeshes[i].numElements, vcMF_IndexShort));
  }

  *ppModel = pNewModel;
  pNewModel = nullptr;

epilogue:
  if (pNewModel)
  {
    for (int i = 0; i < pNewModel->meshCount; ++i)
      vcMesh_Destroy(&pNewModel->pMeshes[i].pMesh);

    udFree(pNewModel->pMeshes);
    udFree(pNewModel);
  }
  return result;
}

udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL)
{
  udResult result = udR_Failure_;
  void *pMemory = nullptr;
  int64_t fileLength = 0;

  UD_ERROR_CHECK(udFile_Load(pURL, &pMemory, &fileLength));
  UD_ERROR_CHECK(vcPolygonModel_CreateFromMemory(ppModel, (char*)pMemory, (int)fileLength));

epilogue:
  udFree(pMemory);

  return result;
}

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, vcTexture *pDiffuseOverride /*= nullptr*/)
{
  if (pModel == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  for (int i = 0; i < pModel->meshCount; ++i)
  {
    vcPolygonModelMesh *pModelMesh = &pModel->pMeshes[i];
    vcPolygonModelShader *pPolygonShader = &gShaders[pModelMesh->materialID];

    vcShader_Bind(pPolygonShader->pShader);

    pPolygonShader->everyFrame.u_viewProjectionMatrix = udFloat4x4::create(viewProjectionMatrix * modelMatrix);

    float s = 1.0f / 255.0f;
    udFloat4 colour = udFloat4::create(
      ((pModel->pMeshes[i].material.colour >> 8) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 16) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 24) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 0) & 0xFF) * s);

    pPolygonShader->everyObject.u_colour = colour;
    pPolygonShader->everyObject.u_world = udFloat4x4::identity();

    vcShader_BindConstantBuffer(pPolygonShader->pShader, pPolygonShader->pEveryObjectConstantBuffer, &pPolygonShader->everyObject, sizeof(vcPolygonModelShader::everyObject));
    vcShader_BindConstantBuffer(pPolygonShader->pShader, pPolygonShader->pEveryFrameConstantBuffer, &pPolygonShader->everyFrame, sizeof(vcPolygonModelShader::everyFrame));

    vcTexture *pDiffuseTexture = pModelMesh->material.pTexture;
    if (pDiffuseOverride)
      pDiffuseTexture = pDiffuseOverride;

    vcShader_BindTexture(pPolygonShader->pShader, pDiffuseTexture, 0, pPolygonShader->pDiffuseSampler);

    vcMesh_Render(pModelMesh->pMesh);
  }

  // todo: handle failures
  return result;
}

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel)
{
  if (ppModel == nullptr || *ppModel == nullptr)
    return udR_InvalidParameter_;

  vcPolygonModel *pModel = (*ppModel);

  for (int i = 0; i < pModel->meshCount; ++i)
  {
    vcTexture_Destroy(&pModel->pMeshes[i].material.pTexture);
    vcMesh_Destroy(&pModel->pMeshes[i].pMesh);
  }

  udFree(pModel->pMeshes);
  udFree(pModel);

  return udR_Success;
}


udResult vcPolygonModel_CreateShaders()
{
  ++gPolygonShaderRefCount;
  if (gPolygonShaderRefCount != 1)
    return udR_Success;

  udResult result = udR_Success;

  vcPolygonModelShader *pPolygonShader = &gShaders[vcPMST_P1N1UV1];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP1N1UV1VertexShader, g_PolygonP1N1UV1FragmentShader, vcPolygonModelVertexLayout, (uint32_t)udLengthOf(vcPolygonModelVertexLayout)), udR_InternalError);
  vcShader_GetConstantBuffer(&pPolygonShader->pEveryFrameConstantBuffer, pPolygonShader->pShader, "u_EveryFrame", sizeof(vcPolygonModelShader::everyFrame));
  vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject));
  vcShader_GetSamplerIndex(&pPolygonShader->pDiffuseSampler, pPolygonShader->pShader, "u_texture");

epilogue:
  return result;
}

udResult vcPolygonModel_DestroyShaders()
{
  --gPolygonShaderRefCount;
  if (gPolygonShaderRefCount != 0)
    return udR_Success;

  for (int i = 0; i < vcPMST_Count; ++i)
  {
    vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryObjectConstantBuffer);
    vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryFrameConstantBuffer);
    vcShader_DestroyShader(&gShaders[i].pShader);
  }

  return udR_Success;
}
