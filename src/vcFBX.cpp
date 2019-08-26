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

struct vcFBXTexture
{
  FbxFileTexture *pTex;
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
  bool byControlPoint;
  FbxLayerElementArrayTemplate<int> *pIArray;
  FbxLayerElementArrayTemplate<FbxVector2> *pDArray;
};

struct vcFBXMaterial
{
  FbxArray<vcFBXTexture> textures;
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

  FbxArray<vcFBXMaterial> materials;
  FbxArray<FbxVector2> uvQueue;

  FbxLayerElementArrayTemplate<int> *pIndex;
  FbxLayerElement::EMappingMode map;
  FbxLayerElement::EReferenceMode ref;
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

void vcFBX_FileTexture(vcFBX *pFBX, FbxFileTexture *pTexture, int material, bool layered)
{
  vcFBXTexture texture = {};

  FbxTexture::EMappingType map = pTexture->GetMappingType();
  FbxTexture::ETextureUse use = pTexture->GetTextureUse();
  FbxString UVSet = pTexture->UVSet.Get();

  if (map != FbxTexture::eUV || use != FbxTexture::eStandard)
    return;

  texture.layered = layered;
  texture.pName = udStrdup(UVSet);

  texture.scale = pTexture->Scaling;
  texture.translate = pTexture->Translation;
  texture.swap = pTexture->GetSwapUV();

  const FbxGeometryElementUV *pUVElement = pFBX->pMesh->GetElementUV(UVSet);

  if (!pUVElement)
    pUVElement = pFBX->pMesh->GetElementUV(0);

  texture.pFilename = pTexture->GetFileName();
  texture.pTex = pTexture;

  texture.pIArray = &pUVElement->GetIndexArray();
  texture.pDArray = &pUVElement->GetDirectArray();

  if (texture.pFilename != nullptr && !udStrEqual(texture.pFilename, ""))
  {
    for (int i = 0; i < pFBX->materials.Size(); ++i)
    {
      for (int j = 0; j < pFBX->materials[i].textures.Size(); ++j)
      {
        if (udStrEquali(texture.pFilename, pFBX->materials[i].textures[j].pFilename))
        {
          texture.pPixels = pFBX->materials[i].textures[j].pPixels;
          texture.width = pFBX->materials[i].textures[j].width;
          texture.height = pFBX->materials[i].textures[j].height;
          texture.channels = pFBX->materials[i].textures[j].channels;
          pFBX->materials[material].textures.Add(texture);
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
      pFBX->materials[material].textures.Add(texture);
    }
  }

  return;
}

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
    // Adds unnecessary materials atm
    pFBX->materials.Add(mat);

    FbxProperty diffuse = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);

    if (diffuse.IsValid())
    {
      if (diffuse.GetSrcObjectCount<FbxTexture>() > 0)
      {
        for (int j = 0; j < diffuse.GetSrcObjectCount<FbxFileTexture>(); ++j)
        {
          FbxFileTexture *pTexture = diffuse.GetSrcObject<FbxFileTexture>(j);
          vcFBX_FileTexture(pFBX, pTexture, i, false);
        }
        for (int j = 0; j < diffuse.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
        {
          FbxLayeredTexture *pLayer = diffuse.GetSrcObject<FbxLayeredTexture>(j);

          for (int k = 0; k < pLayer->GetSrcObjectCount<FbxFileTexture>(); ++k)
          {
            vcFBX_FileTexture(pFBX, pLayer->GetSrcObject<FbxFileTexture>(k), i, true);
          }
        }
      }
      else
      {
        vcFBXTexture texture = {};
        texture.pTex = nullptr;

        FbxDataType type = diffuse.GetPropertyDataType();

        if (FbxDouble3DT == type || FbxColor3DT == type)
        {
          const FbxDouble3 col = diffuse.Get<FbxDouble3>();

          for (int j = 0; j < 3; ++j)
            texture.diffuse += (uint8_t)(col[j] * 0xFF) << (24 - j * 8);

          texture.diffuse += 0xFF;

          mat.textures.Add(texture);
        }
      }
    }
  }
}

vdkError vcFBX_Open(vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, vdkConvertCustomItemFlags flags)
{
  udUnused(origin);

  vdkError result;
  vcFBX *pFBX = (vcFBX*)pConvertInput->pData;
  FbxVector4 min, max, center;
  pFBX->pManager = FbxManager::Create();
  bool calcBounds = false;

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
  fbxsdk::FbxMaterialConverter matCon = fbxsdk::FbxMaterialConverter(*pFBX->pManager);

  // Import File
  UD_ERROR_IF(!pImporter->Initialize(pConvertInput->pName, -1, pFBX->pManager->GetIOSettings()), vE_ReadFailure);
  pFBX->pScene = FbxScene::Create(pFBX->pManager, "");

  matCon.ConnectTexturesToMaterials(*pFBX->pScene);

  UD_ERROR_IF(!pImporter->Import(pFBX->pScene), vE_OpenFailure);
  pImporter->Destroy(); // Once file is imported, importer can be destroyed

  // If model isn't already scaled to metres, resize model
  if (pFBX->pScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
    FbxSystemUnit::m.ConvertScene(pFBX->pScene);

  pConvertInput->pointCountIsEstimate = true;
  pConvertInput->sourceResolution = pointResolution;

  pFBX->everyNth = udMax(everyNth, (uint32_t)1);
  pFBX->convertFlags = flags;

  pFBX->totalMeshes = pFBX->pScene->GetGeometryCount();
  UD_ERROR_IF(pFBX->totalMeshes < 1, vE_NotFound);

  UD_ERROR_IF(!geoCon.Triangulate(pFBX->pScene, true), vE_Failure);
  
  if (!pFBX->pScene->ComputeBoundingBoxMinMaxCenter(min, max, center))
  {
    calcBounds = true;
  }

  for (uint16_t i = 0; i < pFBX->totalMeshes; ++i)
  {
    if (calcBounds)
    {
      pFBX->pMesh->ComputeBBox();
      FbxDouble3 tempMax = pFBX->pMesh->BBoxMax.Get();
      FbxDouble3 tempMin = pFBX->pMesh->BBoxMin.Get();
      for (int j = 0; j < 3; ++j)
      {
        pConvertInput->boundMax[j] = udMax(tempMax[j], pConvertInput->boundMax[j]);
        pConvertInput->boundMin[j] = udMin(tempMin[j], pConvertInput->boundMin[j]);
      }
    }
    else
    {
      for (int j = 0; j < 3; ++j)
      {
        pConvertInput->boundMax[j] = max[j];
        pConvertInput->boundMin[j] = min[j];
      }
      pConvertInput->boundsKnown = true;
    }

    pFBX->totalPolygons += pFBX->pScene->GetGeometry(pFBX->currMesh)->GetNode()->GetMesh()->GetPolygonCount();
  }

  pFBX->lastTouchedPoly = 1; // Forces handling 0, then gets set to 0
  pFBX->lastTouchedMesh = 1;

  UD_ERROR_CHECK(vdkTriangleVoxelizer_Create(&pFBX->pTrivox, pointResolution));

  result = vE_Success;

epilogue:

  if (result != vE_Success)
  {
    pFBX->pManager->Destroy(); // Destroying manager destroys all objects that were created with it
    udFree(pFBX);
    pFBX = nullptr;
  }

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
    // Handle new mesh
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

      pFBX->pMesh->SetPivot(LocalTransform);
      pFBX->pMesh->ApplyPivot();

      pFBX->totalMeshPolygons = pFBX->pMesh->GetPolygonCount();
      pFBX->totalMeshPoints = pFBX->pMesh->GetPolygonVertexCount();
      pFBX->currMeshPointCount = 0;
      pFBX->currMeshPolygon = 0;
      pFBX->pPoints = pFBX->pMesh->GetControlPoints();
      pFBX->pIndices = pFBX->pMesh->GetPolygonVertices();

      // Colour processing
      if (pConvertInput->content & vdkAC_ARGB)
        vcFBX_GetTextures(pFBX, pFBX->pNode);

    } // End new mesh handling

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
          vcFBXMaterial *pMat;

          uint32_t colour = 0;
          uint32_t thisColour = 0;

          if (pFBX->map != FbxLayerElement::eAllSame)
            pMat = &pFBX->materials[pFBX->pIndex->GetAt(pFBX->currMeshPointCount)];
          else
            pMat = &pFBX->materials[0];

          for (int tex = 0; tex < pMat->textures.Size(); ++tex)
          {
            vcFBXTexture *pTex = &pMat->textures[tex];
            if (pTex->pTex != nullptr)
            {
              if (!pTex->byControlPoint)
              {
                int w = pTex->width;
                int h = pTex->height;

                FbxVector2 newVec = { 0, 0 };
                bool unmapped = true;
                if (pFBX->pMesh->GetPolygonVertexUV(pFBX->currMeshPolygon, 0, pTex->pName, newVec, unmapped))
                {
                  newVec[1] = 1 - newVec[1];

                  if (pTex->swap)
                  {
                    double swap = newVec[0];
                    newVec[0] = newVec[1];
                    newVec[1] = swap;
                  }

                  int u = (int)udMod(udRound(newVec[0] * w), w);
                  u = (pTex->pTex->GetWrapModeU() == FbxTexture::eRepeat ? (u + w) % w : udClamp(u, 0, w));
                  int v = (int)udMod(udRound(newVec[1] * h), h);
                  v = (pTex->pTex->GetWrapModeV() == FbxTexture::eRepeat ? (v + h) % h : udClamp(v, 0, h));

                  // Change before wrapping?
                  u = (int) (u / pTex->scale[0] + pTex->translate[0]);
                  v = (int) (v / pTex->scale[1] + pTex->translate[1]);

                  thisColour = pTex->pPixels[u + v * w];
                }
              }
              //else // TODO: Do something with this pTex->pPixels[pixel] at the end of this block (EVC-802)
              //{
              //  
              //  int index = pFBX->currMeshPointCount;
              //  if (pTex->pIArray)
              //    index = pTex->pIArray->GetAt(index);
              //  
              //  FbxDouble2 uv = pTex->pDArray->GetAt(index);
              //  int pixel = (int)(uv[0] + uv[1] * pTex->width);
              //  pTex->pPixels[pixel];
              //}

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

        uint32_t numPoints = UINT32_MAX;

        while (numPoints != 0)
        {
          UD_ERROR_CHECK(vdkTriangleVoxelizer_GetPoints(pFBX->pTrivox, &pFBX->pTriPositions, (double**)&pFBX->pTriWeights, &numPoints, pBuffer->pointsAllocated - pBuffer->pointCount));

          if (numPoints > 0)
          {
            vcFBXMaterial *pMat;

            if (pConvertInput->content & vdkAC_ARGB)
            {
              if (pFBX->map == FbxLayerElement::eByPolygon)
                pMat = &pFBX->materials[pFBX->pIndex->GetAt(pFBX->currMeshPolygon)];
              else
                pMat = &pFBX->materials[0];

              uint32_t thisColour = 0;

              for (int currTex = 0; currTex < pMat->textures.Size(); ++currTex)
              {
                vcFBXTexture *pTex = &pMat->textures[currTex];

                if (pTex->pTex != nullptr)
                {
                  for (int i = 0; i < 3; ++i)
                  {
                    FbxVector2 newVec = { 0, 0 };
                    int index = 0;
                    switch (pFBX->map)
                    {
                    case FbxLayerElement::eByPolygon:
                      index = pFBX->currMeshPolygon;
                      break;
                    case FbxLayerElement::eByPolygonVertex:
                      index = pFBX->currMeshPolygon * 3 + i;
                      break;
                    case FbxLayerElement::eAllSame:
                      index = 0;
                      break;
                    }

                    index = pTex->pIArray->GetAt(index);

                    newVec = pTex->pDArray->GetAt(index);
                    newVec[1] = 1 - newVec[1];

                    if (pTex->swap)
                    {
                      double swap = newVec[0];
                      newVec[0] = newVec[1];
                      newVec[1] = swap;
                    }

                    pFBX->uvQueue.Add(newVec);
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
                uint32_t colour = 0;
                if (pConvertInput->content & vdkAC_ARGB)
                {
                  for (int currTex = 0; currTex < pMat->textures.Size(); ++currTex)
                  {
                    vcFBXTexture *pTex = &pMat->textures[currTex];

                    int w = pTex->width;
                    int h = pTex->height;

                    if (pTex->pTex != nullptr)
                    {
                      FbxVector2 pointUV = pFBX->uvQueue[0 + currTex * 3] * pFBX->pTriWeights[point].x + pFBX->uvQueue[1 + currTex * 3] * pFBX->pTriWeights[point].y + pFBX->uvQueue[2 + currTex * 3] * pFBX->pTriWeights[point].z;

                      int u = (int)udMod(udRound(pointUV[0] * w), w);
                      u = (pTex->pTex->GetWrapModeU() == FbxTexture::eRepeat ? (u + w) % w : udClamp(u, 0, w));
                      int v = (int)udMod(udRound(1 - pointUV[1] * h), h);
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
                  pAttr = &pAttr[sizeof(uint32_t)];
                } // End if colour
              } // End for numpoints

              pBuffer->pointCount += numPoints / pFBX->everyNth;
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

  // Clean up
  for (int i = 0; i < pFBX->materials.Size(); ++i)
  {
    for (int j = 0; j < pFBX->materials[i].textures.Size(); ++j)
    {
      if (pFBX->materials[i].textures[j].pPixels)
        stbi_image_free(pFBX->materials[i].textures[j].pPixels);
      udFree(pFBX->materials[i].textures[j].pName);
    }
    pFBX->materials[i].textures.Clear();
  }

  pFBX->materials.Clear();

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
    if (pFBX->pTrivox)
      vdkTriangleVoxelizer_Destroy(&pFBX->pTrivox);

    if (pFBX->pManager != nullptr)
      pFBX->pManager->Destroy();

    udFree(pConvertInput->pName);
    udFree(pFBX);
  }
}

vdkError vcFBX_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, const char *pFilename)
{
  vdkConvertCustomItem customItem = {};

  vcFBX *pFBX = udAllocType(vcFBX, 1, udAF_Zero);
  if (pFBX == nullptr)
    return vE_MemoryAllocationFailure;

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
