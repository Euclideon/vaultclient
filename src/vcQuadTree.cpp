#include "vcQuadTree.h"
#include "vcGIS.h"
#include "gl/vcTexture.h"

#define INVALID_NODE_INDEX 0xffffffff

// 0.55 degrees and below is *roughly* where we want the 'horizon' culling to occur
const double tileToCameraCullAngle = UD_DEG2RAD(0.2);

// Cap depth at level 19 (system doesn't have access to these tiles yet)
enum
{
  NodeChildCount = 4,
};

static const int vcTRMQToDepthModifiers[vcTRMQ_Total] = { 4, 2, 0 };

// Returns -1=outside, 0=inside, >0=partial (bits of planes crossed)
static int vcQuadTree_FrustumTest(const udDouble4 frustumPlanes[6], const udDouble3 &boundCenter, const udDouble3 &boundExtents)
{
  int partial = 0;

  for (int i = 0; i < 6; ++i)
  {
    double distToCenter = udDot4(udDouble4::create(boundCenter, 1.0), frustumPlanes[i]);
    //optimized for case where boxExtents are all same: udFloat radiusBoxAtPlane = udDot3(boxExtents, udAbs(udVec3(curPlane)));
    double radiusBoxAtPlane = udDot3(boundExtents, udAbs(frustumPlanes[i].toVector3()));
    if (distToCenter < -radiusBoxAtPlane)
      return -1; // Box is entirely behind at least one plane
    else if (distToCenter <= radiusBoxAtPlane) // If spanned (not entirely infront)
      partial |= (1 << i);
  }

  return partial;
}

udResult vcQuadTree_ExpandCapacity(vcQuadTree *pQuadTree)
{
  udResult result = udR_Success;
  vcQuadTreeNode *pNewNodeMemory = nullptr;
  uint32_t newCapacity = pQuadTree->nodes.capacity * 2;

  pNewNodeMemory = udAllocType(vcQuadTreeNode, newCapacity, udAF_Zero);
  UD_ERROR_NULL(pNewNodeMemory, udR_MemoryAllocationFailure);

  if (pQuadTree->nodes.pPool)
  {
    memcpy(pNewNodeMemory, pQuadTree->nodes.pPool, pQuadTree->nodes.capacity * sizeof(vcQuadTreeNode));
    udFree(pQuadTree->nodes.pPool);
  }

  pQuadTree->nodes.pPool = pNewNodeMemory;
  pQuadTree->nodes.capacity = newCapacity;
  pQuadTree->metaData.memUsageMB = (sizeof(vcQuadTreeNode) * newCapacity) >> 20;

epilogue:
  return result;
}

uint32_t vcQuadTree_GetNodeIndex(vcQuadTree *pQuadTree, const udInt3 &slippy)
{
  vcQuadTreeNode *pNode = nullptr;
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
  {
    pNode = &pQuadTree->nodes.pPool[i];
    if (pNode->isUsed && pNode->slippyPosition == slippy)
      return i;
  }

  return INVALID_NODE_INDEX;
}

uint32_t vcQuadTree_FindFreeChildBlock(vcQuadTree *pQuadTree)
{
  for (uint32_t blockIndex = 0; blockIndex < pQuadTree->nodes.used; blockIndex += NodeChildCount)
  {
    if (!pQuadTree->nodes.pPool[blockIndex].isUsed) // whole blocks are marked as unused
      return blockIndex;
  }

  if (pQuadTree->nodes.used >= pQuadTree->nodes.capacity)
    return INVALID_NODE_INDEX;

  pQuadTree->nodes.used += NodeChildCount;
  return pQuadTree->nodes.used - NodeChildCount;
}

