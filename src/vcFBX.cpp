#ifdef FBXSDK_ON

#include "udPlatform.h"

#if UDPLATFORM_MACOSX
# pragma GCC diagnostic ignored "-Wpragma-pack"
# pragma clang diagnostic ignored "-Wpragma-pack"
#endif

#include "fbxsdk.h"

#include "stb_image.h"

#include "udChunkedArray.h"
#include "udFile.h"
#include "udStringUtil.h"
#include "udMath.h"
#include "udPlatformUtil.h"

#include "vcFBX.h"
#include "vdkTriangleVoxelizer.h"

struct vcFBXTexture
{
  FbxFileTexture *pTex;
  fbxsdk::FbxLayerElement::EMappingMode map;
  fbxsdk::FbxLayerElement::EReferenceMode ref;
  const char *pFilename;
  const char *pName;

  uint32_t diffuse;

  uint32_t *pPixels;
  int width;
  int height;
  int channels;

  FbxDouble3 scale;
  FbxDouble3 translate;
  bool swap;

  bool layered;
  FbxLayerElementArrayTemplate<int> *pIArray;
  FbxLayerElementArrayTemplate<FbxVector2> *pDArray;
};

struct vcFBXMaterial
{
  udChunkedArray<vcFBXTexture> textures;
};

struct vcFBX
{
  FbxManager *pManager;

  FbxScene *pScene; // Pointer to the scene containing the imported .fbx model
  FbxNode *pNode; // Pointer to the current mesh node
  FbxMesh *pMesh; // Pointer to the current mesh

  FbxVector4 *pPoints;
  int *pIndices;

  double pointResolution;
  vdkConvertCustomItemFlags convertFlags;
  uint32_t everyNth;
  uint32_t everyNthAccum;

  vdkTriangleVoxelizer *pTrivox;
  double *pTriPositions;
  udDouble3 *pTriWeights;
  uint64_t pointsReturned;

  uint32_t currMeshPointCount; // Number of points processed so far in the current mesh
  uint32_t currPointCount; // Total number of points processed so far for all meshes
  uint32_t totalMeshPoints; // Number of points in the current mesh
  uint32_t currMesh; // Number of current mesh
  uint32_t totalMeshes; // Total number of meshes
  uint32_t currMeshPolygon; // Number of polygons processed so far in the current mesh
  uint32_t currProcessedPolygons;
  uint32_t totalMeshPolygons; // Number of polygons in the current mesh
  uint32_t totalPolygons;
  uint32_t lastTouchedPoly;
  uint32_t lastTouchedMesh;

  udChunkedArray<vcFBXMaterial> materials;
  udChunkedArray<udDouble2> uvQueue;

  FbxLayerElementArrayTemplate<int> *pIndex;
  fbxsdk::FbxLayerElement::EMappingMode map;
  fbxsdk::FbxLayerElement::EReferenceMode ref;
};

inline void vcFBX_Alpha(uint8_t *pDest, const uint8_t *pSrc, bool premultiplied)
{
  double multiply = premultiplied ? 1 : pSrc[3] / 0xFF;
  pDest[0] = (uint8_t)udMin((int)(pDest[0] * ((0xFF - pSrc[0]) / 0xFF) + pSrc[0] * multiply), 0xFF);
  pDest[1] = (uint8_t)udMin((int)(pDest[1] * ((0xFF - pSrc[1]) / 0xFF) + pSrc[1] * multiply), 0xFF);
  pDest[2] = (uint8_t)udMin((int)(pDest[2] * ((0xFF - pSrc[2]) / 0xFF) + pSrc[2] * multiply), 0xFF);
  pDest[3] = (uint8_t)udMin((int)(pDest[3] * ((0xFF - pSrc[3]) / 0xFF) + pSrc[3]), 0xFF);
}
inline void vcFBX_Multiply(uint8_t *pDest, const uint8_t *pSrc)
{
  pDest[0] = (uint8_t)udMin((pDest[0] / 0xFF) * (pSrc[0] / 0xFF), 0xFF);
  pDest[1] = (uint8_t)udMin((pDest[1] / 0xFF) * (pSrc[1] / 0xFF), 0xFF);
  pDest[2] = (uint8_t)udMin((pDest[2] / 0xFF) * (pSrc[2] / 0xFF), 0xFF);
  pDest[3] = (uint8_t)udMin((pDest[3] / 0xFF) * (pSrc[3] / 0xFF), 0xFF);
}
inline void vcFBX_Additive(uint8_t *pDest, const uint8_t *pSrc, bool premultiplied)
{
  double multiply = premultiplied ? 1 : pSrc[3] / 0xFF;
  pDest[0] = (uint8_t)udMin(pDest[0] + (uint8_t)(pSrc[0] * multiply), 0xFF);
  pDest[1] = (uint8_t)udMin(pDest[1] + (uint8_t)(pSrc[1] * multiply), 0xFF);
  pDest[2] = (uint8_t)udMin(pDest[2] + (uint8_t)(pSrc[2] * multiply), 0xFF);
}

