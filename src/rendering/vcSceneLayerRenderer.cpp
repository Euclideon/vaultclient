#include "vcSceneLayerRenderer.h"

#include <vector>

#include "stb_image.h"

#include "vcSceneLayerHelper.h"
#include "vcSettings.h"
#include "vcPolygonModel.h"

#include "gl/vcTexture.h"
#include "gl/vcGLState.h"

#include "udPlatform/udJSON.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udGeoZone.h"
#include "vCore/vWorkerThread.h"

static const char *pSceneLayerInfoFile = "3dSceneLayer.json";
static const char *pNodeInfoFile = "3dNodeIndexDocument.json";

enum
{
  // TODO: (EVC-541) Determine actual max URL length of I3S
  vcMaxURLLength = vcMaxPathLength,

  // TODO: (EVC-547) This is arbitrary
  vcMaxRealtimeUploadsBytesPerFrame = 30 * 1024, // bytes gpu
};

struct vcSceneLayerRendererNode
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
  char id[vcMaxPathLength]; // TODO: How big should this be?
  const char *pURL;
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

struct vcSceneLayerRenderer
{
  vcSceneLayerRendererMode mode;

  const vcSettings *pSettings;
  vWorkerThreadPool *pThreadPool;
  size_t maxUploadsPerFrameBytes;

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

udResult vcSceneLayerRenderer_LoadNodeData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode);

void vcSceneLayerRenderer_AsyncLoadNode(void *pSceneLayerPtr)
{
  vcLoadNodeJobData *pLoadData = (vcLoadNodeJobData*)pSceneLayerPtr;

  // Note: this will load the entirety of the nodes data - which could be unnecessary?
  vcSceneLayerRenderer_LoadNodeData(pLoadData->pSceneLayer, pLoadData->pNode);
}

// TODO: (EVC-549) This should be removed from here and put somewhere public
void vcNormalizePath(const char **ppDest, const char *pRoot, const char *pAppend)
{
  const char *pRemoveCharacters = "\\";

  char *pNewAppend = udStrdup(pAppend);
  size_t len = udStrlen(pNewAppend);
  size_t index = 0;
  while (udStrchr(pNewAppend, pRemoveCharacters, &index) != nullptr)
  {
    memcpy(pNewAppend + index, pNewAppend + (index + 1), (len - index));
    len -= 1;
  }

  // Remove leading `./`
  if (udStrlen(pNewAppend) >= 2 && pNewAppend[0] == '.' && pNewAppend[1] == '/')
  {
    len -= 2;
    memcpy(pNewAppend, pNewAppend + 2, len);
  }

  // TODO: (EVC-541) What else?

  pNewAppend[len] = '\0';
  if (pRoot)
    udSprintf(ppDest, "%s/%s", pRoot, pNewAppend);
  else
    udSprintf(ppDest, "%s", pNewAppend);
  udFree(pNewAppend);
}

void vcSceneLayerRenderer_RecursiveDestroyNode(vcSceneLayerRendererNode *pNode)
{

  for (size_t i = 0; i < pNode->sharedResourceCount; ++i)
  {
    udFree(pNode->pSharedResources[i].pURL);
  }

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udFree(pNode->pFeatureData[i].pURL);
  }

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udFree(pNode->pGeometryData[i].pURL);

    vcPolygonModel_Destroy(&pNode->pGeometryData[i].pModel);
    udFree(pNode->pGeometryData[i].pData);
  }

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    udFree(pNode->pTextureData[i].pURL);

    vcTexture_Destroy(&pNode->pTextureData[i].pTexture);
    udFree(pNode->pTextureData[i].pData);
  }

  for (size_t i = 0; i < pNode->attributeDataCount; ++i)
  {
    udFree(pNode->pAttributeData[i].pURL);
  }

  udFree(pNode->pSharedResources);
  udFree(pNode->pFeatureData);
  udFree(pNode->pGeometryData);
  udFree(pNode->pTextureData);
  udFree(pNode->pAttributeData);

  for (size_t i = 0; i < pNode->childrenCount; ++i)
    vcSceneLayerRenderer_RecursiveDestroyNode(&pNode->pChildren[i]);

  udFree(pNode->pChildren);
  udFree(pNode->pURL);
}

