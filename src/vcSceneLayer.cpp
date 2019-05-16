#include "vcSceneLayer_Internal.h"

#include "stb_image.h"

#include "vcSceneLayerHelper.h"
#include "vcPolygonModel.h"

#include "vCore/vWorkerThread.h"

#include "gl/vcTexture.h"
#include "gl/vcGLState.h"

#include "udPlatform/udFile.h"

static const char *pSceneLayerInfoFilename = "3dSceneLayer.json";
static const char *pNodeInfoFilename = "3dNodeIndexDocument.json";


struct vcSceneLayer_LoadNodeJobData
{
  vcSceneLayer *pSceneLayer;
  vcSceneLayerNode *pNode;
  vcSceneLayerNodeLoadOptions loadOptions;
};

udResult vcSceneLayer_LoadNodeData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);

void vcSceneLayer_LoadNodeJob(void *pData)
{
  vcSceneLayer_LoadNodeJobData *pLoadData = (vcSceneLayer_LoadNodeJobData*)pData;

  if (!pLoadData->pSceneLayer->isActive)
  {
    pLoadData->pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    return;
  }

  // TODO: (JIRA 548) this will load the entirety of the nodes data - which could be unnecessary?
  vcSceneLayer_LoadNode(pLoadData->pSceneLayer, pLoadData->pNode);
}

void vcSceneLayer_LoadNodeInternalsJob(void *pData)
{
  vcSceneLayer_LoadNodeJobData *pLoadData = (vcSceneLayer_LoadNodeJobData*)pData;

  if (!pLoadData->pSceneLayer->isActive)
  {
    pLoadData->pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    return;
  }

  // TODO: (JIRA 548) this will load the entirety of the nodes data - which could be unnecessary?
  vcSceneLayer_LoadNodeData(pLoadData->pSceneLayer, pLoadData->pNode);
}

// TODO: (EVC-549) This should be in udPlatform
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

void vcSceneLayer_RecursiveDestroyNode(vcSceneLayerNode *pNode)
{
  while (pNode->loadState == vcSceneLayerNode::vcLS_InQueue || pNode->loadState == vcSceneLayerNode::vcLS_Loading)
    udYield();

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
    vcSceneLayer_RecursiveDestroyNode(&pNode->pChildren[i]);

  udFree(pNode->pChildren);
  udFree(pNode->pURL);
}

