#ifndef vcQuadTree_h__
#define vcQuadTree_h__

#include "udPlatform/udMath.h"

struct vcQuadTreeCreateInfo
{
  int16_t srid;
  udInt3 slippyCoords;
  udDouble3 cameraPosition;
  double quadTreeWorldSize;
  double visibleDistance;
  double quadTreeHeightOffset;
};

struct vcQuadTreeMetaData
{
  int maxTreeDepth;
  int memUsageMB; // node usage
  int leafNodeCount;
};

struct vcQuadTreeNode
{
  uint32_t parentIndex;
  int childMask; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  udInt2 slippyPosition;; // sw, se, nw, ne
  int level;
  bool isVisible;
};

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, const vcQuadTreeCreateInfo &createInfo, vcQuadTreeMetaData *pMetaData = nullptr);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

#endif//vcQuadTree_h__