#include "vcPolygonModel.h"

#include "udPlatformUtil.h"
#include "udFile.h"
#include "udStringUtil.h"
#include "udWorkerPool.h"

#include "gl/vcRenderShaders.h"
#include "gl/vcMesh.h"
#include "gl/vcTexture.h"

#include "parsers/vcOBJ.h"

static int gPolygonShaderRefCount = 0;

const vcVertexLayoutTypes *pDefaultMeshLayout = vcP3N3UV2VertexLayout;
const int DefaultTotalTypes = (int)udLengthOf(vcP3N3UV2VertexLayout);

vcTexture *pWhiteTexture = nullptr;

enum
{
  MaxTextureSize = 1024, // any texture dimension above this will be resized down to this
};

enum vcPolygonModelShaderType
{
  vcPMST_P3N3UV2_Opaque,
  vcPMST_P3N3UV2_FlatColour,
  vcPMST_P3N3UV2_DepthOnly,

  vcPMST_Count
};

static struct vcPolygonModelShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_worldViewProjectionMatrix;
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

struct AsyncPolygonModelLoadInfo
{
  // todo add udResult here
  vcPolygonModel **ppPolygonModel;
  const char *pURL;

  udWorkerPool *pWorkerPool;
};

struct AsyncPolygonModelLoadMeshInfo
{
  vcPolygonModel *pPolygonModel;
  vcPolygonModelMesh *pMesh;
  vcP3N3UV2Vertex *pVerts;
};

void vcPolygonModel_MainThreadCreateMesh(void *pData)
{
  AsyncPolygonModelLoadMeshInfo *pLoadInfo = (AsyncPolygonModelLoadMeshInfo *)pData;
  if (!pLoadInfo->pPolygonModel->keepLoading)
    return;

  vcMesh_Create(&pLoadInfo->pMesh->pMesh, pDefaultMeshLayout, DefaultTotalTypes, pLoadInfo->pVerts, pLoadInfo->pMesh->numVertices, nullptr, 0, vcMF_NoIndexBuffer);

  udFree(pLoadInfo->pVerts);
}

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

  pPolygonModel->modelOffset = udDouble4x4::identity();

  // Check for unsupported vertex format
  if (pPolygonModel->pMeshes[0].materialID == vcPMST_Count)
    UD_ERROR_SET(udR_Unsupported);

  UD_ERROR_CHECK(vcMesh_Create(&pPolygonModel->pMeshes[0].pMesh, pMeshLayout, totalTypes, pVerts, pPolygonModel->pMeshes[0].numVertices, pIndices, indexCount, (pIndices == nullptr) ? vcMF_NoIndexBuffer : vcMF_IndexShort));

  *ppPolygonModel = pPolygonModel;
  pPolygonModel = nullptr;

epilogue:
  if (pPolygonModel != nullptr)
    vcPolygonModel_Destroy(&pPolygonModel);

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

  pNewModel->keepLoading = true;
  pNewModel->meshCount = header.numMeshes;
  pNewModel->modelOffset = udDouble4x4::identity();

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
    uint16_t valueU16 = 0;

    UD_ERROR_CHECK(udReadFromPointer(&valueU16, pFilePos, &dataLength));
    pNewModel->pMeshes[i].flags = valueU16;

    UD_ERROR_CHECK(udReadFromPointer(&valueU16, pFilePos, &dataLength));
    pNewModel->pMeshes[i].materialID = valueU16;

    UD_ERROR_CHECK(udReadFromPointer(&valueU16, pFilePos, &dataLength));
    pNewModel->pMeshes[i].LOD = valueU16;

    UD_ERROR_CHECK(udReadFromPointer(&valueU16, pFilePos, &dataLength));
    pNewModel->pMeshes[i].numVertices = valueU16;

    UD_ERROR_CHECK(udReadFromPointer(&valueU16, pFilePos, &dataLength));
    pNewModel->pMeshes[i].numElements = valueU16;

    // override material id for now
    pNewModel->pMeshes[i].materialID = vcPMST_P3N3UV2_Opaque;

    vcP3N3UV2Vertex *pVerts = (vcP3N3UV2Vertex *)pFilePos;
    pFilePos += sizeof(*pVerts) * pNewModel->pMeshes[i].numVertices;

    // TODO: Assume these all need flipping
    for (uint32_t v = 0; v < pNewModel->pMeshes[i].numVertices; ++v)
    {
      vcP3N3UV2Vertex *pVert = &pVerts[v];
      pVert->uv.y = 1.0f - pVert->uv.y;
    }

    uint16_t *pIndices = (uint16_t *)pFilePos;
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

