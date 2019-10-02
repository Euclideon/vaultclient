#include "vcPolygonModel.h"

#include "udPlatformUtil.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "gl/vcRenderShaders.h"
#include "gl/vcMesh.h"
#include "gl/vcTexture.h"

static int gPolygonShaderRefCount = 0;

enum vcPolygonModelShaderType
{
  vcPMST_P3N3UV2_Opaque,
  vcPMST_P3N3UV2_FlatColour,

  vcPMST_Count
};

static struct vcPolygonModelShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryFrameConstantBuffer;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_modelViewProjectionMatrix;
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

vcPolygonModelShaderType vcPolygonModel_GetShaderType(const vcVertexLayoutTypes *pMeshLayout, int totalTypes)
{
  if (totalTypes == 3 && (pMeshLayout[0] == vcVLT_Position3 && pMeshLayout[1] == vcVLT_Normal3 && pMeshLayout[2] == vcVLT_TextureCoords2))
    return vcPMST_P3N3UV2_Opaque;

  if (totalTypes == 4 && (pMeshLayout[0] == vcVLT_Position3 && pMeshLayout[1] == vcVLT_Normal3 && pMeshLayout[2] == vcVLT_TextureCoords2) && pMeshLayout[3] == vcVLT_ColourBGRA)
    return vcPMST_P3N3UV2_Opaque; // TODO: (EVC-540) Re-use for now, ignoring colour attribute

  if (totalTypes >= 3)
    return vcPMST_P3N3UV2_Opaque; // TODO: (EVC-540) Re-use for now, ignoring other attributes

  return vcPMST_Count;
}