udResult vcSceneLayer_UploadDataToGPUIfPossible(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, bool force = false)
{
  udResult result;
  bool uploadsCompleted = true;

  if (!force && !vcGLState_IsGPUDataUploadAllowed())
    return udR_Success;

  // geometry
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    if (pNode->pGeometryData[i].loaded)
      continue;

    // TODO: (EVC-553) Let `vcPolygonModel_CreateFromData()` handle this
    if (!force && !vcGLState_IsGPUDataUploadAllowed()) // allow partial uploads
    {
      uploadsCompleted = false;
      break;
    }

    UD_ERROR_IF(pNode->pGeometryData[i].vertCount > UINT16_MAX, udR_Failure_);
    UD_ERROR_CHECK(vcPolygonModel_CreateFromData(&pNode->pGeometryData[i].pModel, pNode->pGeometryData[i].pData, (uint16_t)pNode->pGeometryData[i].vertCount, pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount));
    udFree(pNode->pGeometryData[i].pData);

    pNode->pGeometryData[i].loaded = true;
    vcGLState_GPUDidWork(0, 0, (pNode->pGeometryData[i].vertexStride * pNode->pGeometryData[i].vertCount));
  }

  // textures
  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    if (pNode->pTextureData[i].loaded || pNode->pTextureData[i].pData == nullptr)
      continue;

    // TODO: (EVC-553) Let `vcTexture_Create()` handle this
    if (!force && !vcGLState_IsGPUDataUploadAllowed()) // allow partial uploads
    {
      uploadsCompleted = false;
      break;
    }

    UD_ERROR_CHECK(vcTexture_Create(&pNode->pTextureData[i].pTexture, pNode->pTextureData[i].width, pNode->pTextureData[i].height, pNode->pTextureData[i].pData, vcTextureFormat_RGBA8, vcTFM_Linear, false));
    udFree(pNode->pTextureData[i].pData);

    pNode->pTextureData[i].loaded = true;
    vcGLState_GPUDidWork(0, 0, (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4));
  }

  if (uploadsCompleted)
    pNode->loadState = vcSceneLayerNode::vcLS_Success;

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayer_LoadFeatureData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
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
    pNode->pFeatureData[i].position.z = featuresJSON.Get("featureData[%zu].position[2]", i).AsDouble(0.0); // may not exist
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
udResult vcSceneLayer_LoadGeometryData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  uint64_t featureCount = 0;
  uint64_t faceCount = 0;
  char *pCurrentFile = nullptr;
  uint32_t vertexOffset = 0;
  uint32_t attributeSize = 0;
  size_t headerElementCount = 0;
  udFloat3 vertI3S = {};
  udDouble3 pointCartesian = {};
  udDouble3 originCartesian = udDouble3::create(DBL_MAX, DBL_MAX, DBL_MAX);
  udFloat3 finalVertPosition = {};
  const char *pPropertyName = nullptr;
  const char *pTypeName = nullptr;

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "%s/%s.bin", pNode->pURL, pNode->pGeometryData[i].pURL);
    UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));

    pCurrentFile = pFileData;

    /// Header
    // TODO: (EVC-548) Calculate once on node load... not per geometry
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
    // TODO: (EVC-542)
    pNode->pGeometryData[i].vertexStride = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);
    UD_ERROR_IF(pNode->pGeometryData[i].vertCount * pNode->pGeometryData[i].vertexStride == 0, udR_ParseError);
    pNode->pGeometryData[i].pData = udAllocType(uint8_t, pNode->pGeometryData[i].vertCount * pNode->pGeometryData[i].vertexStride, udAF_Zero);

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

          memcpy(pNode->pGeometryData[i].pData + vertexOffset + (v * pNode->pGeometryData[i].vertexStride), &finalVertPosition, attributeSize);
          pCurrentFile += attributeSize;
        }

        pNode->pGeometryData[i].originMatrix.axis.t.z += pNode->minimumBoundingSphere.position.z;
      }
      else
      {
        for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
        {
          memcpy(pNode->pGeometryData[i].pData + vertexOffset + (v * pNode->pGeometryData[i].vertexStride), pCurrentFile, attributeSize);
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
udResult vcSceneLayer_LoadTextureData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result = udR_Success;
  const char *pPathBuffer = nullptr;
  uint8_t *pPixelData = nullptr;
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

      pPixelData = (uint8_t*)stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      pNode->pTextureData[i].width = width;
      pNode->pTextureData[i].height = height;
      pNode->pTextureData[i].pData = (uint8_t*)udMemDup(pPixelData, sizeof(uint32_t)*width*height, 0, udAF_None);

      udFree(pFileData);
      stbi_image_free(pPixelData);
      break;
    }
  }

  //epilogue:
  udFree(pPathBuffer);
  return result;
}

template<typename T>
udResult vcSceneLayer_AllocateNodeData(T **ppData, size_t *pDataCount, const udJSON &nodeJSON, const char *pKey)
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

void vcSceneLayer_CalculateNodeBounds(vcSceneLayerNode *pNode, const udDouble4 &mbsLatLongHeightRadius)
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
}