// TODO: DEM effect
double vcQuadTree_PointToRectDistance(udDouble2 edges[4], const udDouble3 &point)
{
  static const udInt2 edgePairs[] =
  {
    udInt2::create(0, 1), // top
    udInt2::create(0, 2), // left
    udInt2::create(2, 3), // bottom
    udInt2::create(1, 3), // right
  };

  // Not true distance, XY plane distance has more weighting
  double closestEdgeDistance = 0.0;

  // test each edge to find minimum distance to quadrant shape (2d)
  for (int e = 0; e < 4; ++e)
  {
    udDouble2 p1 = edges[edgePairs[e].x];
    udDouble2 p2 = edges[edgePairs[e].y];

    udDouble2 edge = p2 - p1;
    double r = udDot2(edge, (point.toVector2() - p1)) / udMagSq2(edge);

    // 2d edge has been found, now factor in z for distances to camera
    // TODO: tile heights (DEM)
    udDouble3 closestPointOnEdge = udDouble3::create(p1 + udClamp(r, 0.0, 1.0) * edge, 0.0);

    double distToEdge = udMag3(closestPointOnEdge - point);
    closestEdgeDistance = (e == 0) ? distToEdge : udMin(closestEdgeDistance, distToEdge);
  }

  return closestEdgeDistance;
}

void vcQuadTree_CleanupNode(vcQuadTreeNode *pNode)
{
  vcTexture_Destroy(&pNode->renderInfo.pTexture);
  udFree(pNode->renderInfo.pData);

  memset(pNode, 0, sizeof(vcQuadTreeNode));
}

void vcQuadTree_CalculateNodeBounds(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  for (int edge = 0; edge < 4; ++edge)
  {
    udDouble3 localCorner;
    vcGIS_SlippyToLocal(&pQuadTree->gisSpace, &localCorner, udInt2::create(pNode->slippyPosition.x + (edge & 1), pNode->slippyPosition.y + (edge >> 1)), pNode->slippyPosition.z);
    pNode->worldBounds[edge] = localCorner.toVector2();
  }

  udDouble2 boundsMin = udDouble2::create(udMin(udMin(udMin(pNode->worldBounds[0].x, pNode->worldBounds[1].x), pNode->worldBounds[2].x), pNode->worldBounds[3].x),
    udMin(udMin(udMin(pNode->worldBounds[0].y, pNode->worldBounds[1].y), pNode->worldBounds[2].y), pNode->worldBounds[3].y));
  udDouble2 boundsMax = udDouble2::create(udMax(udMax(udMax(pNode->worldBounds[0].x, pNode->worldBounds[1].x), pNode->worldBounds[2].x), pNode->worldBounds[3].x),
    udMax(udMax(udMax(pNode->worldBounds[0].y, pNode->worldBounds[1].y), pNode->worldBounds[2].y), pNode->worldBounds[3].y));

  pNode->tileCenter = (boundsMax + boundsMin) * 0.5;
  pNode->tileExtents = (boundsMax - boundsMin) * 0.5;
}

void vcQuadTree_InitNode(vcQuadTree *pQuadTree, uint32_t slotIndex, const udInt3 &childSlippy)
{
  vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[slotIndex];

  pNode->isUsed = true;
  pNode->childBlockIndex = INVALID_NODE_INDEX;
  pNode->parentIndex = INVALID_NODE_INDEX;
  pNode->slippyPosition = childSlippy;

  vcQuadTree_CalculateNodeBounds(pQuadTree, pNode);
}

bool vcQuadTree_IsNodeVisible(const vcQuadTree *pQuadTree, const vcQuadTreeNode *pNode)
{
  return -1 < vcQuadTree_FrustumTest(pQuadTree->frustumPlanes, udDouble3::create(pNode->tileCenter, pQuadTree->quadTreeHeightOffset), udDouble3::create(pNode->tileExtents, 0.0));
}

inline bool vcQuadTree_ShouldSubdivide(vcQuadTree *pQuadTree, double distanceMS, int depth)
{
  return distanceMS < (1.0 / (1 << (depth + vcTRMQToDepthModifiers[pQuadTree->pSettings->maptiles.mapQuality])));
}

