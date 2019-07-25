#ifdef FBXSDK_ON

#include "fbxsdk.h"

#include "stb_image.h"

#include "udFile.h"
#include "udStringUtil.h"
#include "udPlatform.h"
#include "udMath.h"
#include "udPlatformUtil.h"

#include "vcFBX.h"
#include "vdkTriangleVoxelizer.h"

struct vcFBX_UVSet
{
  const char *pName;
  bool indexed;
  FbxGeometryElement::EMappingMode mapping;
  FbxLayerElementArrayTemplate<int> *pIndexes;
  FbxLayerElementArrayTemplate<FbxVector2> *pDirect;
};

struct vcFBXTexture
{
  FbxFileTexture *pTex;
  int uvSet;

  uint32_t diffuse;

  uint32_t *pPixels;
  int width;
  int height;
  int channels;
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

  FbxArray<vcFBX_UVSet> uvSets;
  FbxArray<vcFBXTexture> textures;
  FbxArray<FbxVector2> uvQueue;
};

// TODO: (EVC-719) Texture Blending
/*
inline void vcFBX_Alpha(uint8_t *pDest, const uint8_t *pSrc, bool premultiplied)
{
  double multiply = premultiplied ? 1 : pSrc[3] / 0xFF;
  pDest[0] = pDest[0] * ((0xFF - pSrc[0]) / 0xFF) + pSrc[0] * multiply;
  pDest[1] = pDest[1] * ((0xFF - pSrc[1]) / 0xFF) + pSrc[1] * multiply;
  pDest[2] = pDest[2] * ((0xFF - pSrc[2]) / 0xFF) + pSrc[2] * multiply;
  pDest[3] = pDest[3] * ((0xFF - pSrc[3]) / 0xFF) + pSrc[3];
}
inline void vcFBX_Multiply(uint8_t *pDest, const uint8_t *pSrc)
{
  pDest[0] = udMin((pDest[0] / 0xFF) * (pSrc[0] / 0xFF), 0xFF);
  pDest[1] = udMin((pDest[1] / 0xFF) * (pSrc[1] / 0xFF), 0xFF);
  pDest[2] = udMin((pDest[2] / 0xFF) * (pSrc[2] / 0xFF), 0xFF);
  pDest[3] = udMin((pDest[3] / 0xFF) * (pSrc[3] / 0xFF), 0xFF);
}*/
inline void vcFBX_Additive(uint8_t *pDest, const uint8_t *pSrc, bool premultiplied)
{
  double multiply = premultiplied ? 1 : pSrc[3] / 0xFF;
  pDest[0] = (uint8_t)udMin(pDest[0] + (uint8_t)(pSrc[0] * multiply), 0xFF);
  pDest[1] = (uint8_t)udMin(pDest[1] + (uint8_t)(pSrc[1] * multiply), 0xFF);
  pDest[2] = (uint8_t)udMin(pDest[2] + (uint8_t)(pSrc[2] * multiply), 0xFF);
}

void vcFBX_GetUVSets(vcFBX *pFBX, FbxMesh *pMesh)
{
  // Get all UV set names
  FbxStringList UVSetNameList;
  pMesh->GetUVSetNames(UVSetNameList);

  // Iterate over all uv sets
  for (int i = 0; i < UVSetNameList.GetCount(); i++)
  {
    vcFBX_UVSet set = {};

    const char *pName = UVSetNameList[i];
    set.pName = udStrdup(pName);
    const FbxGeometryElementUV *pUVElement = pMesh->GetElementUV(set.pName);

    if (!pUVElement)
      continue;

    set.mapping = pUVElement->GetMappingMode();

    // Only support mapping mode eByPolygonVertex and eByControlPoint
    if (set.mapping != FbxGeometryElement::eByPolygonVertex && set.mapping != FbxGeometryElement::eByControlPoint)
      continue;

    set.indexed = pUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;

    set.pIndexes = &pUVElement->GetIndexArray();
    set.pDirect = &pUVElement->GetDirectArray();

    pFBX->uvSets.Add(set);
  }
}

