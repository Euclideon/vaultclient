#ifndef vcQuadTree_h__
#define vcQuadTree_h__

#include "vcSettings.h"

#include "udMath.h"
#include "vcGIS.h"

#define INVALID_NODE_INDEX 0xffffffff

enum
{
  // Always descend a certain absolute depth to ensure the resulting geometry
  // is somewhat the shape of the projection (e.g. ECEF).
  vcQuadTree_MinimumDescendLayer = 2
};

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
  int loadRetryCount;
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

  int serverId;
  udInt3 slippyPosition;

  uint32_t parentIndex;
  uint32_t childBlockIndex;
  uint32_t childMask; // [1, 2, 4, 8] for each corner [bottom left, bottom right, top left, top right]
  udInt2 morten;
  int neighbours;

  bool visible;
  volatile bool touched;

  // if a node was rendered with missing information (only considers colour at the moment, includes any failed descendents)
  bool completeRender;

  // cached
  udDouble3 tileCenter, tileExtents;
  udDouble3 worldBounds[9]; // 3x3 grid of cartesian points
                            // [0, 1, 2,
                            //  3, 4, 5,
                            //  6, 7, 8]

  udDouble3 worldNormals[9];
  udDouble3 worldBitangents[9];

  enum vcDemBoundsState
  {
    vcDemBoundsState_None,
    vcDemBoundsState_Inherited, // passed from child or/parent
    vcDemBoundsState_Absolute, // this node has downloaded its dem data
  } demBoundsState;
  udInt2 demMinMax;  // in DEM units. For calculating AABB of node
  udInt2 activeDemMinMax; // DEM can turned off / on, so cache this state

  // cache fine DEM data on CPU.
  int16_t *pDemHeightsCopy;
  udInt2 demHeightsCopySize;

  // node payloads
  vcNodeRenderInfo colourInfo;
  vcNodeRenderInfo demInfo;

  // TODO: (AB#1751)
  // Note: 'normalInfo' piggy backs a lot of its state from 'demInfo'.
  vcNodeRenderInfo normalInfo;
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
  int serverId;
  vcSettings *pSettings;

  // store map height state to detect changes
  bool demWasEnabled;
  float previousMapHeight;

  vcQuadTreeMetaData metaData;
  udDouble4 frustumPlanes[6];
  udGeoZone geozone;
  udInt3 rootSlippyCoords;
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
  udGeoZone *pGeozone;
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
void vcQuadTree_UpdateView(vcQuadTree *pQuadTree, const udDouble3 &cameraPosition, const udDouble4x4 &viewProjectionMatrix);

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

void vcQuadTree_CalculateNodeAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode);

vcQuadTreeNode* vcQuadTree_GetLeafNodeFromCartesian(vcQuadTree *pQuadTree, const udDouble3 &point);

#endif//vcQuadTree_h__
