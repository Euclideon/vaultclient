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
  };

  udInterlockedInt32 loadStatus;

  bool tryLoad;
  float timeoutTime; // after a failed load, tiles have a time before they will request again

  // node owns this memory
  struct
  {
    vcTexture *pTexture;
    int32_t width, height;
    void *pData;
  } data;

  struct
  {
    vcTexture *pTexture; // which texture to draw this node with for this frame. Note: may belong to an ancestor node.
    udFloat2 uvStart;
    udFloat2 uvEnd;
  } drawInfo;
};

struct vcQuadTreeNode
{
  bool isUsed;

  udInt3 slippyPosition;

  uint32_t parentIndex;
  uint32_t childBlockIndex;
  uint32_t childMask; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  udInt2 morten;
  int neighbours;

  bool visible;
  volatile bool touched;
  bool rendered;

  // cached
  udDouble3 tileCenter, tileExtents;
  udDouble3 worldBounds[9]; // 3x3 grid of cartesian points
                            // [0, 1, 2,
                            //  3, 4, 5,
                            //  6, 7, 8]

  udInt2 demMinMax;  // in DEM units. For calculating AABB of node
  udDouble3 worldNormal;

  // node payloads
  vcNodeRenderInfo colourInfo;
  vcNodeRenderInfo demInfo;
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
  double cameraDistanceZeroAltitude;

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
  udDouble3 cameraPositionZeroAltitude;
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

inline bool vcQuadTree_HasDemData(vcQuadTreeNode *pNode)
{
  // Note: Not good enough, 0,0 is a valid dem min/max
  return pNode->demMinMax[0] != 0 || pNode->demMinMax[1] != 0;
}

void vcQuadTree_CalculateNodeAABB(vcQuadTreeNode *pNode);

#endif//vcQuadTree_h__