udResult vcFBX_GetTextures(vcFBX *pFBX, FbxNode *pNode)
{
  // Process materials to prepare colour handling
  const char *pFilename = nullptr;
  int totalMats = pNode->GetSrcObjectCount<FbxSurfaceMaterial>();

  for (int i = 0; i < totalMats; ++i)
  {
    vcFBXTexture texture = {};
    texture.uvSet = -1;

    FbxSurfaceMaterial *pMaterial = pFBX->pNode->GetSrcObject<FbxSurfaceMaterial>(i);
    if (!pMaterial)
      continue;

    FbxProperty diffuse = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);

    if (diffuse.IsValid())
    {
      const FbxDouble3 col = diffuse.Get<FbxDouble3>();

      for (int j = 0; j < 3; ++j)
        texture.diffuse += (uint8_t)(col[j] * 0xFF) << (24 - j * 8);

      texture.diffuse += 0xFF;
    }

    int textureCount = diffuse.GetSrcObjectCount<FbxTexture>();
    for (int j = 0; j < textureCount; ++j)
    {
      texture.uvSet = -1;
      texture.pTex = nullptr;

      FbxFileTexture *pTexture = FbxCast<FbxFileTexture>(diffuse.GetSrcObject<FbxFileTexture>(j));
      FbxTexture::EMappingType map = pTexture->GetMappingType();
      FbxTexture::ETextureUse use = pTexture->GetTextureUse();
      FbxString UVSet = pTexture->UVSet.Get();

      for (int k = 0; k < pFBX->uvSets.GetCount(); ++k)
      {
        if (UVSet.Compare(pFBX->uvSets[k].pName) == 0)
        {
          texture.uvSet = k;
          break;
        }
      }

      if (!pTexture->UseMaterial || texture.uvSet == -1 || map != FbxTexture::eUV || use != FbxTexture::eStandard)
        continue;

      pFilename = pTexture->GetFileName();
      if (pFilename != nullptr && !udStrEqual(pFilename, ""))
      {
        void *pMem = nullptr;
        int64_t fileLength;

        udResult res = udFile_Load(pFilename, &pMem, &fileLength);
        if (res == udR_Success)
        {
          texture.pPixels = (uint32_t*)stbi_load_from_memory((const stbi_uc*)pMem, (int)fileLength, &texture.width, &texture.height, &texture.channels, 4);
          texture.pTex = pTexture;

          // For now breaking with first texture loaded, because the test model I'm using has 3 of the same materials referencing a single texture
          // with a single UVset, so actually applying them all uses the textures blend mode (additive) and outputs basically only white...
          udFree(pMem);
          break;
        }
        else
        {
          return res;
        }
      }

      pFBX->textures.Add(texture);
    }
    pFBX->textures.Add(texture);
  }

  return udR_Success;
}


