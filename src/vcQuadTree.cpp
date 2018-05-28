
#include "vcQuadTree.h"

#define INVALID_NODE_INDEX uint32_t(-1)

const float rectToCircleRadius = udSqrt(2.0f); // distance from center-to-edge vs. distance center-to-corner of 2d rect

struct vcQuadTree
{
  vcQuadTreeMetaData metaData;

  vcQuadTreeNode *pNodes;
  int used;
  int capacity;
};

udResult vcQuadTree_Init(vcQuadTree **ppQuadTree)
{
  udResult result = udR_Success;
  vcQuadTree *pQuadTree = nullptr;

  UD_ERROR_NULL(ppQuadTree, udR_InvalidParameter_);

  pQuadTree = udAllocType(vcQuadTree, 1, udAF_Zero);
  UD_ERROR_NULL(pQuadTree, udR_MemoryAllocationFailure);

  *ppQuadTree = pQuadTree;
epilogue:
  return result;
}

udResult vcQuadTree_Destroy(vcQuadTree **ppQuadTree)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(ppQuadTree, udR_InvalidParameter_);

  udFree(*ppQuadTree);
  *ppQuadTree = nullptr;

epilogue:
  return result;
}

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


void RecurseGenerateTree(vcQuadTree *pQuadTree, int currentNodeIndex, const udFloat2 &position, const udFloat2 &halfExtents, int currentDepth, int maxDepth)
{
  if (currentDepth >= maxDepth)
  {
    pQuadTree->metaData.leafNodeCount++;
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
    pQuadTree->pNodes[childIndex].childSize = pCurrentNode->childSize * 0.5f;
    pQuadTree->pNodes[childIndex].parentIndex = currentNodeIndex;
    pQuadTree->pNodes[childIndex].position = pCurrentNode->position + udFloat2::create(float(i % 2), float(i / 2)) * pCurrentNode->childSize;
    pQuadTree->pNodes[childIndex].level = currentDepth + 1;

    // get closest point on child quadrant to view center
    udFloat2 minBox = pQuadTree->pNodes[childIndex].position;
    udFloat2 maxBox = pQuadTree->pNodes[childIndex].position + udFloat2::create(pCurrentNode->childSize);
    udFloat2 closestPoint = udClamp(position, minBox, maxBox);

    // distance check closest point with view center to determine how
    // far down we should traverse this child, if any
    udFloat2 p0 = closestPoint;
    udFloat2 p1 = position;
    float r1 = halfExtents.x * rectToCircleRadius * pQuadTree->metaData.maxTreeDepth; // this value calculation isn't correct i dont think...
    float overlap = r1 - udMag2(p0 - p1);

    if (overlap > 0)
    {
      int maxChildDepth = udMin(maxDepth, int(currentDepth + udCeil((overlap / r1) * (pQuadTree->metaData.maxTreeDepth - currentDepth))));
      RecurseGenerateTree(pQuadTree, childIndex, position, halfExtents, currentDepth + 1, maxChildDepth);
    }
    else
    {
      // don't divide the child, it is now a leaf node
      pQuadTree->metaData.leafNodeCount++;
    }
  }
}

int CalculateTreeDepthFromViewDistance(const udFloat2 &halfExtents)
{
  return int(udLog2(1.0f / (halfExtents.x * 2.0f * rectToCircleRadius)));
}

void vcQuadTree_GenerateNodeList(vcQuadTree *pQuadTree, vcQuadTreeNode **ppNodes, int *pNodeCount, const udFloat2 &viewPositionMS, const udFloat2 &viewPositionSizeMS, vcQuadTreeMetaData *pMetaData /*= nullptr*/)
{
  // completely rebuild tree for now
  udFree(pQuadTree->pNodes);
  pQuadTree->pNodes = nullptr;

  int maxTreeDepth = CalculateTreeDepthFromViewDistance(viewPositionSizeMS);
  int startingCapacity = 50; // hard coded for now - this could be guessed from maxTreeDepth...
  pQuadTree->capacity = startingCapacity / 2;
  ExpandTreeCapacity(pQuadTree);
  pQuadTree->metaData.leafNodeCount = 0;
  pQuadTree->metaData.maxTreeDepth = maxTreeDepth;

  // Initialize root
  pQuadTree->used = 1;
  pQuadTree->pNodes[0].parentIndex = INVALID_NODE_INDEX;
  pQuadTree->pNodes[0].childSize = 0.5f;

  RecurseGenerateTree(pQuadTree, 0, viewPositionMS, viewPositionSizeMS, 0, pQuadTree->metaData.maxTreeDepth);
  
  *ppNodes = pQuadTree->pNodes;
  *pNodeCount = pQuadTree->used;

  if (pMetaData)
    memcpy(pMetaData, &pQuadTree->metaData, sizeof(vcQuadTreeMetaData));
}