void vcQuadTree_RecurseGenerateTree(vcQuadTree *pQuadTree, uint32_t currentNodeIndex, int currentDepth)
{
  vcQuadTreeNode *pCurrentNode = &pQuadTree->nodes.pPool[currentNodeIndex];
  pCurrentNode->childMask = 0;

  if (currentDepth >= pQuadTree->metaData.maxTreeDepth)
  {
    ++pQuadTree->metaData.leafNodeCount;
    return;
  }

  if (pCurrentNode->childBlockIndex == INVALID_NODE_INDEX)
  {
    uint32_t freeBlockIndex = vcQuadTree_FindFreeChildBlock(pQuadTree);
    if (freeBlockIndex == INVALID_NODE_INDEX)
      return;

    udInt3 childSlippy = udInt3::create(pCurrentNode->slippyPosition.x << 1, pCurrentNode->slippyPosition.y << 1, pCurrentNode->slippyPosition.z + 1);
    for (uint32_t i = 0; i < NodeChildCount; ++i)
      vcQuadTree_InitNode(pQuadTree, freeBlockIndex + i, childSlippy + udInt3::create(i & 1, i >> 1, 0));

    pCurrentNode->childBlockIndex = freeBlockIndex;
  }

  udInt2 pViewSlippyCoords;
  vcGIS_LocalToSlippy(&pQuadTree->gisSpace, &pViewSlippyCoords, pQuadTree->cameraWorldPosition, pQuadTree->slippyCoords.z + currentDepth + 1);

  //subdivide
  // 0 == bottom left
  // 1 == bottom right
  // 2 == top left
  // 3 == top right
  for (uint32_t childQuadrant = 0; childQuadrant < NodeChildCount; ++childQuadrant)
  {
    pCurrentNode->childMask |= 1 << childQuadrant;

    uint32_t childIndex = pCurrentNode->childBlockIndex + childQuadrant;
    vcQuadTreeNode *pChildNode = &pQuadTree->nodes.pPool[childIndex];
    pChildNode->touched = true;

    // leave this here as we could be fixing up a re-root
    pChildNode->parentIndex = currentNodeIndex;
    pChildNode->level = currentDepth + 1;
    pChildNode->visible = pCurrentNode->visible && vcQuadTree_IsNodeVisible(pQuadTree, pChildNode);

    // TODO: tile heights (DEM)
    double distanceToQuadrant;

    int32_t slippyManhattanDist = udAbs(pViewSlippyCoords.x - pChildNode->slippyPosition.x) + udAbs(pViewSlippyCoords.y - pChildNode->slippyPosition.y);
    if (slippyManhattanDist != 0)
    {
      distanceToQuadrant = vcQuadTree_PointToRectDistance(pChildNode->worldBounds, pQuadTree->cameraTreePosition);
      bool withinHorizon = udAbs(udASin(pQuadTree->cameraTreePosition.z / distanceToQuadrant)) >= tileToCameraCullAngle;
      pChildNode->visible = pChildNode->visible && withinHorizon;
    }
    else
    {
      distanceToQuadrant = udAbs(pQuadTree->cameraTreePosition.z);
    }

    // Artificially change the distances of tiles based on their relative depths.
    // Flattens out lower layers, while raising levels of tiles further away.
    // This is done because of perspectiveness, we actually want a non-uniform quad tree.
    // Note: these values were just 'trial and error'ed
    int nodeDepthToTreeDepth = pQuadTree->expectedTreeDepth - currentDepth;
    distanceToQuadrant *= udLerp(1.0, (0.6 + 0.25 * nodeDepthToTreeDepth), udClamp(nodeDepthToTreeDepth, 0, 1));

    ++pQuadTree->metaData.nodeTouchedCount;
    if (pChildNode->visible)
      ++pQuadTree->metaData.visibleNodeCount;
    else if ((pQuadTree->pSettings->maptiles.mapOptions & vcTRF_OnlyRequestVisibleTiles) != 0)
      continue;

    // this `10000000.0` is arbitrary trial and error'd
    double distanceMS = (distanceToQuadrant / udMin(10000000.0, pQuadTree->quadTreeWorldSize));
    if (vcQuadTree_ShouldSubdivide(pQuadTree, distanceMS, currentDepth))
      vcQuadTree_RecurseGenerateTree(pQuadTree, childIndex, currentDepth + 1);
    else
      ++pQuadTree->metaData.leafNodeCount;
  }
}

void vcQuadTree_CleanupNodes(vcQuadTree *pQuadTree)
{
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
    vcQuadTree_CleanupNode(&pQuadTree->nodes.pPool[i]);

  pQuadTree->nodes.used = 0;
}

