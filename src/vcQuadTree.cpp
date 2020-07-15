#include "vcQuadTree.h"
#include "vcGIS.h"
#include "vcTexture.h"

enum
{
  NodeChildCount = 4,
};

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

double vcQuadTree_PointToRectDistance(udDouble3 edges[vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution], const udDouble3 &point, const udDouble3 &demMax)
{
  static const udInt2 edgePairs[] =
  {
    udInt2::create(0, vcQuadTreeNodeVertexResolution - 1), // top
    udInt2::create(0, (vcQuadTreeNodeVertexResolution - 1) * vcQuadTreeNodeVertexResolution), // left
    udInt2::create((vcQuadTreeNodeVertexResolution - 1) * vcQuadTreeNodeVertexResolution, (vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution) - 1), // bottom
    udInt2::create(vcQuadTreeNodeVertexResolution - 1, (vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution) - 1), // right
  };

  double closestEdgeDistance = 0.0;

  // test each edge to find minimum distance to quadrant shape (3d)
  for (int e = 0; e < 4; ++e)
  {
    udDouble3 p1 = edges[edgePairs[e].x] + demMax;
    udDouble3 p2 = edges[edgePairs[e].y] + demMax;

    udDouble3 edge = p2 - p1;
    double r = udDot3(edge, (point - p1)) / udMagSq3(edge);

    udDouble3 closestPointOnEdge = (p1 + udClamp(r, 0.0, 1.0) * edge);

    double distToEdge = udMag3(closestPointOnEdge - point);
    closestEdgeDistance = (e == 0) ? distToEdge : udMin(closestEdgeDistance, distToEdge);
  }

  return closestEdgeDistance;
}

void vcQuadTree_CleanupNode(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  for (int i = 0; i < pQuadTree->pSettings->maptiles.activeLayerCount; ++i)
  {
    vcTexture_Destroy(&pNode->pPayloads[i].data.pTexture);
    udFree(pNode->pPayloads[i].data.pData);

    pNode->pPayloads[i].loadStatus.Set(vcNodeRenderInfo::vcTLS_None);
  }

  vcTexture_Destroy(&pNode->demInfo.data.pTexture);
  udFree(pNode->demInfo.data.pData);

  vcTexture_Destroy(&pNode->normalInfo.data.pTexture);
  udFree(pNode->normalInfo.data.pData);

  udFree(pNode->pDemHeightsCopy);

  pNode->demInfo.loadStatus.Set(vcNodeRenderInfo::vcTLS_None);

  memset(pNode, 0, sizeof(vcQuadTreeNode));
}

void vcQuadTree_UpdateNodesActiveDEM(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  pNode->activeDemMinMax = pNode->demMinMax;

  if (!pQuadTree->pSettings->maptiles.demEnabled)
    pNode->activeDemMinMax = udInt2::zero();
}

void vcQuadTree_CalculateNodeAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  vcQuadTree_UpdateNodesActiveDEM(pQuadTree, pNode);

  udDouble3 boundsMin = udDouble3::create(FLT_MAX, FLT_MAX, FLT_MAX);
  udDouble3 boundsMax = udDouble3::create(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (int edge = 0; edge < vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution; ++edge)
  {
    udDouble3 p0 = pNode->worldBounds[edge] + pNode->worldNormals[edge] * pNode->activeDemMinMax[0];
    udDouble3 p1 = pNode->worldBounds[edge] + pNode->worldNormals[edge] * pNode->activeDemMinMax[1];
    boundsMin = udMin(p0, udMin(p1, boundsMin));
    boundsMax = udMax(p0, udMax(p1, boundsMax));
  }

  pNode->tileCenter = (boundsMax + boundsMin) * 0.5;
  pNode->tileExtents = (boundsMax - boundsMin) * 0.5;
}