// Loads a single file texture if it expects to be used in a 'correct' way
void vcFBX_FileTexture(vcFBX *pFBX, FbxFileTexture *pTexture, vcFBXMaterial *pMaterial, fbxsdk::FbxTexture::EBlendMode blend)
{
  vcFBXTexture texture = {};

  FbxTexture::EMappingType map = pTexture->GetMappingType();
  FbxTexture::ETextureUse use = pTexture->GetTextureUse();
  FbxString UVSet = pTexture->UVSet.Get();

  if (map != FbxTexture::eUV || use != FbxTexture::eStandard)
    return;

  texture.layered = (blend != FbxTexture::eTranslucent);
  texture.pName = udStrdup(UVSet);

  texture.scale = pTexture->Scaling;
  texture.translate = pTexture->Translation;
  texture.swap = pTexture->GetSwapUV();

  const FbxGeometryElementUV *pUV = pFBX->pMesh->GetElementUV(UVSet);
  if (pUV == nullptr)
    pUV = pFBX->pMesh->GetElementUV(0);

  texture.map = pUV->GetMappingMode();
  texture.ref = pUV->GetReferenceMode();
  texture.pFilename = pTexture->GetFileName();
  texture.pTex = pTexture;

  texture.pIArray = &pUV->GetIndexArray();
  texture.pDArray = &pUV->GetDirectArray();

  if (texture.pFilename != nullptr && !udStrEqual(texture.pFilename, ""))
  {
    for (uint32_t i = 0; i < pFBX->materials.length; ++i)
    {
      for (uint32_t j = 0; j < pFBX->materials[i].textures.length; ++j)
      {
        if (udStrEquali(texture.pFilename, pFBX->materials[i].textures[j].pFilename))
        {
          texture.pPixels = pFBX->materials[i].textures[j].pPixels;
          texture.width = pFBX->materials[i].textures[j].width;
          texture.height = pFBX->materials[i].textures[j].height;
          texture.channels = pFBX->materials[i].textures[j].channels;
          pMaterial->textures.PushBack(texture);
          return;
        }
      }
    }

    void *pMem = nullptr;
    int64_t fileLength;

    udResult result = udFile_Load(texture.pFilename, &pMem, &fileLength);
    if (result == udR_Success)
    {
      texture.pPixels = (uint32_t*)stbi_load_from_memory((const stbi_uc*)pMem, (int)fileLength, &texture.width, &texture.height, &texture.channels, 4);

      udFree(pMem);
      pMaterial->textures.PushBack(texture);
    }
  }

  return;
}

