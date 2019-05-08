#include "vcSceneLayerRenderer.h"

#include <vector>

#include "stb_image.h"

#include "vcSceneLayerHelper.h"
#include "vcSettings.h"
#include "vcPolygonModel.h"

#include "gl/vcTexture.h"

#include "udPlatform/udJSON.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udGeoZone.h"
#include "vCore/vWorkerThread.h"

static const char *pSceneLayerInfoFile = "3dSceneLayer.json";
static const char *pNodeInfoFile = "3dNodeIndexDocument.json";

// Shared, reused dynamically allocated
static char *pSharedStringBuffer = nullptr;

enum
{
  // TODO: (EVC-541) Determine actual max URL length of I3S
  vcMaxURLLength = vcMaxPathLength,
  vcMaxUploadsPerFrame = 3, // gpu
};

struct vcSceneLayerRendererNode
{
  enum vcLoadState
  {
    vcLS_NotLoaded,
    vcLS_Loading,
    vcLS_InMemory,
    vcLS_Success,

    vcLS_Failed,
  };

  vcLoadState loadState;

  char id[vcMaxPathLength]; // TODO: How big should this be?
  char url[vcMaxURLLength];
  vcSceneLayerRendererNode *pChildren;
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
    char url[vcMaxURLLength];

  } *pSharedResources;
  size_t sharedResourceCount;

  struct FeatureData
  {
    char url[vcMaxURLLength];

    udDouble3 position;
    udDouble3 pivotOffset;
    udDouble4 minimumBoundingBox;
  } *pFeatureData;
  size_t featureDataCount;

  struct GeometryData
  {
    char url[vcMaxURLLength];

    udDouble4x4 originMatrix;
    udDouble4x4 transformation;
    //int type;
    vcPolygonModel *pModel;

    // cached for delayed upload to gpu
    char *pVertsData;
    uint64_t vertCount;
  } *pGeometryData;
  size_t geometryDataCount;

  struct TextureData
  {
    char url[vcMaxURLLength];

    vcTexture *pTexture;

    // cached for delayed upload to gpu
    void *pData;
    int width;
    int height;
  } *pTextureData;
  size_t textureDataCount;

  struct AttributeData
  {
    char url[vcMaxURLLength];

  } *pAttributeData;
  size_t attributeDataCount;
};

struct vcSceneLayerRenderer
{
  const vcSettings *pSettings;
  vWorkerThreadPool *pThreadPool;
  int uploadsThisFrame;

  char sceneLayerURL[vcMaxURLLength];
  udJSON description;
  udDouble4 extent;
  vcSceneLayerRendererNode root;

  vcVertexLayoutTypes *pDefaultGeometryLayout;
  size_t defaultGeometryLayoutCount;
};

struct vcLoadNodeJobData
{
  vcSceneLayerRenderer *pSceneLayer;
  vcSceneLayerRendererNode *pNode;
};

udResult vcSceneLayerRenderer_LoadNodeData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, bool recursiveLoadChildren = false, bool fullNodeLoad = false);

void vcSceneLayerRenderer_AsyncLoadNode(void *pSceneLayerPtr)
{
  vcLoadNodeJobData *pLoadData = (vcLoadNodeJobData*)pSceneLayerPtr;

  // Note: this will load the entirety of the nodes data - which could be unnecessary?
  vcSceneLayerRenderer_LoadNodeData(pLoadData->pSceneLayer, pLoadData->pNode);
}

static int gRefCount = 0;
void vcSceneLayerRenderer_Create()
{
  ++gRefCount;
  if (gRefCount == 1)
  {
    pSharedStringBuffer = udAllocType(char, vcMaxURLLength, udAF_Zero);
  }
}

void vcSceneLayerRenderer_Destroy()
{
  --gRefCount;
  if (gRefCount == 0)
  {
    udFree(pSharedStringBuffer);
  }
}

