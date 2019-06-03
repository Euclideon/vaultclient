#include "vcSceneLayer_Internal.h"

#include "stb_image.h"

#include "vcSceneLayerHelper.h"
#include "vcPolygonModel.h"

#include "vCore/vWorkerThread.h"

#include "gl/vcTexture.h"
#include "gl/vcGLState.h"

#include "udFile.h"
#include "udStringUtil.h"

// TODO: (EVC-553) This is temporary
uint64_t gpuBytesUploadedThisFrame = 0;

struct vcSceneLayer_LoadNodeJobData
{
  vcSceneLayer *pSceneLayer;
  vcSceneLayerNode *pNode;
  vcSceneLayerLoadType loadType;
};

udResult vcSceneLayer_LoadNodeInternals(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);

void vcSceneLayer_LoadNodeJob(void *pData)
{
  vcSceneLayer_LoadNodeJobData *pLoadData = (vcSceneLayer_LoadNodeJobData*)pData;

  if (!pLoadData->pSceneLayer->isActive)
  {
    pLoadData->pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    return;
  }

  // TODO: (JIRA 548) this will load the entirety of the nodes data - which could be unnecessary?
  vcSceneLayer_LoadNode(pLoadData->pSceneLayer, pLoadData->pNode, vcSLLT_Rendering);
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
  vcSceneLayer_LoadNodeInternals(pLoadData->pSceneLayer, pLoadData->pNode);
}

// TODO: (EVC-549) This should be in udCore
void vcNormalizePath(const char **ppDest, const char *pRoot, const char *pAppend, char separator)
{
  char *pNewRoot = nullptr;
  char *pNewAppend = udStrdup(pAppend);
  size_t len = udStrlen(pNewAppend);
  size_t index = 0;

  // TODO: why are there '\\/' separator characters??
  // Remove any '\\/' characters
  while (udStrstr(pNewAppend, len, "\\/", &index) != nullptr)
  {
    memcpy(pNewAppend + index + 1, pNewAppend + (index + 1 + 1), (len - index));
    len -= 1;
    pNewAppend[len] = '\0';
  }

  // Remove leading "./" or leading ".\\"
  if (udStrlen(pNewAppend) >= 2 && pNewAppend[0] == '.' && (pNewAppend[1] == '\\' || pNewAppend[1] == '/'))
  {
    len -= 2;
    memcpy(pNewAppend, pNewAppend + 2, len);
    pNewAppend[len] = '\0';
  }

  // resolve "../" or "..\\" prefix
  // TODO: This isn't good enough! (assumptions)
  if (len >= 3 && udStrBeginsWith(pNewAppend, udTempStr("..%c", separator)))
  {
    if (udStrrchr(pRoot, udTempStr("%c", separator), &index) != nullptr)
    {
      pNewRoot = udStrdup(pRoot);
      pNewRoot[index] = '\0';
      pRoot = pNewRoot;

      pNewAppend += 3; // remove the "../"
      len -= 3;
      pNewAppend[len] = '\0';
    }
  }

  // TODO: (EVC-541) What else?

  if (pRoot)
    udSprintf(ppDest, "%s%c%s", pRoot, separator, pNewAppend);
  else
    udSprintf(ppDest, "%s", pNewAppend);

  udFree(pNewRoot);
  udFree(pNewAppend);
}

udResult udFile_LoadGZIP(const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes = nullptr)
{
  udResult result;
  char *pBufferData = nullptr;
  int64_t rawFileLen = 0;
  char *pRawFileData = nullptr;
  size_t bufferLen = 0;

  UD_ERROR_CHECK(udFile_Load(pFilename, (void**)&pRawFileData, &rawFileLen));
  bufferLen = rawFileLen / 2;

  do
  {
    bufferLen = bufferLen * 2;
    pBufferData = (char*)udRealloc(pBufferData, bufferLen);
    UD_ERROR_NULL(pBufferData, udR_MemoryAllocationFailure);

    result = udCompression_Inflate(pBufferData, bufferLen, pRawFileData, rawFileLen, nullptr, udCT_GzipDeflate);
  } while (result == udR_BufferTooSmall);
  UD_ERROR_CHECK(result);

  *ppMemory = pBufferData;
  pBufferData = nullptr;
  if (pFileLengthInBytes)
    *pFileLengthInBytes = bufferLen;
  result = udR_Success;

epilogue:
  if (pBufferData != nullptr)
    udFree(pBufferData);

  udFree(pRawFileData);
  return result;
}