udResult vcPolygonModel_CreateFromRawVertexData(vcPolygonModel **ppPolygonModel, const void *pVerts, const uint16_t vertCount, const vcVertexLayoutTypes *pMeshLayout, const int totalTypes, const uint16_t *pIndices /*= nullptr*/, const uint16_t indexCount /*= 0*/)
{
  if (ppPolygonModel == nullptr || pVerts == nullptr || pMeshLayout == nullptr || vertCount == 0 || totalTypes <= 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  vcPolygonModel *pPolygonModel = nullptr;

  pPolygonModel = udAllocType(vcPolygonModel, 1, udAF_Zero);
  UD_ERROR_NULL(pPolygonModel, udR_MemoryAllocationFailure);

  pPolygonModel->pMeshes = udAllocType(vcPolygonModelMesh, 1, udAF_Zero);
  UD_ERROR_NULL(pPolygonModel->pMeshes, udR_MemoryAllocationFailure);

  pPolygonModel->meshCount = 1;

  pPolygonModel->pMeshes[0].material.flags = 0;
  pPolygonModel->pMeshes[0].material.colour = 0xffffffff;

  // TODO: (EVC-570) Calculate and actually use flags
  pPolygonModel->pMeshes[0].flags = 0;//vcPMVF_Normals | vcPMVF_UVs;
  pPolygonModel->pMeshes[0].LOD = 0;
  pPolygonModel->pMeshes[0].numVertices = vertCount;
  pPolygonModel->pMeshes[0].numElements = indexCount;
  pPolygonModel->pMeshes[0].material.pTexture = nullptr;
  pPolygonModel->pMeshes[0].materialID = (uint16_t)vcPolygonModel_GetShaderType(pMeshLayout, totalTypes);

  // Check for unsupported vertex format
  if (pPolygonModel->pMeshes[0].materialID == vcPMST_Count)
    UD_ERROR_SET(udR_Unsupported);

  UD_ERROR_CHECK(vcMesh_Create(&pPolygonModel->pMeshes[0].pMesh, pMeshLayout, totalTypes, pVerts, pPolygonModel->pMeshes[0].numVertices, pIndices, indexCount, (pIndices == nullptr) ? vcMF_NoIndexBuffer : vcMF_IndexShort));

  *ppPolygonModel = pPolygonModel;

epilogue:
  if (result != udR_Success)
  {
    vcPolygonModel_Destroy(&pPolygonModel);
  }

  return result;
}


udResult vcPolygonModel_CreateFromVSMFInMemory(vcPolygonModel **ppModel, char *pData, int dataLength)
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

  if (header.versionID > 1)
    UD_ERROR_SET(udR_Unsupported);

  pNewModel = udAllocType(vcPolygonModel, 1, udAF_Zero);
  UD_ERROR_NULL(pNewModel, udR_MemoryAllocationFailure);

  pNewModel->pMeshes = udAllocType(vcPolygonModelMesh, header.numMeshes, udAF_Zero);
  UD_ERROR_NULL(pNewModel->pMeshes, udR_MemoryAllocationFailure);

  pNewModel->meshCount = header.numMeshes;

  // Materials
  for (int i = 0; i < header.numMaterials; ++i)
  {
    if (i >= header.numMeshes) // TODO: (EVC-570) material count should not exceed mesh count?
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
    pNewModel->pMeshes[i].materialID = vcPMST_P3N3UV2_Opaque;

    vcP3N3UV2Vertex *pVerts = (vcP3N3UV2Vertex *)pFilePos;
    pFilePos += sizeof(*pVerts) * pNewModel->pMeshes[i].numVertices;

    // TODO: Assume these all need flipping
    for (uint16_t v = 0; v < pNewModel->pMeshes[i].numVertices; ++v)
    {
      vcP3N3UV2Vertex *pVert = &pVerts[v];
      pVert->uv.y = 1.0f - pVert->uv.y;
    }

    uint16_t *pIndices = (uint16_t*)pFilePos;
    pFilePos += sizeof(*pIndices) * pNewModel->pMeshes[i].numElements;

    for (int t = 0; t < header.numTextures; ++t)
    {
      uint32_t textureFileSize = 0;
      UD_ERROR_CHECK(udReadFromPointer(&textureFileSize, pFilePos, &dataLength));

      // TODO: (EVC-570) only handle 1 texture at the moment
      if (t == 0)
      {
        void *pTextureData = pFilePos;
        if (!vcTexture_CreateFromMemory(&pNewModel->pMeshes[i].material.pTexture, pTextureData, textureFileSize))
        {
          // TODO: (EVC-570)
        }
      }
      pFilePos += textureFileSize;
    }

    UD_ERROR_CHECK(vcMesh_Create(&pNewModel->pMeshes[i].pMesh, vcP3N3UV2VertexLayout, (int)udLengthOf(vcP3N3UV2VertexLayout), pVerts, pNewModel->pMeshes[i].numVertices, pIndices, pNewModel->pMeshes[i].numElements, vcMF_IndexShort));
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
  udResult result;
  void *pMemory = nullptr;
  int64_t fileLength = 0;

  UD_ERROR_CHECK(udFile_Load(pURL, &pMemory, &fileLength));
  UD_ERROR_CHECK(vcPolygonModel_CreateFromVSMFInMemory(ppModel, (char*)pMemory, (int)fileLength));

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcPolygonModel_Destroy(ppModel);

  udFree(pMemory);
  return result;
}

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, const vcPolyModelPass &passType /*= vcPMP_Standard*/, vcTexture *pDiffuseOverride /*= nullptr*/, const udFloat4 *pColourOverride /*= nullptr*/)
{
  if (pModel == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  for (int i = 0; i < pModel->meshCount; ++i)
  {
    vcPolygonModelMesh *pModelMesh = &pModel->pMeshes[i];
    vcPolygonModelShader *pPolygonShader = &gShaders[pModelMesh->materialID];
    if (passType == vcPMP_ColourOnly)
      pPolygonShader = &gShaders[vcPMST_P3N3UV2_FlatColour];

    vcShader_Bind(pPolygonShader->pShader);

    pPolygonShader->everyFrame.u_modelViewProjectionMatrix = udFloat4x4::create(viewProjectionMatrix * modelMatrix);

    float s = 1.0f / 255.0f;
    udFloat4 colour = udFloat4::create(
      ((pModel->pMeshes[i].material.colour >> 8) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 16) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 24) & 0xFF) * s,
      ((pModel->pMeshes[i].material.colour >> 0) & 0xFF) * s);

    if (pColourOverride)
      colour = *pColourOverride;

    pPolygonShader->everyObject.u_colour = colour;
    pPolygonShader->everyObject.u_world = udFloat4x4::create(modelMatrix);

    vcShader_BindConstantBuffer(pPolygonShader->pShader, pPolygonShader->pEveryObjectConstantBuffer, &pPolygonShader->everyObject, sizeof(vcPolygonModelShader::everyObject));
    vcShader_BindConstantBuffer(pPolygonShader->pShader, pPolygonShader->pEveryFrameConstantBuffer, &pPolygonShader->everyFrame, sizeof(vcPolygonModelShader::everyFrame));

    vcTexture *pDiffuseTexture = pModelMesh->material.pTexture;
    if (pDiffuseOverride)
      pDiffuseTexture = pDiffuseOverride;

    vcShader_BindTexture(pPolygonShader->pShader, pDiffuseTexture, 0, pPolygonShader->pDiffuseSampler);

    vcMesh_Render(pModelMesh->pMesh);
  }

  // TODO: (EVC-570) handle failures
  return result;
}

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel)
{
  if (ppModel == nullptr || *ppModel == nullptr)
    return udR_InvalidParameter_;

  vcPolygonModel *pModel = (*ppModel);
  *ppModel = nullptr;

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
  udResult result;
  vcPolygonModelShader *pPolygonShader = nullptr;
  ++gPolygonShaderRefCount;

  UD_ERROR_IF(gPolygonShaderRefCount != 1, udR_Success);

  pPolygonShader = &gShaders[vcPMST_P3N3UV2_Opaque];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP3N3UV2VertexShader, g_PolygonP3N3UV2FragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryFrameConstantBuffer, pPolygonShader->pShader, "u_EveryFrame", sizeof(vcPolygonModelShader::everyFrame)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pPolygonShader->pDiffuseSampler, pPolygonShader->pShader, "u_texture"), udR_InternalError);

  pPolygonShader = &gShaders[vcPMST_P3N3UV2_FlatColour];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP3N3UV2VertexShader, g_FlatColour_FragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryFrameConstantBuffer, pPolygonShader->pShader, "u_EveryFrame", sizeof(vcPolygonModelShader::everyFrame)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject)), udR_InternalError);

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcPolygonModel_DestroyShaders();

  return result;
}

udResult vcPolygonModel_DestroyShaders()
{
  udResult result;
  --gPolygonShaderRefCount;

  UD_ERROR_IF(gPolygonShaderRefCount != 0, udR_Success);

  for (int i = 0; i < vcPMST_Count; ++i)
  {
    UD_ERROR_IF(!vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryObjectConstantBuffer), udR_InternalError);
    UD_ERROR_IF(!vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryFrameConstantBuffer), udR_InternalError);
    vcShader_DestroyShader(&gShaders[i].pShader);
  }

  result = udR_Success;

epilogue:
  return result;
}