// TODO: Consider moving this functionality out into udPlatform
const char *vcSceneLayerRenderer_AppendURL(const char *pRootURL, const char *pURL)
{
  // TODO: (EVC-541) Overcome udTempStr()'s max size of 2048 (max size of URL is 4096)
  if (udStrlen(pURL) >= 2 && pURL[0] == '.' && pURL[1] == '/')
    return udTempStr("%s/%s", pRootURL, pURL + 2);

  // TODO: ??! One of the datasets has paths like this?
  if (udStrlen(pURL) >= 4 && pURL[0] == '.' && pURL[1] == '\\' && pURL[2] == '/')
    return udTempStr("%s/%s", pRootURL, pURL + 3);

  if (udStrlen(pURL) >= 2 && pURL[0] == '.' && pURL[1] == '.')
    return udTempStr("%s/%s", pRootURL, pURL + 2);

  // TODO: (EVC-541) What else?

  return udTempStr("%s/%s", pRootURL, pURL);
}

void vcSceneLayerRenderer_RecursiveDestroyNode(vcSceneLayerRendererNode *pNode)
{
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    vcPolygonModel_Destroy(&pNode->pGeometryData[i].pModel);
    udFree(pNode->pGeometryData[i].pVertsData);
  }

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    vcTexture_Destroy(&pNode->pTextureData[i].pTexture);
    udFree(pNode->pTextureData[i].pData);
  }

  udFree(pNode->pSharedResources);
  udFree(pNode->pFeatureData);
  udFree(pNode->pGeometryData);
  udFree(pNode->pTextureData);
  udFree(pNode->pAttributeData);

  for (size_t i = 0; i < pNode->childrenCount; ++i)
    vcSceneLayerRenderer_RecursiveDestroyNode(&pNode->pChildren[i]);

  udFree(pNode->pChildren);
}

udResult vcSceneLayerRenderer_LoadNodeFeatureData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  udJSON featuresJSON;

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udSprintf(pSharedStringBuffer, vcMaxURLLength, "%s.json", pNode->pFeatureData[i].url);
    UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendURL(pNode->url, pSharedStringBuffer), (void**)&pFileData, &fileLen));
    UD_ERROR_CHECK(featuresJSON.Parse(pFileData));

    // TODO: JIRA task add a udJSON.AsDouble2()
    // Note: 'position' is either (x/y/z) OR just (x/y).
    pNode->pFeatureData[i].position.x = featuresJSON.Get("featureData[%zu].position[0]", i).AsDouble();
    pNode->pFeatureData[i].position.y = featuresJSON.Get("featureData[%zu].position[1]", i).AsDouble();
    pNode->pFeatureData[i].position.z = featuresJSON.Get("featureData[%zu].position[2]", i).AsDouble();
    pNode->pFeatureData[i].pivotOffset = featuresJSON.Get("featureData[%zu].pivotOffset", i).AsDouble3();
    pNode->pFeatureData[i].minimumBoundingBox = featuresJSON.Get("featureData[%zu].mbb", i).AsDouble4();

    // TODO: (EVC-542) Where does this go?
    //pNode->geometries.transformation = featuresJSON.Get("geometryData[0].transformation").AsDouble4x4();

    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pFileData);
  return result;
}

