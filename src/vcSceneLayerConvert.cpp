#include "vcSceneLayerConvert.h"
#include "vcSceneLayer_Internal.h"

#include "udPlatform/udChunkedArray.h"

#include "vdkConvertCustom.h"
#include "vdkTriangleVoxelizer.h"

struct vcSceneLayerConvert
{
  vcSceneLayer *pSceneLayer;

  int pointCount;
  int totalPoints;
  int polygonCount;
  double *pTriPositions;
  double *pTriWeights;
  bool triangleDone;
  vdkTriangleVoxelizer *pTrivox;

  int currentNodeIndex;
  udChunkedArray<vcSceneLayerNode*> leafNodes;
};

udResult vcSceneLayerConvert_Create(vcSceneLayerConvert **ppSceneLayerConvert, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  (*ppSceneLayerConvert) = udAllocType(vcSceneLayerConvert, 1, udAF_Zero);

  vcSceneLayer_Create(&(*ppSceneLayerConvert)->pSceneLayer, pWorkerThreadPool, pSceneLayerURL);

  (*ppSceneLayerConvert)->leafNodes.Init(128);

  return udR_Success;
}

udResult vcSceneLayerConvert_Destroy(vcSceneLayerConvert **ppSceneLayerConvert)
{
  vcSceneLayer_Destroy(&(*ppSceneLayerConvert)->pSceneLayer);
  (*ppSceneLayerConvert)->leafNodes.Deinit();

  udFree(*ppSceneLayerConvert);

  return udR_Success;
}

void vcSceneLayerConvert_RecursiveGenerateLeafNodeList(vcSceneLayerNode *pNode, udChunkedArray<vcSceneLayerNode*> &leafNodes)
{
  if (pNode->childrenCount == 0)
  {
    leafNodes.PushBack(pNode);
    return;
  }

  for (size_t i = 0; i < pNode->childrenCount; ++i)
    vcSceneLayerConvert_RecursiveGenerateLeafNodeList(&pNode->pChildren[i], leafNodes);
}

void vcSceneLayerConvert_GenerateLeafNodeList(vcSceneLayerConvert *pSceneLayerConvert)
{
  vcSceneLayerConvert_RecursiveGenerateLeafNodeList(&pSceneLayerConvert->pSceneLayer->root, pSceneLayerConvert->leafNodes);
}


vdkError vcSceneLayerConvert_Open(vdkConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, vdkConvertCustomItemFlags flags)
{
  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  vdkError result;

  pConvertInput->sourceResolution = pointResolution;
  pSceneLayerConvert->totalPoints = 100000;//pSceneLayer->root.pChildren[0].pGeometryData[0].vertCount;//pConvertInput->pointCount;
  pSceneLayerConvert->currentNodeIndex = 0;

  //UD_ERROR_CHECK()
  vcSceneLayer_LoadNodeData(pSceneLayerConvert->pSceneLayer);
  vcSceneLayerConvert_GenerateLeafNodeList(pSceneLayerConvert);

  // TODO: Do actual loading of nodes here
  // enum vcSceneLayerLoadFlag { vcSLLF_LoadAllNodes, vcSLLF_LoadTextures };
  // vcSceneLayerLoadFlag flags = vcSLLF_LoadAllNodes;
  // if (pBuffer->content & vdkCAC_ARGB)
  //  flags |= vcSLLF_LoadTextures;
  // vcSceneLayer_Load(flags);

  UD_ERROR_CHECK(vdkTriangleVoxelizer_Create(&pSceneLayerConvert->pTrivox, pointResolution));

  result = vE_Success;
epilogue:

  return result;
}

void vcSceneLayerConvert_Close(vdkConvertCustomItem *pConvertInput)
{
  if (pConvertInput->pData != nullptr)
  {
    vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
    vdkTriangleVoxelizer_Destroy(&pSceneLayerConvert->pTrivox);
    vcSceneLayerConvert_Destroy(&pSceneLayerConvert);
    udFree(pSceneLayerConvert);
  }
}