// Gets all textures associated with a Mesh
void vcFBX_GetTextures(vcFBX *pFBX, FbxNode *pNode)
{
  // Process materials to prepare colour handling
  int totalMats = pNode->GetMaterialCount();

  FbxMesh *pMesh = pNode->GetMesh();
  pFBX->map = pMesh->GetElementMaterial()->GetMappingMode();
  pFBX->ref = pMesh->GetElementMaterial()->GetReferenceMode();
  pFBX->pIndex = &pMesh->GetElementMaterial()->GetIndexArray();

  for (int i = 0; i < totalMats; ++i)
  {
    FbxSurfaceMaterial *pMaterial = pFBX->pNode->GetMaterial(i);
    if (!pMaterial)
      continue;

    vcFBXMaterial mat = {};
    mat.textures.Init(4);

    FbxProperty diffuse = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);

    if (diffuse.IsValid())
    {
      if (diffuse.GetSrcObjectCount<FbxTexture>() > 0)
      {
        for (int j = 0; j < diffuse.GetSrcObjectCount<FbxFileTexture>(); ++j)
        {
          FbxFileTexture *pTexture = diffuse.GetSrcObject<FbxFileTexture>(j);
          vcFBX_FileTexture(pFBX, pTexture, &mat, FbxTexture::eTranslucent);
        }
        for (int j = 0; j < diffuse.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
        {
          FbxLayeredTexture *pLayer = diffuse.GetSrcObject<FbxLayeredTexture>(j);

          for (int k = 0; k < pLayer->GetSrcObjectCount<FbxFileTexture>(); ++k)
          {
            vcFBX_FileTexture(pFBX, pLayer->GetSrcObject<FbxFileTexture>(k), &mat, pLayer->GetBlendMode());
          }
        }
      }
      else
      {
        vcFBXTexture texture = {};
        texture.pTex = nullptr;

        FbxDataType type = diffuse.GetPropertyDataType();

        if (FbxDouble3DT == type || FbxColor3DT == type || FbxColor4DT == type || FbxDouble4DT == type)
        {
          const FbxColor col = diffuse.Get<FbxColor>();

          texture.diffuse |= (((uint8_t)(col.mBlue * 0xFF)) << 16);
          texture.diffuse |= (((uint8_t)(col.mGreen * 0xFF)) << 8);
          texture.diffuse |= ((uint8_t)(col.mRed * 0xFF));

          texture.diffuse |= 0xFF000000;

          mat.textures.PushBack(texture);
        }
      }
    }

    // Need filler mats even if no textures to preserve material indices
    pFBX->materials.PushBack(mat);
  }
}

FbxAMatrix vcFBX_GetGeometryTransformation(FbxNode *inNode)
{
  const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
  const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
  const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

  return FbxAMatrix(lT, lR, lS);
}

void vcFBX_CleanMaterials(udChunkedArray<vcFBXMaterial> *pMats)
{
  // Clean up
  for (uint32_t i = 0; i < pMats->length; ++i)
  {
    for (uint32_t j = 0; j < (*pMats)[i].textures.length; ++j)
    {
      if ((*pMats)[i].textures[j].pPixels)
      {
        stbi_image_free((*pMats)[i].textures[j].pPixels);

        // Clean up all references to this allocation
        void *pPtr = (*pMats)[i].textures[j].pPixels;
        for (uint32_t k = i; k < pMats->length; ++k)
        {
          for (uint32_t l = j; l < (*pMats)[k].textures.length; ++l)
          {
            if (pPtr == (*pMats)[k].textures[l].pPixels)
              (*pMats)[k].textures[l].pPixels = nullptr;
          }
        }
      }
      udFree((*pMats)[i].textures[j].pName);
    }
    (*pMats)[i].textures.Clear();
    (*pMats)[i].textures.Deinit();
  }

  pMats->Clear();
  pMats->Deinit();
}