udResult vcSceneLayerRenderer_LoadNodeFeatureData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  udJSON featuresJSON;

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "%s/%s.json", pNode->pURL, pNode->pFeatureData[i].pURL);
    UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));
    UD_ERROR_CHECK(featuresJSON.Parse(pFileData));

    // TODO: JIRA task add a udJSON.AsDouble2()
    // Note: 'position' is either (x/y/z) OR just (x/y).
    pNode->pFeatureData[i].position.x = featuresJSON.Get("featureData[%zu].position[0]", i).AsDouble();
    pNode->pFeatureData[i].position.y = featuresJSON.Get("featureData[%zu].position[1]", i).AsDouble();
    pNode->pFeatureData[i].position.z = featuresJSON.Get("featureData[%zu].position[2]", i).AsDouble(); // may not exist
    pNode->pFeatureData[i].pivotOffset = featuresJSON.Get("featureData[%zu].pivotOffset", i).AsDouble3();
    pNode->pFeatureData[i].minimumBoundingBox = featuresJSON.Get("featureData[%zu].mbb", i).AsDouble4();

    // TODO: (EVC-542) Where does this go?
    //pNode->geometries.transformation = featuresJSON.Get("geometryData[0].transformation").AsDouble4x4();

    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

// Assumed the nodes features have already been loaded
udResult vcSceneLayerRenderer_LoadNodeGeometryData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udResult result;
  const char *pPathBuffer = nullptr;
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
    udSprintf(&pPathBuffer, "%s/%s.bin", pNode->pURL, pNode->pGeometryData[i].pURL);
    UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));

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

    pNode->pGeometryData[i].pData = udAllocType(uint8_t, pNode->pGeometryData[i].vertCount * vertexSize, udAF_Zero);

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

          memcpy(pNode->pGeometryData[i].pData + vertexOffset + (v * vertexSize), &finalVertPosition, attributeSize);
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
          memcpy(pNode->pGeometryData[i].pData + vertexOffset + (v * vertexSize), pCurrentFile, attributeSize);
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
  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

// Assumed nodes features has already been loaded
udResult vcSceneLayerRenderer_LoadNodeTextureData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result = udR_Success;
  const char *pPathBuffer = nullptr;
  uint8_t *pData = nullptr;
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
      udSprintf(&pPathBuffer, "%s/%s.%s", pNode->pURL, pNode->pTextureData[i].pURL, pExtensions[f]);
      if (udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen) != udR_Success)
        continue;

      pData = (uint8_t*)stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      pNode->pTextureData[i].width = width;
      pNode->pTextureData[i].height = height;
      pNode->pTextureData[i].pData = (uint8_t*)udMemDup(pData, sizeof(uint32_t)*width*height, 0, udAF_None);

      udFree(pFileData);
      stbi_image_free(pData);
      break;
    }
  }

//epilogue:
  udFree(pPathBuffer);
  return result;
}

void vcSceneLayerRenderer_UploadNodeDataGPUIfPossible(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, size_t maxUploadsAllowed)
{
  if (pSceneLayer->maxUploadsPerFrameBytes >= maxUploadsAllowed)
    return;

  bool uploadsCompleted = true;
  size_t vertexSize = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);

  // TODO: handle failures
  // geometry
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    if (pNode->pGeometryData[i].loaded)
      continue;

    if (pSceneLayer->maxUploadsPerFrameBytes >= maxUploadsAllowed) // allowing partial uploads
    {
      uploadsCompleted = false;
      break;
    }

    //printf("Uploading node GEOMETRY data: %s (%zu kb)\n", pNode->id, (vertexSize * pNode->pGeometryData[i].vertCount) / 1024);

    //UD_ERROR_IF(vertCount > UINT16_MAX, udR_Failure_);
    //UD_ERROR_CHECK()
    vcPolygonModel_CreateFromData(&pNode->pGeometryData[i].pModel, pNode->pGeometryData[i].pData, (uint16_t)pNode->pGeometryData[i].vertCount, pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);
    udFree(pNode->pGeometryData[i].pData);

    pNode->pGeometryData[i].loaded = true;
    pSceneLayer->maxUploadsPerFrameBytes += (vertexSize * pNode->pGeometryData[i].vertCount);
  }

  // textures
  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    if (pNode->pTextureData[i].loaded)
      continue;

    if (pSceneLayer->maxUploadsPerFrameBytes >= maxUploadsAllowed) // allowing partial uploads
    {
      uploadsCompleted = false;
      break;
    }

    //printf("Uploading node TEXTURE data: %s (%zu kb)\n", pNode->id, (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4) / 1024);

    vcTexture_Create(&pNode->pTextureData[i].pTexture, pNode->pTextureData[i].width, pNode->pTextureData[i].height, pNode->pTextureData[i].pData, vcTextureFormat_RGBA8, vcTFM_Linear, false);
    udFree(pNode->pTextureData[i].pData);

    pNode->pTextureData[i].loaded = true;
    pSceneLayer->maxUploadsPerFrameBytes += (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4);
  }

  if (uploadsCompleted)
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
    vcNormalizePath(&pData[i].pURL, nullptr, nodeJSON.Get("%s[%zu].href", pKey, i).AsString());

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
 // printf("Origin: %f, %f\n", pNode->minimumBoundingSphere.position.x, pNode->minimumBoundingSphere.position.y);
 // printf("SRID: %d\n", sridCode);
}