void vcQuadTree_CalculateNodeBounds(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  pNode->tileNormal = udDouble3::zero();
  for (int edge = 0; edge < vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution; ++edge)
  {
    udInt2 slippySampleCoord = udInt2::create((pNode->slippyPosition.x * (vcQuadTreeNodeVertexResolution - 1)) + (edge % vcQuadTreeNodeVertexResolution),
      (pNode->slippyPosition.y * (vcQuadTreeNodeVertexResolution - 1)) + (edge / vcQuadTreeNodeVertexResolution));

    vcGIS_SlippyToLocal(pQuadTree->geozone, &pNode->worldBounds[edge], slippySampleCoord, pNode->slippyPosition.z + (vcQuadTreeNodeVertexResolution - 2));

    pNode->worldNormals[edge] = vcGIS_GetWorldLocalUp(pQuadTree->geozone, pNode->worldBounds[edge]);
    pNode->worldBitangents[edge] = vcGIS_GetWorldLocalNorth(pQuadTree->geozone, pNode->worldBounds[edge]);
    pNode->tileNormal += pNode->worldNormals[edge];
  }

  pNode->tileNormal = udNormalize3(pNode->tileNormal);

  vcQuadTree_CalculateNodeAABB(pQuadTree, pNode);
}

void vcQuadTree_RecursiveUpdateNodesAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  vcQuadTree_CalculateNodeAABB(pQuadTree, pNode);

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
      vcQuadTree_RecursiveUpdateNodesAABB(pQuadTree, &pQuadTree->nodes.pPool[pNode->childBlockIndex + c]);
  }
}

void vcQuadTree_InitBlock(vcQuadTree *pQuadTree, uint32_t slotIndex, const udInt3 &blockSlippy, const udInt2 &parentDemMinMax)
{
  // children share vertices, so only calculate these once
  const int blockControlPointSize = vcQuadTreeNodeVertexResolution + 1;
  const int blockSize = blockControlPointSize * blockControlPointSize;
  udDouble3 blockBounds[blockSize] = {};
  udDouble3 blockNormals[blockSize] = {};
  udDouble3 blockBitangents[blockSize] = {};

  for (int edge = 0; edge < blockSize; ++edge)
  {
    udInt2 slippySampleCoord = udInt2::create((blockSlippy.x * (vcQuadTreeNodeVertexResolution - 1)) + (edge % blockControlPointSize),
      (blockSlippy.y * (vcQuadTreeNodeVertexResolution - 1)) + (edge / blockControlPointSize));

    vcGIS_SlippyToLocal(pQuadTree->geozone, &blockBounds[edge], slippySampleCoord, blockSlippy.z + (vcQuadTreeNodeVertexResolution - 2));

    blockNormals[edge] = vcGIS_GetWorldLocalUp(pQuadTree->geozone, blockBounds[edge]);
    blockBitangents[edge] = vcGIS_GetWorldLocalNorth(pQuadTree->geozone, blockBounds[edge]);
  }

  for (int c = 0; c < NodeChildCount; ++c)
  {
    vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[slotIndex + c];

    pNode->isUsed = true;
    pNode->serverId = pQuadTree->serverId;
    pNode->childBlockIndex = INVALID_NODE_INDEX;
    pNode->parentIndex = INVALID_NODE_INDEX;
    pNode->slippyPosition = blockSlippy + udInt3::create(c & 1, c >> 1, 0);
    pNode->completeRender = false;
    pNode->demMinMax = parentDemMinMax;
    pNode->demBoundsState = vcQuadTreeNode::vcDemBoundsState_Inherited;
    if (pNode->demMinMax[0] == 0 && pNode->demMinMax[1] == 0)
      pNode->demBoundsState = vcQuadTreeNode::vcDemBoundsState_None;

    pNode->tileNormal = udDouble3::zero();
    for (int ye = 0; ye < vcQuadTreeNodeVertexResolution; ++ye)
    {
      for (int xe = 0; xe < vcQuadTreeNodeVertexResolution; ++xe)
      {
        udInt2 offset = udInt2::create(c & 1, c >> 1);
        int writeIndex = xe + ye * vcQuadTreeNodeVertexResolution;
        int readIndex = (offset.x + xe) + (offset.y + ye) * 3;
        pNode->worldBounds[writeIndex] = blockBounds[readIndex];
        pNode->worldNormals[writeIndex] = blockNormals[readIndex];
        pNode->worldBitangents[writeIndex] = blockBitangents[readIndex];

        pNode->tileNormal += pNode->worldNormals[writeIndex];
      }
    }

    pNode->tileNormal = udNormalize3(pNode->tileNormal);

    vcQuadTree_CalculateNodeAABB(pQuadTree, pNode);
  }
}