// Assumed the nodes features have already been loaded
udResult vcSceneLayerRenderer_LoadNodeGeometryData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udResult result;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  uint64_t featureCount = 0;
  uint64_t faceCount = 0;
  char *pCurrentFile = nullptr;
  uint32_t vertexSize = 0;
  uint32_t vertexOffset = 0;
  uint32_t attributeSize = 0;
  size_t headerElementCount = 0;
  udFloat3 vertI3S = {};
  udDouble3 pointCartesian = {};
  udDouble3 originCartesian = {};
  udFloat3 finalVertPosition = {};
  const char *pPropertyName = nullptr;
  const char *pTypeName = nullptr;

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(pSharedStringBuffer, vcMaxURLLength, "%s.bin", pNode->pGeometryData[i].url);
    UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendURL(pNode->url, pSharedStringBuffer), (void**)&pFileData, &fileLen));

    pCurrentFile = pFileData;

    /// Header
    // TODO: Calculate once on node load... not per geometry
    headerElementCount = pSceneLayer->description.Get("store.defaultGeometrySchema.header").ArrayLength();
    for (size_t h = 0; h < headerElementCount; ++h)
    {
      pPropertyName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].property", h).AsString();
      pTypeName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].type", h).AsString();
      if (udStrEqual(pPropertyName, "vertexCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&pNode->pGeometryData[i].vertCount, pCurrentFile, pTypeName);
      else if (udStrEqual(pPropertyName, "featureCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&featureCount, pCurrentFile, pTypeName);
      else if (udStrEqual(pPropertyName, "faceCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&faceCount, pCurrentFile, pTypeName);
      else
      {
        // TODO: (EVC-542) Unknown property type
        UD_ERROR_SET(udR_ParseError);
      }
    }

    /// Geometry
    // TODO: EVC-542
    vertexSize = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);
    UD_ERROR_IF(pNode->pGeometryData[i].vertCount * vertexSize == 0, udR_ParseError);

    pNode->pGeometryData[i].pVertsData = udAllocType(char, pNode->pGeometryData[i].vertCount * vertexSize, udAF_Zero);

    // vertices are non-interleaved.
    // TODO: (EVC-542) Is that what "Topology: PerAttributeArray" signals?
    for (size_t attributeIndex = 0; attributeIndex < pSceneLayer->defaultGeometryLayoutCount; ++attributeIndex)
    {
      attributeSize = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout[attributeIndex]);

      if (pSceneLayer->pDefaultGeometryLayout[attributeIndex] == vcVLT_Position3)
      {
        // positions require additional processing

        // Get the first position, and use that as an origin matrix (fixes floating point precision issue)
        // TODO: (EVC-540) Handle different sized positions
        memcpy(&vertI3S, pCurrentFile, sizeof(vertI3S));
        pointCartesian = udGeoZone_ToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + vertI3S.x, pNode->latLong.y + vertI3S.y, 0.0), true);
        originCartesian = udDouble3::create(pointCartesian.x, pointCartesian.y, vertI3S.z + pNode->pFeatureData[i].position.z);
        pNode->pGeometryData[i].originMatrix = udDouble4x4::translation(originCartesian);

        for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
        {
          memcpy(&vertI3S, pCurrentFile, sizeof(vertI3S));
          pointCartesian = udGeoZone_ToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + vertI3S.x, pNode->latLong.y + vertI3S.y, 0.0), true);
          pointCartesian.z = vertI3S.z; // Elevation (the z component of the vertex position) is specified in meters
          finalVertPosition = udFloat3::create(pointCartesian - originCartesian);

          memcpy(pNode->pGeometryData[i].pVertsData + vertexOffset + (v * vertexSize), &finalVertPosition, attributeSize);
          pCurrentFile += attributeSize;
        }

        // TODO: (EVC-543) The heights appear to be normalized? So I did this
        // to line it up so that the minimum vertex height is at 0.0. I don't know
        // if this is correct, I've probably missed something in the model format.
        pNode->pGeometryData[i].originMatrix.axis.t.z += pNode->minimumBoundingSphere.position.z;
      }
      else
      {
        for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
        {
          memcpy(pNode->pGeometryData[i].pVertsData + vertexOffset + (v * vertexSize), pCurrentFile, attributeSize);
          pCurrentFile += attributeSize;
        }
      }
      vertexOffset += attributeSize;
    }

    //uint64_t featureAttributeId = *(uint64_t*)(pCurrentFile);
    //uint32_t featureAttributeFaceRangeMin = *(uint32_t*)(pCurrentFile + 8);
    //uint32_t featureAttributeFaceRangeMax = *(uint32_t*)(pCurrentFile + 12);

    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pFileData);
  return result;
}