udResult vcSceneLayerRenderer_LoadNodeData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udResult result;
  udJSON nodeJSON;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  vcSceneLayerRendererNode *pChildNode = nullptr;

  pNode->loadState = vcSceneLayerRendererNode::vcLS_Loading;

  // Load the nodes info
  udSprintf(&pPathBuffer, "%s/%s", pNode->pURL, pNodeInfoFile);
  UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  // potentially already calculated some node info (it's stored in this nodes parents 'children' array)
  if (pNode->id[0] == '\0')
  {
    udStrcpy(pNode->id, sizeof(pNode->id), nodeJSON.Get("id").AsString());
    vcSceneLayerRenderer_CalculateNodeBounds(pNode, nodeJSON.Get("mbs").AsDouble4());
  }

  pNode->level = nodeJSON.Get("level").AsInt();
  pNode->loadSelectionValue = nodeJSON.Get("lodSelection[0].maxError").AsDouble(); // TODO: I've made an assumptions here (e.g. always has at least one lod metric)

  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, "featureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, "textureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, "attributeData"));

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
      vcNormalizePath(&pChildNode->pURL, pNode->pURL, nodeJSON.Get("children[%zu].href", i).AsString());

      vcSceneLayerRenderer_CalculateNodeBounds(pChildNode, nodeJSON.Get("children[%zu].mbs", i).AsDouble4());
    }
  }

  // TODO: (EVC-548) - these may actually not be needed yet during realtime rendering
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeFeatureData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeGeometryData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeTextureData(pSceneLayer, pNode));

  pNode->loadState = vcSceneLayerRendererNode::vcLS_InMemory;

  if (pSceneLayer->mode == vcSLRM_Convert)
  {
    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeData(pSceneLayer, &pNode->pChildren[i]));
    }
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerRendererNode::vcLS_Failed;

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, const vcSettings *pSettings, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL, const vcSceneLayerRendererMode &mode /* = vcSLRM_Rendering */)
{
  udResult result;
  vcSceneLayerRenderer *pSceneLayer = nullptr;
  const char *pPathBuffer = nullptr;
  const char *pFileData = nullptr;
  int64_t fileLen = 0;
  const char *pAttributeName = nullptr;

  pSceneLayer = udAllocType(vcSceneLayerRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer, udR_MemoryAllocationFailure);

  pSceneLayer->mode = mode;
  pSceneLayer->pSettings = pSettings;
  pSceneLayer->pThreadPool = pWorkerThreadPool;
  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform change
  udSprintf(&pPathBuffer, "%s/%s", pSceneLayer->sceneLayerURL, pSceneLayerInfoFile);
  UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));
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

  // Load the root
  vcNormalizePath(&pSceneLayer->root.pURL, pSceneLayer->sceneLayerURL, pSceneLayer->description.Get("store.rootNode").AsString());
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeData(pSceneLayer, &pSceneLayer->root));

  *ppSceneLayer = pSceneLayer;
  result = udR_Success;