bool vcQuadTree_IsNodeVisible(const vcQuadTree *pQuadTree, const vcQuadTreeNode *pNode)
{
  // camera inside bounds test
  if (pQuadTree->cameraWorldPosition.x >= pNode->tileCenter.x - pNode->tileExtents.x &&
    pQuadTree->cameraWorldPosition.x <= pNode->tileCenter.x + pNode->tileExtents.x &&
    pQuadTree->cameraWorldPosition.y >= pNode->tileCenter.y - pNode->tileExtents.y &&
    pQuadTree->cameraWorldPosition.y <= pNode->tileCenter.y + pNode->tileExtents.y &&
    pQuadTree->cameraWorldPosition.z >= pNode->tileCenter.z - pNode->tileExtents.z &&
    pQuadTree->cameraWorldPosition.z <= pNode->tileCenter.z + pNode->tileExtents.z)
  {
    return true;
  }


  // frustum test
  udDouble3 mapHeightOffset = pNode->worldNormals[4] * pQuadTree->pSettings->maptiles.layers[0].mapHeight;
  int frustumTest = vcQuadTree_FrustumTest(pQuadTree->frustumPlanes, pNode->tileCenter + mapHeightOffset, pNode->tileExtents);
  if (frustumTest == -1)
    return false;

  for (int i = 0; i < vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution; ++i)
  {
    if (udDot3(pNode->worldBounds[i] - pQuadTree->cameraWorldPosition, pNode->worldNormals[i]) < 0)
      return true;
  }

  if (udDot3(pNode->tileCenter - pQuadTree->cameraWorldPosition, pNode->tileNormal) < 0)
    return true;

  return false;
}

inline bool vcQuadTree_ShouldSubdivide(double distance, int depth)
{
  // trial and error'd this heuristic
  const int RootRegionSize = 15000000;
  return distance < (RootRegionSize >> depth);
}