udResult vcPolygonModel_CreateFromOBJ(vcPolygonModel **ppPolygonModel, const char *pFilepath, udWorkerPool *pWorkerPool)
{
  udResult result;
  vcPolygonModel *pPolygonModel = nullptr;
  vcOBJ *pOBJReader = nullptr;
  udChunkedArray<vcOBJ::Face::Vert> *pMaterialVerticesMap = nullptr;
  uint32_t internalMeshIndex = 0;

  UD_ERROR_NULL(ppPolygonModel, udR_InvalidParameter_);
  UD_ERROR_NULL(pFilepath, udR_InvalidParameter_);

  pPolygonModel = udAllocType(vcPolygonModel, 1, udAF_Zero);
  UD_ERROR_NULL(pPolygonModel, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcOBJ_Load(&pOBJReader, pFilepath));

  // always have one
  if (pOBJReader->materials.length == 0)
  {
    vcOBJ::Material *pMat = pOBJReader->materials.PushBack();
    pMat->Ka = udFloat3::create(1.f);
    pMat->Kd = udFloat3::create(1.f);
  }

  // just pick the first vert as the origin
  pPolygonModel->origin = pOBJReader->positions[pOBJReader->faces[0].verts[0].pos];
  pPolygonModel->modelOffset = udDouble4x4::translation(pPolygonModel->origin);

  // preprocess vertices into appropriate material bins
  pMaterialVerticesMap = udAllocType(udChunkedArray<vcOBJ::Face::Vert>, pOBJReader->materials.length, udAF_Zero);
  for (size_t i = 0; i < pOBJReader->materials.length; ++i)
    pMaterialVerticesMap[i].Init(1 << 16); // assumption

  for (uint32_t f = 0; f < pOBJReader->faces.length; ++f)
  {
    vcOBJ::Face *pFace = &pOBJReader->faces[f];
    if (pFace->mat == -1)
      continue;

    pMaterialVerticesMap[pFace->mat].PushBack(pFace->verts[0]);
    pMaterialVerticesMap[pFace->mat].PushBack(pFace->verts[1]);
    pMaterialVerticesMap[pFace->mat].PushBack(pFace->verts[2]);
  }

  // count valid meshes (e.g. has vertices)
  for (size_t material = 0; material < pOBJReader->materials.length; ++material)
  {
    if (pMaterialVerticesMap[material].length != 0)
      ++pPolygonModel->meshCount;
  }

  pPolygonModel->pMeshes = udAllocType(vcPolygonModelMesh, pPolygonModel->meshCount, udAF_Zero);
  UD_ERROR_NULL(pPolygonModel->pMeshes, udR_MemoryAllocationFailure);

  // Assign before worker thread hand off
  pPolygonModel->keepLoading = true;
  *ppPolygonModel = pPolygonModel;

  // build meshes
  for (size_t material = 0; material < pOBJReader->materials.length; ++material)
  {
    udChunkedArray<vcOBJ::Face::Vert> *pMaterialVertexList = &pMaterialVerticesMap[material];
    if (pMaterialVertexList->length == 0)
      continue;

    vcOBJ::Material *pMaterial = &pOBJReader->materials[material];
    vcPolygonModelMesh *pMesh = &pPolygonModel->pMeshes[internalMeshIndex];

    pMesh->numVertices = (uint32_t)pMaterialVertexList->length;

    vcP3N3UV2Vertex *pVerts = udAllocType(vcP3N3UV2Vertex, pMesh->numVertices, udAF_Zero);
    UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

    for (uint32_t f = 0; f < pMesh->numVertices / 3; ++f)
    {
      for (int i = 0; i < 3; ++i)
      {
        uint32_t index = i + f * 3;
        vcOBJ::Face::Vert *pVertex = pMaterialVertexList->GetElement(index);

        // store every position relative to model origin
        pVerts[index].position = udFloat3::create(pOBJReader->positions[pVertex->pos] - pPolygonModel->origin);

        // TODO: Better handle meshes with different vertex layouts
        if (pVertex->nrm >= 0)
          pVerts[index].normal = udFloat3::create(pOBJReader->normals[pVertex->nrm]);
        else
          pVerts[index].normal = udFloat3::create(0.0f, 0.0f, 1.0f);

        // NOTE: inverted y
        if (pVertex->uv >= 0)
          pVerts[index].uv = udFloat2::create(pOBJReader->uvs[pVertex->uv].x, 1.0f - pOBJReader->uvs[pVertex->uv].y);
      }
    }

    // BGRA
    pMesh->material.colour = 0x000000ff | (uint32_t(pMaterial->Kd.x * 0xff) << 8) | (uint32_t(pMaterial->Kd.y * 0xff) << 16) | (uint32_t(pMaterial->Kd.z * 0xff) << 24);
    pMesh->material.pName = udStrdup(pMaterial->name);

    // TODO: (EVC-570) Calculate and actually use flags
    pMesh->flags = 0;//vcPMVF_Normals | vcPMVF_UVs;
    pMesh->LOD = 0;
    pMesh->numElements = 0;
    pMesh->materialID = (uint16_t)vcPolygonModel_GetShaderType(pDefaultMeshLayout, DefaultTotalTypes);\

    // Check for unsupported vertex format
    if (pPolygonModel->pMeshes[0].materialID == vcPMST_Count)
      UD_ERROR_SET(udR_Unsupported);

    const char *pTextureFilepath = udTempStr("%s%s", pOBJReader->basePath.GetPath(), pMaterial->map_Kd);
    vcTexture_AsyncCreateFromFilename(&pMesh->material.pTexture, pWorkerPool, pTextureFilepath, vcTFM_Linear, false, vcTWM_Repeat, MaxTextureSize);
   //pPolygonModel->assetLoadRemaining++; // texture

    AsyncPolygonModelLoadMeshInfo *pLoadInfo = udAllocType(AsyncPolygonModelLoadMeshInfo, 1, udAF_Zero);
    UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

    pLoadInfo->pPolygonModel = pPolygonModel;
    pLoadInfo->pMesh = pMesh;
    pLoadInfo->pVerts = pVerts;

    if (udWorkerPool_AddTask(pWorkerPool, nullptr, pLoadInfo, true, vcPolygonModel_MainThreadCreateMesh) != udR_Success)
    {
      udFree(pVerts);
      udFree(pLoadInfo);
    }

    ++internalMeshIndex;
  }

  pPolygonModel = nullptr;
  result = udR_Success;

epilogue:
  if (pOBJReader != nullptr)
  {
    for (size_t i = 0; i < pOBJReader->materials.length; ++i)
      pMaterialVerticesMap[i].Deinit();
  }

  udFree(pMaterialVerticesMap);

  if (pPolygonModel != nullptr)
  {
    // TODO: This is not a complete cleanup in some circumstances. (I've assumed failure at a certain point)
    udFree(pPolygonModel->pMeshes);
    udFree(pPolygonModel);
    *ppPolygonModel = nullptr;
  }

  vcOBJ_Destroy(&pOBJReader);
  return result;
}

udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL, udWorkerPool *pWorkerPool)
{
  udResult result;
  udFilename fn(pURL);

  if (udStrEquali(fn.GetExt(), ".obj"))
  {
    UD_ERROR_CHECK(vcPolygonModel_CreateFromOBJ(ppModel, pURL, pWorkerPool));
  }
  else if (udStrEquali(fn.GetExt(), ".vsm"))
  {
    void *pMemory = nullptr;
    int64_t fileLength = 0;

    UD_ERROR_CHECK(udFile_Load(pURL, &pMemory, &fileLength));
    UD_ERROR_CHECK(vcPolygonModel_CreateFromVSMFInMemory(ppModel, (char *)pMemory, (int)fileLength));

    udFree(pMemory);
  }
  else
  {
    UD_ERROR_SET(udR_Unsupported);
  }

  result = udR_Success;

epilogue:
  //if (result != udR_Success)
   // vcPolygonModel_Destroy(ppModel);

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
    vcTexture *pDiffuseTexture = pModelMesh->material.pTexture;

    if (pModelMesh->pMesh == nullptr)// || pModelMesh->material.pTexture == nullptr)
      continue;

    if (pDiffuseOverride)
      pDiffuseTexture = pDiffuseOverride;

    if (pDiffuseTexture == nullptr)
      pDiffuseTexture = pWhiteTexture;

    // conditionally override
    if (passType == vcPMP_ColourOnly)
      pPolygonShader = &gShaders[vcPMST_P3N3UV2_FlatColour];
    else if (passType == vcPMP_Shadows)
      pPolygonShader = &gShaders[vcPMST_P3N3UV2_DepthOnly];

    vcShader_Bind(pPolygonShader->pShader);

    float s = 1.0f / 255.0f;
    udFloat4 colour = udFloat4::create(
      ((pModel->pMeshes[i].material.colour >> 8) & 0xFF) *s,
      ((pModel->pMeshes[i].material.colour >> 16) & 0xFF) *s,
      ((pModel->pMeshes[i].material.colour >> 24) & 0xFF) *s,
      ((pModel->pMeshes[i].material.colour >> 0) & 0xFF) *s);

    if (pColourOverride)
      colour = *pColourOverride;

    pPolygonShader->everyObject.u_colour = colour;
    pPolygonShader->everyObject.u_world = udFloat4x4::create(modelMatrix);
    pPolygonShader->everyObject.u_worldViewProjectionMatrix = udFloat4x4::create(viewProjectionMatrix * modelMatrix * pModel->modelOffset);

    vcShader_BindConstantBuffer(pPolygonShader->pShader, pPolygonShader->pEveryObjectConstantBuffer, &pPolygonShader->everyObject, sizeof(vcPolygonModelShader::everyObject));
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

  pModel->keepLoading = false;

  for (int i = 0; i < pModel->meshCount; ++i)
  {
    vcPolygonModelMesh *pMesh = &pModel->pMeshes[i];
    vcTexture_Destroy(&pMesh->material.pTexture);
    vcMesh_Destroy(&pMesh->pMesh);
    udFree(pMesh->material.pName);
  }

  udFree(pModel->pMeshes);
  udFree(pModel);

  return udR_Success;
}