// Assumed nodes features has already been loaded
udResult vcSceneLayerRenderer_LoadNodeTextureData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result = udR_Success;
  void *pData = nullptr;
  void *pFileData = nullptr;
  int64_t fileLen = 0;
  int channelCount = 0;
  int width = 0;
  int height = 0;

  // TODO: (EVC-542), (EVC-544)
  // TODO: Handle texture load failures
  const char *pExtensions[] = { "jpg", "bin" };//, "bin.dds" };

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    // TODO: (EVC-544)
    //result = udR_Failure_;

    // TODO: (EVC-542) other formats (this information is in sharedResource.json)
    for (size_t f = 0; f < udLengthOf(pExtensions); ++f)
    {
      udSprintf(pSharedStringBuffer, vcMaxURLLength, "%s.%s", pNode->pTextureData[i].url, pExtensions[f]);
      if (udFile_Load(vcSceneLayerRenderer_AppendURL(pNode->url, pSharedStringBuffer), (void**)&pFileData, &fileLen) != udR_Success)
        continue;

      pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      pNode->pTextureData[i].width = width;
      pNode->pTextureData[i].height = height;
      pNode->pTextureData[i].pData = udMemDup(pData, sizeof(uint32_t)*width*height, 0, udAF_None);

      udFree(pFileData);
      stbi_image_free(pData);
      break;
    }
  }

//epilogue:
  return result;
}

void vcSceneLayerRenderer_UploadNodeData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  printf("Uploading node data: %s\n", pNode->id);

  // TODO: handle failures
  // geometry
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    //UD_ERROR_IF(vertCount > UINT16_MAX, udR_Failure_);
    //UD_ERROR_CHECK()
    vcPolygonModel_CreateFromData(&pNode->pGeometryData[i].pModel, pNode->pGeometryData[i].pVertsData, (uint16_t)pNode->pGeometryData[i].vertCount, pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);
    udFree(pNode->pGeometryData[i].pVertsData);

    ++pSceneLayer->uploadsThisFrame;
  }

  // textures
  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    vcTexture_Create(&pNode->pTextureData[i].pTexture, pNode->pTextureData[i].width, pNode->pTextureData[i].height, pNode->pTextureData[i].pData, vcTextureFormat_RGBA8, vcTFM_Linear, true);
    udFree(pNode->pTextureData[i].pData);

    ++pSceneLayer->uploadsThisFrame;
  }

  pNode->loadState = vcSceneLayerRendererNode::vcLS_Success;
}

template<typename T>
udResult vcSceneLayerRenderer_AllocateNodeData(T **ppData, size_t *pDataCount, const udJSON &nodeJSON, const char *pKey)
{
  udResult result;
  T *pData = nullptr;
  size_t count = 0;

  count = nodeJSON.Get("%s", pKey).ArrayLength();
  UD_ERROR_IF(count == 0, udR_Success); // valid

  pData = udAllocType(T, count, udAF_Zero);
  UD_ERROR_NULL(pData, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < count; ++i)
    udStrcpy(pData[i].url, sizeof(pData[i].url), nodeJSON.Get("%s", udTempStr("%s[%zu].href", pKey, i)).AsString());

  (*ppData) = pData;
  (*pDataCount) = count;
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udFree(pData);

  return result;
}

void vcSceneLayerRenderer_CalculateNodeBounds(vcSceneLayerRendererNode *pNode, const udDouble4 &mbsLatLongHeightRadius)
{
  int sridCode = 0;

  pNode->latLong.x = mbsLatLongHeightRadius.x;
  pNode->latLong.y = mbsLatLongHeightRadius.y;

  // Calculate cartesian minimum bounding sphere
  udDouble3 originLatLong = udDouble3::create(mbsLatLongHeightRadius.x, mbsLatLongHeightRadius.y, 0.0);
  udGeoZone_FindSRID(&sridCode, originLatLong, true);
  udGeoZone_SetFromSRID(&pNode->zone, sridCode);

  pNode->minimumBoundingSphere.position = udGeoZone_ToCartesian(pNode->zone, originLatLong, true);
  pNode->minimumBoundingSphere.position.z = mbsLatLongHeightRadius.z;
  pNode->minimumBoundingSphere.radius = mbsLatLongHeightRadius.w;

  // debugging
  printf("Origin: %f, %f\n", pNode->minimumBoundingSphere.position.x, pNode->minimumBoundingSphere.position.y);
  printf("SRID: %d\n", sridCode);
}