epilogue:
  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayer)
{
  udResult result;
  vcSceneLayerRenderer *pSceneLayer = nullptr;

  UD_ERROR_NULL(ppSceneLayer, udR_InvalidParameter_);
  UD_ERROR_NULL(*ppSceneLayer, udR_InvalidParameter_);

  pSceneLayer = *ppSceneLayer;
  *ppSceneLayer = nullptr;

  vcSceneLayerRenderer_RecursiveDestroyNode(&pSceneLayer->root);

  pSceneLayer->description.Destroy();

  udFree(pSceneLayer->pDefaultGeometryLayout);
  udFree(pSceneLayer);
  result = udR_Success;

epilogue:
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

  // TODO: (EVC-548) Verify this `2.0` is correct (because I only calculate the half size above)
  // I'm 75% sure it is, but changing it to 1.0 does cause a very slight noticeable LOD pop, but is that meant to happen?
  double r1 = udMag(center.toVector3() - p1.toVector3()) * 2.0;
  double r2 = udMag(center.toVector3() - p2.toVector3()) * 2.0;
  udDouble2 screenSize = udDouble2::create(screenResolution.x * r1, screenResolution.y * r2);
  return udMax(screenSize.x, screenSize.y);
}

bool vcSceneLayerRenderer_HandleNodeLoadState(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  if (pNode->loadState <= vcSceneLayerRendererNode::vcLS_Loading)
  {
    if (pNode->loadState == vcSceneLayerRendererNode::vcLS_NotLoaded)
    {
      pNode->loadState = vcSceneLayerRendererNode::vcLS_Loading;

      // TODO: (EVC-548) Consider loading nodes based on distance (sound familiar?)
      //printf("Queuing node: %s\n", pNode->id);
      vcLoadNodeJobData *pLoadData = udAllocType(vcLoadNodeJobData, 1, udAF_Zero);
      pLoadData->pSceneLayer = pSceneLayer;
      pLoadData->pNode = pNode;
      vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayerRenderer_AsyncLoadNode, pLoadData);
    }
  }
  else if (pNode->loadState == vcSceneLayerRendererNode::vcLS_InMemory)
  {
    vcSceneLayerRenderer_UploadNodeDataGPUIfPossible(pSceneLayer, pNode, vcMaxRealtimeUploadsBytesPerFrame);
  }

  return (pNode->loadState == vcSceneLayerRendererNode::vcLS_Success);
}

void vcSceneLayerRenderer_RenderNode(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix)
{
  //printf("Drawing node %s\n", pNode->id);
  udUnused(pSceneLayer);

  for (size_t geometry = 0; geometry < pNode->geometryDataCount; ++geometry)
  {
    if (!pNode->pGeometryData[geometry].loaded)
      continue;

    vcTexture *pDrawTexture = nullptr;
    if (pNode->textureDataCount > 0 && pNode->pTextureData[0].loaded)
      pDrawTexture = pNode->pTextureData[0].pTexture; // TODO: pretty sure i need to read the material for this...

    vcPolygonModel_Render(pNode->pGeometryData[geometry].pModel, pNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, pDrawTexture);

    // TODO: (EVC-542) other geometry types
  }
}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udDouble4 frustumPlanes[6], const udUInt2 &screenResolution)
{
  if (!vcSceneLayerRenderer_IsNodeVisible(pNode, frustumPlanes))
    return true; // consume, but do nothing

  if (!vcSceneLayerRenderer_HandleNodeLoadState(pSceneLayer, pNode))
    return false;

  double nodeScreenSize = vcSceneLayerRenderer_CalculateScreenSize(pNode, viewProjectionMatrix, screenResolution);
  if (pNode->childrenCount == 0 || nodeScreenSize < pNode->loadSelectionValue)
  {
    vcSceneLayerRenderer_RenderNode(pSceneLayer, pNode, viewProjectionMatrix);
    return true;
  }

  bool childrenAllRendered = true;
  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    vcSceneLayerRendererNode *pChildNode = &pNode->pChildren[i];
    childrenAllRendered = vcSceneLayerRenderer_RecursiveRender(pSceneLayer, pChildNode, viewProjectionMatrix, frustumPlanes, screenResolution) && childrenAllRendered;
  }

  // TODO: (EVC-548)
  // If children are not loaded in yet, render this LOD.
  // At the moment this can cause incorrect parent occluding child
  if (!childrenAllRendered)
    vcSceneLayerRenderer_RenderNode(pSceneLayer, pNode, viewProjectionMatrix);

  return true;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  if (pSceneLayer == nullptr || (pSceneLayer->mode != vcSLRM_Rendering))
    return false;

  pSceneLayer->maxUploadsPerFrameBytes = 0;

  // TODO: (EVC-548) This is duplicated work across i3s models
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
