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
  udDouble4x4 viewProjectionMatrix;
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
  int childMaskInParent; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  udInt2 slippyPosition;
  int level;
  bool isVisible;
};

struct vcQuadTree;

udResult vcQuadTree_Init(vcQuadTree **ppQuadTree);
udResult vcQuadTree_Destroy(vcQuadTree **ppQuadTree);

udResult vcQuadTree_GenerateNodeList(vcQuadTree *pQuadTree, const vcQuadTreeCreateInfo &createInfo, vcQuadTreeMetaData *pMetaData = nullptr);
udResult vcQuadTree_GetNodeList(vcQuadTree *pQuadTree, vcQuadTreeNode **ppNodes, int *pNodeCount);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

#endif//vcQuadTree_h__