udResult vcSceneLayer_LoadNodeData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result;

  UD_ERROR_NULL(pNode, udR_InvalidParameter_);

  UD_ERROR_CHECK(vcSceneLayer_LoadFeatureData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayer_LoadGeometryData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayer_LoadTextureData(pSceneLayer, pNode));

  pNode->loadState = vcSceneLayerNode::vcLS_InMemory;
  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayer_RecursiveLoadNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, const vcSceneLayerNodeLoadOptions &options)
{
  udResult result;
  udJSON nodeJSON;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  int64_t fileLen = 0;

  pNode->loadState = vcSceneLayerNode::vcLS_Loading;

  // Load the nodes info
  udSprintf(&pPathBuffer, "%s/%s", pNode->pURL, pNodeInfoFilename);
  UD_ERROR_CHECK(udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  // potentially already calculated some node info (it's stored in this nodes parents 'children' array)
  if (pNode->id[0] == '\0')
  {
    udStrcpy(pNode->id, sizeof(pNode->id), nodeJSON.Get("id").AsString());
    vcSceneLayer_CalculateNodeBounds(pNode, nodeJSON.Get("mbs").AsDouble4());
  }

  pNode->level = nodeJSON.Get("level").AsInt();
  pNode->lodSelectionValue = nodeJSON.Get("lodSelection[0].maxError").AsDouble(); // TODO: I've made an assumptions here (e.g. always has at least one lod metric)

  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, "featureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, "textureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, "attributeData"));

  // Hierarchy
  pNode->childrenCount = nodeJSON.Get("children").ArrayLength();
  if (pNode->childrenCount > 0)
  {
    pNode->pChildren = udAllocType(vcSceneLayerNode, pNode->childrenCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pChildren, udR_MemoryAllocationFailure);

    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      vcSceneLayerNode *pChildNode = &pNode->pChildren[i];
      pChildNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;

      udStrcpy(pChildNode->id, sizeof(pChildNode->id), nodeJSON.Get("children[%zu].id", i).AsString());
      vcNormalizePath(&pChildNode->pURL, pNode->pURL, nodeJSON.Get("children[%zu].href", i).AsString());

      vcSceneLayer_CalculateNodeBounds(pChildNode, nodeJSON.Get("children[%zu].mbs", i).AsDouble4());
    }
  }

  // TODO: (EVC-548) - these may actually not be needed
  // E.g. During convert, where we're trying to just get the leaf nodes...
  // for non-leaf nodes none of these are actually needed!
  // E.g. For rendering, maybe we want to skip over a few levels
  // (options & vsSLNLO_ShallowLoad == 0) &&
  if (((options & vsSLNLO_CompleteNodeLoad) || ((options & vsSLNLO_OnlyLoadLeaves) == 0 || pNode->childrenCount == 0)))
  {
    UD_ERROR_CHECK(vcSceneLayer_LoadNodeData(pSceneLayer, pNode));

    if (options & vsSLNLO_CompleteNodeLoad)
      UD_ERROR_CHECK(vcSceneLayer_UploadDataToGPUIfPossible(pSceneLayer, pNode, true));
  }
  else
  {
    pNode->loadState = vcSceneLayerNode::vcLS_PartialLoad;
  }

  if (options & vsSLNLO_RecursiveLoad)
  {
    for (size_t i = 0; i < pNode->childrenCount; ++i)
      UD_ERROR_CHECK(vcSceneLayer_RecursiveLoadNode(pSceneLayer, &pNode->pChildren[i], options));
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerNode::vcLS_Failed;

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayer_LoadNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode /* = nullptr*/, const vcSceneLayerNodeLoadOptions &options /* = vcSLNLO_None*/)
{
  udResult result;

  if (pNode != nullptr)
    UD_ERROR_SET(vcSceneLayer_RecursiveLoadNode(pSceneLayer, pNode, options));

  // Load the entire model
  for (size_t i = 0; i < pSceneLayer->root.childrenCount; ++i)
    UD_ERROR_CHECK(vcSceneLayer_RecursiveLoadNode(pSceneLayer, &pSceneLayer->root.pChildren[i], options));

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayer_Create(vcSceneLayer **ppSceneLayer, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayer *pSceneLayer = nullptr;
  const char *pPathBuffer = nullptr;
  const char *pFileData = nullptr;
  int64_t fileLen = 0;
  const char *pAttributeName = nullptr;

  pSceneLayer = udAllocType(vcSceneLayer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer, udR_MemoryAllocationFailure);

  pSceneLayer->isActive = true;
  pSceneLayer->pThreadPool = pWorkerThreadPool;
  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform change
  udSprintf(&pPathBuffer, "%s/%s", pSceneLayer->sceneLayerURL, pSceneLayerInfoFilename);
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
  // TODO: (EVC-548) Does the root actually need to be loaded here?
  vcNormalizePath(&pSceneLayer->root.pURL, pSceneLayer->sceneLayerURL, pSceneLayer->description.Get("store.rootNode").AsString());
  UD_ERROR_CHECK(vcSceneLayer_LoadNode(pSceneLayer, &pSceneLayer->root));

  *ppSceneLayer = pSceneLayer;
  result = udR_Success;

epilogue:
  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayer_Destroy(vcSceneLayer **ppSceneLayer)
{
  udResult result;
  vcSceneLayer *pSceneLayer = nullptr;

  UD_ERROR_NULL(ppSceneLayer, udR_InvalidParameter_);
  UD_ERROR_NULL(*ppSceneLayer, udR_InvalidParameter_);

  pSceneLayer = *ppSceneLayer;
  *ppSceneLayer = nullptr;

  pSceneLayer->isActive = false;
  vcSceneLayer_RecursiveDestroyNode(&pSceneLayer->root);
  pSceneLayer->description.Destroy();

  udFree(pSceneLayer->pDefaultGeometryLayout);
  udFree(pSceneLayer);
  result = udR_Success;

epilogue:
  return result;
}

bool vcSceneLayer_TouchNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  if (pNode->loadState == vcSceneLayerNode::vcLS_NotLoaded)
  {
    pNode->loadState = vcSceneLayerNode::vcLS_InQueue;

    // TODO: (EVC-548) Consider loading nodes based on distance (sound familiar?)
    vcSceneLayer_LoadNodeJobData *pLoadData = udAllocType(vcSceneLayer_LoadNodeJobData, 1, udAF_Zero);
    pLoadData->pSceneLayer = pSceneLayer;
    pLoadData->pNode = pNode;
    // TODO: (EVC-548)
    //pLoadData->loadOptions = needsDrawing ? vcSLNLO_None : vsSLNLO_ShallowLoad;
    vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeJob, pLoadData);
  }
  else if (pNode->loadState == vcSceneLayerNode::vcLS_PartialLoad)
  {
    // TODO: (EVC-569) Partial node uploading has not been fully tested!
    vcSceneLayer_LoadNodeJobData *pLoadData = udAllocType(vcSceneLayer_LoadNodeJobData, 1, udAF_Zero);
    pLoadData->pSceneLayer = pSceneLayer;
    pLoadData->pNode = pNode;
    pLoadData->loadOptions = vcSLNLO_None;
    vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeInternalsJob, pLoadData);
  }

  if (pNode->loadState == vcSceneLayerNode::vcLS_InMemory)
    vcSceneLayer_UploadDataToGPUIfPossible(pSceneLayer, pNode);

  return pNode->loadState == vcSceneLayerNode::vcLS_Success;
}

bool vcSceneLayer_IsNodeMetadataLoaded(vcSceneLayerNode *pNode)
{
  return pNode->loadState == vcSceneLayerNode::vcLS_PartialLoad || pNode->loadState == vcSceneLayerNode::vcLS_InMemory || pNode->loadState == vcSceneLayerNode::vcLS_Success;
}


vcSceneLayerVertex* vcSceneLayer_GetVertex(vcSceneLayerNode::GeometryData *pGeometry, uint64_t index)
{
  if (pGeometry == nullptr || pGeometry->pData == nullptr || index >= pGeometry->vertCount)
    return nullptr;

  // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
  return (vcSceneLayerVertex*)&pGeometry->pData[index * pGeometry->vertexStride];
}