udResult vcPolygonModel_CreateShaders()
{
  udResult result;
  vcPolygonModelShader *pPolygonShader = nullptr;
  const uint32_t WhitePixel = 0xFFFFFFFF;
  ++gPolygonShaderRefCount;

  UD_ERROR_IF(gPolygonShaderRefCount != 1, udR_Success);

  pPolygonShader = &gShaders[vcPMST_P3N3UV2_Opaque];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP3N3UV2VertexShader, g_PolygonP3N3UV2FragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pPolygonShader->pDiffuseSampler, pPolygonShader->pShader, "u_texture"), udR_InternalError);

  pPolygonShader = &gShaders[vcPMST_P3N3UV2_FlatColour];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP3N3UV2VertexShader, g_FlatColour_FragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject)), udR_InternalError);

  pPolygonShader = &gShaders[vcPMST_P3N3UV2_DepthOnly];
  UD_ERROR_IF(!vcShader_CreateFromText(&pPolygonShader->pShader, g_PolygonP3N3UV2VertexShader, g_DepthOnly_FragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pPolygonShader->pEveryObjectConstantBuffer, pPolygonShader->pShader, "u_EveryObject", sizeof(vcPolygonModelShader::everyObject)), udR_InternalError);

  UD_ERROR_CHECK(vcTexture_Create(&pWhiteTexture, 1, 1, &WhitePixel));

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
    vcShader_DestroyShader(&gShaders[i].pShader);
  }

  vcTexture_Destroy(&pWhiteTexture);

  result = udR_Success;

epilogue:
  return result;
}
