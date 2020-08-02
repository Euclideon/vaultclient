#include "vcSceneLayerConvert.h"
#include "vcSceneLayer_Internal.h"

#include "udChunkedArray.h"
#include "udPlatformUtil.h"

#include "udPointCloud.h"
#include "udConvertCustom.h"
#include "udTriangleVoxelizer.h"

struct vcSceneLayerConvert
{
  vcSceneLayer *pSceneLayer;

  size_t leafIndex;
  size_t geometryIndex;
  size_t primIndex;
  size_t lastPrimedPrimitive;

  size_t totalPrimIndex;
  size_t totalPrimCount;
  size_t totalPoints;
  udTriangleVoxelizer *pTrivox;

  udConvertCustomItemFlags flags;
  size_t pointsReturned;
  int64_t triangleArea;
  int64_t triangleAreaReturned; // Area of triangles returned so far (used to estimate how many to go)
  size_t everyNth;
  size_t everyNthAccum; // Accumulator for everyNth mode so calculations span triangles nicely

  udDouble3 worldOrigin;

  udChunkedArray<vcSceneLayerNode*> leafNodes;
};

udResult vcSceneLayerConvert_Create(vcSceneLayerConvert **ppSceneLayerConvert, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;

  UD_ERROR_NULL(ppSceneLayerConvert, udR_InvalidParameter_);
  UD_ERROR_NULL(pSceneLayerURL, udR_InvalidParameter_);

  pSceneLayerConvert = udAllocType(vcSceneLayerConvert, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayerConvert, udR_MemoryAllocationFailure);

  // Convert doesn't use a worker pool at the moment
  UD_ERROR_CHECK(vcSceneLayer_Create(&pSceneLayerConvert->pSceneLayer, nullptr, pSceneLayerURL));

  *ppSceneLayerConvert = pSceneLayerConvert;
  pSceneLayerConvert = nullptr;
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

  vcSceneLayer_Destroy(&pSceneLayerConvert->pSceneLayer);
  udFree(pSceneLayerConvert);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayerConvert_RecursiveGenerateLeafNodeList(vcSceneLayerConvert *pSceneLayerConvert, vcSceneLayerNode *pNode, udChunkedArray<vcSceneLayerNode*> &leafNodes)
{
  udResult result;

  // Root already loaded
  if (pNode != &pSceneLayerConvert->pSceneLayer->root)
    UD_ERROR_CHECK(vcSceneLayer_LoadNode(pSceneLayerConvert->pSceneLayer, pNode));

  if (pNode->childrenCount == 0)
  {
    leafNodes.PushBack(pNode);
    UD_ERROR_SET(udR_Success);
  }

  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    UD_ERROR_CHECK(vcSceneLayerConvert_RecursiveGenerateLeafNodeList(pSceneLayerConvert, &pNode->pChildren[i], leafNodes));
  }

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayerConvert_GenerateLeafNodeList(vcSceneLayerConvert *pSceneLayerConvert)
{
  return vcSceneLayerConvert_RecursiveGenerateLeafNodeList(pSceneLayerConvert , &pSceneLayerConvert->pSceneLayer->root, pSceneLayerConvert->leafNodes);
}

udResult vcSceneLayerConvert_GatherEstimates(vcSceneLayerConvert *pSceneLayerConvert, double gridResolution)
{
  udResult result;

  UD_ERROR_NULL(pSceneLayerConvert, udR_InvalidParameter_);

  pSceneLayerConvert->triangleArea = 0;
  pSceneLayerConvert->totalPrimCount = 0;

  for (size_t i = 0; i < pSceneLayerConvert->leafNodes.length; ++i)
  {
    vcSceneLayerNode *pNode = pSceneLayerConvert->leafNodes[i];
    for (size_t g = 0; g < pNode->geometryDataCount; ++g)
    {
      vcSceneLayerNode::GeometryData *pGeometry = &pNode->pGeometryData[g];
      size_t geometryPrimitiveCount = pGeometry->vertCount / 3; // TOOD: (EVC-540) other primitive types
      pSceneLayerConvert->totalPrimCount += geometryPrimitiveCount;

      for (size_t v = 0; v < geometryPrimitiveCount; ++v)
      {
        // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
        vcSceneLayerVertex* pVert0 = vcSceneLayer_GetVertex(pGeometry, v * 3 + 0);
        vcSceneLayerVertex* pVert1 = vcSceneLayer_GetVertex(pGeometry, v * 3 + 1);
        vcSceneLayerVertex* pVert2 = vcSceneLayer_GetVertex(pGeometry, v * 3 + 2);

        const udLong3 v0 = udLong3::create(pVert0->position / gridResolution);
        const udLong3 v1 = udLong3::create(pVert1->position / gridResolution);
        const udLong3 v2 = udLong3::create(pVert2->position / gridResolution);
        pSceneLayerConvert->triangleArea += (int64_t)udRound(udMag(udFloat3::create(udCross((v1 - v0), (v2 - v0)))) * 0.5);
      }
    }
  }

  result = udR_Success;

epilogue:
  return result;
}

udError vcSceneLayerConvert_Open(udConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, udConvertCustomItemFlags flags)
{
  udUnused(origin);

  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  udError result;

  pConvertInput->sourceResolution = pointResolution;
  pSceneLayerConvert->flags = flags;
  pSceneLayerConvert->everyNth = everyNth;
  pSceneLayerConvert->totalPoints = pConvertInput->pointCount;
  pSceneLayerConvert->totalPrimCount = 0;
  pSceneLayerConvert->totalPrimIndex = 0;
  pSceneLayerConvert->lastPrimedPrimitive = 0xffffffff;
  pSceneLayerConvert->worldOrigin = udDouble3::create(origin[0], origin[1], origin[2]);
  pSceneLayerConvert->leafIndex = 0;

  if (pSceneLayerConvert->leafNodes.Init(128) != udR_Success)
    UD_ERROR_SET(udE_Failure);
  
  // Recursively load child nodes
  if (vcSceneLayerConvert_GenerateLeafNodeList(pSceneLayerConvert) != udR_Success)
    UD_ERROR_SET(udE_Failure);

  if (vcSceneLayerConvert_GatherEstimates(pSceneLayerConvert, pConvertInput->sourceResolution) != udR_Success)
    UD_ERROR_SET(udE_Failure);

  UD_ERROR_CHECK(udTriangleVoxelizer_Create(&pSceneLayerConvert->pTrivox, pointResolution));

  result = udE_Success;
epilogue:

  return result;
}

void vcSceneLayerConvert_Close(udConvertCustomItem *pConvertInput)
{
  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  pSceneLayerConvert->leafNodes.Deinit();
  udTriangleVoxelizer_Destroy(&pSceneLayerConvert->pTrivox);
}

void vcSceneLayerConvert_Destroy(udConvertCustomItem *pConvertInput)
{
  vcSceneLayerConvert *pSceneLayerConvert = (vcSceneLayerConvert *)pConvertInput->pData;
  vcSceneLayerConvert_Destroy(&pSceneLayerConvert);
  udAttributeSet_Destroy(&pConvertInput->attributes);
  pConvertInput->pData = nullptr;
}

udError vcSceneLayerConvert_ReadPointsInt(udConvertCustomItem *pConvertInput, udPointBufferI64 *pBuffer)
{
  udError result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;
  uint32_t pointCount = 0;
  udDouble3 sceneLayerOrigin = udDouble3::zero();
  bool allPointsReturned = false;
  udDouble3 localJobOriginOffset = udDouble3::zero();

  UD_ERROR_NULL(pConvertInput, udE_Failure);
  UD_ERROR_NULL(pConvertInput->pData, udE_Failure);
  UD_ERROR_NULL(pBuffer, udE_Failure);
  UD_ERROR_NULL(pBuffer->pAttributes, udE_Failure);

  pSceneLayerConvert = (vcSceneLayerConvert*)pConvertInput->pData;
  memset(pBuffer->pAttributes, 0, pBuffer->attributeStride * pBuffer->pointsAllocated);
  pBuffer->pointCount = 0;
  sceneLayerOrigin = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position - udDouble3::create(pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius);
  localJobOriginOffset = sceneLayerOrigin - pSceneLayerConvert->worldOrigin;

  // For each node
  while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->leafIndex < pSceneLayerConvert->leafNodes.length)
  {
    vcSceneLayerNode *pNode = pSceneLayerConvert->leafNodes[pSceneLayerConvert->leafIndex];
    if (pNode->internalsLoadState == vcSceneLayerNode::vcILS_BasicNodeData) // begin the node
      vcSceneLayer_LoadNodeInternals(pSceneLayerConvert->pSceneLayer, pNode);

    // For each geometry
    while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->geometryIndex < pNode->geometryDataCount)
    {
      vcSceneLayerNode::GeometryData *pGeometry = &pNode->pGeometryData[pSceneLayerConvert->geometryIndex];
      vcSceneLayerNode::TextureData *pTextureData = nullptr;
      uint64_t geometryPrimitiveCount = pGeometry->vertCount / 3; // TOOD: (EVC-540) other primitive types

      if (pNode->textureDataCount > 0)
        pTextureData = &pNode->pTextureData[0]; // TODO: (EVC-542) validate!

      udDouble3 geometryOriginOffset = pGeometry->originMatrix.axis.t.toVector3() - sceneLayerOrigin;

      // For each primitive
      while (pBuffer->pointCount < pBuffer->pointsAllocated && pSceneLayerConvert->primIndex < geometryPrimitiveCount)
      {
        uint32_t maxPoints = pBuffer->pointsAllocated - pBuffer->pointCount;

        // Get Vertices
        // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
        vcSceneLayerVertex *v0 = vcSceneLayer_GetVertex(pGeometry, pSceneLayerConvert->primIndex * 3 + 0);
        vcSceneLayerVertex *v1 = vcSceneLayer_GetVertex(pGeometry, pSceneLayerConvert->primIndex * 3 + 1);
        vcSceneLayerVertex *v2 = vcSceneLayer_GetVertex(pGeometry, pSceneLayerConvert->primIndex * 3 + 2);

        if (pSceneLayerConvert->lastPrimedPrimitive != pSceneLayerConvert->primIndex)
        {
          double p0[3], p1[3], p2[3];
          for (int i = 0; i < 3; ++i)
          {
            p0[i] = v0->position[i] + geometryOriginOffset[i];
            p1[i] = v1->position[i] + geometryOriginOffset[i];
            p2[i] = v2->position[i] + geometryOriginOffset[i];
          }

          UD_ERROR_CHECK(udTriangleVoxelizer_SetTriangle(pSceneLayerConvert->pTrivox, p0, p1, p2));
          pSceneLayerConvert->lastPrimedPrimitive = pSceneLayerConvert->primIndex;
        }

        double *pTriPositions = nullptr;
        double *pTriWeights = nullptr;
        UD_ERROR_CHECK(udTriangleVoxelizer_GetPoints(pSceneLayerConvert->pTrivox, &pTriPositions, &pTriWeights, &pointCount, maxPoints));

        // Handle everyNth here in one place, slightly inefficiently but with the benefit of simplicity for the rest of the function
        if (pSceneLayerConvert->everyNth > 1)
        {
          uint32_t i, pc;
          for (i = pc = 0; i < pointCount; ++i)
          {
            ++pSceneLayerConvert->everyNthAccum;

            if (pSceneLayerConvert->everyNthAccum >= pSceneLayerConvert->everyNth)
            {
              memcpy(&pTriPositions[pc * 3], &pTriPositions[i * 3], sizeof(double) * 3);
              memcpy(&pTriWeights[pc * 3], &pTriWeights[i * 3], sizeof(double) * 3);
              ++pc;
              pSceneLayerConvert->everyNthAccum -= pSceneLayerConvert->everyNth;
            }
          }
          pointCount = pc;
        }

        // write all points from current triangle
        for (uint64_t i = 0; i < pointCount; ++i)
        {
          for (size_t j = 0; j < 3; ++j)
            pBuffer->pPositions[(pBuffer->pointCount + i) * 3 + j] = (int64_t)((pTriPositions[3 * i + j] + localJobOriginOffset[j]) / pConvertInput->sourceResolution);
        }

        // Assign attributes
        if (pBuffer->attributes.content & udSAC_ARGB)
        {
          // TODO: (EVC-540) other handle variant attribute layouts
          // Actually use `udAttribute_GetAttributeOffset(udCAC_ARGB, pBuffer->content)` correctly
          uint32_t *pColour = (uint32_t*)udAddBytes(pBuffer->pAttributes, pBuffer->pointCount * pBuffer->attributeStride);

          if (pTextureData != nullptr)
          {
            for (uint64_t i = 0; i < pointCount; ++i, pColour = udAddBytes(pColour, pBuffer->attributeStride))
            {
              udFloat2 pointUV = { 0, 0 };
              pointUV += v0->uv0 * pTriWeights[3 * i + 0];
              pointUV += v1->uv0 * pTriWeights[3 * i + 1];
              pointUV += v2->uv0 * pTriWeights[3 * i + 2];

              float width = (float)pTextureData->width;
              float height = (float)pTextureData->height;
              int u = (int)udMod(udMod(udRound(pointUV.x * width), width) + width, width);
              int v = (int)udMod(udMod(udRound(pointUV.y * height), height) + height, height);
              uint32_t colour = *(uint32_t*)(&pTextureData->pData[(u + v * pTextureData->width) * 4]);
              *pColour = 0xff000000 | ((colour & 0xff) << 16) | (colour & 0xff00) | ((colour & 0xff0000) >> 16);
            }
          }
          else
          {
            for (uint64_t i = 0; i < pointCount; ++i, pColour = udAddBytes(pColour, pBuffer->attributeStride))
              *pColour = 0xffffffff;
          }
        }

        // if current prim is done, go to next
        if (pointCount == 0)
        {
          // If using triangle area estimate method, calculate area returned as we go
          if (pSceneLayerConvert->triangleArea)
          {
            const udLong3 vl0 = udLong3::create(v0->position / pConvertInput->sourceResolution);
            const udLong3 vl1 = udLong3::create(v1->position / pConvertInput->sourceResolution);
            const udLong3 vl2 = udLong3::create(v2->position / pConvertInput->sourceResolution);
            pSceneLayerConvert->triangleAreaReturned += (int64_t)udRound(udMag(udFloat3::create(udCross((vl1 - vl0), (vl2 - vl0)))) * 0.5);
          }

          // current primitive done, move on to next
          ++pSceneLayerConvert->primIndex;
          ++pSceneLayerConvert->totalPrimIndex;
        }
        pBuffer->pointCount += pointCount;
      }

      if (pSceneLayerConvert->primIndex >= geometryPrimitiveCount)
      {
        // current geometry done, move onto nexet
        ++pSceneLayerConvert->geometryIndex;
        pSceneLayerConvert->primIndex = 0;
      }
    }

    if (pSceneLayerConvert->geometryIndex >= pNode->geometryDataCount)
    {
      // current leaf done, move onto next
      vcSceneLayer_RecursiveDestroyNode(pNode);

      ++pSceneLayerConvert->leafIndex;
      pSceneLayerConvert->primIndex = 0;
      pSceneLayerConvert->geometryIndex = 0;
    }
  }

  pSceneLayerConvert->pointsReturned += pBuffer->pointCount;
  allPointsReturned = (pSceneLayerConvert->totalPrimIndex == pSceneLayerConvert->totalPrimCount);

  if (allPointsReturned)  // Always assign the actual points at the end
    pConvertInput->pointCount = pSceneLayerConvert->pointsReturned;
  else if (pSceneLayerConvert->triangleArea && pSceneLayerConvert->triangleAreaReturned) // If udOBJ_EstimatePoints was called, we can safely improve the accuracy of that estimate
    pConvertInput->pointCount = pSceneLayerConvert->triangleArea * pSceneLayerConvert->pointsReturned / pSceneLayerConvert->triangleAreaReturned;
  else if (pSceneLayerConvert->totalPrimIndex)
    pConvertInput->pointCount = pSceneLayerConvert->totalPrimCount * pSceneLayerConvert->pointsReturned / pSceneLayerConvert->totalPrimIndex;

  result = udE_Success;