udResult vcSceneLayer_LoadFeatureData(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  udJSON featuresJSON;

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "zip://%s%c%s.json.gz", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pFeatureData[i].pURL);
    UD_ERROR_CHECK(udFile_LoadGZIP(pPathBuffer, (void**)&pFileData));
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

    /// Layout
    // TODO: I'm assuming that featureCount == geometryCount, and that feature X relates to geometry X
    //       This may be wrong, maybe the 'id's link them, instead of index
    const udJSON &vertexAttributesJSON = featuresJSON.Get("geometryData[%zu].params.vertexAttributes", i);

    pNode->pFeatureData[i].geometryLayoutCount = vertexAttributesJSON.MemberCount();
    UD_ERROR_IF(pNode->pFeatureData[i].geometryLayoutCount == 0, udR_ParseError);

    pNode->pFeatureData[i].pGeometryLayout = udAllocType(vcVertexLayoutTypes, pNode->pFeatureData[i].geometryLayoutCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pFeatureData[i].pGeometryLayout, udR_MemoryAllocationFailure);

    for (size_t a = 0; a < pNode->pFeatureData[i].geometryLayoutCount; ++a)
    {
      const udJSON *pVertexAttributeJSON = vertexAttributesJSON.GetMember(a);
      const char *pAttributeName = vertexAttributesJSON.GetMemberName(a);
      size_t attributeElementSize = vcSceneLayerHelper_GetSceneLayerTypeSize(pVertexAttributeJSON->Get("valueType").AsString());
      int attributeElementCount = pVertexAttributeJSON->Get("valuesPerElement").AsInt();
      size_t attributeSize = (attributeElementCount * attributeElementSize);

      if (udStrEqual(pAttributeName, "position"))
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Position3;
      else if (udStrEqual(pAttributeName, "normal"))
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Normal3;
      else if (udStrEqual(pAttributeName, "uv0"))
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_TextureCoords2;
      else if (udStrEqual(pAttributeName, "color"))
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_ColourBGRA;
      else
      {
        // the renderer will ignore anything else
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Unsupported;
      }

      // store in geometry data
      pNode->pGeometryData[i].vertexStride += attributeSize;
    }

    UD_ERROR_IF(pNode->pGeometryData[i].vertexStride == 0, udR_Unsupported);
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
  uint64_t featureCount = 0;
  uint64_t faceCount = 0;
  char *pCurrentFile = nullptr;
  uint32_t attributeOffset = 0;
  udFloat3 vertI3S = {};
  udFloat3 finalVertPosition = {};
  const char *pPropertyName = nullptr;
  const char *pTypeName = nullptr;
  int64_t fileLen = 0;

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "zip://%s%c%s.bin.gz", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pGeometryData[i].pURL);
    UD_ERROR_CHECK(udFile_LoadGZIP(pPathBuffer, (void**)&pFileData, &fileLen));

    pCurrentFile = pFileData;

    /// Header
    // TODO: (EVC-548) The header layout is consistent for all nodes, calculate once on node load... not per geometry
    size_t headerElementCount = pSceneLayer->description.Get("store.defaultGeometrySchema.header").ArrayLength();
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
        // UD_ERROR_SET(udR_ParseError);
      }
    }

    /// Geometry
    // TODO: (EVC-542)
    UD_ERROR_IF(pNode->pGeometryData[i].vertCount * pNode->pGeometryData[i].vertexStride == 0, udR_ParseError);
    pNode->pGeometryData[i].pData = udAllocType(uint8_t, pNode->pGeometryData[i].vertCount * pNode->pGeometryData[i].vertexStride, udAF_Zero);
    UD_ERROR_NULL(pNode->pGeometryData[i].pData, udR_MemoryAllocationFailure);

    // vertices are non-interleaved.
    // TODO: (EVC-542) Is that what "Topology: PerAttributeArray" signals?
    for (size_t attributeIndex = 0; attributeIndex < pNode->pFeatureData[i].geometryLayoutCount; ++attributeIndex)
    {
      uint32_t attributeSize = vcLayout_GetSize(pNode->pFeatureData[i].pGeometryLayout[attributeIndex]);
      if (pNode->pFeatureData[i].pGeometryLayout[attributeIndex] == vcVLT_Position3)
      {
        // positions require additional processing

        // Get the first position, and use that as an origin matrix (fixes floating point precision issue)
        // TODO: (EVC-540) Handle different sized positions
        memcpy(&vertI3S, pCurrentFile, sizeof(vertI3S));
        udDouble3 pointCartesian = udGeoZone_ToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + vertI3S.x, pNode->latLong.y + vertI3S.y, 0.0), true);
        udDouble3 originCartesian = udDouble3::create(pointCartesian.x, pointCartesian.y, vertI3S.z + pNode->pFeatureData[i].position.z);
        pNode->pGeometryData[i].originMatrix = udDouble4x4::translation(originCartesian);

        for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
        {
          memcpy(&vertI3S, pCurrentFile, sizeof(vertI3S));
          pointCartesian = udGeoZone_ToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + vertI3S.x, pNode->latLong.y + vertI3S.y, 0.0), true);
          pointCartesian.z = vertI3S.z; // Elevation (the z component of the vertex position) is specified in meters
          finalVertPosition = udFloat3::create(pointCartesian - originCartesian);

          memcpy(pNode->pGeometryData[i].pData + attributeOffset + (v * pNode->pGeometryData[i].vertexStride), &finalVertPosition, attributeSize);
          pCurrentFile += attributeSize;
        }

        pNode->pGeometryData[i].originMatrix.axis.t.z += pNode->minimumBoundingSphere.position.z;
      }
      else
      {
        for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
        {
          memcpy(pNode->pGeometryData[i].pData + attributeOffset + (v * pNode->pGeometryData[i].vertexStride), pCurrentFile, attributeSize);
          pCurrentFile += attributeSize;
        }
      }
      attributeOffset += attributeSize;
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

  udResult result = udR_Success; // udR_Failure TODO: (EVC-544) Handle texture load failures
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

  UD_ERROR_IF(pNode->textureDataCount == 0, udR_Success);

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    // TODO: (EVC-544) Handle partial texture load failures
    //result = udR_Failure_;

    // TODO: (EVC-542) other formats (this information is in sharedResource.json)
    for (size_t f = 0; f < udLengthOf(pExtensions); ++f)
    {
      udSprintf(&pPathBuffer, "zip://%s%c%s.%s", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pTextureData[i].pURL, pExtensions[f]);
      if (udFile_Load(pPathBuffer, (void**)&pFileData, &fileLen) != udR_Success)
        continue;

      pPixelData = (uint8_t*)stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      pNode->pTextureData[i].width = width;
      pNode->pTextureData[i].height = height;
      pNode->pTextureData[i].pData = (uint8_t*)udMemDup(pPixelData, sizeof(uint32_t)*width*height, 0, udAF_None);

      udFree(pFileData);
      stbi_image_free(pPixelData);
      result = udR_Success;
      break;
    }
  }

