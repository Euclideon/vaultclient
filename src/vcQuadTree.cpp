#include "vcQuadTree.h"
#include "vcTileRenderer.h"
#include "vcGIS.h"
#include "gl/vcTexture.h"

#define INVALID_NODE_INDEX 0xffffffff

// 0.55 degrees and below is *roughly* where we want the 'horizon' culling to occur
const double tileToCameraCullAngle = UD_DEG2RAD(0.2);

// Cap depth at level 19 (system doesn't have access to these tiles yet)
enum
{
  NodeChildCount = 4,
  MaxVisibleLevel = 19,
};

static const int vcTRMQToValue[] = { 5, 2, 0 };
UDCOMPILEASSERT(udLengthOf(vcTRMQToValue) == vcTRMQ_Total, "Not Enough TileRendererMapQuality Options");

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
    double r = udDot2(edge, (point.toVector2() - p1));
    r /= udPow(udMag2(edge), 2.0);

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

void vcQuadTree_InitNode(vcQuadTree *pQuadTree, uint32_t slotIndex, const udInt3 &childSlippy)
{
  vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[slotIndex];

  pNode->isUsed = true;
  pNode->childBlockIndex = INVALID_NODE_INDEX;
  pNode->parentIndex = INVALID_NODE_INDEX;
  pNode->slippyPosition = childSlippy;

  for (int edge = 0; edge < 4; ++edge)
  {
    udDouble3 localCorner;
    vcGIS_SlippyToLocal(pQuadTree->pSpace, &localCorner, udInt2::create(pNode->slippyPosition.x + (edge & 1), pNode->slippyPosition.y + (edge >> 1)), pNode->slippyPosition.z);
    pNode->worldBounds[edge] = localCorner.toVector2();
  }
}

bool vcQuadTree_IsNodeVisible(const vcQuadTree *pQuadTree, const vcQuadTreeNode *pNode)
{
  udDouble2 min = udDouble2::create(pNode->worldBounds[0].x, pNode->worldBounds[2].y);
  udDouble2 max = udDouble2::create(pNode->worldBounds[3].x, pNode->worldBounds[1].y);
  udDouble3 center = udDouble3::create((max + min) * 0.5, pQuadTree->quadTreeHeightOffset);
  udDouble3 extents = udDouble3::create((max - min) * 0.5, 0.0);
  return vcQuadTree_FrustumTest(pQuadTree->frustumPlanes, center, extents) > -1;
}

// distance is in quad tree space [0, 1]
inline bool vcQuadTree_ShouldSubdivide(vcQuadTree *pQuadTree, double distance, int depth)
{
  return distance < (1.0 / (1 << (depth + vcTRMQToValue[pQuadTree->pSettings->maptiles.mapQuality])));
}

