#ifndef vcQuadTree_h__
#define vcQuadTree_h__

#include "vcSettings.h"

#include "udMath.h"
#include "vcGIS.h"

struct vcTexture;

struct vcNodeRenderInfo
{
  enum vcTileLoadStatus
  {
    vcTLS_None,
    vcTLS_InQueue,
    vcTLS_Downloading,
    vcTLS_Downloaded,
    vcTLS_Loaded,

    vcTLS_Failed,
  } volatile loadStatus;

  bool tryLoad;
  float timeoutTime; // after a failed load, tiles have a time before they will request again

  vcTexture *pTexture;
  int32_t width, height;
  void *pData;

  bool fadingIn;
  float transparency;

  // cached
  udDouble2 center;
};

struct vcQuadTreeNode
{
  bool isUsed;

  udInt3 slippyPosition;

  uint32_t parentIndex;
  uint32_t childBlockIndex;
  uint32_t childMask; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  int level;

  bool visible;
  volatile bool touched;
  bool rendered;

  // cached
  udDouble2 worldBounds[4]; // corners
  udDouble2 tileCenter, tileExtents;

  vcNodeRenderInfo renderInfo;
};

struct vcQuadTreeMetaData
{
  int maxTreeDepth;
  int memUsageMB; // node usage
  float generateTimeMs;

  int nodeTouchedCount;
  int leafNodeCount;
  int visibleNodeCount;
  int nodeRenderCount;
};

struct vcQuadTree
{
  vcSettings *pSettings;

  vcQuadTreeMetaData metaData;
  udDouble4 frustumPlanes[6];
  vcGISSpace gisSpace;
  udInt3 slippyCoords;
  udDouble3 cameraWorldPosition;
  udDouble3 cameraTreePosition;
  int expectedTreeDepth; // depth of the deepest node
  double quadTreeWorldSize;
  double quadTreeHeightOffset;

  uint32_t rootIndex;
  bool completeRerootRequired;
  struct
  {
    vcQuadTreeNode *pPool;
    uint32_t used;
    uint32_t capacity;
  } nodes;
};

struct vcQuadTreeViewInfo
{
  vcGISSpace *pSpace;
  udInt3 slippyCoords;
  udDouble3 cameraPosition;
  double quadTreeWorldSize;
  double quadTreeHeightOffset;
  udDouble4x4 viewProjectionMatrix;
  int maxVisibleTileLevel;
};

void vcQuadTree_Create(vcQuadTree *pQuadTree, vcSettings *pSettings);
void vcQuadTree_Destroy(vcQuadTree *pQuadTree);
void vcQuadTree_Reset(vcQuadTree *pQuadTree);

void vcQuadTree_Update(vcQuadTree *pQuadTree, const vcQuadTreeViewInfo &viewInfo);
void vcQuadTree_Prune(vcQuadTree *pQuadTree);

bool vcQuadTree_IsNodeVisible(const vcQuadTree *pQuadTree, const vcQuadTreeNode *pNode);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

inline bool vcQuadTree_IsVisibleLeafNode(const vcQuadTree *pQuadTree, const vcQuadTreeNode *pNode)
{
  if (vcQuadTree_IsLeafNode(pNode))
    return true;

  for (int c = 0; c < 4; ++c)
  {
    if (pQuadTree->nodes.pPool[pNode->childBlockIndex + c].touched)
      return false;
  }

  return true;
}

#endif//vcQuadTree_h__