float initNodesTime = 0.0f;
float initNodesSearch = 0.0f;

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
    uint64_t s = udPerfCounterStart();

    uint32_t freeBlockIndex = vcQuadTree_FindFreeChildBlock(pQuadTree);
    if (freeBlockIndex == INVALID_NODE_INDEX)
      return;

    udInt3 blockSlippy = udInt3::create(pCurrentNode->slippyPosition.x << 1, pCurrentNode->slippyPosition.y << 1, pCurrentNode->slippyPosition.z + 1);
    vcQuadTree_InitBlock(pQuadTree, freeBlockIndex, blockSlippy, pCurrentNode->demMinMax);
    //udInt3 childSlippy = udInt3::create(pCurrentNode->slippyPosition.x << 1, pCurrentNode->slippyPosition.y << 1, pCurrentNode->slippyPosition.z + 1);
    //for (uint32_t i = 0; i < NodeChildCount; ++i)
    //  vcQuadTree_InitNode(pQuadTree, freeBlockIndex + i, childSlippy + udInt3::create(i & 1, i >> 1, 0), pCurrentNode->demMinMax);

    pCurrentNode->childBlockIndex = freeBlockIndex;

    initNodesTime += udPerfCounterMilliseconds(s);
  }

  udInt2 pViewSlippyCoords = {};
  vcGIS_LocalToSlippy(pQuadTree->geozone, &pViewSlippyCoords, pQuadTree->cameraWorldPosition, pQuadTree->rootSlippyCoords.z + currentDepth + 1);

  udInt2 mortenIndices[] = { {0, 0}, {1, 0}, {0, 1}, {1, 1} };

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
    //pChildNode->visible = vcQuadTree_IsNodeVisible(pQuadTree, pChildNode);
    pChildNode->morten.x = pCurrentNode->morten.x | (mortenIndices[childQuadrant].x << (31 - pChildNode->slippyPosition.z));
    pChildNode->morten.y = pCurrentNode->morten.y | (mortenIndices[childQuadrant].y << (31 - pChildNode->slippyPosition.z));

    bool aabbTest = vcQuadTree_IsNodeVisible(pQuadTree, pChildNode);

    double distanceToQuadrant = udMax(0.0, pQuadTree->cameraDistanceZeroAltitude - (pChildNode->activeDemMinMax[1] + pQuadTree->pSettings->maptiles.layers[0].mapHeight));

    //TODO: here
    int32_t slippyManhattanDist = udAbs(pViewSlippyCoords.x - pChildNode->slippyPosition.x) + udAbs(pViewSlippyCoords.y - pChildNode->slippyPosition.y);
    if (slippyManhattanDist != 0)
      distanceToQuadrant = vcQuadTree_PointToRectDistance(pChildNode->worldBounds, pQuadTree->cameraWorldPosition, pChildNode->tileNormal * udDouble3::create(pChildNode->activeDemMinMax[1] + pQuadTree->pSettings->maptiles.layers[0].mapHeight));

    int totalDepth = pQuadTree->rootSlippyCoords.z + currentDepth;

    // always subdivide into the nth node, so visibility culling works better
    bool alwaysSubdivide = totalDepth < 2 || (aabbTest && totalDepth < 3);// || (pChildNode->visible && totalDepth < vcQuadTree_MinimumDescendLayer);
    if (alwaysSubdivide || (vcQuadTree_ShouldSubdivide(distanceToQuadrant, totalDepth) && aabbTest))
      vcQuadTree_RecurseGenerateTree(pQuadTree, childIndex, currentDepth + 1);
    else
      ++pQuadTree->metaData.leafNodeCount;

    ++pQuadTree->metaData.nodeTouchedCount;
  }
}

udInt2 vcQuadTree_DecodeMorten(udInt2 &m, int d)
{
  int mask = ~(0xffffffff - ((2 << d) - 1));
  int shift = 31 - d;
  return { (m.x >> shift) &mask, (m.y >> shift) &mask };
}

vcQuadTreeNode *vcQuadTree_FindNodeFromMorten(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode, const udInt2 &targetMorten, int targetDepth)
{
  if (vcQuadTree_IsLeafNode(pNode) || pNode->slippyPosition.z >= targetDepth)
    return pNode;

  // TODO: handle outside bounds (e.g. morten.x < 0 || morten.y < 0 || morten.x >= ?? || morten.y >= ??)

  int shift = targetDepth - (pNode->slippyPosition.z + 1);
  udInt2 thisLevel = { (targetMorten.x >> shift) & 1, (targetMorten.y >> shift) & 1 };
  int childIndex = thisLevel.x + thisLevel.y * 2;

  vcQuadTreeNode *pChildNode = &pQuadTree->nodes.pPool[pNode->childBlockIndex + childIndex];
  return vcQuadTree_FindNodeFromMorten(pQuadTree, pChildNode, targetMorten, targetDepth);
}

