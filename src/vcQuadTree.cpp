#include "vcQuadTree.h"
#include "vcGIS.h"

#define INVALID_NODE_INDEX uint32_t(-1)
const double rectToCircleRadius = udSqrt(2.0); // distance from center-to-edge vs. distance center-to-corner of 2d rect

// In addition to distance culling, tiles which are not visible due to
// the angle at which the camera is viewing them are also culled.
// 0.55 degrees and below is *roughly* where we want the 'horizon' culling to occur
const double tileToCameraCullAngle = UD_DEG2RAD(0.2);

// Higher value results in more aggressive lodding of tiles.
#define TILE_DISTANCE_CULL_RATE 1.0

// Cap depth at level 19 (system doesn't have access to these tiles yet)
enum
{
  MaxVisibleLevel = 19,
};

struct vcQuadTree
{
  vcQuadTreeMetaData metaData;
  udDouble4x4 viewProjectionMatrix;
  udDouble4 frustumPlanes[6];
  vcGISSpace *pSpace;
  udInt3 slippyCoords;
  udDouble3 cameraWorldPosition;
  double quadTreeWorldSize;
  double visibleDistance;
  double quadTreeHeightOffset;

  vcQuadTreeNode *pNodes;
  int used;
  int capacity;
};

udResult vcQuadTree_ExpandTreeCapacity(vcQuadTree *pQuadTree)
{
  udResult result = udR_Success;
  vcQuadTreeNode *pNewNodeMemory = nullptr;
  int newCapacity = pQuadTree->capacity * 2;

  pNewNodeMemory = udAllocType(vcQuadTreeNode, newCapacity, udAF_Zero);
  UD_ERROR_NULL(pNewNodeMemory, udR_MemoryAllocationFailure);

  if (pQuadTree->pNodes)
  {
    memcpy(pNewNodeMemory, pQuadTree->pNodes, pQuadTree->capacity * sizeof(vcQuadTreeNode));
    udFree(pQuadTree->pNodes);
  }

  pQuadTree->pNodes = pNewNodeMemory;
  pQuadTree->capacity = newCapacity;
  pQuadTree->metaData.memUsageMB = sizeof(vcQuadTreeNode) * newCapacity / 1024 / 1024;

epilogue:
  return result;
}

udResult vcQuadTree_Init(vcQuadTree **ppQuadTree)
{
  udResult result = udR_Success;
  vcQuadTree *pQuadTree = nullptr;
  UD_ERROR_NULL(ppQuadTree, udR_InvalidParameter_);

  pQuadTree = udAllocType(vcQuadTree, 1, udAF_Zero);
  UD_ERROR_IF(!pQuadTree, udR_MemoryAllocationFailure);

  pQuadTree->capacity = 50;
  UD_ERROR_CHECK(vcQuadTree_ExpandTreeCapacity(pQuadTree));

  *ppQuadTree = pQuadTree;
  pQuadTree = nullptr;

epilogue:
  if (pQuadTree)
    vcQuadTree_Destroy(&pQuadTree);

  return result;
}