udResult vcSceneLayerRenderer_LoadNodeData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, bool recursiveLoadChildren /*= false*/, bool fullNodeLoad /*= false*/)
{
  udResult result;
  udJSON nodeJSON;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  vcSceneLayerRendererNode *pChildNode = nullptr;

  pNode->loadState = vcSceneLayerRendererNode::vcLS_Loading;

  // Load the nodes info
  UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendURL(pNode->url, pNodeInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  // this is potentially re-copying
  udStrcpy(pNode->id, sizeof(pNode->id), nodeJSON.Get("id").AsString());
  pNode->loadSelectionValue = nodeJSON.Get("lodSelection[0].maxError").AsDouble(); // TODO: I've made an assumptions here (e.g. always has at least one lod metric)

  // this is potentially re-calculating
  vcSceneLayerRenderer_CalculateNodeBounds(pNode, nodeJSON.Get("mbs").AsDouble4());

  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, "featureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, "textureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, "attribut  eData"));

  // Hierarchy
  pNode->childrenCount = nodeJSON.Get("children").ArrayLength();
  if (pNode->childrenCount > 0)
  {
    pNode->pChildren = udAllocType(vcSceneLayerRendererNode, pNode->childrenCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pChildren, udR_MemoryAllocationFailure);

    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      pChildNode = &pNode->pChildren[i];
      pChildNode->loadState = vcSceneLayerRendererNode::vcLS_NotLoaded;

      udStrcpy(pChildNode->id, sizeof(pChildNode->id), nodeJSON.Get("children[%zu].id", i).AsString());
      udStrcpy(pChildNode->url, sizeof(pChildNode->url), vcSceneLayerRenderer_AppendURL(pNode->url, nodeJSON.Get("children[%zu].href", i).AsString()));

      vcSceneLayerRenderer_CalculateNodeBounds(pChildNode, nodeJSON.Get("children[%zu].mbs", i).AsDouble4());
    }
  }

  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeFeatureData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeGeometryData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeTextureData(pSceneLayer, pNode));

  pNode->loadState = vcSceneLayerRendererNode::vcLS_InMemory;

  if (fullNodeLoad)
    vcSceneLayerRenderer_UploadNodeData(pSceneLayer, pNode);

  if (recursiveLoadChildren)
  {
    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeData(pSceneLayer, &pNode->pChildren[i], recursiveLoadChildren, fullNodeLoad));
    }
  }

  result = udR_Success;
epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerRendererNode::vcLS_Failed;

  udFree(pFileData);
  return result;
}

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, const vcSettings *pSettings, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  vcSceneLayerRenderer_Create();

  udResult result;
  vcSceneLayerRenderer *pSceneLayer = nullptr;
  const char *pFileData = nullptr;
  int64_t fileLen = 0;
  const char *pAttributeName = nullptr;
  const char *pRootURL = nullptr;

  pSceneLayer = udAllocType(vcSceneLayerRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer, udR_MemoryAllocationFailure);

  pSceneLayer->pSettings = pSettings;
  pSceneLayer->pThreadPool = pWorkerThreadPool;
  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform moves
  UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendURL(pSceneLayer->sceneLayerURL, pSceneLayerInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(pSceneLayer->description.Parse(pFileData));

  // Load layout description now
  pSceneLayer->defaultGeometryLayoutCount = pSceneLayer->description.Get("store.defaultGeometrySchema.ordering").ArrayLength();
  UD_ERROR_IF(pSceneLayer->defaultGeometryLayoutCount == 0, udR_ParseError);

  pSceneLayer->pDefaultGeometryLayout = udAllocType(vcVertexLayoutTypes, pSceneLayer->defaultGeometryLayoutCount, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer->pDefaultGeometryLayout, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < pSceneLayer->defaultGeometryLayoutCount; ++i)
  {
    pAttributeName = pSceneLayer->description.Get("store.defaultGeometrySchema.ordering[%zu]", i).AsString();

    // TODO: (EVC-540) Probably need to read vertex attribute info.
    if (udStrEqual(pAttributeName, "position"))
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_Position3;
    else if (udStrEqual(pAttributeName, "normal"))
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_Normal3;
    else if (udStrEqual(pAttributeName, "uv0"))
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_TextureCoords2;
    else if (udStrEqual(pAttributeName, "color"))
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_ColourBGRA;
    else
    {
      // TODO: (EVC-542) Unknown attribute type
      UD_ERROR_SET(udR_ParseError);
    }
  }

  // Initialize the root
  pRootURL = vcSceneLayerRenderer_AppendURL(pSceneLayer->sceneLayerURL, pSceneLayer->description.Get("store.rootNode").AsString());
  udStrcpy(pSceneLayer->root.url, sizeof(pSceneLayer->root.url), pRootURL);
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeData(pSceneLayer, &pSceneLayer->root, false, true));

  *ppSceneLayer = pSceneLayer;
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcSceneLayerRenderer_Destroy();

  udFree(pFileData);
  return result;
}

udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayer)
{
  udResult result;
  vcSceneLayerRenderer *pSceneLayer = nullptr;

  UD_ERROR_NULL(ppSceneLayer, udR_InvalidParameter_);

  pSceneLayer = *ppSceneLayer;
  *ppSceneLayer = nullptr;

  vcSceneLayerRenderer_RecursiveDestroyNode(&pSceneLayer->root);

  pSceneLayer->description.Destroy();

  udFree(pSceneLayer->pDefaultGeometryLayout);
  udFree(pSceneLayer);
  result = udR_Success;

epilogue:
  vcSceneLayerRenderer_Destroy();
  return result;
}

// TODO: This is a copy of the implementation from vcTileRenderer.cpp
// Returns -1=outside, 0=inside, >0=partial (bits of planes crossed)
static int vcSceneLayerRenderer_FrustumTest(const udDouble4 frustumPlanes[6], const udDouble3 &boundCenter, const udDouble3 &boundExtents)
{
  int partial = 0;

  for (int i = 0; i < 6; ++i)
  {
    double distToCenter = udDot4(udDouble4::create(boundCenter, 1.0), frustumPlanes[i]);
    //optimized for case where boxExtents are all same: udFloat radiusBoxAtPlane = udDot3(boxExtents, udAbs(udVec3(curPlane)));
    double radiusBoxAtPlane = udDot3(boundExtents, udAbs(frustumPlanes[i].toVector3()));
    if (distToCenter < -radiusBoxAtPlane)
      return -1; // Box is entirely behind at least one plane
    else if (distToCenter <= radiusBoxAtPlane) // If spanned (not entirely infront)
      partial |= (1 << i);
  }

  return partial;
}

bool vcSceneLayerRenderer_IsNodeVisible(vcSceneLayerRendererNode *pNode, const udDouble4 frustumPlanes[6])
{
  return -1 < vcSceneLayerRenderer_FrustumTest(frustumPlanes, pNode->minimumBoundingSphere.position, udDouble3::create(pNode->minimumBoundingSphere.radius));
}