void vcQuadTree_Create(vcQuadTree *pQuadTree, vcSettings *pSettings)
{
  pQuadTree->nodes.capacity = 1024; // best guess, should hold enough nodes for usage
  vcQuadTree_ExpandCapacity(pQuadTree);

  vcQuadTree_CleanupNodes(pQuadTree);
  vcQuadTree_Reset(pQuadTree);

  pQuadTree->pSettings = pSettings;
}

void vcQuadTree_Destroy(vcQuadTree *pQuadTree)
{
  vcQuadTree_CleanupNodes(pQuadTree);

  udFree(pQuadTree->nodes.pPool);
  pQuadTree->nodes.capacity = 0;
}

void vcQuadTree_Reset(vcQuadTree *pQuadTree)
{
  memset(&pQuadTree->metaData, 0, sizeof(vcQuadTreeMetaData));
  pQuadTree->completeRerootRequired = true;
}

uint32_t vcQuadTree_NodeIndexToBlockIndex(uint32_t nodeIndex)
{
  return nodeIndex & ~3;
}

void vcQuadTree_Reroot(vcQuadTree *pQuadTree, const udInt3 &slippyCoords)
{
  // First determine if that node already exists
  uint32_t newRootIndex = vcQuadTree_GetNodeIndex(pQuadTree, slippyCoords);
  if (newRootIndex == INVALID_NODE_INDEX)
  {
    // Could not find node, add a new root one layer higher
    vcQuadTreeNode *pOldRoot = &pQuadTree->nodes.pPool[pQuadTree->rootIndex];
    uint32_t newRootBlockIndex = vcQuadTree_FindFreeChildBlock(pQuadTree);
    if (newRootBlockIndex == INVALID_NODE_INDEX || pOldRoot->slippyPosition.z < slippyCoords.z)
    {
      // signal a complete re-root is required
      pQuadTree->completeRerootRequired = true;
      return;
    }

    udInt3 newRootSlippy = udInt3::create(pOldRoot->slippyPosition.x >> 1, pOldRoot->slippyPosition.y >> 1, pOldRoot->slippyPosition.z - 1);
    udInt3 newRootBlockSlippy = udInt3::create((newRootSlippy.x >> 1) << 1, (newRootSlippy.y >> 1) << 1, newRootSlippy.z);
    for (uint32_t c = 0; c < NodeChildCount; ++c)
    {
      vcQuadTree_InitNode(pQuadTree, newRootBlockIndex + c, newRootBlockSlippy + udInt3::create(c & 1, c >> 1, 0));

      // preserve spatial position of root in its root block
      if (pQuadTree->nodes.pPool[newRootBlockIndex + c].slippyPosition == newRootSlippy)
      {
        uint32_t oldRootBlockIndex = vcQuadTree_NodeIndexToBlockIndex(pQuadTree->rootIndex);
        newRootIndex = newRootBlockIndex + c;

        // hook up old root and new root
        for (uint32_t c2 = 0; c2 < NodeChildCount; ++c2)
          pQuadTree->nodes.pPool[oldRootBlockIndex + c2].parentIndex = newRootIndex;

        pQuadTree->nodes.pPool[newRootIndex].childBlockIndex = oldRootBlockIndex;
      }
    }
  }
  else
  {
    // New root is existing node
    uint32_t rootBlockIndex = vcQuadTree_NodeIndexToBlockIndex(newRootIndex);
    for (uint32_t c = 0; c < NodeChildCount; ++c)
      pQuadTree->nodes.pPool[rootBlockIndex + c].parentIndex = INVALID_NODE_INDEX;
  }

  pQuadTree->rootIndex = newRootIndex;
  pQuadTree->completeRerootRequired = false;
}

void vcQuadTree_CompleteReroot(vcQuadTree *pQuadTree, const udInt3 &slippyCoords)
{
  uint32_t freeBlockIndex = vcQuadTree_FindFreeChildBlock(pQuadTree);
  if (freeBlockIndex == INVALID_NODE_INDEX)
  {
    // complete re-root has failed, must wait for nodes to be freed at a later time
    return;
  }

  udInt3 rootBlockCoords = udInt3::create((slippyCoords.x >> 1) << 1, (slippyCoords.y >> 1) << 1, slippyCoords.z);
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    vcQuadTree_InitNode(pQuadTree, freeBlockIndex + c, rootBlockCoords + udInt3::create(c & 1, c >> 1, 0));

    // preserve spatial position of root in its block
    pQuadTree->rootIndex = (pQuadTree->nodes.pPool[freeBlockIndex + c].slippyPosition == slippyCoords) ? (freeBlockIndex + c) : pQuadTree->rootIndex;
  }

  pQuadTree->completeRerootRequired = false;
}