vdkError vcFBX_Open(vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, vdkConvertCustomItemFlags flags)
{
  udUnused(origin);

  vdkError result;
  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
  FbxVector4 min, max, center;
  FbxAMatrix transform;
  pFBX->pManager = FbxManager::Create();

  // Configure IO Settings
  FbxIOSettings *pIOS = FbxIOSettings::Create(pFBX->pManager, IOSROOT);
  pIOS->SetBoolProp(IMP_FBX_MATERIAL, false);
  pIOS->SetBoolProp(IMP_FBX_TEXTURE, false);
  pIOS->SetBoolProp(IMP_FBX_LINK, false);
  pIOS->SetBoolProp(IMP_FBX_SHAPE, false);
  pIOS->SetBoolProp(IMP_FBX_GOBO, false);
  pIOS->SetBoolProp(IMP_FBX_ANIMATION, false);
  pIOS->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, false);
  pFBX->pManager->SetIOSettings(pIOS);
  FbxImporter *pImporter = FbxImporter::Create(pFBX->pManager, "");
  FbxGeometryConverter geoCon = FbxGeometryConverter(pFBX->pManager);

  // Import File
  UD_ERROR_IF(!pImporter->Initialize(pConvertInput->pName, -1, pFBX->pManager->GetIOSettings()), vE_ReadFailure);
  pFBX->pScene = FbxScene::Create(pFBX->pManager, "");
  UD_ERROR_IF(!pImporter->Import(pFBX->pScene), vE_OpenFailure);
  pImporter->Destroy(); // Once file is imported, importer can be destroyed

  // If model isn't already scaled to metres, resize model
  if (pFBX->pScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
    FbxSystemUnit::m.ConvertScene(pFBX->pScene);

  UD_ERROR_IF(!pFBX->pScene->ComputeBoundingBoxMinMaxCenter(min, max, center), vE_Failure);

  for (int i = 0; i < 3; ++i)
  {
    pConvertInput->boundMax[i] = max[i];
    pConvertInput->boundMin[i] = min[i];
  }
  pConvertInput->boundsKnown = true;

  pConvertInput->pointCountIsEstimate = true;
  pConvertInput->sourceResolution = pointResolution;

  pFBX->everyNth = udMax(everyNth, (uint32_t)1);
  pFBX->convertFlags = flags;

  pFBX->totalMeshes = pFBX->pScene->GetGeometryCount();
  UD_ERROR_IF(pFBX->totalMeshes < 1, vE_NotFound);

  UD_ERROR_IF(!geoCon.Triangulate(pFBX->pScene, true), vE_Failure);

  for (uint16_t i = 0; i < pFBX->totalMeshes; ++i)
    pFBX->totalPolygons += pFBX->pScene->GetGeometry(pFBX->currMesh)->GetNode()->GetMesh()->GetPolygonCount();

  pFBX->lastTouchedPoly = 1; // Forces handling 0, then gets set to 0
  pFBX->lastTouchedMesh = 1;

  UD_ERROR_CHECK(vdkTriangleVoxelizer_Create(&pFBX->pTrivox, pointResolution));

  return vE_Success;

epilogue:
  pFBX->pManager->Destroy(); // Destroying manager destroys all objects that were created with it
  udFree(pFBX);
  pFBX = nullptr;

  return result;
}