udResult vcQuadTree_Destroy(vcQuadTree **ppQuadTree)
{
  if (ppQuadTree == nullptr || *ppQuadTree == nullptr)
    return udR_InvalidParameter_;

  udFree((*ppQuadTree)->pNodes);
  udFree(*ppQuadTree);
  *ppQuadTree = nullptr;

  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author Dave Pevreal, July 2017
// Returns -1=outside, 0=inside, >0=partial (bits of planes crossed)
static int vcQuadTree_FrustumTest(udDouble4 frustumPlanes[6], const udDouble3 &boundCenter, const udDouble3 &boundExtents)
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

void vcQuadTree_RecurseGenerateTree(vcQuadTree *pQuadTree, int currentNodeIndex, int currentDepth)
{
  if (currentDepth >= pQuadTree->metaData.maxTreeDepth)
  {
    pQuadTree->metaData.leafNodeCount++;
    pQuadTree->pNodes[currentNodeIndex].isVisible = true;
    return;
  }

  if (pQuadTree->used + 4 >= pQuadTree->capacity)
    vcQuadTree_ExpandTreeCapacity(pQuadTree);

  int childrenIndex[] =
  {
    pQuadTree->used + 0,
    pQuadTree->used + 1,
    pQuadTree->used + 2,
    pQuadTree->used + 3,
  };

  pQuadTree->used += 4;

  //subdivide
  // 0 == bottom left
  // 1 == bottom right
  // 2 == top left
  // 3 == top right
  for (int i = 0; i < 4; ++i)
  {
    vcQuadTreeNode *pCurrentNode = &pQuadTree->pNodes[currentNodeIndex];
    pCurrentNode->childMask |= 1 << i;

    int childIndex = childrenIndex[i];
    pQuadTree->pNodes[childIndex].parentIndex = currentNodeIndex;
    pQuadTree->pNodes[childIndex].childMaskInParent = 1 << i;
    pQuadTree->pNodes[childIndex].level = currentDepth + 1;

    pQuadTree->pNodes[childIndex].slippyPosition = pCurrentNode->slippyPosition * 2;
    pQuadTree->pNodes[childIndex].slippyPosition.x += (i % 2);
    pQuadTree->pNodes[childIndex].slippyPosition.y += (i / 2);

    bool descendChild = false;
    bool tileVisible = false;

    udInt2 pViewSlippyCoords;
    vcGIS_LocalToSlippy(pQuadTree->pSpace, &pViewSlippyCoords, pQuadTree->cameraWorldPosition, pQuadTree->slippyCoords.z + pQuadTree->pNodes[childIndex].level);

    udInt2 slippyManhattanDist = udInt2::create(udAbs(pViewSlippyCoords.x - pQuadTree->pNodes[childIndex].slippyPosition.x),
      udAbs(pViewSlippyCoords.y - pQuadTree->pNodes[childIndex].slippyPosition.y));

    double quadTreeDistanceToTile = 0.0;
    udDouble3 distanceTestPoint = pQuadTree->cameraWorldPosition;
    distanceTestPoint.z -= pQuadTree->quadTreeHeightOffset; // relative height

    if (udMagSq2(slippyManhattanDist) == 0) // simple case, view point is entirely inside
    {
      // TODO: tile heights (DEM)
      quadTreeDistanceToTile = udAbs(distanceTestPoint.z / pQuadTree->quadTreeWorldSize);
      tileVisible = true;
    }
    else
    {
      // more complicated 'closest-point-to-edges' distance checking is needed
      static const udInt2 edgePairs[] =
      {
        udInt2::create(0, 1), // top
        udInt2::create(0, 2), // left
        udInt2::create(2, 3), // bottom
        udInt2::create(1, 3), // right
      };

      double closestEdgeFudgedDistance = 0.0;

      udDouble2 localCorners2D[4];
      udDouble3 localMin, localMax;
      for (int t = 0; t < 4; ++t)
      {
        udDouble3 localCorner;
        vcGIS_SlippyToLocal(pQuadTree->pSpace, &localCorner, udInt2::create(pQuadTree->pNodes[childIndex].slippyPosition.x + (t % 2), pQuadTree->pNodes[childIndex].slippyPosition.y + (t / 2)), pQuadTree->slippyCoords.z + pQuadTree->pNodes[childIndex].level);
        localCorners2D[t] = localCorner.toVector2();
        localMin = (t == 0) ? localCorner : udMin(localMin, localCorner);
        localMax = (t == 0) ? localCorner : udMax(localMax, localCorner);
      }

      // test each edge to find minimum distance to quadrant shape (2d)
      for (int e = 0; e < 4; ++e)
      {
        udDouble2 p1 = localCorners2D[edgePairs[e].x];
        udDouble2 p2 = localCorners2D[edgePairs[e].y];

        udDouble2 edge = p2 - p1;
        double r = udDot2(edge, (distanceTestPoint.toVector2() - p1));
        r /= udPow(udMag2(edge), 2.0);

        // 2d edge has been found, now factor in z for distances to camera
        // TODO: tile heights (DEM)
        udDouble3 closestPointOnEdge = udDouble3::create(p1 + udClamp(r, 0.0, 1.0) * edge, 0.0);

        double distToEdge = udMag3(closestPointOnEdge - distanceTestPoint);
        double horizontalDist = udMag2(closestPointOnEdge.toVector2() - distanceTestPoint.toVector2());
        double fudgedDistance = distToEdge + horizontalDist * TILE_DISTANCE_CULL_RATE; // horizontal distance has more influence
        closestEdgeFudgedDistance = closestEdgeFudgedDistance == 0 ? fudgedDistance : udMin(closestEdgeFudgedDistance, fudgedDistance);
      }

      quadTreeDistanceToTile = closestEdgeFudgedDistance / pQuadTree->quadTreeWorldSize;

      // TODO: This assumes a unit cube, and does not take into account
      // the distortion of the quadrants, occassionally yielding false
      // positives.
      udDouble3 center = (localMax + localMin) * 0.5;
      center.z += pQuadTree->quadTreeHeightOffset;
      udDouble3 extents = (localMax - localMin) * 0.5;
      tileVisible = vcQuadTree_FrustumTest(pQuadTree->frustumPlanes, center, extents) > -1;

      double tileToCameraAngle = udSin(distanceTestPoint.z / closestEdgeFudgedDistance);
      tileVisible = tileVisible && tileToCameraAngle >= tileToCameraCullAngle;
    }

    descendChild = quadTreeDistanceToTile < (1.0 / (1 << currentDepth));

    if (descendChild)
    {
      vcQuadTree_RecurseGenerateTree(pQuadTree, childIndex, currentDepth + 1);
    }
    else if (tileVisible)
    {
      // don't divide the child, it is now a leaf node
      pQuadTree->metaData.leafNodeCount++;
      pQuadTree->pNodes[childIndex].isVisible = true;
    }
  }
}


udResult vcQuadTree_GenerateNodeList(vcQuadTree *pQuadTree, const vcQuadTreeCreateInfo &createInfo, vcQuadTreeMetaData *pMetaData /*= nullptr*/)
{
  udResult result = udR_Success;
  udDouble4x4 transposedViewProjection = udDouble4x4::identity();

  UD_ERROR_NULL(pQuadTree, udR_InvalidParameter_);

  // Clear memory
  memset(pQuadTree->pNodes, 0, sizeof(vcQuadTreeNode) * pQuadTree->capacity);

  pQuadTree->viewProjectionMatrix = createInfo.viewProjectionMatrix;
  pQuadTree->pSpace = createInfo.pSpace;
  pQuadTree->slippyCoords = createInfo.slippyCoords;
  pQuadTree->cameraWorldPosition = createInfo.cameraPosition;
  pQuadTree->quadTreeWorldSize = createInfo.quadTreeWorldSize;
  pQuadTree->visibleDistance = createInfo.visibleDistance;
  pQuadTree->quadTreeHeightOffset = createInfo.quadTreeHeightOffset;

  pQuadTree->metaData.leafNodeCount = 0;
  pQuadTree->metaData.maxTreeDepth = udMax(0, MaxVisibleLevel - createInfo.slippyCoords.z);

  // extract frustum planes
  transposedViewProjection = udTranspose(pQuadTree->viewProjectionMatrix);
  pQuadTree->frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  pQuadTree->frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  pQuadTree->frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  pQuadTree->frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  pQuadTree->frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  pQuadTree->frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far

  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    pQuadTree->frustumPlanes[j] /= udMag3(pQuadTree->frustumPlanes[j]);

  // initialize root
  pQuadTree->used = 1;
  pQuadTree->pNodes[0].parentIndex = INVALID_NODE_INDEX;
  pQuadTree->pNodes[0].slippyPosition = createInfo.slippyCoords.toVector2();

  vcQuadTree_RecurseGenerateTree(pQuadTree, 0, 0);

  if (pMetaData)
    memcpy(pMetaData, &pQuadTree->metaData, sizeof(pQuadTree->metaData));

epilogue:
  return result;
}

udResult vcQuadTree_GetNodeList(vcQuadTree *pQuadTree, vcQuadTreeNode **ppNodes, int *pNodeCount)
{
  udResult result = udR_Success;
  UD_ERROR_NULL(pQuadTree, udR_InvalidParameter_);
  UD_ERROR_NULL(ppNodes, udR_InvalidParameter_);
  UD_ERROR_NULL(pNodeCount, udR_InvalidParameter_);

  *ppNodes = pQuadTree->pNodes;
  *pNodeCount = pQuadTree->used;

epilogue:
  return result;
}