void vcQuadTree_ConditionalReroot(vcQuadTree *pQuadTree, const udInt3 &slippyCoords)
{
  while (!pQuadTree->completeRerootRequired && pQuadTree->nodes.pPool[pQuadTree->rootIndex].slippyPosition != slippyCoords)
    vcQuadTree_Reroot(pQuadTree, slippyCoords);

  if (pQuadTree->completeRerootRequired)
    vcQuadTree_CompleteReroot(pQuadTree, slippyCoords);
}

void vcQuadTree_Update(vcQuadTree *pQuadTree, const vcQuadTreeViewInfo &viewInfo)
{
  bool zoneChangeOccurred = pQuadTree->gisSpace.SRID != viewInfo.pSpace->SRID;
  pQuadTree->gisSpace = *viewInfo.pSpace;

  // invalidate so we can detect nodes that need pruning
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
  {
    vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[i];
    pNode->rendered = false;
    pNode->touched = false;
    pNode->visible = false;

    if (zoneChangeOccurred)
      vcQuadTree_CalculateNodeBounds(pQuadTree, pNode);
  }

  pQuadTree->slippyCoords = viewInfo.slippyCoords;
  pQuadTree->cameraWorldPosition = viewInfo.cameraPosition;
  pQuadTree->quadTreeWorldSize = viewInfo.quadTreeWorldSize;
  pQuadTree->quadTreeHeightOffset = viewInfo.quadTreeHeightOffset;

  pQuadTree->metaData.nodeTouchedCount = 0;
  pQuadTree->metaData.leafNodeCount = 0;
  pQuadTree->metaData.visibleNodeCount = 0;
  pQuadTree->metaData.nodeRenderCount = 0;
  pQuadTree->metaData.maxTreeDepth = udMax(0, (viewInfo.maxVisibleTileLevel - 1) - viewInfo.slippyCoords.z);

  pQuadTree->cameraTreePosition = pQuadTree->cameraWorldPosition;
  pQuadTree->cameraTreePosition.z -= pQuadTree->quadTreeHeightOffset; // relative height

  pQuadTree->expectedTreeDepth = 0;
  double distanceToQuadrant = udAbs(pQuadTree->cameraTreePosition.z) / pQuadTree->quadTreeWorldSize;
  while (pQuadTree->expectedTreeDepth < (viewInfo.maxVisibleTileLevel - 1) && vcQuadTree_ShouldSubdivide(pQuadTree, distanceToQuadrant, pQuadTree->expectedTreeDepth))
    ++pQuadTree->expectedTreeDepth;

  // extract frustum planes
  udDouble4x4 transposedViewProjection = udTranspose(viewInfo.viewProjectionMatrix);
  pQuadTree->frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  pQuadTree->frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  pQuadTree->frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  pQuadTree->frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  pQuadTree->frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  pQuadTree->frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    pQuadTree->frustumPlanes[j] /= udMag3(pQuadTree->frustumPlanes[j]);

  vcQuadTree_ConditionalReroot(pQuadTree, viewInfo.slippyCoords);

  // Must re-check `completeRerootRequired` condition here, because complete re-rooting can fail (finite amount of nodes)
  if (!pQuadTree->completeRerootRequired)
  {
    // validate the entire root block
    uint32_t rootBlockIndex = vcQuadTree_NodeIndexToBlockIndex(pQuadTree->rootIndex);
    for (uint32_t c = 0; c < NodeChildCount; ++c)
      pQuadTree->nodes.pPool[rootBlockIndex + c].touched = true;
    pQuadTree->nodes.pPool[pQuadTree->rootIndex].visible = true;

    vcQuadTree_RecurseGenerateTree(pQuadTree, pQuadTree->rootIndex, 0);
  }
}