vdkError vcFBX_ReadPointsInt(vdkConvertCustomItem *pConvertInput, vdkConvertPointBufferInt64 *pBuffer)
{
  if (pBuffer == nullptr || pBuffer->pAttributes == nullptr || pConvertInput == nullptr || pConvertInput->pData == nullptr)
    return vE_InvalidParameter;

  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
  vdkError result;

  pBuffer->pointCount = 0;
  memset(pBuffer->pAttributes, 0, pBuffer->attributeSize * pBuffer->pointsAllocated);

  // For each mesh
  while (pFBX->currMesh < pFBX->totalMeshes)
  {
    // -------------------------------------------------------- Handle new mesh
    if (pFBX->currMesh != pFBX->lastTouchedMesh)
    {
      pFBX->lastTouchedMesh = pFBX->currMesh;
      pFBX->pNode = pFBX->pScene->GetGeometry(pFBX->currMesh)->GetNode();
      pFBX->pMesh = pFBX->pNode->GetMesh();

      if (pFBX->pMesh == nullptr)
      {
        ++pFBX->currMesh;
        continue;
      }

      FbxAMatrix transform = pFBX->pNode->EvaluateGlobalTransform();
      pFBX->pMesh->SetPivot(transform);
      pFBX->pMesh->ApplyPivot();
      pFBX->totalMeshPolygons = pFBX->pMesh->GetPolygonCount();
      pFBX->totalMeshPoints = pFBX->pMesh->GetPolygonVertexCount();
      pFBX->currMeshPointCount = 0;
      pFBX->currMeshPolygon = 0;
      pFBX->pPoints = pFBX->pMesh->GetControlPoints();
      pFBX->pIndices = pFBX->pMesh->GetPolygonVertices();

      // ---------- Colour processing
      if (pConvertInput->content & vdkAC_ARGB)
      {
        vcFBX_GetUVSets(pFBX, pFBX->pMesh);
        if (vcFBX_GetTextures(pFBX, pFBX->pNode) != udR_Success)
          return vE_NotFound;
      }
    } // ---------------------------------------------- End new mesh handling

    // Vertex only case makes a point per vertex
    if (pFBX->convertFlags & vdkCCIF_PolygonVerticesOnly)
    {
      // For each vertex
      while (pFBX->currMeshPointCount < pFBX->totalMeshPoints)
      {
        uint8_t *pAttr = &pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeSize];

        // Write point to buffer
        FbxVector4 currPoint = pFBX->pPoints[pFBX->pIndices[pFBX->currMeshPointCount]];
        for (int coord = 0; coord < 3; ++coord)
          pBuffer->pPositions[pBuffer->pointCount][coord] = (int64_t)((currPoint[coord] - pConvertInput->boundMin[coord]) / pConvertInput->sourceResolution);

        // Handle colour
        if (pConvertInput->content & vdkAC_ARGB)
        {
          uint32_t colour = 0;

          for (int tex = 0; tex < pFBX->textures.Size(); ++tex)
          {
            uint32_t thisColour = 0;

            if (pFBX->textures[tex].pTex != nullptr)
            {
              vcFBX_UVSet &UV = pFBX->uvSets[pFBX->textures[tex].uvSet];

              int w = pFBX->textures[tex].width;
              int h = pFBX->textures[tex].height;

              int PolyVertIndex = -1;

              if (UV.mapping == FbxGeometryElement::eByControlPoint)
                PolyVertIndex = pFBX->pMesh->GetPolygonVertex(pFBX->currMeshPointCount / 3, pFBX->currMeshPointCount % 3);
              else if (UV.mapping == FbxGeometryElement::eByPolygonVertex)
                PolyVertIndex = pFBX->currMeshPointCount;

              int UVIndex = UV.indexed ? UV.pIndexes->GetAt(PolyVertIndex) : PolyVertIndex;
              FbxVector2 newVec = UV.pDirect->GetAt(UVIndex);
              newVec[1] = 1 - newVec[1];

              int u = (int)udMod(udRound(newVec[0] * w), w);
              u = (pFBX->textures[tex].pTex->GetWrapModeU() == FbxTexture::eRepeat ? (u + w) % w : udClamp(u, 0, w));
              int v = (int)udMod(udRound(newVec[1] * h), h);
              v = (pFBX->textures[tex].pTex->GetWrapModeV() == FbxTexture::eRepeat ? (v + h) % h : udClamp(v, 0, h));

              thisColour = pFBX->textures[tex].pPixels[u + v * w];

              /*
              bool premultiplied = pFBX->textures[tex].pTex->GetPremultiplyAlpha();

              switch (pFBX->textures[tex].pTex->GetBlendMode())
              {
              case FbxTexture::eTranslucent:
                //vcFBX_Alpha((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                break;
              case FbxTexture::eAdditive:
                vcFBX_Additive((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                break;
              case FbxTexture::eModulate:
                //vcFBX_Multiply((uint8_t*)&colour, (uint8_t*)&thisColour);
                break;
              case FbxTexture::eModulate2:
                break;
              case FbxTexture::eOver:
                colour = thisColour;
                break;
              }*/

              colour = thisColour;
            }
            else
            {
              colour = pFBX->textures[tex].diffuse;
            }
          }

          colour = 0xff000000 | ((colour & 0xff) << 16) | (colour & 0xff00) | ((colour & 0xff0000) >> 16);
          memcpy(pAttr, &colour, sizeof(uint32_t));
          pAttr = &pAttr[sizeof(uint32_t)];
        }

        pFBX->currMeshPointCount += pFBX->everyNth;
        ++pBuffer->pointCount;
      }
    }
    else
    {
      // For each polygon
      while (pFBX->currMeshPolygon < pFBX->totalMeshPolygons)
      {
        uint8_t *pAttr = &pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeSize];

        // If we need to set a new triangle
        if (pFBX->currMeshPolygon != pFBX->lastTouchedPoly)
        {
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

        uint32_t numPoints = 0;
        UD_ERROR_CHECK(vdkTriangleVoxelizer_GetPoints(pFBX->pTrivox, &pFBX->pTriPositions, (double**)&pFBX->pTriWeights, &numPoints, pBuffer->pointsAllocated - pBuffer->pointCount));

        if (numPoints > 0)
        {
          if (pConvertInput->content & vdkAC_ARGB)
          {
            for (int tex = 0; tex < pFBX->textures.Size(); ++tex)
            {
              if (pFBX->textures[tex].uvSet >= 0)
              {
                vcFBX_UVSet &UV = pFBX->uvSets[pFBX->textures[tex].uvSet];

                if (pFBX->textures[tex].pTex != nullptr)
                {
                  for (int i = 0; i < 3; ++i)
                  {
                    int PolyVertIndex = -1;

                    if (UV.mapping == FbxGeometryElement::eByControlPoint)
                      PolyVertIndex = pFBX->pMesh->GetPolygonVertex(pFBX->currMeshPolygon, i);
                    else if (UV.mapping == FbxGeometryElement::eByPolygonVertex)
                      PolyVertIndex = pFBX->currMeshPolygon * 3 + i;

                    int UVIndex = UV.indexed ? UV.pIndexes->GetAt(PolyVertIndex) : PolyVertIndex;
                    FbxVector2 newVec = UV.pDirect->GetAt(UVIndex);
                    newVec[1] = 1 - newVec[1];
                    pFBX->uvQueue.Add(newVec);
                  }
                }
              }
            }
          }

          // Write points from current triangle
          for (uint32_t point = 0; point < numPoints; point += pFBX->everyNth)
          {
            // Position
            for (int coord = 0; coord < 3; ++coord)
              pBuffer->pPositions[pBuffer->pointCount + point / pFBX->everyNth][coord] = (int64_t)(pFBX->pTriPositions[3 * point + coord] / pConvertInput->sourceResolution);

            // Colour
            if (pConvertInput->content & vdkAC_ARGB)
            {
              uint32_t colour = 0;

              for (int tex = 0; tex < pFBX->textures.Size(); ++tex)
              {
                uint32_t thisColour;

                if (pFBX->textures[tex].uvSet >= 0)
                {
                  int w = pFBX->textures[tex].width, h = pFBX->textures[tex].height;
                  FbxVector2 pointUV = pFBX->uvQueue[0] * pFBX->pTriWeights[point].x + pFBX->uvQueue[1] * pFBX->pTriWeights[point].y + pFBX->uvQueue[2] * pFBX->pTriWeights[point].z;

                  pFBX->uvQueue.RemoveRange(0, 3);

                  int u = (int)udMod(udRound(pointUV[0] * w), w);
                  u = (pFBX->textures[tex].pTex->GetWrapModeU() == FbxTexture::eRepeat ? (u + w) % w : udClamp(u, 0, w));
                  int v = (int)udMod(udRound(pointUV[1] * h), h);
                  v = (pFBX->textures[tex].pTex->GetWrapModeV() == FbxTexture::eRepeat ? (v + h) % h : udClamp(v, 0, h));

                  uint32_t pixel = (uint32_t)u + v * w;
                  thisColour = pFBX->textures[tex].pPixels[pixel];

                  /*bool premultiplied = pFBX->textures[tex].pTex->GetPremultiplyAlpha();

                  switch (pFBX->textures[tex].pTex->GetBlendMode())
                  {
                  case FbxTexture::eTranslucent:
                    //vcFBX_Alpha((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                    break;
                  case FbxTexture::eAdditive:
                    vcFBX_Additive((uint8_t*)&colour, (uint8_t*)&thisColour, premultiplied);
                    break;
                  case FbxTexture::eModulate:
                    //vcFBX_Multiply((uint8_t*)&colour, (uint8_t*)&thisColour);
                    break;
                  case FbxTexture::eModulate2:
                    break;
                  case FbxTexture::eOver:
                    colour = thisColour;
                    break;
                  }
                  */

                  colour = thisColour;
                }
                else
                {
                  colour = pFBX->textures[tex].diffuse;
                }
              }

              colour = 0xff000000 | ((colour & 0xff) << 16) | (colour & 0xff00) | ((colour & 0xff0000) >> 16);
              memcpy(pAttr, &colour, sizeof(uint32_t));
              pAttr = &pAttr[sizeof(uint32_t)];
            } // End if colour
          } // End for numpoints
          pBuffer->pointCount += numPoints / pFBX->everyNth;
        }

        if (pBuffer->pointCount == pBuffer->pointsAllocated)
        {
          pFBX->pointsReturned += pBuffer->pointCount;

          // Improve estimate
          pConvertInput->pointCount = pFBX->pointsReturned / pFBX->currProcessedPolygons * pFBX->totalPolygons;

          return vE_Success;
        }

        ++pFBX->currMeshPolygon;
        pFBX->uvQueue.Clear();
      } // End while poly < total polys
    }
    ++pFBX->currMesh;

    for (int i = 0; i < pFBX->textures.Size(); ++i)
    {
      if (pFBX->textures[i].pPixels != nullptr)
        stbi_image_free(pFBX->textures[i].pPixels);
    }

    for (int i = 0; i < pFBX->uvSets.Size(); ++i)
      udFree(pFBX->uvSets[i].pName);

    pFBX->textures.Clear();
    pFBX->uvSets.Clear();
  } // End while mesh < total meshes

  // If finished update point counts
  pFBX->pointsReturned += pBuffer->pointCount;
  pConvertInput->pointCount = pFBX->pointsReturned;

  result = vE_Success;

