#ifndef vcQuadTree_h__
#define vcQuadTree_h__

#include "udPlatform/udMath.h"

struct vcQuadTreeMetaData
{
  int maxTreeDepth;
  int memUsageMB; // node usage
  int leafNodeCount;
  int visibleNodeCount;
};

struct vcQuadTreeNode
{
  uint32_t parentIndex;
  int childMask; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  float childSize;
  udFloat2 position; // origin of the node (bottom left corner)
  int level;
  bool isVisible;
};

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, int realRootDepth, const udFloat2 &viewPositionMS, const udFloat2 &viewPositionSizeMS, vcQuadTreeMetaData *pMetaData = nullptr);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

#endif//vcQuadTree_h__