void vcQuadTree_CalculateNeighbours(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  if (vcQuadTree_IsLeafNode(pNode))
  {
    udInt2 morten = vcQuadTree_DecodeMorten(pNode->morten, pNode->slippyPosition.z);
    vcQuadTreeNode *pRootNode = &pQuadTree->nodes.pPool[pQuadTree->rootIndex];

    vcQuadTreeNode *pNeighbourUp = vcQuadTree_FindNodeFromMorten(pQuadTree, pRootNode, morten + udInt2::create(0, -1), pNode->slippyPosition.z);
    vcQuadTreeNode *pNeighbourRight = vcQuadTree_FindNodeFromMorten(pQuadTree, pRootNode, morten + udInt2::create(1, 0), pNode->slippyPosition.z);
    vcQuadTreeNode *pNeighbourDown = vcQuadTree_FindNodeFromMorten(pQuadTree, pRootNode, morten + udInt2::create(0, 1), pNode->slippyPosition.z);
    vcQuadTreeNode *pNeighbourLeft = vcQuadTree_FindNodeFromMorten(pQuadTree, pRootNode, morten + udInt2::create(-1, 0), pNode->slippyPosition.z);

    pNode->neighbours = 0;
    pNode->neighbours |= 0x1 * int(pNeighbourUp->slippyPosition.z < pNode->slippyPosition.z);
    pNode->neighbours |= 0x2 * int(pNeighbourRight->slippyPosition.z < pNode->slippyPosition.z);
    pNode->neighbours |= 0x4 * int(pNeighbourDown->slippyPosition.z < pNode->slippyPosition.z);
    pNode->neighbours |= 0x8 * int(pNeighbourLeft->slippyPosition.z < pNode->slippyPosition.z);
  }
  else
  {
    for (int c = 0; c < NodeChildCount; ++c)
    {
      vcQuadTreeNode *pChildNode = &pQuadTree->nodes.pPool[pNode->childBlockIndex + c];
      vcQuadTree_CalculateNeighbours(pQuadTree, pChildNode);
    }
  }
}

void vcQuadTree_CleanupNodes(vcQuadTree *pQuadTree)
{
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
    vcQuadTree_CleanupNode(pQuadTree, &pQuadTree->nodes.pPool[i]);

  pQuadTree->nodes.used = 0;
}

void vcQuadTree_Create(vcQuadTree *pQuadTree, vcSettings *pSettings)
{
  pQuadTree->serverId = 0;
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
  pQuadTree->serverId++;

  memset(&pQuadTree->metaData, 0, sizeof(vcQuadTreeMetaData));
  pQuadTree->completeRerootRequired = true;
}

uint32_t vcQuadTree_NodeIndexToBlockIndex(uint32_t nodeIndex)
{
  return nodeIndex & ~3;
}

void vcQuadTree_InitRootBlock(vcQuadTree *pQuadTree)
{
  uint32_t rootBlockIndex = vcQuadTree_NodeIndexToBlockIndex(pQuadTree->rootIndex);
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    pQuadTree->nodes.pPool[rootBlockIndex + c].parentIndex = INVALID_NODE_INDEX;
  }

  pQuadTree->nodes.pPool[rootBlockIndex].morten = udInt2::zero();
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

    vcQuadTree_InitBlock(pQuadTree, newRootBlockIndex, newRootBlockSlippy, udInt2::zero());

    for (uint32_t c = 0; c < NodeChildCount; ++c)
    {
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

  vcQuadTree_InitRootBlock(pQuadTree);
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
  vcQuadTree_InitBlock(pQuadTree, freeBlockIndex, rootBlockCoords, udInt2::zero());
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    // preserve spatial position of root in its block
    pQuadTree->rootIndex = (pQuadTree->nodes.pPool[freeBlockIndex + c].slippyPosition == slippyCoords) ? (freeBlockIndex + c) : pQuadTree->rootIndex;
  }

  pQuadTree->completeRerootRequired = false;
  vcQuadTree_InitRootBlock(pQuadTree);
}

void vcQuadTree_ConditionalReroot(vcQuadTree *pQuadTree, const udInt3 &slippyCoords)
{
  while (!pQuadTree->completeRerootRequired && pQuadTree->nodes.pPool[pQuadTree->rootIndex].slippyPosition != slippyCoords)
    vcQuadTree_Reroot(pQuadTree, slippyCoords);

  if (pQuadTree->completeRerootRequired)
    vcQuadTree_CompleteReroot(pQuadTree, slippyCoords);
}

