#include "vcSceneLayer.h"

#include "vcSettings.h"

#include "vcLayout.h"

#include "udGeoZone.h"
#include "udJSON.h"

struct vcPolygonModel;
struct vcTexture;
struct udFile;
struct udMutex;

enum vcSceneLayerNormalReferenceFrame
{
  vcSLNRF_EarthCentered, // default
  vcSLNRF_EastNorthUp,
  vcSLNRF_VertexReferenceFrame

};

struct vcSceneLayerNode
{
  enum vcLoadState
  {
    vcLS_NotLoaded,
    vcLS_Failed,

    vcLS_InQueue,
    vcLS_Loading,

    vcLS_Success,
  };

  enum vcInternalsLoadState
  {
    vcILS_None,
    vcILS_BasicNodeData,  // Enough information has been loaded to determine if this node should be rendered or not
    vcILS_NodeInternals,  // All the node information has been loaded into main memory
    vcILS_UploadedToGPU,  // The node is completely ready for rendering

    vcILS_FreedGPUResources, // special state, the nodes texture data has been freed, and must be entirely reloaded
  };

  volatile vcLoadState loadState;
  volatile vcInternalsLoadState internalsLoadState;

  int level;
  char *pID;
  const char *pURL; // Note: the parent of this node is responsible for this memory. Placing this memory here just simplifies the loading processes of nodes.
  vcSceneLayerNode *pChildren;
  size_t childrenCount;

  udDouble2 latLong; // this is taken from the MBS
  struct
  {
    udDouble3 position;
    double radius;
  } minimumBoundingSphere; // Cartesian
  //int lodSelectionMetric; // TODO
  double lodSelectionValue;
  udGeoZone zone;

  struct SharedResource
  {
    const char *pURL;

  } *pSharedResources;
  size_t sharedResourceCount;

  struct FeatureData
  {
    const char *pURL;

    udDouble3 position;
    udDouble3 pivotOffset;
    udDouble4 minimumBoundingBox;

    vcVertexLayoutTypes *pGeometryLayout;
    uint64_t *pAttributeOffset;
    size_t geometryLayoutCount;

  } *pFeatureData;
  size_t featureDataCount;

  struct GeometryData
  {
    bool loaded;
    const char *pURL;

    udDouble4x4 originMatrix;
    //udDouble4x4 transformation;
    //int type;
    vcPolygonModel *pModel;

    // cpu
    uint8_t *pData;
    uint64_t vertCount;
    size_t vertexStride;
  } *pGeometryData;
  size_t geometryDataCount;

  struct TextureData
  {
    bool loaded;
    const char *pURL;

    vcTexture *pTexture;

    // cpu
    uint8_t *pData;
    int width, height;
  } *pTextureData;
  size_t textureDataCount;

  struct AttributeData
  {
    const char *pURL;

  } *pAttributeData;
  size_t attributeDataCount;
};

struct vcSceneLayer
{
  udFile *pFile;
  udMutex *pFileReadMutex;
  udWorkerPool *pThreadPool;

  char pathSeparatorChar; // '/' or '\\'
  udJSON description;
  udDouble4 extent;
  vcSceneLayerNormalReferenceFrame normalReferenceFrame;

  vcSceneLayerNode root;

  udInterlockedInt32 loadNodeJobsInFlight;
};

udResult vcSceneLayer_LoadNode(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);
udResult vcSceneLayer_LoadNodeInternals(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);
void vcSceneLayer_RecursiveDestroyNode(vcSceneLayerNode *pNode);

void vcSceneLayer_CheckNodePruneCandidancy(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);

// Prepares the node for use
// Returns true if the node is ready for use
bool vcSceneLayer_TouchNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, const udDouble3 &cameraPosition);
bool vcSceneLayer_ExpandNodeForRendering(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);

// TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
struct vcSceneLayerVertex
{
  udFloat3 position;
  udFloat3 normal;
  udFloat2 uv0;
  uint32_t colour;
};
vcSceneLayerVertex* vcSceneLayer_GetVertex(vcSceneLayerNode::GeometryData *pGeometry, uint64_t index);