bool vcQuadTree_IsBlockUsed(vcQuadTree *pQuadTree, uint32_t blockIndex)
{
  bool isUsed = false;
  for (uint32_t c = 0; c < NodeChildCount && !isUsed; ++c)
    isUsed = pQuadTree->nodes.pPool[blockIndex + c].isUsed;
  return isUsed;
}

bool vcQuadTree_RecursiveDescendentHasRenderData(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  if (pNode->renderInfo.pTexture)
    return true;

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (uint32_t c = 0; c < NodeChildCount; ++c)
    {
      if (vcQuadTree_RecursiveDescendentHasRenderData(pQuadTree, &pQuadTree->nodes.pPool[pNode->childBlockIndex + c]))
        return true;
    }
  }

  return false;
}

bool vcQuadTree_ShouldFreeBlock(vcQuadTree *pQuadTree, uint32_t blockIndex)
{
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    vcQuadTreeNode *pChildNode = &pQuadTree->nodes.pPool[blockIndex + c];
    if (pChildNode->touched || pChildNode->renderInfo.fadingIn || pChildNode->renderInfo.loadStatus == vcNodeRenderInfo::vcTLS_Downloading)
      return false;
  }

  // determine if this block is being used for rendering
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[blockIndex + c];

    // case #1: its a leaf node that could be being used for rendering by an ancestor
    if (vcQuadTree_IsLeafNode(pNode) && pNode->renderInfo.pTexture)
    {
      uint32_t parentIndex = pNode->parentIndex;
      while (parentIndex != INVALID_NODE_INDEX)
      {
        vcQuadTreeNode *pParentNode = &pQuadTree->nodes.pPool[parentIndex];
        if (pParentNode->touched)
        {
          // We have an ancestor that has no texture, or is fading
          if (!pParentNode->renderInfo.pTexture || pParentNode->renderInfo.fadingIn)
            return false;

          break;
        }
        else if (pParentNode->renderInfo.pTexture && !pParentNode->renderInfo.fadingIn)
          return true;

        parentIndex = pParentNode->parentIndex;
      }
    }

    // case #2: its not a leaf node, it cannot be used for rendering BUT one of its descendents could be used for rendering
    else if (!vcQuadTree_IsLeafNode(pNode))
    {
      if (!pNode->renderInfo.pTexture)
      {
        if (vcQuadTree_RecursiveDescendentHasRenderData(pQuadTree, pNode))
          return false;
      }
    }
  }

  return true;
}

void vcQuadTree_Prune(vcQuadTree *pQuadTree)
{
  for (uint32_t blockIndex = 0; blockIndex < pQuadTree->nodes.used; blockIndex += NodeChildCount)
  {
    if (pQuadTree->nodes.pPool[blockIndex].isUsed && vcQuadTree_ShouldFreeBlock(pQuadTree, blockIndex))
    {
      vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[blockIndex];

      // inform parent its children are dead
      if (pNode->parentIndex != INVALID_NODE_INDEX)
      {
        vcQuadTreeNode *pParentNode = &pQuadTree->nodes.pPool[pNode->parentIndex];
        pParentNode->childMask = 0;
        pParentNode->childBlockIndex = INVALID_NODE_INDEX;
      }

      // cleanup block
      for (uint32_t c = 0; c < NodeChildCount; ++c)
      {
        pNode = &pQuadTree->nodes.pPool[blockIndex + c];

        // inform children its parent is dead (this can still happen if e.g. child mid-download)
        uint32_t childBlockIndex = pNode->childBlockIndex;
        if (childBlockIndex != INVALID_NODE_INDEX)
        {
          for (uint32_t c2 = 0; c2 < NodeChildCount; ++c2)
            pQuadTree->nodes.pPool[childBlockIndex + c2].parentIndex = INVALID_NODE_INDEX;
        }

        vcQuadTree_CleanupNode(pNode);
      }
    }
  }

  // trim
  while (pQuadTree->nodes.used > 0 && !vcQuadTree_IsBlockUsed(pQuadTree, pQuadTree->nodes.used - NodeChildCount))
    pQuadTree->nodes.used -= NodeChildCount;
}