vdkError vcFBX_Open(vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, vdkConvertCustomItemFlags flags)
{
  udUnused(origin);

  vdkError result = vE_Failure;
  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;

  memset(pFBX, 0, sizeof(vcFBX));
  pFBX->pManager = FbxManager::Create();
  pFBX->materials.Init(4);
  pFBX->uvQueue.Init(4);

  // Configure IO Settings
  FbxIOSettings *pIOS = FbxIOSettings::Create(pFBX->pManager, IOSROOT);
  pIOS->SetBoolProp(IMP_FBX_LINK, false);
  pIOS->SetBoolProp(IMP_FBX_SHAPE, false);
  pIOS->SetBoolProp(IMP_FBX_GOBO, false);
  pIOS->SetBoolProp(IMP_FBX_ANIMATION, false);
  pFBX->pManager->SetIOSettings(pIOS);
  FbxImporter *pImporter = FbxImporter::Create(pFBX->pManager, "");
  FbxGeometryConverter geoCon = FbxGeometryConverter(pFBX->pManager);
  fbxsdk::FbxMaterialConverter matCon = fbxsdk::FbxMaterialConverter(*pFBX->pManager);

  const fbxsdk::FbxSystemUnit::ConversionOptions conversionOptions = {
    false, /* mConvertRrsNodes */
    true, /* mConvertAllLimits */
    true, /* mConvertClusters */
    true, /* mConvertLightIntensity */
    true, /* mConvertPhotometricLProperties */
    true  /* mConvertCameraClipPlanes */
  };

  // Import File
  UD_ERROR_IF(!pImporter->Initialize(pConvertInput->pName, -1, pFBX->pManager->GetIOSettings()), vE_ReadFailure);
  pFBX->pScene = FbxScene::Create(pFBX->pManager, "");

  matCon.ConnectTexturesToMaterials(*pFBX->pScene);

  UD_ERROR_IF(!pImporter->Import(pFBX->pScene), vE_OpenFailure);
  pImporter->Destroy(); // Once file is imported, importer can be destroyed

  // If model isn't already scaled to metres, resize model
  if (pFBX->pScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
    FbxSystemUnit::m.ConvertScene(pFBX->pScene, conversionOptions);

  pConvertInput->pointCountIsEstimate = true;
  pConvertInput->sourceResolution = pointResolution;

  pFBX->everyNth = udMax(everyNth, (uint32_t)1);
  pFBX->convertFlags = flags;

  pFBX->totalMeshes = pFBX->pScene->GetGeometryCount();
  UD_ERROR_IF(pFBX->totalMeshes < 1, vE_NotFound);

  UD_ERROR_IF(!geoCon.Triangulate(pFBX->pScene, true), vE_Failure);

  for (uint16_t i = 0; i < pFBX->totalMeshes; ++i)
  {
    pFBX->pNode = pFBX->pScene->GetGeometry(i)->GetNode();

    if (pFBX->pNode == nullptr)
      continue;

    pFBX->pMesh = pFBX->pNode->GetMesh();

    if (pFBX->pMesh == nullptr)
      continue;

    pFBX->totalPolygons += pFBX->pMesh->GetPolygonCount();
  }

  pFBX->lastTouchedPoly = 1; // Forces handling 0, then gets set to 0
  pFBX->lastTouchedMesh = 1;

  UD_ERROR_CHECK(vdkTriangleVoxelizer_Create(&pFBX->pTrivox, pointResolution));

  result = vE_Success;

epilogue:

  if (result != vE_Success)
  {
    pFBX->pManager->Destroy();
    udFree(pFBX);
    pFBX = nullptr;
  }

  return result;
}

vdkError vcFBX_ReadPointsInt(vdkConvertCustomItem *pConvertInput, vdkPointBufferI64 *pBuffer)
{
  if (pBuffer == nullptr || pBuffer->pAttributes == nullptr || pConvertInput == nullptr || pConvertInput->pData == nullptr)
    return vE_InvalidParameter;

  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
  vdkError result = vE_Failure;

  pBuffer->pointCount = 0;
  memset(pBuffer->pAttributes, 0, pBuffer->attributeStride * pBuffer->pointsAllocated);

  // For each mesh
  while (pFBX->currMesh < pFBX->totalMeshes)
  {
    // Handle new mesh
    if (pFBX->currMesh != pFBX->lastTouchedMesh)
    {
      pFBX->lastTouchedMesh = pFBX->currMesh;
      pFBX->pNode = pFBX->pScene->GetGeometry(pFBX->currMesh)->GetNode();
      
      if (pFBX->pNode == nullptr)
      {
        ++pFBX->currMesh;
        continue;
      }

      pFBX->pMesh = pFBX->pNode->GetMesh();

      if (pFBX->pMesh == nullptr)
      {
        ++pFBX->currMesh;
        continue;
      }

      pFBX->pMesh->SplitPoints(FbxLayerElement::eTextureDiffuse);

      // https://www.gamedev.net/forums/topic/698619-fbx-sdk-skinned-animation/
      // The global node transform is equal to your local skeleton root if there is no parent bone
      FbxAMatrix LocalTransform = pFBX->pNode->EvaluateGlobalTransform();

      FbxNodeAttribute* Attribute = pFBX->pNode->GetNodeAttribute();
      if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
      {
        // Do we have a parent bone, if yes then evaluate its global transform and apply the inverse to this nodes global transform
        if (FbxNode* Parent = pFBX->pNode->GetParent())
        {
          FbxNodeAttribute* ParentAttribute = Parent->GetNodeAttribute();
          if (ParentAttribute && ParentAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
          {
            FbxAMatrix GlobalParentTransform = Parent->EvaluateGlobalTransform();
            LocalTransform = GlobalParentTransform.Inverse() * LocalTransform;
          }
        }
      }

      LocalTransform = vcFBX_GetGeometryTransformation(pFBX->pNode) * LocalTransform;

      pFBX->pMesh->SetPivot(LocalTransform);
      pFBX->pMesh->ApplyPivot();

      pFBX->totalMeshPolygons = pFBX->pMesh->GetPolygonCount();
      pFBX->totalMeshPoints = pFBX->pMesh->GetPolygonVertexCount();
      pFBX->currMeshPointCount = 0;
      pFBX->currMeshPolygon = 0;
      pFBX->pPoints = pFBX->pMesh->GetControlPoints();
      pFBX->pIndices = pFBX->pMesh->GetPolygonVertices();

      // Colour processing
      if (pConvertInput->attributes.standardContent & vdkSAC_ARGB)
        vcFBX_GetTextures(pFBX, pFBX->pNode);

    } // End new mesh handling

    // Vertex only case makes a point per vertex
    if (pFBX->convertFlags & vdkCCIF_PolygonVerticesOnly)
    {
      // For each vertex
      while (pFBX->currMeshPointCount < pFBX->totalMeshPoints)
      {
        uint8_t *pAttr = &pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride];

        // Write point to buffer
        FbxVector4 currPoint = pFBX->pPoints[pFBX->pIndices[pFBX->currMeshPointCount]];
        for (int coord = 0; coord < 3; ++coord)
          pBuffer->pPositions[pBuffer->pointCount * 3 + coord] = (int64_t)((currPoint[coord] - pConvertInput->boundMin[coord]) / pConvertInput->sourceResolution);

        vcFBXMaterial *pMat = nullptr;

        // Handle colour
        if (pConvertInput->attributes.standardContent & vdkSAC_ARGB && pFBX->materials.length > 0) // must be at least one material
        {
          uint32_t colour = 0;
          uint32_t thisColour = 0;

          uint32_t index = 0;

          {
            switch (pFBX->map)
            {
            case fbxsdk::FbxLayerElement::eByPolygonVertex: // falls through
            case fbxsdk::FbxLayerElement::eByControlPoint:
              index = pFBX->pIndex->GetAt(pFBX->currMeshPolygon * 3);
              break;
            case fbxsdk::FbxLayerElement::eByPolygon:
              index = pFBX->pIndex->GetAt(pFBX->currMeshPolygon);
              break;
            case fbxsdk::FbxLayerElement::eByEdge: // falls through
            case fbxsdk::FbxLayerElement::eAllSame:
              index = (uint32_t)pFBX->materials.length - 1;
              break;
            case fbxsdk::FbxLayerElement::eNone:
              break;
            }
          }

          if (index < pFBX->materials.length)
            pMat = &pFBX->materials[index];

          UDASSERT(pMat != nullptr, "Material index incorrectly looked up, or FBX file corrupted");

          for (uint32_t tex = 0; tex < pMat->textures.length; ++tex)
          {
            vcFBXTexture *pTex = &pMat->textures[tex];
            if (pTex->pTex != nullptr)
            {
              uint32_t uvIndex = pFBX->currMeshPointCount;
              if (pTex->ref != FbxGeometryElement::eDirect)
                uvIndex = pTex->pIArray->GetAt(uvIndex);

              FbxDouble2 uv = pTex->pDArray->GetAt(uvIndex);
              int pixel = (int)(uv[0] + (1 - uv[1]) * pTex->width);
              thisColour = pTex->pPixels[pixel];

              if (pTex->layered)
              {
                bool premultiplied = pTex->pTex->GetPremultiplyAlpha();

                switch (pTex->pTex->GetBlendMode())
                {
                case FbxTexture::eTranslucent:
                  vcFBX_Alpha((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                  break;
                case FbxTexture::eAdditive:
                  vcFBX_Additive((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                  break;
                case FbxTexture::eModulate:
                  vcFBX_Multiply((uint8_t*)&colour, (uint8_t*)&thisColour);
                  break;
                case FbxTexture::eModulate2:
                  break;
                case FbxTexture::eOver:
                  colour = thisColour;
                  break;
                }
              }
              else
              {
                colour = thisColour;
              }
            }
            else
            {
              colour = pTex->diffuse;
            }

            colour = 0xff000000 | ((colour & 0xff) << 16) | (colour & 0xff00) | ((colour & 0xff0000) >> 16);
            memcpy(pAttr, &colour, sizeof(uint32_t));
            pAttr = &pAttr[sizeof(uint32_t)];
          }
          pFBX->currMeshPointCount += pFBX->everyNth;
          ++pBuffer->pointCount;
        }
      }
    }
    else
    {
      // For each polygon
      while (pFBX->currMeshPolygon < pFBX->totalMeshPolygons)
      {
        uint8_t *pAttr = &pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride];

        // If we need to set a new triangle
        if (pFBX->currMeshPolygon != pFBX->lastTouchedPoly)
        {
          if (pFBX->pMesh->GetPolyHoleInfo(pFBX->currMeshPolygon))
          {
            ++pFBX->currProcessedPolygons;
            ++pFBX->currMeshPolygon;
            continue;
          }

          double p0[3];
          double p1[3];
          double p2[3];

          // Get Vertices
          FbxVector4 f0 = pFBX->pPoints[pFBX->pIndices[pFBX->currMeshPolygon * 3]];
          FbxVector4 f1 = pFBX->pPoints[pFBX->pIndices[pFBX->currMeshPolygon * 3 + 1]];
          FbxVector4 f2 = pFBX->pPoints[pFBX->pIndices[pFBX->currMeshPolygon * 3 + 2]];
          for (int coord = 0; coord < 3; ++coord)
          {
            p0[coord] = f0[coord] - pConvertInput->boundMin[coord];
            p1[coord] = f1[coord] - pConvertInput->boundMin[coord];
            p2[coord] = f2[coord] - pConvertInput->boundMin[coord];
          }

          UD_ERROR_CHECK(vdkTriangleVoxelizer_SetTriangle(pFBX->pTrivox, p0, p1, p2));
          pFBX->lastTouchedPoly = pFBX->currMeshPolygon;
          ++pFBX->currProcessedPolygons;
        }

        uint32_t numPoints = UINT32_MAX;

        while (numPoints != 0)
        {
          UD_ERROR_CHECK(vdkTriangleVoxelizer_GetPoints(pFBX->pTrivox, &pFBX->pTriPositions, (double**)&pFBX->pTriWeights, &numPoints, pBuffer->pointsAllocated - pBuffer->pointCount));

          if (numPoints > 0)
          {
            vcFBXMaterial *pMat = nullptr;

            if (pConvertInput->attributes.standardContent & vdkSAC_ARGB && pFBX->materials.length > 0)
            {
              uint32_t index = 0;

              {
                switch (pFBX->map)
                {
                case fbxsdk::FbxLayerElement::eByPolygonVertex: // falls through
                case fbxsdk::FbxLayerElement::eByControlPoint:
                  index = pFBX->pIndex->GetAt(pFBX->currMeshPolygon * 3);
                  break;
                case fbxsdk::FbxLayerElement::eByPolygon:
                  index = pFBX->pIndex->GetAt(pFBX->currMeshPolygon);
                  break;
                case fbxsdk::FbxLayerElement::eByEdge: // falls through
                case fbxsdk::FbxLayerElement::eAllSame:
                  index = (uint32_t)pFBX->materials.length - 1;
                  break;
                case fbxsdk::FbxLayerElement::eNone:
                  break;
                }
              }

              if (index < pFBX->materials.length)
                pMat = &pFBX->materials[index];

              UDASSERT(pMat != nullptr, "Material index incorrectly looked up, or FBX file corrupted");

              uint32_t thisColour = 0;

              for (uint32_t currTex = 0; currTex < pMat->textures.length; ++currTex)
              {
                vcFBXTexture *pTex = &pMat->textures[currTex];

                if (pTex->pTex != nullptr)
                {
                  for (int i = 0; i < 3; ++i)
                  {
                    FbxVector2 newVec = { 0, 0 };
                    uint32_t uvIndex = 0;
                    switch (pTex->map)
                    {
                    case fbxsdk::FbxLayerElement::eByControlPoint:
                      uvIndex = (uint32_t)pFBX->pMesh->GetPolygonVertex(pFBX->currMeshPolygon, i);
                      break;
                    case fbxsdk::FbxLayerElement::eByPolygon:
                      uvIndex = pFBX->currMeshPolygon;
                      break;
                    case fbxsdk::FbxLayerElement::eByPolygonVertex:
                      uvIndex = pFBX->currMeshPolygon * 3 + i;
                      break;
                    case fbxsdk::FbxLayerElement::eNone: // falls through
                    case fbxsdk::FbxLayerElement::eByEdge: // falls through
                    case fbxsdk::FbxLayerElement::eAllSame:
                      uvIndex = 0;
                      break;
                    }

                    if (pTex->ref != FbxGeometryElement::eDirect)
                      uvIndex = pTex->pIArray->GetAt(uvIndex);

                    newVec = pTex->pDArray->GetAt(uvIndex);

                    udDouble2 UV = udDouble2::create(newVec[0], 1 - newVec[1]);

                    if (pTex->swap)
                    {
                      double swap = UV[0];
                      UV[0] = UV[1];
                      UV[1] = swap;
                    }

                    pFBX->uvQueue.PushBack(UV);
                  }
                }
              }

              // Handle everyNth here in one place, slightly inefficiently but with the benefit of simplicity for the rest of the function
              if (pFBX->everyNth > 1)
              {
                uint32_t i = 0;
                uint32_t pc = 0;

                do
                {
                  uint32_t remaining = numPoints - i;
                  if (pFBX->everyNthAccum + remaining < pFBX->everyNth)
                  {
                    pFBX->everyNthAccum += remaining;
                    break;
                  }
                  else
                  {
                    i += (pFBX->everyNth - pFBX->everyNthAccum);
                    memcpy(&pFBX->pTriPositions[pc * 3], &pFBX->pTriPositions[i * 3], sizeof(double) * 3);
                    memcpy(&pFBX->pTriWeights[pc], &pFBX->pTriWeights[i], sizeof(double) * 3);
                    ++pc;
                    pFBX->everyNthAccum = 0;
                  }

                } while (i < numPoints);

                numPoints = pc;
              }

              // Write points from current triangle
              for (uint32_t point = 0; point < numPoints; ++point)
              {
                // Position
                for (int coord = 0; coord < 3; ++coord)
                  pBuffer->pPositions[(pBuffer->pointCount + point) * 3 + coord] = (int64_t)(pFBX->pTriPositions[3 * point + coord] / pConvertInput->sourceResolution);

                // Colour
                uint32_t colour = 0;
                if (pConvertInput->attributes.standardContent & vdkSAC_ARGB)
                {
                  for (uint32_t currTex = 0; currTex < pMat->textures.length; ++currTex)
                  {
                    vcFBXTexture *pTex = &pMat->textures[currTex];

                    int w = pTex->width;
                    int h = pTex->height;

                    if (pTex->pTex != nullptr)
                    {
                      udDouble2 pointUV = pFBX->uvQueue[currTex * 3] * pFBX->pTriWeights[point].x + pFBX->uvQueue[currTex * 3 + 1] * pFBX->pTriWeights[point].y + pFBX->uvQueue[currTex * 3 + 2] * pFBX->pTriWeights[point].z;

                      int u = (int)udMod(udRound(pointUV[0] * w), w);
                      u = (pTex->pTex->GetWrapModeU() == FbxTexture::eRepeat ? (u + w) % w : udClamp(u, 0, w));
                      int v = (int)udMod(udRound(pointUV[1] * h), h);
                      v = (pTex->pTex->GetWrapModeV() == FbxTexture::eRepeat ? (v + h) % h : udClamp(v, 0, h));

                      thisColour = pTex->pPixels[u + v * w];

                      if (pTex->layered)
                      {
                        bool premultiplied = pTex->pTex->GetPremultiplyAlpha();

                        switch (pTex->pTex->GetBlendMode())
                        {
                        case FbxTexture::eTranslucent:
                          vcFBX_Alpha((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                          break;
                        case FbxTexture::eAdditive:
                          vcFBX_Additive((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                          break;
                        case FbxTexture::eModulate:
                          vcFBX_Multiply((uint8_t*)&colour, (uint8_t*)&thisColour);
                          break;
                        case FbxTexture::eModulate2:
                          break;
                        case FbxTexture::eOver:
                          colour = thisColour;
                          break;
                        }
                      }
                      else
                      {
                        colour = thisColour;
                      }
                    }
                    else
                    {
                      colour = pTex->diffuse;
                    }
                  }

                  colour = 0xff000000 | ((colour & 0xff) << 16) | (colour & 0xff00) | ((colour & 0xff0000) >> 16);
                  memcpy(pAttr, &colour, sizeof(uint32_t));
                  pAttr += pBuffer->attributeStride;
                } // End if colour
              } // End for numpoints

              pBuffer->pointCount += numPoints;
            } // End colour processing

            if (pBuffer->pointCount == pBuffer->pointsAllocated)
            {
              pFBX->pointsReturned += pBuffer->pointCount;

              // Improve estimate
              pConvertInput->pointCount = pFBX->pointsReturned / pFBX->currProcessedPolygons * pFBX->totalPolygons;

              return vE_Success;
            }
          } // End if numpoints
        }
        ++pFBX->currMeshPolygon;
        pFBX->uvQueue.Clear();
      } // End while poly < total polys
    }
    ++pFBX->currMesh;
  }

  vcFBX_CleanMaterials(&pFBX->materials);

  // If finished update point counts
  pFBX->pointsReturned += pBuffer->pointCount;
  pConvertInput->pointCount = pFBX->pointsReturned;

  result = vE_Success;

epilogue:
  return result;
}

void vcFBX_Close(vdkConvertCustomItem *pConvertInput)
{
  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
  if (pFBX->pScene != nullptr)
  {
    pFBX->pScene->Destroy();
    pFBX->pScene = nullptr;
  }
  if (pFBX->pManager != nullptr)
  {
    pFBX->pManager->Destroy();
    pFBX->pManager = nullptr;
  }
  pFBX->uvQueue.Deinit();

  vcFBX_CleanMaterials(&pFBX->materials);

  if (pFBX->pTrivox)
    vdkTriangleVoxelizer_Destroy(&pFBX->pTrivox);
}

void vcFBX_Destroy(vdkConvertCustomItem* pConvertInput)
{
  vcFBX* pFBX = (vcFBX*)pConvertInput->pData;

  vdkAttributeSet_Free(&pConvertInput->attributes);

  udFree(pConvertInput->pName);
  udFree(pFBX);
}

vdkError vcFBX_AddItem(vdkConvertContext *pConvertContext, const char *pFilename)
{
  vdkConvertCustomItem customItem = {};

  vcFBX *pFBX = udAllocType(vcFBX, 1, udAF_Zero);
  if (pFBX == nullptr)
    return vE_MemoryAllocationFailure;

  // Populate customItem
  customItem.pData = pFBX;
  customItem.pOpen = vcFBX_Open;
  customItem.pClose = vcFBX_Close;
  customItem.pDestroy = vcFBX_Destroy;
  customItem.pReadPointsInt = vcFBX_ReadPointsInt;
  customItem.pName = udStrdup(pFilename);
  customItem.srid = 0;
  customItem.sourceProjection = vdkCSP_SourceCartesian;
  customItem.pointCount = -1;
  vdkAttributeSet_Generate(&customItem.attributes, vdkSAC_ARGB, 0);

  customItem.boundsKnown = false;
  for (int i = 0; i < 3; ++i)
  {
    customItem.boundMax[i] = 5000000;
    customItem.boundMin[i] = -5000000;
  }
  return vdkConvert_AddCustomItem(pConvertContext, &customItem);
}

#endif
