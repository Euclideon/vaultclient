#ifndef vcQuadTree_h__
#define vcQuadTree_h__

#include "vcSettings.h"

#include "udPlatform/udMath.h"

struct vcTexture;
struct vcGISSpace;

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
  } loadStatus;

  bool tryLoad;
  int loadAttempts;

  vcTexture *pTexture;
  int32_t width, height;
  void *pData;

  float lastUsedTime;

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
  bool touched;
  bool rendered;

  udDouble2 worldBounds[4]; // corners
  vcNodeRenderInfo renderInfo;
};

struct vcQuadTreeMetaData
{
  int maxTreeDepth;
  int memUsageMB; // node usage
  float generateTimeMs;
};

struct vcQuadTree
{
  vcSettings *pSettings;

  vcQuadTreeMetaData metaData;
  udDouble4 frustumPlanes[6];
  vcGISSpace *pSpace;
  udInt3 slippyCoords;
  udDouble3 cameraWorldPosition;
  double quadTreeWorldSize;
  double visibleDistance;
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
  double visibleDistance;
  double quadTreeHeightOffset;
  udDouble4x4 viewProjectionMatrix;
};

void vcQuadTree_Create(vcQuadTree *pQuadTree, vcSettings *pSettings);
void vcQuadTree_Destroy(vcQuadTree *pQuadTree);
void vcQuadTree_Reset(vcQuadTree *pQuadTree);

void vcQuadTree_Update(vcQuadTree *pQuadTree, const vcQuadTreeViewInfo &viewInfo);
void vcQuadTree_Prune(vcQuadTree *pQuadTree);

inline bool vcQuadTree_IsLeafNode(const vcQuadTreeNode *pNode)
{
  return pNode->childMask == 0;
}

#endif//vcQuadTree_h__
