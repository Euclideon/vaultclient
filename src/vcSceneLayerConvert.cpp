#include "vcSceneLayerConvert.h"
#include "vcSceneLayer_Internal.h"

#include "udPlatform/udChunkedArray.h"

#include "vdkConvertCustom.h"
#include "vdkTriangleVoxelizer.h"

struct vcSceneLayerConvert
{
  vcSceneLayer *pSceneLayer;

  size_t leafIndex;
  size_t geometryIndex;
  size_t primIndex;
  size_t pointIndex;

  size_t totalPoints;
  vdkTriangleVoxelizer *pTrivox;

  vdkConvertCustomItemFlags flags;
  size_t everyNth;
  size_t everyNthAccum; // Accumulator for everyNth mode so calculations span triangles nicely

  udChunkedArray<vcSceneLayerNode*> leafNodes;
};

udResult vcSceneLayerConvert_Create(vcSceneLayerConvert **ppSceneLayerConvert, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;

  UD_ERROR_NULL(ppSceneLayerConvert, udR_InvalidParameter_);
  UD_ERROR_NULL(pWorkerThreadPool, udR_InvalidParameter_);
  UD_ERROR_NULL(pSceneLayerURL, udR_InvalidParameter_);

  pSceneLayerConvert = udAllocType(vcSceneLayerConvert, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayerConvert, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcSceneLayer_Create(&pSceneLayerConvert->pSceneLayer, pWorkerThreadPool, pSceneLayerURL));
  UD_ERROR_CHECK(pSceneLayerConvert->leafNodes.Init(128));

  *ppSceneLayerConvert = pSceneLayerConvert;
  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayerConvert_Destroy(vcSceneLayerConvert **ppSceneLayerConvert)
{
  udResult result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;

  UD_ERROR_NULL(ppSceneLayerConvert, udR_InvalidParameter_);
  UD_ERROR_NULL(*ppSceneLayerConvert, udR_InvalidParameter_);

  pSceneLayerConvert = (*ppSceneLayerConvert);
  *ppSceneLayerConvert = nullptr;

  UD_ERROR_CHECK(vcSceneLayer_Destroy(&pSceneLayerConvert->pSceneLayer));
  vdkTriangleVoxelizer_Destroy(&pSceneLayerConvert->pTrivox);
  UD_ERROR_CHECK(pSceneLayerConvert->leafNodes.Deinit());

  udFree(pSceneLayerConvert);

  result = udR_Success;
epilogue:
  return result;
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
  pSceneLayerConvert->flags = flags;
  pSceneLayerConvert->everyNth = everyNth;
  pSceneLayerConvert->totalPoints = pConvertInput->pointCount;

  if (vcSceneLayer_LoadNodeData(pSceneLayerConvert->pSceneLayer) != udR_Success)
    UD_ERROR_SET(vE_Failure);

  vcSceneLayerConvert_GenerateLeafNodeList(pSceneLayerConvert);

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
    vcSceneLayerConvert_Destroy(&pSceneLayerConvert);
  }
}