void vcQuadTree_UpdateView(vcQuadTree *pQuadTree, const udDouble3 &cameraPosition, const udDouble4x4 &viewProjectionMatrix)
{
  pQuadTree->cameraWorldPosition = cameraPosition;

  // extract frustum planes
  udDouble4x4 transposedViewProjection = udTranspose(viewProjectionMatrix);
  pQuadTree->frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  pQuadTree->frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  pQuadTree->frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  pQuadTree->frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  pQuadTree->frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  pQuadTree->frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    pQuadTree->frustumPlanes[j] /= udMag3(pQuadTree->frustumPlanes[j]);

  // TODO: map height
  // Detect changes to DEM settings
  bool demChangeOccurred = pQuadTree->demWasEnabled != pQuadTree->pSettings->maptiles.demEnabled;
  demChangeOccurred = demChangeOccurred || (udAbs(pQuadTree->previousMapHeight - pQuadTree->pSettings->maptiles.layers[0].mapHeight) > UD_EPSILON);
  pQuadTree->demWasEnabled = pQuadTree->pSettings->maptiles.demEnabled;
  pQuadTree->previousMapHeight = pQuadTree->pSettings->maptiles.layers[0].mapHeight;

  if (demChangeOccurred)
    vcQuadTree_RecursiveUpdateNodesAABB(pQuadTree, &pQuadTree->nodes.pPool[pQuadTree->rootIndex]);
}

void vcQuadTree_Update(vcQuadTree *pQuadTree, const vcQuadTreeViewInfo &viewInfo)
{
  uint64_t totalStart = udPerfCounterStart();

  bool zoneChangeOccurred = (pQuadTree->geozone.srid != viewInfo.pGeozone->srid);
  pQuadTree->geozone = *viewInfo.pGeozone;

  // invalidate so we can detect nodes that need pruning
  for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
  {
    vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[i];
    pNode->touched = false;
  }

  if (zoneChangeOccurred)
  {
    for (uint32_t i = 0; i < pQuadTree->nodes.used; ++i)
    {
      vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[i];
      vcQuadTree_CalculateNodeBounds(pQuadTree, pNode);
    }
  }

  pQuadTree->metaData.nodeTouchedCount = 0;
  pQuadTree->metaData.leafNodeCount = 0;

  pQuadTree->rootSlippyCoords = viewInfo.slippyCoords;
  pQuadTree->cameraDistanceZeroAltitude = udMag3(pQuadTree->cameraWorldPosition - viewInfo.cameraPositionZeroAltitude);

  pQuadTree->metaData.maxTreeDepth = udMax(0, (viewInfo.maxVisibleTileLevel - 1) - viewInfo.slippyCoords.z);

  uint64_t startReroot = udPerfCounterStart();
  vcQuadTree_ConditionalReroot(pQuadTree, viewInfo.slippyCoords);
  float timeReroot = udPerfCounterMilliseconds(startReroot);

  initNodesTime = 0;
  initNodesSearch = 0;

  uint64_t startGenTree = udPerfCounterStart();
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
  float timeGenTree = udPerfCounterMilliseconds(startGenTree);

  uint64_t startPrune = udPerfCounterStart();
  vcQuadTree_Prune(pQuadTree);
  float timePrune = udPerfCounterMilliseconds(startPrune);

  uint64_t startNeighbours = udPerfCounterStart();
  vcQuadTree_CalculateNeighbours(pQuadTree, &pQuadTree->nodes.pPool[pQuadTree->rootIndex]);
  float timeNeighbours = udPerfCounterMilliseconds(startNeighbours);

  float totalTime = udPerfCounterMilliseconds(totalStart);
  if (totalTime >= 1.0f)
    printf("Quad tree: %f...reroot:%f, gen:%f, prune:%f, neighbours:%f..initting:%f, search:%f\n", totalTime, timeReroot, timeGenTree, timePrune, timeNeighbours, initNodesTime, initNodesSearch);
}

bool vcQuadTree_IsBlockUsed(vcQuadTree *pQuadTree, uint32_t blockIndex)
{
  bool isUsed = false;
  for (uint32_t c = 0; c < NodeChildCount && !isUsed; ++c)
    isUsed = pQuadTree->nodes.pPool[blockIndex + c].isUsed;
  return isUsed;
}