double vcSceneLayerRenderer_CalculateScreenSize(vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  udDouble4 center = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position, 1.0);
  udDouble4 p1 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(pNode->minimumBoundingSphere.radius, 0.0, 0.0), 1.0);
  udDouble4 p2 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(0.0, pNode->minimumBoundingSphere.radius, 0.0), 1.0);
  center /= center.w;
  p1 /= p1.w;
  p2 /= p2.w;

  // TODO: Verify this `2.0` is correct...I'm 75% sure it is, but changing it to 1.0 does cause a very slight noticeable LOD pop, but is that meant to happen?
  double r1 = udMag(center.toVector3() - p1.toVector3()) * 2.0;
  double r2 = udMag(center.toVector3() - p2.toVector3()) * 2.0;
  udDouble2 screenSize = udDouble2::create(screenResolution.x * r1, screenResolution.y * r2);
  return udMax(screenSize.x, screenSize.y);
}

void vcSceneLayerRenderer_RenderNode(vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix)
{
  //printf("Drawing node %s\n", pNode->id);

  for (size_t geometry = 0; geometry < pNode->geometryDataCount; ++geometry)
  {
    vcTexture *pDrawTexture = nullptr;
    if (pNode->textureDataCount > 0)
      pDrawTexture = pNode->pTextureData[0].pTexture; // TODO: pretty sure i need to read the material for this...
    vcPolygonModel_Render(pNode->pGeometryData[geometry].pModel, pNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, pDrawTexture);

    // TODO: (EVC-542) other geometry types
  }
}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udDouble4 frustumPlanes[6], const udUInt2 &screenResolution)
{
  if (!vcSceneLayerRenderer_IsNodeVisible(pNode, frustumPlanes))
    return true; // consume, but do nothing

  if (pNode->loadState <= vcSceneLayerRendererNode::vcLS_Loading)
  {
    if (pNode->loadState == vcSceneLayerRendererNode::vcLS_NotLoaded)
    {
      pNode->loadState = vcSceneLayerRendererNode::vcLS_Loading;

      // TODO: Consider loading nodes based on distance (sound familiar?)
      printf("Queuing node: %s\n", pNode->id);
      vcLoadNodeJobData *pLoadData = udAllocType(vcLoadNodeJobData, 1, udAF_Zero);
      pLoadData->pSceneLayer = pSceneLayer;
      pLoadData->pNode = pNode;
      vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayerRenderer_AsyncLoadNode, pLoadData);
    }

    return false;
  }

  double nodeScreenSize = vcSceneLayerRenderer_CalculateScreenSize(pNode, viewProjectionMatrix, screenResolution);
  if (pNode->childrenCount == 0 || nodeScreenSize < pNode->loadSelectionValue)
  {
    if (pNode->loadState == vcSceneLayerRendererNode::vcLS_InMemory && pSceneLayer->uploadsThisFrame < vcMaxUploadsPerFrame)
      vcSceneLayerRenderer_UploadNodeData(pSceneLayer, pNode);

    vcSceneLayerRenderer_RenderNode(pNode, viewProjectionMatrix);
    return true;
  }

  bool childrenAllRendered = true;
  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    vcSceneLayerRendererNode *pChildNode = &pNode->pChildren[i];
    childrenAllRendered = vcSceneLayerRenderer_RecursiveRender(pSceneLayer, pChildNode, viewProjectionMatrix, frustumPlanes, screenResolution) && childrenAllRendered;
  }

  // If children are not loaded in yet, render this LOD
  // TODO this is broken
  if (!childrenAllRendered)
    vcSceneLayerRenderer_RenderNode(pNode, viewProjectionMatrix);

  return true;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  if (pSceneLayer == nullptr)
    return false;

  pSceneLayer->uploadsThisFrame = 0;

  // extract frustum planes
  udDouble4x4 transposedViewProjection = udTranspose(viewProjectionMatrix);
  udDouble4 frustumPlanes[6];
  frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    frustumPlanes[j] /= udMag3(frustumPlanes[j]);

  return vcSceneLayerRenderer_RecursiveRender(pSceneLayer, &pSceneLayer->root, viewProjectionMatrix, frustumPlanes, screenResolution);
}