vdkError vcSceneLayerConvert_ReadPointsInt(vdkConvertCustomItem *pConvertInput, vdkConvertPointBufferInt64 *pBuffer)
{
  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  vdkError result;

  if (pBuffer->pAttributes == nullptr)
    return vE_Failure;

  memset(pBuffer->pAttributes, 0, pBuffer->attributeSize * pBuffer->pointsAllocated);
  pBuffer->pointCount = 0;

  while (pSceneLayerConvert->currentNodeIndex < pSceneLayerConvert->leafNodes.length)
  {
    vcSceneLayerNode *pNode = pSceneLayerConvert->leafNodes[pSceneLayerConvert->currentNodeIndex];
    if (pNode->geometryDataCount == 0)
    {
      pSceneLayerConvert->currentNodeIndex++;
      pSceneLayerConvert->polygonCount = 0;
      continue;
    }

    vcSceneLayerNode::GeometryData *pGeomData = &pNode->pGeometryData[0]; // todo: keep track of, and loop over all geometry
    uint64_t totalPolygons = pGeomData->vertCount / 3;
    vcSceneLayerNode::TextureData *pTextureData = nullptr;
    if (pNode->textureDataCount >= 0)
      pTextureData = &pNode->pTextureData[0]; // todo: validate!

    udDouble3 sceneLayerOrigin = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position - udDouble3::create(pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius);
    udDouble3 geomOrigin = pGeomData->originMatrix.axis.t.toVector3();
    udDouble3 geometryOriginOffset = geomOrigin - sceneLayerOrigin;

    while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->polygonCount < totalPolygons)
    {
      uint8_t *pAttr = &pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeSize];
      size_t maxPoints = pBuffer->pointsAllocated - pBuffer->pointCount;
      uint64_t numPoints = 0;
      udFloat2 vertexUVs[3] = {};

      // Get Vertices
      // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
      udFloat3 f0 = *((udFloat3*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 0) * pSceneLayerConvert->pSceneLayer->geometryVertexStride]);
      udFloat3 f1 = *((udFloat3*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 1) * pSceneLayerConvert->pSceneLayer->geometryVertexStride]);
      udFloat3 f2 = *((udFloat3*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 2) * pSceneLayerConvert->pSceneLayer->geometryVertexStride]);

      double p0[3], p1[3], p2[3];

      for (int i = 0; i < 3; ++i)
      {
        p0[i] = f0[i] + geometryOriginOffset[i];
        p1[i] = f1[i] + geometryOriginOffset[i];
        p2[i] = f2[i] + geometryOriginOffset[i];
      }

      UD_ERROR_CHECK(vdkTriangleVoxelizer_SetTriangle(pSceneLayerConvert->pTrivox, p0, p1, p2));
      UD_ERROR_CHECK(vdkTriangleVoxelizer_GetPoints(pSceneLayerConvert->pTrivox, &pSceneLayerConvert->pTriPositions, &pSceneLayerConvert->pTriWeights, &numPoints, maxPoints));

      if (pBuffer->content & vdkCAC_ARGB)
      {
        // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
        vertexUVs[0] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 0) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
        vertexUVs[1] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 1) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
        vertexUVs[2] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->polygonCount * 3 + 2) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
      }

      // write all points from current triangle
      for (size_t i = 0; i < numPoints; ++i)
      {
        for (size_t j = 0; j < 3; ++j)
          pBuffer->pPositions[pBuffer->pointCount + i][j] = (int64_t)(pSceneLayerConvert->pTriPositions[3 * i + j] / pConvertInput->sourceResolution);

        // Assign attributes
        if (pBuffer->content & vdkCAC_ARGB && pTextureData != nullptr)
        {
          udFloat2 pointUV = { 0, 0 };
          for (int j = 0; j < 3; ++j)
            pointUV += vertexUVs[j] * pSceneLayerConvert->pTriWeights[3 * i + j];

          int w = pTextureData->width;
          int h = pTextureData->height;
          int u = udMod(udRound(pointUV[0] * w), w);
          u += u < 0 ? w : 0; // ModMod would handle negatives in a single line but that has an extra division operation so this method is faster
          int v = udMod(udRound(pointUV[1] * h), h); // Invert v
          v += v < 0 ? h : 0;
          uint32_t colour = *(uint32_t*)(&pTextureData->pData[(u + v * w) * 4]); // ARGB
          uint8_t colours[4] = { (colour & 0xFF0000) >> 16, (colour & 0xFF00) >> 8, (colour & 0xFF), (colour & 0xFF000000) >> 24 }; // RGBA
          memcpy(pAttr, &colours, sizeof(colours));
          pAttr = &pAttr[sizeof(colours)];
        }
      }

      // if current triangle is done, go to next
      if (numPoints < maxPoints)
        ++pSceneLayerConvert->polygonCount;

      pBuffer->pointCount += numPoints;
    }

    if (pBuffer->pointCount >= pBuffer->pointsAllocated)
      break;
    else if (pSceneLayerConvert->polygonCount >= totalPolygons)
    {
      ++pSceneLayerConvert->currentNodeIndex;
      pSceneLayerConvert->polygonCount = 0;
    }
  }
  pSceneLayerConvert->pointCount += pBuffer->pointCount;

  result = vE_Success;
epilogue:

  return result;
}

vdkError vcSceneLayerConvert_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  vdkError result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;
  vdkConvertCustomItem customItem = {};

  // UD_ERROR_CHECK()
  vcSceneLayerConvert_Create(&pSceneLayerConvert, pWorkerThreadPool, pSceneLayerURL);

  // Populate customItem
  customItem.pData = pSceneLayerConvert;
  customItem.pOpen = vcSceneLayerConvert_Open;
  customItem.pClose = vcSceneLayerConvert_Close;
  customItem.pReadPointsInt = vcSceneLayerConvert_ReadPointsInt;
  customItem.pName = pSceneLayerURL;

  // TEMP - enable colour by default
  customItem.content = vdkCAC_ARGB;
  //customItem.content = vdkCAC_Intensity;

  customItem.srid = pSceneLayerConvert->pSceneLayer->root.zone.srid;
  customItem.sourceProjection = vdkCSP_SourceCartesian;

  // TEMP - use default res value here to get point estimate
  customItem.sourceResolution = 0.25; // 1;
                                      /*
  // Determine point count
  if (pFBX->totalPolygons > 0)
  {
    UD_ERROR_CHECK(vcFBX_EstimatePoints(pFBX, customItem.sourceResolution, &customItem.pointCount));
    customItem.pointCountIsEstimate = true;
  }
  else
  {
    customItem.pointCount = pFBX->pMesh->GetControlPointsCount();
    customItem.pointCountIsEstimate = false;
  }
  */

  double bounds = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius;
  customItem.boundMin[0] = 0.0;
  customItem.boundMin[1] = 0.0;
  customItem.boundMin[2] = 0.0;
  customItem.boundMax[0] = bounds * 2.0;
  customItem.boundMax[1] = bounds * 2.0;
  customItem.boundMax[2] = bounds * 2.0;
  customItem.boundsKnown = true;

  return vdkConvert_AddCustomItem(pContext, pConvertContext, &customItem);

epilogue:
  //pFBX->pManager->Destroy(); // destroying manager destroys all objects that were created with it
  //udFree(pFBX);

  return result;
}

