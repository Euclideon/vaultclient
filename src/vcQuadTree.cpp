#include "vcQuadTree.h"
#include "vcGIS.h"

#define INVALID_NODE_INDEX uint32_t(-1)

const double rectToCircleRadius = udSqrt(2.0); // distance from center-to-edge vs. distance center-to-corner of 2d rect

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

void RecurseGenerateTree(vcQuadTree *pQuadTree, int currentNodeIndex, uint16_t srid, const udInt3 slippyCoords, const udDouble3 &position, const udDouble2 &halfExtents, const double visibleFarPlane, int currentDepth, int maxDepth)
{
  if (currentDepth >= maxDepth)
  {
    pQuadTree->metaData.leafNodeCount++;
    pQuadTree->pNodes[currentNodeIndex].isVisible = true;
    pQuadTree->metaData.visibleNodeCount++;
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
    //pQuadTree->pNodes[childIndex].childSize = pCurrentNode->childSize * 0.5f;
    pQuadTree->pNodes[childIndex].parentIndex = currentNodeIndex;
    //pQuadTree->pNodes[childIndex].position = pCurrentNode->position + udFloat2::create(float(i % 2), float(i / 2)) * pCurrentNode->childSize;
    pQuadTree->pNodes[childIndex].level = currentDepth + 1;

   // int childSlippyGridSize = 1 << (slippyCoords.z + pQuadTree->pNodes[childIndex].level);
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
    

    if (udMagSq2(slippyManhattanDist) == 0) // simple case, view point is entirely inside (effectively tests if we're 'inside' the region)
    {
      descendChild = true;
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

      double minDist = 0.0;
      static const udInt2 edgePairs[] =
      {
        udInt2::create(0, 1), // top
        udInt2::create(0, 2), // left
        udInt2::create(2, 3), // bottom
        udInt2::create(1, 3), // right
      };

      udDouble2 p0 = position.toVector2();

      // test each edge to find minimum distance to quadrant shape
      for (int e = 0; e < 4; ++e)
      {
        udDouble2 p1 = localCorners2D[edgePairs[e].x];
        udDouble2 p2 = localCorners2D[edgePairs[e].y];

        udDouble2 edge = p2 - p1;
        double r = udDot2(edge, (p0 - p1));
        r /= udPow(udMag2(edge), 2.0);

        udDouble2 closestPointOnEdge = (p1 + udClamp(r, 0.0, 1.0) * edge);
        double distToEdge = udMag2(closestPointOnEdge - p0);
        minDist = minDist == 0 ? distToEdge : udMin(minDist, distToEdge);
      }

      double normalizedViewDistance = minDist / visibleFarPlane;

      descendChild = normalizedViewDistance < (1.0 / (1 << currentDepth)); // depth is a factor in determining whether we should descend this child
      tileVisible = normalizedViewDistance < 1.0;
    }

    if (descendChild)
    {
      RecurseGenerateTree(pQuadTree, childIndex, srid, slippyCoords, position, halfExtents, visibleFarPlane, currentDepth + 1, maxChildDepth);
    }
    else if (tileVisible)
    {
      // don't divide the child, it is now a leaf node
      pQuadTree->metaData.leafNodeCount++;
      pQuadTree->pNodes[childIndex].isVisible = true;
      pQuadTree->metaData.visibleNodeCount++;
      //pCurrentNode->isVisible = false; // but its not visible
    }
  }
}


int CalculateTreeDepthFromViewDistance(const udDouble2 &halfExtents)
{
  return int(udLog2(1.0 / (halfExtents.x * rectToCircleRadius)));
}

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, uint16_t srid, const udInt3 &slippyCoords, const udDouble3 &cameraPosition, const udDouble2 &cameraViewSizeRatio, const double visibleFarPlane, vcQuadTreeMetaData *pMetaData /*= nullptr*/)
{
  vcQuadTree quadTree = {};

  int maxLocalTreeDepth = CalculateTreeDepthFromViewDistance(cameraViewSizeRatio);
  
  maxLocalTreeDepth = udMin(19, maxLocalTreeDepth + slippyCoords.z) - slippyCoords.z; // cap real depth at level 19 (we don't have access to these tiles yet)
  maxLocalTreeDepth = udMax(0, maxLocalTreeDepth); // and just for safety because 'CalculateTreeDepthFromViewDistance()' is relatively untested

  int startingCapacity = 50; // hard coded for now - this could be guessed from maxTreeDepth...
  quadTree.capacity = startingCapacity / 2;
  ExpandTreeCapacity(&quadTree);
  quadTree.metaData.leafNodeCount = 0;
  quadTree.metaData.visibleNodeCount = 0;
  quadTree.metaData.maxTreeDepth = maxLocalTreeDepth;

  // Initialize root
  quadTree.used = 1;
  quadTree.pNodes[0].parentIndex = INVALID_NODE_INDEX;
  quadTree.pNodes[0].slippyPosition = slippyCoords.toVector2();

  RecurseGenerateTree(&quadTree, 0, srid, slippyCoords, cameraPosition, cameraViewSizeRatio, visibleFarPlane, 0, quadTree.metaData.maxTreeDepth);

  *ppNodes = quadTree.pNodes;
  *pNodeCount = quadTree.used;

  if (pMetaData)
    memcpy(pMetaData, &quadTree.metaData, sizeof(vcQuadTreeMetaData));
}