epilogue:
  return result;
}

void vcFBX_Close(vdkConvertCustomItem *pConvertInput)
{
  if (pConvertInput->pData != nullptr)
  {
    vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
    vdkTriangleVoxelizer_Destroy(&pFBX->pTrivox);

    if (pFBX->pManager != nullptr)
      pFBX->pManager->Destroy();

    udFree(pConvertInput->pName);

    for (int i = 0; i < pFBX->uvSets.Size(); ++i)
      udFree(pFBX->uvSets[i].pName);

    udFree(pFBX);
  }
}

vdkError vcFBX_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, const char *pFilename)
{
  vdkConvertCustomItem customItem = {};

  vcFBX *pFBX = udAllocType(vcFBX, 1, udAF_Zero);

  // Populate customItem
  customItem.pData = pFBX;
  customItem.pOpen = vcFBX_Open;
  customItem.pClose = vcFBX_Close;
  customItem.pReadPointsInt = vcFBX_ReadPointsInt;
  customItem.pName = udStrdup(pFilename);
  customItem.srid = 0;
  customItem.sourceProjection = vdkCSP_SourceCartesian;
  customItem.pointCount = -1;
  customItem.content = vdkAC_ARGB; // Colour is the only content attribute in an fbx model

  // Bounds will be overwritten
  customItem.boundsKnown = false;
  for (int i = 0; i < 3; ++i)
    customItem.boundMax[i] = 1000000000;

  return vdkConvert_AddCustomItem(pContext, pConvertContext, &customItem);
}

#endif