epilogue:
  udFree(pPathBuffer);
  return result;
}

template<typename T>
udResult vcSceneLayer_AllocateNodeData(T **ppData, size_t *pDataCount, const udJSON &nodeJSON, char pathSeparator, const char *pKey)
{
  udResult result;
  T *pData = nullptr;
  size_t count = 0;

  count = nodeJSON.Get("%s", pKey).ArrayLength();
  UD_ERROR_IF(count == 0, udR_Success); // valid

  pData = udAllocType(T, count, udAF_Zero);
  UD_ERROR_NULL(pData, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < count; ++i)
    vcNormalizePath(&pData[i].pURL, nullptr, nodeJSON.Get("%s[%zu].href", pKey, i).AsString(), pathSeparator);

  (*ppData) = pData;
  (*pDataCount) = count;
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udFree(pData);

  return result;
}

void vcSceneLayer_CalculateNodeBounds(vcSceneLayerNode *pNode, const udDouble4 &latLongHeightRadius)
{
  int sridCode = 0;

  pNode->latLong.x = latLongHeightRadius.x;
  pNode->latLong.y = latLongHeightRadius.y;

  // Calculate cartesian minimum bounding sphere
  udDouble3 originLatLong = udDouble3::create(latLongHeightRadius.x, latLongHeightRadius.y, 0.0);
  udGeoZone_FindSRID(&sridCode, originLatLong, true);
  udGeoZone_SetFromSRID(&pNode->zone, sridCode);

  pNode->minimumBoundingSphere.position = udGeoZone_ToCartesian(pNode->zone, originLatLong, true);
  pNode->minimumBoundingSphere.position.z = latLongHeightRadius.z;
  pNode->minimumBoundingSphere.radius = latLongHeightRadius.w;
}

