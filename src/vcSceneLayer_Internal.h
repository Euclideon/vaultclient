#include "vcSceneLayer.h"

#include "vcSettings.h"

#include "gl/vcLayout.h"

#include "udPlatform/udGeoZone.h"
#include "udPlatform/udJSON.h"

struct vcPolygonModel;
struct vcTexture;

enum
{
  // TODO: (EVC-541) Determine actual max URL length of I3S
  vcMaxURLLength = vcMaxPathLength,
};

struct vcSceneLayerNode
{
  enum vcLoadState
  {
    // Note: some logic depends on the ordering of these enums
    vcLS_NotLoaded,

    vcLS_Loading,
    vcLS_InMemory,

    vcLS_Success,
    vcLS_Failed,
  };

  vcLoadState loadState;

  int level;
  char id[vcMaxPathLength]; // TODO: (EVC-541) How big should this be?
  const char *pURL;
  vcSceneLayerNode *pChildren;
  size_t childrenCount;

  udDouble2 latLong; // this is taken from the MBS
  struct
  {
    udDouble3 position;
    double radius;
  } minimumBoundingSphere; // Cartesian
  //int lodSelectionMetric; // TODO
  double loadSelectionValue;
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
  } *pFeatureData;
  size_t featureDataCount;

  struct GeometryData
  {
    bool loaded;
    const char *pURL;

    udDouble4x4 originMatrix;
    udDouble4x4 transformation;
    //int type;
    vcPolygonModel *pModel;

    // cpu
    uint8_t *pData;
    uint64_t vertCount;
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
  vWorkerThreadPool *pThreadPool;

  char sceneLayerURL[vcMaxURLLength];
  udJSON description;
  udDouble4 extent;

  vcSceneLayerNode root;

  vcVertexLayoutTypes *pDefaultGeometryLayout;
  size_t defaultGeometryLayoutCount;
  size_t geometryVertexStride;
};

// TODO: (EVC-548) This will the nodes data entirely - (this actually may not be necessary, for
// example during convert, we only need the leaf nodes data...so why load non-leaf node geometry data etc.?)
// if `pNode` is nullptr, the entire model will be loaded into memory
// Note: Does not upload to GPU here
udResult vcSceneLayer_LoadNodeData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode = nullptr);

// Prepares the node for use
// Returns true if the node is ready for use
bool vcSceneLayer_TouchNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);