vdkError vcSceneLayerConvert_ReadPointsInt(vdkConvertCustomItem *pConvertInput, vdkConvertPointBufferInt64 *pBuffer)
{
  vdkError result;
  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  size_t pointCount = 0;
  udDouble3 sceneLayerOrigin = udDouble3::zero();

  if (pBuffer->pAttributes == nullptr)
    return vE_Failure;

  memset(pBuffer->pAttributes, 0, pBuffer->attributeSize * pBuffer->pointsAllocated);
  pBuffer->pointCount = 0;
  sceneLayerOrigin = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position - udDouble3::create(pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius);

  // For each node
  while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->leafIndex < pSceneLayerConvert->leafNodes.length)
  {
    vcSceneLayerNode *pNode = pSceneLayerConvert->leafNodes[pSceneLayerConvert->leafIndex];

    // For each geometry
    while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->geometryIndex < pNode->geometryDataCount)
    {
      vcSceneLayerNode::GeometryData *pGeomData = &pNode->pGeometryData[pSceneLayerConvert->geometryIndex];
      vcSceneLayerNode::TextureData *pTextureData = nullptr;
      uint64_t totalPrimitives = pGeomData->vertCount / 3; // TOOD: (EVC-540) other primitive types

      if (pNode->textureDataCount >= 0)
        pTextureData = &pNode->pTextureData[0]; // TODO: validate!

      udDouble3 geometryOriginOffset = pGeomData->originMatrix.axis.t.toVector3() - sceneLayerOrigin;

      // For each primitive
      while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->primIndex < totalPrimitives)
      {
        size_t maxPoints = pBuffer->pointsAllocated - pBuffer->pointCount;
        udFloat2 uvs[3] = {};
        udFloat3 positions[3] = {};

        // Get Vertices
        // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
        size_t v0ByteOffset = (pSceneLayerConvert->primIndex * 3 + 0) * pSceneLayerConvert->pSceneLayer->geometryVertexStride;
        size_t v1ByteOffset = (pSceneLayerConvert->primIndex * 3 + 1) * pSceneLayerConvert->pSceneLayer->geometryVertexStride;
        size_t v2ByteOffset = (pSceneLayerConvert->primIndex * 3 + 2) * pSceneLayerConvert->pSceneLayer->geometryVertexStride;
        memcpy(&positions[0], &pGeomData->pData[v0ByteOffset], sizeof(positions[0]));
        memcpy(&positions[1], &pGeomData->pData[v1ByteOffset], sizeof(positions[1]));
        memcpy(&positions[2], &pGeomData->pData[v2ByteOffset], sizeof(positions[2]));

        if (pBuffer->content & vdkCAC_ARGB)
        {
          // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
          size_t uvByteOffset = 24;
          memcpy(&uvs[0], &pGeomData->pData[v0ByteOffset + uvByteOffset], sizeof(uvs[0]));
          memcpy(&uvs[1], &pGeomData->pData[v1ByteOffset + uvByteOffset], sizeof(uvs[1]));
          memcpy(&uvs[2], &pGeomData->pData[v2ByteOffset + uvByteOffset], sizeof(uvs[2]));
          //uvs[0] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->primIndex * 3 + 0) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
          //uvs[1] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->primIndex * 3 + 1) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
          //uvs[2] = *((udFloat2*)&pGeomData->pData[(pSceneLayerConvert->primIndex * 3 + 2) * pSceneLayerConvert->pSceneLayer->geometryVertexStride + 24]);
        }

        double p0[3], p1[3], p2[3];
        for (int i = 0; i < 3; ++i)
        {
          p0[i] = positions[0][i] + geometryOriginOffset[i];
          p1[i] = positions[1][i] + geometryOriginOffset[i];
          p2[i] = positions[2][i] + geometryOriginOffset[i];
        }

        double *pTriPositions;
        double *pTriWeights;
        bool triangleDone = false;

        UD_ERROR_CHECK(vdkTriangleVoxelizer_SetTriangle(pSceneLayerConvert->pTrivox, p0, p1, p2));
        UD_ERROR_CHECK(vdkTriangleVoxelizer_GetPoints(pSceneLayerConvert->pTrivox, &pTriPositions, &pTriWeights, &pointCount, maxPoints));

        // Handle everyNth here in one place, slightly inefficiently but with the benefit of simplicity for the rest of the function
        if (pSceneLayerConvert->everyNth > 1)
        {
          size_t i, pc;
          for (i = pc = 0; i < pointCount; ++i)
          {
            if (++pSceneLayerConvert->everyNthAccum >= pSceneLayerConvert->everyNth)
            {
              memcpy(&pTriPositions[pc], &pTriPositions[i], sizeof(double) * 3);
              memcpy(&pTriWeights[pc], &pTriWeights[i], sizeof(double) * 3);
              ++pc;
              pSceneLayerConvert->everyNthAccum -= pSceneLayerConvert->everyNth;
            }
          }
          pointCount = pc;
        }

        // write all points from current triangle
        for (size_t i = 0; i < pointCount; ++i)
        {
          for (size_t j = 0; j < 3; ++j)
            pBuffer->pPositions[pBuffer->pointCount + i][j] = (int64_t)(pTriPositions[3 * i + j] / pConvertInput->sourceResolution);
        }

        // Assign attributes
        if (pBuffer->content & vdkCAC_ARGB)
        {
          // TODO: other handle variant attribute layouts
          uint32_t *pColour = (uint32_t*)udAddBytes(pBuffer->pAttributes, pBuffer->pointCount * pBuffer->attributeSize);// +udAttribute_GetAttributeOffset(udA_ARGB, pBuffer->content));

          if (pTextureData != nullptr)
          {
            for (size_t i = 0; i < pointCount; ++i, pColour = udAddBytes(pColour, pBuffer->attributeSize))
            {
              udFloat2 pointUV = { 0, 0 };
              for (int j = 0; j < 3; ++j)
                pointUV += uvs[j] * pTriWeights[3 * i + j];

              int u = (int)udMod(udRound(pointUV[0] * pTextureData->width) + pTextureData->width, (float)pTextureData->width);
              int v = (int)udMod(udRound(pointUV[1] * pTextureData->height) + pTextureData->height, (float)pTextureData->height);
              uint32_t colour = *(uint32_t*)(&pTextureData->pData[(u + v * pTextureData->width) * 4]);
              *pColour = 0xff000000 | ((colour >> 16) & 0xff) | (((colour >> 8) & 0xff) << 8) | (((colour >> 0) & 0xff) << 16);
            }
          }
          else
          {
            for (size_t i = 0; i < pointCount; ++i, pColour = udAddBytes(pColour, pBuffer->attributeSize))
              *pColour = 0xffffffff;
          }
        }

        // if current prim is done, go to next
        if (pointCount < maxPoints)
          ++pSceneLayerConvert->primIndex;

        pBuffer->pointCount += pointCount;
      }

      // Done with the current primitive?
      if (pSceneLayerConvert->primIndex >= totalPrimitives)
      {
        ++pSceneLayerConvert->geometryIndex;
        pSceneLayerConvert->primIndex = 0;
      }
    }

    // Done with the current geometry?
    if (pSceneLayerConvert->geometryIndex >= pNode->geometryDataCount)
    {
      ++pSceneLayerConvert->leafIndex;
      pSceneLayerConvert->primIndex = 0;
      pSceneLayerConvert->geometryIndex = 0;
    }
  }

  pSceneLayerConvert->pointIndex += pBuffer->pointCount;
  result = vE_Success;

epilogue:
  return result;
}

vdkError vcSceneLayerConvert_AddItem(vdkContext *pContext, vdkConvertContext *pConvertContext, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  vdkError result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;
  vdkConvertCustomItem customItem = {};

  if (vcSceneLayerConvert_Create(&pSceneLayerConvert, pWorkerThreadPool, pSceneLayerURL) != udR_Success)
    UD_ERROR_SET(vE_Failure);

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
  customItem.sourceResolution = 1.0; // 1;
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

  double radius = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius;
  customItem.boundMin[0] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.x - radius;
  customItem.boundMin[1] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.y - radius;
  customItem.boundMin[2] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.z - radius;
  customItem.boundMax[0] = customItem.boundMin[0] + radius * 2.0;
  customItem.boundMax[1] = customItem.boundMin[1] + radius * 2.0;
  customItem.boundMax[2] = customItem.boundMin[2] + radius * 2.0;
  customItem.boundsKnown = true;

  return vdkConvert_AddCustomItem(pContext, pConvertContext, &customItem);

epilogue:
  vcSceneLayerConvert_Destroy(&pSceneLayerConvert);
  return result;
}

