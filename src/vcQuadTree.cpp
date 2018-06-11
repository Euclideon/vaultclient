#include "vcQuadTree.h"
#include "vcGIS.h"

#define INVALID_NODE_INDEX uint32_t(-1)

const float rectToCircleRadius = udSqrt(2.0f); // distance from center-to-edge vs. distance center-to-corner of 2d rect

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

void RecurseGenerateTree(vcQuadTree *pQuadTree, int currentNodeIndex, uint16_t srid, const udInt3 slippyCoords, const udDouble3 &position, const udFloat2 &halfExtents, int currentDepth, int maxDepth)
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

    udDouble3 localCorners[4];
    for (int t = 0; t < 4; ++t)
    {
      vcGIS_SlippyToLocal(srid, &localCorners[t], udInt2::create(pQuadTree->pNodes[childIndex].slippyPosition.x + (t % 2), pQuadTree->pNodes[childIndex].slippyPosition.y + (t / 2)), slippyCoords.z + pQuadTree->pNodes[childIndex].level);
    }

    // TODO: interpolate along edge
    bool insideQuadrant = position.x >= localCorners[0].x && position.x <= localCorners[1].x && position.y >= localCorners[2].y && position.y <= localCorners[0].y;
    // get closest point on child quadrant to view center
    //udFloat2 minBox = pQuadTree->pNodes[childIndex].position;
   // udFloat2 maxBox = pQuadTree->pNodes[childIndex].position + udFloat2::create(pCurrentNode->childSize);
    //udFloat2 closestPoint = udClamp(position, minBox, maxBox);
    //
    //// distance check closest point with view center to determine how
    //// far down we should traverse this child, if any
    //udFloat2 p0 = closestPoint;
    //udFloat2 p1 = position;
    //
    ////float r1 = halfExtents.x * 2.0f * rectToCircleRadius * pQuadTree->metaData.maxTreeDepth; // this value calculation isn't correct i dont think...
    //float overlap = udMag2(p0 - p1);//r1 - udMag2(p0 - p1);


   // if (overlap == 0.0f)
    //if (position.x >= minBox.x && position.x <= maxBox.x && position.y >= minBox.y && position.y <= maxBox.y)
    //if ((slippyCoords.x << pQuadTree->pNodes[childIndex].level) == pQuadTree->pNodes[childIndex].slippyPosition.x && (slippyCoords.y << pQuadTree->pNodes[childIndex].level) == pQuadTree->pNodes[childIndex].slippyPosition.y)
    if (insideQuadrant)
    {
      int maxChildDepth = maxDepth;// udMin(maxDepth, int(currentDepth + udCeil((overlap / r1) * (pQuadTree->metaData.maxTreeDepth - currentDepth))));
      RecurseGenerateTree(pQuadTree, childIndex, srid, slippyCoords, position, halfExtents, currentDepth + 1, maxChildDepth);
    }
    else
    {
      // don't divide the child, it is now a leaf node
      pQuadTree->metaData.leafNodeCount++;
      pQuadTree->pNodes[childIndex].isVisible = true;
      pQuadTree->metaData.visibleNodeCount++;
      //pCurrentNode->isVisible = false; // but its not visible
    }
  }
}

int CalculateTreeDepthFromViewDistance(const udFloat2 &halfExtents)
{
  return int(udLog2(1.0f / (halfExtents.x * rectToCircleRadius)));
}

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, uint16_t srid, const udInt3 &slippyCoords, const udDouble3 &localViewPos, const udFloat2 &viewSizeMS, vcQuadTreeMetaData *pMetaData /*= nullptr*/)
{
  vcQuadTree quadTree = {};

  int maxLocalTreeDepth = CalculateTreeDepthFromViewDistance(viewSizeMS);
  
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
  //quadTree.pNodes[0].childSize = 0.5f;

  RecurseGenerateTree(&quadTree, 0, srid, slippyCoords, localViewPos, viewSizeMS, 0, quadTree.metaData.maxTreeDepth);

  *ppNodes = quadTree.pNodes;
  *pNodeCount = quadTree.used;

  if (pMetaData)
    memcpy(pMetaData, &quadTree.metaData, sizeof(vcQuadTreeMetaData));
}