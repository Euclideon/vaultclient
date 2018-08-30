#include "vcQuadTree.h"
#include "vcGIS.h"

#define INVALID_NODE_INDEX uint32_t(-1)
const double rectToCircleRadius = udSqrt(2.0); // distance from center-to-edge vs. distance center-to-corner of 2d rect

// In addition to distance culling, tiles which are not visible due to
// the angle at which the camera is viewing them are also culled.
// 0.55 degrees and below is *roughly* where we want the 'horizon' culling to occur
const double tileToCameraCullAngle = UD_DEG2RAD(0.55);

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

  vcQuadTreeNode *pNodes;
  int used;
  int capacity;
};


udResult ExpandTreeCapacity(vcQuadTree *pQuadTree)
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

void RecurseGenerateTree(vcQuadTree *pQuadTree, int currentNodeIndex, uint16_t srid, const udInt3 slippyCoords, const udDouble3 &position, double quadTreeWorldSize, double visibleDistance, double quadTreeHeightOffset, int currentDepth, int maxDepth)
{
  if (currentDepth >= maxDepth)
  {
    pQuadTree->metaData.leafNodeCount++;
    pQuadTree->pNodes[currentNodeIndex].isVisible = true;
    return;
  }

  if (pQuadTree->used + 4 >= pQuadTree->capacity)
    ExpandTreeCapacity(pQuadTree);

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
    int maxChildDepth = maxDepth;

    udInt2 pViewSlippyCoords;
    vcGIS_LocalToSlippy(srid, &pViewSlippyCoords, position, slippyCoords.z + pQuadTree->pNodes[childIndex].level);

    udInt2 slippyManhattanDist = udInt2::create(udAbs(pViewSlippyCoords.x - pQuadTree->pNodes[childIndex].slippyPosition.x),
      udAbs(pViewSlippyCoords.y - pQuadTree->pNodes[childIndex].slippyPosition.y));

    double quadTreeDistanceToTile = 0.0;
    udDouble3 distanceTestPoint = position;
    distanceTestPoint.z -= quadTreeHeightOffset; // relative height

    if (udMagSq2(slippyManhattanDist) == 0) // simple case, view point is entirely inside
    {
      // TODO: tile heights (DEM)
      quadTreeDistanceToTile = udAbs(distanceTestPoint.z / quadTreeWorldSize);
      tileVisible = true;
    }
    else
    {
      // more complicated 'closest-point-to-edges' distance checking is needed
      udDouble2 localCorners2D[4];

      for (int t = 0; t < 4; ++t)
      {
        udDouble3 localCorners;
        vcGIS_SlippyToLocal(srid, &localCorners, udInt2::create(pQuadTree->pNodes[childIndex].slippyPosition.x + (t % 2), pQuadTree->pNodes[childIndex].slippyPosition.y + (t / 2)), slippyCoords.z + pQuadTree->pNodes[childIndex].level);
        localCorners2D[t] = localCorners.toVector2();
      }

      static const udInt2 edgePairs[] =
      {
        udInt2::create(0, 1), // top
        udInt2::create(0, 2), // left
        udInt2::create(2, 3), // bottom
        udInt2::create(1, 3), // right
      };

      double minDist = 0.0;

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
        minDist = minDist == 0 ? fudgedDistance : udMin(minDist, fudgedDistance);
      }

      quadTreeDistanceToTile = minDist / quadTreeWorldSize;

      double tileToCameraAngle = udSin(distanceTestPoint.z / minDist);
      tileVisible = minDist < visibleDistance && tileToCameraAngle >= tileToCameraCullAngle;
    }

    descendChild = quadTreeDistanceToTile < (1.0 / (1 << currentDepth));

    if (descendChild)
    {
      RecurseGenerateTree(pQuadTree, childIndex, srid, slippyCoords, position, quadTreeWorldSize, visibleDistance, quadTreeHeightOffset, currentDepth + 1, maxChildDepth);
    }

    if (tileVisible && !descendChild)
    {
      // don't divide the child, it is now a leaf node
      pQuadTree->metaData.leafNodeCount++;
      pQuadTree->pNodes[childIndex].isVisible = true;
    }
  }
}

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, const vcQuadTreeCreateInfo &createInfo, vcQuadTreeMetaData *pMetaData /*= nullptr*/)
{
  vcQuadTree quadTree = {};

  int startingCapacity = 50; // arbitrary
  quadTree.capacity = startingCapacity >> 1;
  ExpandTreeCapacity(&quadTree);
  quadTree.metaData.leafNodeCount = 0;
  quadTree.metaData.maxTreeDepth = udMax(0, MaxVisibleLevel - createInfo.slippyCoords.z);

  // Initialize root
  quadTree.used = 1;
  quadTree.pNodes[0].parentIndex = INVALID_NODE_INDEX;
  quadTree.pNodes[0].slippyPosition = createInfo.slippyCoords.toVector2();

  RecurseGenerateTree(&quadTree, 0, createInfo.srid, createInfo.slippyCoords, createInfo.cameraPosition, createInfo.quadTreeWorldSize, createInfo.visibleDistance, createInfo.quadTreeHeightOffset, 0, quadTree.metaData.maxTreeDepth);

  *ppNodes = quadTree.pNodes;
  *pNodeCount = quadTree.used;

  if (pMetaData)
    memcpy(pMetaData, &quadTree.metaData, sizeof(vcQuadTreeMetaData));
}

void vcQuadTree_DestroyNodeList(vcQuadTreeNode **ppNodes, int /*nodeCount*/)
{
  udFree(*ppNodes);
}
