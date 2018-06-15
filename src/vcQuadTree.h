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
  udInt2 slippyPosition;; // sw, se, nw, ne
  int level;
  bool isVisible;
};

void vcQuadTree_GenerateNodeList(vcQuadTreeNode **ppNodes, int *pNodeCount, uint16_t srid, const udInt3 &slippyCoords, const udDouble3 &cameraPosition, const udDouble2 &cameraViewSizeRatio, const double visibleFarPlane, vcQuadTreeMetaData *pMetaData = nullptr);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

#endif//vcQuadTree_h__