void vcQuadTree_RecurseGenerateTree(vcQuadTree *pQuadTree, uint32_t currentNodeIndex, int currentDepth)
{
  vcQuadTreeNode *pCurrentNode = &pQuadTree->nodes.pPool[currentNodeIndex];
  pCurrentNode->childMask = 0;

  if (currentDepth >= pQuadTree->metaData.maxTreeDepth)
    return;

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
  vcGIS_LocalToSlippy(pQuadTree->pSpace, &pViewSlippyCoords, pQuadTree->cameraWorldPosition, pQuadTree->slippyCoords.z + currentDepth + 1);

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
    pChildNode->visible = vcQuadTree_IsNodeVisible(pQuadTree, pChildNode);

    // TODO: tile heights (DEM)
    double distanceToQuadrant = udAbs(pQuadTree->cameraTreePosition.z);

    udInt2 slippyManhattanDist = udInt2::create(udAbs(pViewSlippyCoords.x - pChildNode->slippyPosition.x), udAbs(pViewSlippyCoords.y - pChildNode->slippyPosition.y));
    if (udMagSq2(slippyManhattanDist) != 0)
    {
      distanceToQuadrant = vcQuadTree_PointToRectDistance(pChildNode->worldBounds, pQuadTree->cameraTreePosition);
      pChildNode->visible = pChildNode->visible && (udAbs(udSin(pQuadTree->cameraTreePosition.z / distanceToQuadrant)) >= tileToCameraCullAngle);
    }

    double distanceInTreeSpace = (distanceToQuadrant / pQuadTree->quadTreeWorldSize);

    // Artificially change the distances of tiles based on their relative depths.
    // Essentially flattens out lower layers, while simultaneously raising levels of tiles further away
    int depthDiffToView = pQuadTree->expectedTreeDepth - currentDepth;
    if (depthDiffToView > 0)
      distanceInTreeSpace *= (0.6 + 0.25 * depthDiffToView);

    if (vcQuadTree_ShouldSubdivide(pQuadTree, distanceInTreeSpace, currentDepth))
      vcQuadTree_RecurseGenerateTree(pQuadTree, childIndex, currentDepth + 1);
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

void vcQuadTree_Update(vcQuadTree *pQuadTree, const vcQuadTreeViewInfo &viewInfo)
{
  // invalidate so we can detect nodes that need pruning
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
  {
    pQuadTree->nodes.pPool[i].rendered = false;
    pQuadTree->nodes.pPool[i].touched = false;
    pQuadTree->nodes.pPool[i].visible = false;
  }

  pQuadTree->pSpace = viewInfo.pSpace;
  pQuadTree->slippyCoords = viewInfo.slippyCoords;
  pQuadTree->cameraWorldPosition = viewInfo.cameraPosition;
  pQuadTree->quadTreeWorldSize = viewInfo.quadTreeWorldSize;
  pQuadTree->visibleDistance = viewInfo.visibleDistance;
  pQuadTree->quadTreeHeightOffset = viewInfo.quadTreeHeightOffset;

  pQuadTree->metaData.maxTreeDepth = udMax(0, MaxVisibleLevel - viewInfo.slippyCoords.z);

  pQuadTree->cameraTreePosition = pQuadTree->cameraWorldPosition;
  pQuadTree->cameraTreePosition.z -= pQuadTree->quadTreeHeightOffset; // relative height

  pQuadTree->expectedTreeDepth = 0;
  double distanceToQuadrant = udAbs(pQuadTree->cameraTreePosition.z) / pQuadTree->quadTreeWorldSize;
  while (vcQuadTree_ShouldSubdivide(pQuadTree, distanceToQuadrant, pQuadTree->expectedTreeDepth))
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

  while (!pQuadTree->completeRerootRequired && pQuadTree->nodes.pPool[pQuadTree->rootIndex].slippyPosition != viewInfo.slippyCoords)
    vcQuadTree_Reroot(pQuadTree, viewInfo.slippyCoords);

  if (pQuadTree->completeRerootRequired)
    vcQuadTree_CompleteReroot(pQuadTree, viewInfo.slippyCoords);

  // Must re-check `completeRerootRequired` condition here, because complete re-rooting can fail (finite amount of nodes)
  if (!pQuadTree->completeRerootRequired)
    vcQuadTree_RecurseGenerateTree(pQuadTree, pQuadTree->rootIndex, 0);

  // validate the entire root block
  uint32_t rootBlockIndex = vcQuadTree_NodeIndexToBlockIndex(pQuadTree->rootIndex);
  for (uint32_t c = 0; c < NodeChildCount; ++c)
    pQuadTree->nodes.pPool[rootBlockIndex + c].touched = true;
  pQuadTree->nodes.pPool[pQuadTree->rootIndex].visible = true;
}

bool vcQuadTree_IsBlockUsed(vcQuadTree *pQuadTree, uint32_t blockIndex)
{
  bool isUsed = false;
  for (uint32_t c = 0; c < NodeChildCount && !isUsed; ++c)
    isUsed = pQuadTree->nodes.pPool[blockIndex + c].isUsed;
  return isUsed;
}

bool vcQuadTree_ShouldFreeBlock(vcQuadTree *pQuadTree, uint32_t blockIndex)
{
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    if (pQuadTree->nodes.pPool[blockIndex + c].touched || pQuadTree->nodes.pPool[blockIndex + c].renderInfo.loadStatus == vcNodeRenderInfo::vcTLS_Downloading)
      return false;
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