bool vcQuadTree_RecursiveHasDrawData(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode)
{
  if (pNode->demInfo.data.pTexture != nullptr)
    return true;


  // check payloads
  for (int i = 0; i < pQuadTree->pSettings->maptiles.activeLayerCount; ++i)
  {
    if (pNode->pPayloads[i].data.pTexture != nullptr)
      return true;
  }

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (uint32_t c = 0; c < NodeChildCount; ++c)
    {
      if (vcQuadTree_RecursiveHasDrawData(pQuadTree, &pQuadTree->nodes.pPool[pNode->childBlockIndex + c]))
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
    if (pChildNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_Downloading)
      return false;

    // check payloads
    for (int i = 0; i < pQuadTree->pSettings->maptiles.activeLayerCount; ++i)
    {
      if (pChildNode->pPayloads[i].loadStatus.Get() == vcNodeRenderInfo::vcTLS_Downloading)
        return false;

    }

    if (pChildNode->touched && (pChildNode->serverId == pQuadTree->serverId))
      return false;
  }

  // determine if this block is being used for rendering
  for (uint32_t c = 0; c < NodeChildCount; ++c)
  {
    vcQuadTreeNode *pNode = &pQuadTree->nodes.pPool[blockIndex + c];

    // case #1: its a node that it (or its descendent) could be being used for rendering by an ancestor
    if (vcQuadTree_RecursiveHasDrawData(pQuadTree, pNode))
    {
      uint32_t parentIndex = pNode->parentIndex;
      while (parentIndex != INVALID_NODE_INDEX)
      {
        vcQuadTreeNode *pParentNode = &pQuadTree->nodes.pPool[parentIndex];

        bool hasPayloadData = false;
        for (int i = 0; i < pQuadTree->pSettings->maptiles.activeLayerCount; ++i)
          hasPayloadData = hasPayloadData || (pParentNode->pPayloads[i].data.pTexture != nullptr);

        if (pParentNode->touched)
        {
          // We have an ancestor that has no texture, so it will need this node
          if (!hasPayloadData)
            return false;

          break;
        }
        else if (hasPayloadData)
        {
          // there exists a better 'non-touched' ancestor that could be used instead of this one, throw out
          return true;
        }
        parentIndex = pParentNode->parentIndex;
      }
    }
  }

  return true;
}

void vcQuadTree_Prune(vcQuadTree *pQuadTree)
{
  float totalPruneTimeRemaning = 1.0f;

  for (uint32_t blockIndex = 0; blockIndex < pQuadTree->nodes.used && totalPruneTimeRemaning > 0; blockIndex += NodeChildCount)
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

        uint64_t startCleanupTime = udPerfCounterStart();
        vcQuadTree_CleanupNode(pQuadTree, pNode);
        totalPruneTimeRemaning -= udPerfCounterMilliseconds(startCleanupTime);
      }
    }
  }

  // trim
  while (pQuadTree->nodes.used > 0 && !vcQuadTree_IsBlockUsed(pQuadTree, pQuadTree->nodes.used - NodeChildCount))
    pQuadTree->nodes.used -= NodeChildCount;
}

vcQuadTreeNode* vcQuadTree_GetLeafNodeFromCartesian(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode, const udDouble3 &point)
{
  if (vcQuadTree_IsLeafNode(pNode))
    return pNode;

  udInt2 pointSlippy = {};
  vcGIS_LocalToSlippy(pQuadTree->geozone, &pointSlippy, point, pNode->slippyPosition.z + 1);

  udInt2 childOffset = pointSlippy - (pNode->slippyPosition.toVector2() * 2);
  if (childOffset.x < 0 || childOffset.x > 1 || childOffset.y < 0 || childOffset.y > 1)
    return nullptr;

  return vcQuadTree_GetLeafNodeFromCartesian(pQuadTree, &pQuadTree->nodes.pPool[pNode->childBlockIndex + (childOffset.y * 2 + childOffset.x)], point);
}

vcQuadTreeNode* vcQuadTree_GetLeafNodeFromCartesian(vcQuadTree *pQuadTree, const udDouble3 &point)
{
  return vcQuadTree_GetLeafNodeFromCartesian(pQuadTree, &pQuadTree->nodes.pPool[pQuadTree->rootIndex], point);
}