udResult vcSceneLayer_LoadNodeInternals(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
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

udResult vcSceneLayer_RecursiveLoadNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, const vcSceneLayerLoadType &loadType)
{
  udResult result;
  udJSON nodeJSON;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;

  pNode->loadState = vcSceneLayerNode::vcLS_Loading;

  // Load the nodes info
  udSprintf(&pPathBuffer, "zip://%s%c3dNodeIndexDocument.json.gz", pNode->pURL, pSceneLayer->pathSeparatorChar);
  UD_ERROR_CHECK(udFile_LoadGZIP(pPathBuffer, (void**)&pFileData));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  // potentially already calculated some node info (it's stored in this nodes parents 'children' array)
  if (pNode->id[0] == '\0')
  {
    udStrcpy(pNode->id, sizeof(pNode->id), nodeJSON.Get("id").AsString());
    vcSceneLayer_CalculateNodeBounds(pNode, nodeJSON.Get("mbs").AsDouble4());
  }

  pNode->level = nodeJSON.Get("level").AsInt();
  pNode->lodSelectionValue = nodeJSON.Get("lodSelection[0].maxError").AsDouble(); // TODO: I've made an assumptions here (e.g. always has at least one lod metric)

  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, pSceneLayer->pathSeparatorChar, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "featureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "textureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "attributeData"));

  // Load children
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
      vcNormalizePath(&pChildNode->pURL, pNode->pURL, nodeJSON.Get("children[%zu].href", i).AsString(), pSceneLayer->pathSeparatorChar);

      vcSceneLayer_CalculateNodeBounds(pChildNode, nodeJSON.Get("children[%zu].mbs", i).AsDouble4());
    }
  }

  // TODO: (EVC-548) - these may actually not be needed
  // E.g. For rendering, we may not need the internals of certain nodes
  if (loadType == vcSLLT_Rendering || (loadType == vcSLLT_Convert && pNode->childrenCount == 0))
  {
    UD_ERROR_CHECK(vcSceneLayer_LoadNodeInternals(pSceneLayer, pNode));
  }
  else
  {
    pNode->loadState = vcSceneLayerNode::vcLS_PartialLoad;
  }

  if (loadType == vcSLLT_Convert)
  {
    for (size_t i = 0; i < pNode->childrenCount; ++i)
      UD_ERROR_CHECK(vcSceneLayer_RecursiveLoadNode(pSceneLayer, &pNode->pChildren[i], loadType));
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerNode::vcLS_Failed;

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayer_LoadNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode /*= nullptr*/, const vcSceneLayerLoadType &loadType /*= vcSLLT_None*/)
{
  udResult result;

  if (pNode != nullptr)
    UD_ERROR_SET(vcSceneLayer_RecursiveLoadNode(pSceneLayer, pNode, loadType));

  // TODO: (EVC-548) Assume the root has been loaded, so load the rest of the model
  for (size_t i = 0; i < pSceneLayer->root.childrenCount; ++i)
    UD_ERROR_CHECK(vcSceneLayer_RecursiveLoadNode(pSceneLayer, &pSceneLayer->root.pChildren[i], loadType));

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayer_Create(vcSceneLayer **ppSceneLayer, vWorkerThreadPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayer *pSceneLayer = nullptr;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  const char *pRootPath = nullptr;

  pSceneLayer = udAllocType(vcSceneLayer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer, udR_MemoryAllocationFailure);

  pSceneLayer->isActive = true;
  pSceneLayer->pThreadPool = pWorkerThreadPool;
  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  udSprintf(&pPathBuffer, "zip://%s:3dSceneLayer.json.gz", pSceneLayer->sceneLayerURL);
  UD_ERROR_CHECK(udFile_LoadGZIP(pPathBuffer, (void**)&pFileData));
  UD_ERROR_CHECK(pSceneLayer->description.Parse(pFileData));

  // Load the root
  // TODO: (EVC-548) Does the root actually need to be loaded here?
  // Use the root path to determine path separator character
  pRootPath = pSceneLayer->description.Get("store.rootNode").AsString();
  pSceneLayer->pathSeparatorChar = '/';
  if (udStrchr(pRootPath, "\\") != nullptr)
    pSceneLayer->pathSeparatorChar = '\\';

  vcNormalizePath(&pSceneLayer->root.pURL, pSceneLayer->sceneLayerURL, pRootPath, ':');
  UD_ERROR_CHECK(vcSceneLayer_LoadNode(pSceneLayer, &pSceneLayer->root));

  *ppSceneLayer = pSceneLayer;
  pSceneLayer = nullptr;
  result = udR_Success;

epilogue:
  if (pSceneLayer != nullptr)
  {
    pSceneLayer->description.Destroy();
    udFree(pSceneLayer);
  }

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}


void vcSceneLayer_RecursiveDestroyNode(vcSceneLayerNode *pNode)
{
  while (pNode->loadState == vcSceneLayerNode::vcLS_InQueue || pNode->loadState == vcSceneLayerNode::vcLS_Loading)
    udYield();

  for (size_t i = 0; i < pNode->sharedResourceCount; ++i)
    udFree(pNode->pSharedResources[i].pURL);

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udFree(pNode->pFeatureData[i].pURL);

    udFree(pNode->pFeatureData[i].pGeometryLayout);
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
    udFree(pNode->pAttributeData[i].pURL);

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

  udFree(pSceneLayer);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSceneLayer_UploadDataToGPUIfPossible(vcSceneLayerNode *pNode, bool force = false)
{
  udResult result;

  // TODO: (EVC-553)
  if (!force && gpuBytesUploadedThisFrame >= vcGLState_MaxUploadBytesPerFrame)
    UD_ERROR_SET(udR_Success);

  // geometry
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    if (pNode->pGeometryData[i].loaded || (pNode->pGeometryData[i].pData == nullptr))
      continue;

    // TODO: (EVC-553) Let `vcPolygonModel_CreateFromData()` handle this?
    if (!force && gpuBytesUploadedThisFrame >= vcGLState_MaxUploadBytesPerFrame) // allow partial uploads
      UD_ERROR_SET(udR_Success);

    UD_ERROR_IF(pNode->pGeometryData[i].vertCount > UINT16_MAX, udR_Failure_);
    UD_ERROR_CHECK(vcPolygonModel_CreateFromRawVertexData(&pNode->pGeometryData[i].pModel, pNode->pGeometryData[i].pData, (uint16_t)pNode->pGeometryData[i].vertCount, pNode->pFeatureData[i].pGeometryLayout, (int)pNode->pFeatureData[i].geometryLayoutCount));
    udFree(pNode->pGeometryData[i].pData);

    pNode->pGeometryData[i].loaded = true;
    vcGLState_ReportGPUWork(0, 0, (pNode->pGeometryData[i].vertexStride * pNode->pGeometryData[i].vertCount));
    gpuBytesUploadedThisFrame += (pNode->pGeometryData[i].vertexStride * pNode->pGeometryData[i].vertCount);
  }

  // textures
  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    if (pNode->pTextureData[i].loaded || (pNode->pTextureData[i].pData == nullptr))
      continue;

    // TODO: (EVC-553) Let `vcTexture_Create()` handle this?
    if (!force && gpuBytesUploadedThisFrame >= vcGLState_MaxUploadBytesPerFrame) // allow partial uploads
      UD_ERROR_SET(udR_Success);

    // Making an assumption here, use mip maps for 'flat' hierarchies
    bool useMips = (pNode->level <= 2);
    UD_ERROR_CHECK(vcTexture_Create(&pNode->pTextureData[i].pTexture, pNode->pTextureData[i].width, pNode->pTextureData[i].height, pNode->pTextureData[i].pData, vcTextureFormat_RGBA8, vcTFM_Linear, useMips));
    udFree(pNode->pTextureData[i].pData);

    pNode->pTextureData[i].loaded = true;
    vcGLState_ReportGPUWork(0, 0, (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4));
    gpuBytesUploadedThisFrame += (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4);
  }

  pNode->loadState = vcSceneLayerNode::vcLS_Success;
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
    vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeJob, pLoadData);
  }
  else if (pNode->loadState == vcSceneLayerNode::vcLS_PartialLoad)
  {
    // TODO: (EVC-569) Partial node uploading has not been fully tested!
    vcSceneLayer_LoadNodeJobData *pLoadData = udAllocType(vcSceneLayer_LoadNodeJobData, 1, udAF_Zero);
    pLoadData->pSceneLayer = pSceneLayer;
    pLoadData->pNode = pNode;
    vWorkerThread_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeInternalsJob, pLoadData);
  }

  if (pNode->loadState == vcSceneLayerNode::vcLS_InMemory)
    vcSceneLayer_UploadDataToGPUIfPossible(pNode);

  return pNode->loadState == vcSceneLayerNode::vcLS_Success;
}

vcSceneLayerVertex* vcSceneLayer_GetVertex(vcSceneLayerNode::GeometryData *pGeometry, uint64_t index)
{
  if (pGeometry == nullptr || pGeometry->pData == nullptr || index >= pGeometry->vertCount)
    return nullptr;

  // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
  return (vcSceneLayerVertex*)&pGeometry->pData[index * pGeometry->vertexStride];
}

udGeoZone* vcSceneLayer_GetPreferredZone(vcSceneLayer *pSceneLayer)
{
  if (pSceneLayer == nullptr)
    return nullptr;

  return &pSceneLayer->root.zone;
}

udDouble3 vcSceneLayer_GetCenter(vcSceneLayer *pSceneLayer)
{
  if (pSceneLayer == nullptr)
    return udDouble3::zero();

  return pSceneLayer->root.minimumBoundingSphere.position;
}