epilogue:
  return result;
}

udError vcSceneLayerConvert_AddItem(udConvertContext *pConvertContext, const char *pSceneLayerURL)
{
  udError result;
  vcSceneLayerConvert *pSceneLayerConvert = nullptr;
  udConvertCustomItem customItem = {};

  if (vcSceneLayerConvert_Create(&pSceneLayerConvert, pSceneLayerURL) != udR_Success)
    UD_ERROR_SET(udE_Failure);

  // load root data now
  if (vcSceneLayer_LoadNode(pSceneLayerConvert->pSceneLayer, &pSceneLayerConvert->pSceneLayer->root) != udR_Success)
    UD_ERROR_SET(udE_Failure);

  customItem.pData = pSceneLayerConvert;
  customItem.pOpen = vcSceneLayerConvert_Open;
  customItem.pClose = vcSceneLayerConvert_Close;
  customItem.pDestroy = vcSceneLayerConvert_Destroy;
  customItem.pReadPointsInt = vcSceneLayerConvert_ReadPointsInt;
  customItem.pName = pSceneLayerURL;
  udAttributeSet_Create(&customItem.attributes, udSAC_ARGB, 0);
  customItem.srid = pSceneLayerConvert->pSceneLayer->root.zone.srid;
  customItem.sourceProjection = udCSP_SourceCartesian;
  customItem.pointCount = -1;
  customItem.pointCountIsEstimate = true;

  customItem.boundMin[0] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.x - pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius;
  customItem.boundMin[1] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.y - pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius;
  customItem.boundMin[2] = pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.position.z - pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius;
  customItem.boundMax[0] = customItem.boundMin[0] + pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius * 2.0;
  customItem.boundMax[1] = customItem.boundMin[1] + pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius * 2.0;
  customItem.boundMax[2] = customItem.boundMin[2] + pSceneLayerConvert->pSceneLayer->root.minimumBoundingSphere.radius * 2.0;
  customItem.boundsKnown = true;

  UD_ERROR_CHECK(udConvert_AddCustomItem(pConvertContext, &customItem));

  result = udE_Success;

epilogue:
  if (result != udE_Success)
    vcSceneLayerConvert_Destroy(&pSceneLayerConvert);

  return result;
}

