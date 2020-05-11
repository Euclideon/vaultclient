#include "vcSceneLayer_Internal.h"

#include "stb_image.h"

#include "vcSceneLayerHelper.h"
#include "vcPolygonModel.h"


#include "vcTexture.h"
#include "vcGLState.h"

#include "udFile.h"
#include "udStringUtil.h"
#include "udWorkerPool.h"
#include "udThread.h"

enum
{
  MaxNodeLoadQueueLength = 8,
};

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN || UDPLATFORM_ANDROID
static int64_t gMaxAllowedGPUMemory = 2ull << 30; // 2 GB 
#else
static int64_t gMaxAllowedGPUMemory = 4ull << 30; // 4 GB 
#endif

struct vcSceneLayer_LoadNodeJobData
{
  vcSceneLayer *pSceneLayer;
  vcSceneLayerNode *pNode;

  // This job may be cleaned up at any point
  bool validMemory;
};

struct
{
  udInterlockedInt32 refCount;
  int64_t gpuMemoryUsageBytes;
  udChunkedArray<vcSceneLayerNode *> pruneList;

  // safe queue
  struct
  {
    udInterlockedBool keepLoading;
    udMutex *pLock;
    udChunkedArray<vcSceneLayer_LoadNodeJobData> queue; // stored in descending order of priority
  } loadQueue;

} static gSceneLayer = {};

void vcSceneLayer_RecursiveNodeFreeGPUResources(vcSceneLayerNode *pNode);
udResult vcSceneLayer_LoadTextureData(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode);

void vcSceneLayer_Init()
{
  int32_t count = gSceneLayer.refCount++;
  if (count == 0)
  {
    gSceneLayer.loadQueue.pLock = udCreateMutex();
    gSceneLayer.loadQueue.queue.Init(MaxNodeLoadQueueLength);

    gSceneLayer.pruneList.Init(256);

    gSceneLayer.loadQueue.keepLoading = true;
  }
  else
  {
    // Spin until initialized
    while (!gSceneLayer.loadQueue.keepLoading)
      udYield();
  }
}

void vcSceneLayer_Deinit()
{
  int32_t count = --gSceneLayer.refCount;
  if (count == 0)
  {
    gSceneLayer.loadQueue.keepLoading = false;

    udLockMutex(gSceneLayer.loadQueue.pLock);

    // at this point the queue will be empty because all scenelayers will have been destroyed, self removing
    // any of their nodes in the queue

    gSceneLayer.loadQueue.queue.Deinit();
    udReleaseMutex(gSceneLayer.loadQueue.pLock);
    udDestroyMutex(&gSceneLayer.loadQueue.pLock);

    gSceneLayer.pruneList.Deinit();
  }
}

void vcSceneLayer_BeginFrame()
{
  gSceneLayer.pruneList.Clear();
}

void vcSceneLayer_EndFrame()
{
  if (gSceneLayer.gpuMemoryUsageBytes < gMaxAllowedGPUMemory)
    return;

  while ((gSceneLayer.gpuMemoryUsageBytes >= gMaxAllowedGPUMemory) && gSceneLayer.pruneList.length > 0)
  {
    vcSceneLayerNode *pPruneCndidate = nullptr;
    gSceneLayer.pruneList.PopFront(&pPruneCndidate);
    vcSceneLayer_RecursiveNodeFreeGPUResources(pPruneCndidate);
  }
}

void vcSceneLayer_LoadNodeJob(void *pData)
{
  udUnused(pData);

  if (!gSceneLayer.loadQueue.keepLoading)
    return;

  vcSceneLayer_LoadNodeJobData bestJob = {};

  {
    udScopeLock scopeLock(gSceneLayer.loadQueue.pLock);
    gSceneLayer.loadQueue.queue.PopFront(&bestJob);

    // Main thread main may have destroyed this scene layer
    if (!bestJob.validMemory)
      return;

    ++bestJob.pSceneLayer->loadNodeJobsInFlight;
  }

  if (bestJob.pNode->internalsLoadState == vcSceneLayerNode::vcILS_None)
  {
    // Load basic node info
    vcSceneLayer_LoadNode(bestJob.pSceneLayer, bestJob.pNode);
  }
  else if (bestJob.pNode->internalsLoadState == vcSceneLayerNode::vcILS_BasicNodeData)
  {
    // Load node internals to main memory
    vcSceneLayer_LoadNodeInternals(bestJob.pSceneLayer, bestJob.pNode);
  }
  else if (bestJob.pNode->internalsLoadState == vcSceneLayerNode::vcILS_FreedGPUResources)
  {
    // only reload texture data to main memory
    vcSceneLayer_LoadTextureData(bestJob.pSceneLayer, bestJob.pNode);

    bestJob.pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    bestJob.pNode->internalsLoadState = vcSceneLayerNode::vcILS_NodeInternals;
  }

  --bestJob.pSceneLayer->loadNodeJobsInFlight;
}

void vcSceneLayer_InsertFront(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pInsertNode)
{
  udLockMutex(gSceneLayer.loadQueue.pLock);
  vcSceneLayer_LoadNodeJobData jobData = { pSceneLayer, pInsertNode, true };
  gSceneLayer.loadQueue.queue.PushFront(jobData);

  pInsertNode->loadState = vcSceneLayerNode::vcLS_InQueue;
  if (gSceneLayer.loadQueue.queue.length <= MaxNodeLoadQueueLength)
  {
    udWorkerPool_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeJob, nullptr);
  }
  else
  {
    // Remove the least prioritized node to make room for new node
    vcSceneLayer_LoadNodeJobData *pRemovedJob = &gSceneLayer.loadQueue.queue[gSceneLayer.loadQueue.queue.length - 1];
    pRemovedJob->pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    gSceneLayer.loadQueue.queue.PopBack();
  }

  udReleaseMutex(gSceneLayer.loadQueue.pLock);
}

void vcSceneLayer_TryInsertNodeByDistance(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pInsertNode, const udDouble3 &cameraPosition)
{
  udResult result = udR_Failure_;
  vcSceneLayer_LoadNodeJobData jobData = {};

  size_t insertIndex = 0;
  double nodeDistSqr = udMagSq3(cameraPosition - pInsertNode->minimumBoundingSphere.position) - (pInsertNode->minimumBoundingSphere.radius * pInsertNode->minimumBoundingSphere.radius);

  udLockMutex(gSceneLayer.loadQueue.pLock);

  for (insertIndex = 0; insertIndex < gSceneLayer.loadQueue.queue.length; ++insertIndex)
  {
    vcSceneLayerNode *pQueuedNode = gSceneLayer.loadQueue.queue[insertIndex].pNode;
    double queuedNodeDistSqr = udMagSq3(cameraPosition - pQueuedNode->minimumBoundingSphere.position) - (pQueuedNode->minimumBoundingSphere.radius * pQueuedNode->minimumBoundingSphere.radius);
    if (nodeDistSqr < queuedNodeDistSqr)
      break;
  }

  UD_ERROR_IF(insertIndex == MaxNodeLoadQueueLength, udR_Failure_);

  pInsertNode->loadState = vcSceneLayerNode::vcLS_InQueue;

  jobData.pSceneLayer = pSceneLayer;
  jobData.pNode = pInsertNode;
  jobData.validMemory = true;
  gSceneLayer.loadQueue.queue.Insert(insertIndex, &jobData); // note: this does a copy

  if (gSceneLayer.loadQueue.queue.length <= MaxNodeLoadQueueLength)
  {
    // create a job for this index
    udWorkerPool_AddTask(pSceneLayer->pThreadPool, vcSceneLayer_LoadNodeJob, nullptr);
  }
  else
  {
    // update and pop last node
    vcSceneLayer_LoadNodeJobData *pRemovedJob = &gSceneLayer.loadQueue.queue[gSceneLayer.loadQueue.queue.length - 1];
    pRemovedJob->pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    gSceneLayer.loadQueue.queue.PopBack();
  }

  result = udR_Success;
epilogue:
  udReleaseMutex(gSceneLayer.loadQueue.pLock);
}

void vcSceneLayer_AddPruneCandidate(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);
  gSceneLayer.pruneList.PushBack(pNode);
}

// TODO: (EVC-549) This functionality should be in udCore
void vcNormalizePath(const char **ppDest, const char *pRoot, const char *pAppend, char appendSeparator, char internalSeparator = '\0')
{
  char *pNewRoot = nullptr;
  char *pNewAppend = udStrdup(pAppend);
  size_t len = udStrlen(pNewAppend);
  size_t index = 0;

  // TODO: why are there '\\/' separator characters?
  // Replace any '\\/' character sequences with 'internalSeperator' if present
  while (udStrstr(pNewAppend, len, "\\/", &index) != nullptr)
  {
    memmove(pNewAppend + index, pNewAppend + (index + 1), (len - (index + 1)));
    len -= 1;
    pNewAppend[index] = internalSeparator ? internalSeparator : appendSeparator;
    pNewAppend[len] = '\0';
  }

  // Remove leading "./" or leading ".\\"
  if (udStrlen(pNewAppend) >= 2 && pNewAppend[0] == '.' && (pNewAppend[1] == '\\' || pNewAppend[1] == '/'))
  {
    len -= 2;
    memmove(pNewAppend, pNewAppend + 2, len);
    pNewAppend[len] = '\0';
  }

  // resolve "../" or "..\\" prefix
  // TODO: This isn't good enough! (assumptions)
  if (len >= 3 && udStrBeginsWith(pNewAppend, udTempStr("..%c", appendSeparator)))
  {
    if (udStrrchr(pRoot, udTempStr("%c", appendSeparator), &index) != nullptr)
    {
      pNewRoot = udStrdup(pRoot);
      pNewRoot[index] = '\0';
      pRoot = pNewRoot;

      // remove the "../"
      len -= 3;
      memmove(pNewAppend, pNewAppend + 3, len);
      pNewAppend[len] = '\0';
    }
  }

  // TODO: (EVC-541) What else?

  if (pRoot)
    udSprintf(ppDest, "%s%c%s", pRoot, appendSeparator, pNewAppend);
  else
    udSprintf(ppDest, "%s", pNewAppend);

  udFree(pNewRoot);
  udFree(pNewAppend);
}

udResult udFile_LoadGZIP(const vcSceneLayer *pSceneLayer, const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes = nullptr)
{
  udResult result;
  char *pBufferData = nullptr;
  int64_t rawFileLen = 0;
  char *pRawFileData = nullptr;
  size_t bufferLen = 0;

  udLockMutex(pSceneLayer->pFileReadMutex);

  UD_ERROR_CHECK(udFile_SetSubFilename(pSceneLayer->pFile, pFilename, &rawFileLen));

  pRawFileData = udAllocType(char, rawFileLen, udAF_None);
  UD_ERROR_NULL(pRawFileData, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(udFile_Read(pSceneLayer->pFile, pRawFileData, rawFileLen));

  bufferLen = rawFileLen;
  do
  {
    bufferLen *= 2;
    pBufferData = (char *)udRealloc(pBufferData, bufferLen);
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
  udReleaseMutex(pSceneLayer->pFileReadMutex);

  if (pBufferData != nullptr)
    udFree(pBufferData);

  udFree(pRawFileData);
  return result;
}

udResult vcSceneLayer_LoadFeatureData(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  udJSON featuresJSON;

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "%s%c%s.json.gz", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pFeatureData[i].pURL);
    UD_ERROR_CHECK(udFile_LoadGZIP(pSceneLayer, pPathBuffer, (void **)&pFileData));
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
    pNode->pFeatureData[i].pAttributeOffset = udAllocType(uint64_t, pNode->pFeatureData[i].geometryLayoutCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pFeatureData[i].pAttributeOffset, udR_MemoryAllocationFailure);

    pNode->pGeometryData[i].vertexStride = 0;

    for (size_t a = 0; a < pNode->pFeatureData[i].geometryLayoutCount; ++a)
    {
      const udJSON *pVertexAttributeJSON = vertexAttributesJSON.GetMember(a);
      const char *pAttributeName = vertexAttributesJSON.GetMemberName(a);
      size_t attributeElementSize = vcSceneLayerHelper_GetSceneLayerTypeSize(pVertexAttributeJSON->Get("valueType").AsString());
      int attributeElementCount = pVertexAttributeJSON->Get("valuesPerElement").AsInt();
      size_t attributeSize = (attributeElementCount * attributeElementSize);

      if (udStrEqual(pAttributeName, "position"))
      {
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Position3;
      }
      else if (udStrEqual(pAttributeName, "normal"))
      {
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Normal3;
      }
      else if (udStrEqual(pAttributeName, "uv0"))
      {
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_TextureCoords2;
      }
      else if (udStrEqual(pAttributeName, "color"))
      {
        // Shader doesn't support colour, don't upload it unnecessarily
        //pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_ColourBGRA;

        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Unsupported;
        attributeSize = 0;
      }
      else
      {
        // attribute gets ignored
        pNode->pFeatureData[i].pGeometryLayout[a] = vcVLT_Unsupported;
        attributeSize = 0;
      }

      // store in geometry data
      pNode->pGeometryData[i].vertexStride += attributeSize;
      pNode->pFeatureData[i].pAttributeOffset[a] = pVertexAttributeJSON->Get("byteOffset").AsInt64();
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
udResult vcSceneLayer_LoadGeometryData(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  uint64_t featureCount = 0;
  uint64_t faceCount = 0;
  char *pCurrentFile = nullptr;
  uint64_t attributeOffset = 0;
  int64_t fileLen = 0;
  udDouble3 originCartesian = {};

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(&pPathBuffer, "%s%c%s.bin.gz", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pGeometryData[i].pURL);
    UD_ERROR_CHECK(udFile_LoadGZIP(pSceneLayer, pPathBuffer, (void **)&pFileData, &fileLen));

    pCurrentFile = pFileData;

    // Geometry Schema
    // TODO: (EVC-548) The header layout is consistent for all nodes, calculate once on node load... not per geometry
    size_t headerElementCount = pSceneLayer->description.Get("store.defaultGeometrySchema.header").ArrayLength();
    for (size_t h = 0; h < headerElementCount; ++h)
    {
      const char *pPropertyName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].property", h).AsString();
      const char *pTypeName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].type", h).AsString();
      if (udStrEqual(pPropertyName, "vertexCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&pNode->pGeometryData[i].vertCount, pCurrentFile, pTypeName);
      else if (udStrEqual(pPropertyName, "featureCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&featureCount, pCurrentFile, pTypeName);
      else if (udStrEqual(pPropertyName, "faceCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&faceCount, pCurrentFile, pTypeName);
      else
      {
        // TODO: (EVC-542) Unknown property type
        //UD_ERROR_SET(udR_ParseError);
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
      vcVertexLayoutTypes attributeType = pNode->pFeatureData[i].pGeometryLayout[attributeIndex];
      pCurrentFile = pFileData + pNode->pFeatureData[i].pAttributeOffset[attributeIndex];

      if (attributeType == vcVLT_Unsupported)
        continue;

      // positions require additional pre-processing
      if (attributeType == vcVLT_Position3)
      {
        // Get the first position, and use that as an origin matrix (fixes floating point precision issue)
        // TODO: (EVC-540) Handle different sized positions
        udFloat3 currentVertPosition = *(udFloat3 *)pCurrentFile;

        originCartesian = udGeoZone_LatLongToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + currentVertPosition.x, pNode->latLong.y + currentVertPosition.y, 0.0), true);
        originCartesian.z = currentVertPosition.z + pNode->pFeatureData[i].position.z; // Elevation (the z component of the vertex position) is specified in meters

        pNode->pGeometryData[i].originMatrix = udDouble4x4::translation(originCartesian);
        pNode->pGeometryData[i].originMatrix.axis.t.z += pNode->minimumBoundingSphere.position.z;
      }

      for (uint64_t v = 0; v < pNode->pGeometryData[i].vertCount; ++v)
      {
        void *pAttributeOffset = pNode->pGeometryData[i].pData + attributeOffset + (v * pNode->pGeometryData[i].vertexStride);

        switch (attributeType)
        {
        case vcVLT_Position3:
        {
          // Reposition data to be relative to the first vertex
          udFloat3 currentVertPosition = *(udFloat3 *)pCurrentFile;

          udDouble3 pointCartesian = udGeoZone_LatLongToCartesian(pNode->zone, udDouble3::create(pNode->latLong.x + currentVertPosition.x, pNode->latLong.y + currentVertPosition.y, 0.0), true);
          pointCartesian.z = currentVertPosition.z; // Elevation (the z component of the vertex position) is specified in meters

          udFloat3 finalVertPosition = udFloat3::create(pointCartesian - originCartesian);
          memcpy(pAttributeOffset, &finalVertPosition, attributeSize);
          break;
        }

        case vcVLT_Normal3:
        {
          udFloat3 normalData = *(udFloat3 *)pCurrentFile;
          udFloat3 posLatLong = *(udFloat3 *)(pCurrentFile - 12); // TODO: This assumption is a hack, going to improve this whole deserialize process in a future MR
          udFloat3 finalNormal = normalData;

          if (pSceneLayer->normalReferenceFrame == vcSLNRF_EarthCentered)
          {
            // A value of *earth-centered* indicates that normals are stored in a global earth-centered, earth-fixed
            // (ECEF) reference frame where the x-axis points towards Prime meridian (lon = 0°) and Equator (lat = 0°),
            // the y-axis points East towards lon = +90 and lat = 0 and the z-axis points North. It is only valid for
            // geographic vertexCRS.

            // Create an ECEF-to-NED transformation matrix
            const float latRadians = (float)UD_DEG2RAD(pNode->latLong.y + posLatLong.y);
            const float lonRadians = (float)UD_DEG2RAD(pNode->latLong.x + posLatLong.x);
            const udFloat4x4 ECEFToNED = udFloat4x4::create(
              -udSin(latRadians) * udCos(lonRadians), -udSin(lonRadians), -udCos(latRadians) * udCos(lonRadians), 0,
              -udSin(latRadians) * udSin(lonRadians), udCos(lonRadians), -udCos(latRadians) * udSin(lonRadians), 0,
              udCos(latRadians), 0, -udSin(latRadians), 0,
              0, 0, 0, 1
            );

            // First convert ECEF (earth-centered, earth fixed) to NED (north east down)
            finalNormal = (ECEFToNED * udFloat4::create(normalData, 1.0f)).toVector3();

            // Second convert NED to ENU
            // The east north up (ENU) local tangent plane is similar to NED, except for swapping 'down' for 'up' and
            // x for y.
            finalNormal = udFloat3::create(finalNormal.y, finalNormal.x, -finalNormal.z);
          }
          else if (pSceneLayer->normalReferenceFrame == vcSLNRF_EastNorthUp)
          {
            // A value of *east-north-up* indicates that normals are stored in a node local reference frame defined by
            // the easting, northing and up directions at the MBS center, and is only valid for geographic (WGS84) vertexCRS.
          }
          else // vcSLNRF_VertexReferenceFrame
          {
            // A value of *vertex-reference-frame* indicates that normals are stored in the same reference frame as
            // vertices and is only valid for projected vertexCRS.
            // TODO: fixing in a future MR
          }

          memcpy(pAttributeOffset, &finalNormal, attributeSize);
          break;
        }

        default:
        {
          // This attribute data requires no processing
          memcpy(pAttributeOffset, pCurrentFile, attributeSize);
          break;
        }
        }

        pCurrentFile += attributeSize;
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
udResult vcSceneLayer_LoadTextureData(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result = udR_Success; // udR_Failure TODO: (EVC-544) Handle texture load failures
  const char *pPathBuffer = nullptr;
  uint8_t *pPixelData = nullptr;
  void *pFileData = nullptr;
  int64_t fileLen = 0;
  int channelCount = 0;
  int width = 0;
  int height = 0;

  udMutex *pLock = nullptr;

  // TODO: (EVC-542), (EVC-544)
  // TODO: Handle texture load failures
  const char *pExtensions[] = { "jpg", "bin" };//, "bin.dds" };

  UD_ERROR_IF(pNode->textureDataCount == 0, udR_Success);

  pLock = udLockMutex(pSceneLayer->pFileReadMutex);

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    // TODO: (EVC-544) Handle partial texture load failures
    //result = udR_Failure_;

    // TODO: (EVC-542) other formats (this information is in sharedResource.json)
    for (size_t f = 0; f < udLengthOf(pExtensions); ++f)
    {
      udSprintf(&pPathBuffer, "%s%c%s.%s", pNode->pURL, pSceneLayer->pathSeparatorChar, pNode->pTextureData[i].pURL, pExtensions[f]);

      if (udFile_SetSubFilename(pSceneLayer->pFile, pPathBuffer, &fileLen) != udR_Success)
        continue;

      pFileData = udAllocType(char, fileLen, udAF_None);
      UD_ERROR_NULL(pFileData, udR_MemoryAllocationFailure);
        
      if (udFile_Read(pSceneLayer->pFile, pFileData, fileLen) != udR_Success)
      {
        udFree(pFileData);
        continue;
      }

      pPixelData = (uint8_t *)stbi_load_from_memory((stbi_uc *)pFileData, (int)fileLen, (int *)&width, (int *)&height, (int *)&channelCount, 4);

      if (pPixelData != nullptr)
      {
        pNode->pTextureData[i].width = width;
        pNode->pTextureData[i].height = height;
        pNode->pTextureData[i].pData = (uint8_t *)udMemDup(pPixelData, sizeof(uint32_t) * width * height, 0, udAF_None);
      }

      stbi_image_free(pPixelData);
      udFree(pFileData);
      result = udR_Success;
      break;
    }
  }

epilogue:
  udFree(pPathBuffer);
  udFree(pFileData);

  udReleaseMutex(pLock);
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

  pNode->minimumBoundingSphere.position = udGeoZone_LatLongToCartesian(pNode->zone, originLatLong, true);
  pNode->minimumBoundingSphere.position.z = latLongHeightRadius.z;
  pNode->minimumBoundingSphere.radius = latLongHeightRadius.w;
}

udResult vcSceneLayer_LoadNodeInternals(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result;

  UD_ERROR_NULL(pNode, udR_InvalidParameter_);
  UD_ERROR_IF(pNode->internalsLoadState != vcSceneLayerNode::vcILS_BasicNodeData, udR_Success);

  UD_ERROR_CHECK(vcSceneLayer_LoadFeatureData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayer_LoadGeometryData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayer_LoadTextureData(pSceneLayer, pNode));

  pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
  pNode->internalsLoadState = vcSceneLayerNode::vcILS_NodeInternals;
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerNode::vcLS_Failed;

  return result;
}

udResult vcSceneLayer_LoadNode(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  udResult result;
  udJSON nodeJSON;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;

  UD_ERROR_IF(pNode->internalsLoadState != vcSceneLayerNode::vcILS_None, udR_Success);

  pNode->loadState = vcSceneLayerNode::vcLS_Loading;

  // Load the nodes info
  udSprintf(&pPathBuffer, "%s%c3dNodeIndexDocument.json.gz", pNode->pURL, pSceneLayer->pathSeparatorChar);

  UD_ERROR_CHECK(udFile_LoadGZIP(pSceneLayer, pPathBuffer, (void **)&pFileData));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  // root needs to load additional info about itself here
  if (pNode == &pSceneLayer->root)
  {
    pNode->pID = udStrdup(nodeJSON.Get("id").AsString());
    vcSceneLayer_CalculateNodeBounds(pNode, nodeJSON.Get("mbs").AsDouble4());
  }

  pNode->level = nodeJSON.Get("level").AsInt();
  pNode->lodSelectionValue = nodeJSON.Get("lodSelection[0].maxError").AsDouble(); // TODO: I've made an assumptions here (e.g. always has at least one lod metric)

  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, pSceneLayer->pathSeparatorChar, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "featureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "textureData"));
  UD_ERROR_CHECK(vcSceneLayer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, pSceneLayer->pathSeparatorChar, "attributeData"));

  // Load children info
  pNode->childrenCount = nodeJSON.Get("children").ArrayLength();
  if (pNode->childrenCount > 0)
  {
    pNode->pChildren = udAllocType(vcSceneLayerNode, pNode->childrenCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pChildren, udR_MemoryAllocationFailure);

    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      vcSceneLayerNode *pChildNode = &pNode->pChildren[i];
      pChildNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
      pChildNode->internalsLoadState = vcSceneLayerNode::vcILS_None;
      pChildNode->level = pNode->level + 1;
      pChildNode->pID = udStrdup(nodeJSON.Get("children[%zu].id", i).AsString());
      vcNormalizePath(&pChildNode->pURL, pNode->pURL, nodeJSON.Get("children[%zu].href", i).AsString(), pSceneLayer->pathSeparatorChar);

      vcSceneLayer_CalculateNodeBounds(pChildNode, nodeJSON.Get("children[%zu].mbs", i).AsDouble4());
    }
  }

  pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
  pNode->internalsLoadState = vcSceneLayerNode::vcILS_BasicNodeData;

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    pNode->loadState = vcSceneLayerNode::vcLS_Failed;

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

udResult vcSceneLayer_Create(vcSceneLayer **ppSceneLayer, udWorkerPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayer *pSceneLayer = nullptr;
  const char *pPathBuffer = nullptr;
  char *pFileData = nullptr;
  const char *pRootPath = nullptr;
  const char *pBaseURL = 0;

  const char *pNormalReferenceFrame = nullptr;
  static const char *normalReferenceFrameStrings[] =
  {
    "earth-centered", // default
    "east-north-up",
    "vertex-reference-frame"
  };

  vcSceneLayer_Init();

  pSceneLayer = udAllocType(vcSceneLayer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayer, udR_MemoryAllocationFailure);

  pSceneLayer->pThreadPool = pWorkerThreadPool;
  pSceneLayer->pFileReadMutex = udCreateMutex();

  udSprintf(&pPathBuffer, "zip://%s", pSceneLayerURL);

  UD_ERROR_CHECK(udFile_Open(&pSceneLayer->pFile, pPathBuffer, udFOF_Read));

  UD_ERROR_CHECK(udFile_LoadGZIP(pSceneLayer, "3dSceneLayer.json.gz", (void **)&pFileData));
  UD_ERROR_CHECK(pSceneLayer->description.Parse(pFileData));

  // Load root info
  // Use the root path to determine path separator character
  pRootPath = pSceneLayer->description.Get("store.rootNode").AsString();
  pSceneLayer->pathSeparatorChar = '/';
  if (udStrchr(pRootPath, "\\") != nullptr)
    pSceneLayer->pathSeparatorChar = '\\';

  // Load some scene layer meta data
  // Normal Reference Frame
  pNormalReferenceFrame = pSceneLayer->description.Get("store.normalReferenceFrame").AsString("earth-centered");
  for (int i = 0; i < (int)udLengthOf(normalReferenceFrameStrings); ++i)
  {
    if (udStrEqual(pNormalReferenceFrame, normalReferenceFrameStrings[i]))
    {
      pSceneLayer->normalReferenceFrame = (vcSceneLayerNormalReferenceFrame)i;
      break;
    }
  }
  
  vcNormalizePath(&pBaseURL, "", pRootPath, pSceneLayer->pathSeparatorChar, pSceneLayer->pathSeparatorChar);
  udSprintf(&pSceneLayer->root.pURL, "%s", pBaseURL + 1); // skip leading `/`
  udFree(pBaseURL);

  *ppSceneLayer = pSceneLayer;
  pSceneLayer = nullptr;
  result = udR_Success;

epilogue:
  if (pSceneLayer != nullptr)
  {
    udDestroyMutex(&pSceneLayer->pFileReadMutex);
    pSceneLayer->description.Destroy();
    udFree(pSceneLayer);
  }

  udFree(pPathBuffer);
  udFree(pFileData);
  return result;
}

int64_t vcSceneLayer_GetTextureGPUMemoryUsage(const vcSceneLayerNode *pNode)
{
  int64_t usage = 0;

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    if (pNode->pTextureData[i].loaded)
      usage += (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4);
  }

  return usage;
}

int64_t vcSceneLayer_GetGeometryGPUMemoryUsage(const vcSceneLayerNode *pNode)
{
  int64_t usage = 0;

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    if (pNode->pGeometryData[i].loaded)
      usage += vcLayout_GetSize(pNode->pFeatureData[i].pGeometryLayout, (int)pNode->pFeatureData[i].geometryLayoutCount) * pNode->pGeometryData[i].vertCount;
  }

  return usage;
}

void vcSceneLayer_RecursiveNodeFreeGPUResources(vcSceneLayerNode *pNode)
{
  if (pNode->internalsLoadState == vcSceneLayerNode::vcILS_UploadedToGPU)
  {
    gSceneLayer.gpuMemoryUsageBytes -= vcSceneLayer_GetTextureGPUMemoryUsage(pNode);

    for (size_t i = 0; i < pNode->textureDataCount; ++i)
    {
      vcTexture_Destroy(&pNode->pTextureData[i].pTexture);
      pNode->pTextureData[i].loaded = false;
    }

    pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
    pNode->internalsLoadState = vcSceneLayerNode::vcILS_FreedGPUResources; // will need to reload some bits to render fully
  }

  for (size_t i = 0; i < pNode->childrenCount; ++i)
    vcSceneLayer_RecursiveNodeFreeGPUResources(&pNode->pChildren[i]);
}

void vcSceneLayer_RecursiveDestroyNode(vcSceneLayerNode *pNode)
{
  while (pNode->loadState == vcSceneLayerNode::vcLS_Loading)
    udYield();

  for (size_t i = 0; i < pNode->sharedResourceCount; ++i)
    udFree(pNode->pSharedResources[i].pURL);

  gSceneLayer.gpuMemoryUsageBytes -= vcSceneLayer_GetTextureGPUMemoryUsage(pNode);
  gSceneLayer.gpuMemoryUsageBytes -= vcSceneLayer_GetGeometryGPUMemoryUsage(pNode);

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udFree(pNode->pGeometryData[i].pURL);

    vcPolygonModel_Destroy(&pNode->pGeometryData[i].pModel);
    udFree(pNode->pGeometryData[i].pData);
  }

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udFree(pNode->pFeatureData[i].pURL);
    udFree(pNode->pFeatureData[i].pGeometryLayout);
    udFree(pNode->pFeatureData[i].pAttributeOffset);
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
  {
    udFree(pNode->pChildren[i].pURL); // parents are responsible for this memory
    vcSceneLayer_RecursiveDestroyNode(&pNode->pChildren[i]);
  }

  udFree(pNode->pChildren);
  udFree(pNode->pID);
  // Note: `pNode->pURL` is not cleaned up here - remember the parent is responsible for that memory

  pNode->sharedResourceCount = 0;
  pNode->attributeDataCount = 0;
  pNode->textureDataCount = 0;
  pNode->featureDataCount = 0;
  pNode->geometryDataCount = 0;
  pNode->childrenCount = 0;

  pNode->loadState = vcSceneLayerNode::vcLS_NotLoaded;
  pNode->internalsLoadState = vcSceneLayerNode::vcILS_None;
}

udResult vcSceneLayer_Destroy(vcSceneLayer **ppSceneLayer)
{
  udResult result;
  vcSceneLayer *pSceneLayer = nullptr;

  UD_ERROR_NULL(ppSceneLayer, udR_InvalidParameter_);
  UD_ERROR_NULL(*ppSceneLayer, udR_InvalidParameter_);

  pSceneLayer = *ppSceneLayer;
  *ppSceneLayer = nullptr;

  udLockMutex(gSceneLayer.loadQueue.pLock);
  while (pSceneLayer->loadNodeJobsInFlight.Get() > 0)
    udYield();

  // Cancel any pending load jobs for this scene layer
  for (int32_t i = 0; i < int32_t(gSceneLayer.loadQueue.queue.length); ++i)
  {
    if (gSceneLayer.loadQueue.queue[i].pSceneLayer == pSceneLayer)
      gSceneLayer.loadQueue.queue[i].validMemory = false;
  }

  udReleaseMutex(gSceneLayer.loadQueue.pLock);

  vcSceneLayer_RecursiveDestroyNode(&pSceneLayer->root);
  udFree(pSceneLayer->root.pURL);
  pSceneLayer->description.Destroy();

  udFile_Close(&pSceneLayer->pFile);
  udDestroyMutex(&pSceneLayer->pFileReadMutex);

  udFree(pSceneLayer);

  vcSceneLayer_Deinit();

  result = udR_Success;

epilogue:
  return result;
}

void vcSceneLayer_CheckNodePruneCandidancy(const vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  // only pruning nodes that actually have GPU data to prune
  if (pNode->internalsLoadState == vcSceneLayerNode::vcILS_UploadedToGPU)
    vcSceneLayer_AddPruneCandidate(pSceneLayer, pNode);
}

bool vcSceneLayer_ExpandNodeForRendering(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode)
{
  if ((pNode->loadState == vcSceneLayerNode::vcLS_NotLoaded) &&
    ((pNode->internalsLoadState == vcSceneLayerNode::vcILS_BasicNodeData) || (pNode->internalsLoadState == vcSceneLayerNode::vcILS_FreedGPUResources)))
  {
    vcSceneLayer_InsertFront(pSceneLayer, pNode);

    return (pNode->internalsLoadState == vcSceneLayerNode::vcILS_FreedGPUResources);
  }

  // create gpu data now
  if (pNode->internalsLoadState == vcSceneLayerNode::vcILS_NodeInternals)
  {
    // always allow geometry to load
    for (size_t i = 0; i < pNode->geometryDataCount; ++i)
    {
      vcSceneLayerNode::GeometryData *pGeometryData = &pNode->pGeometryData[i];
      vcSceneLayerNode::FeatureData *pFeatureData = &pNode->pFeatureData[i];

      if (pGeometryData->loaded)
        continue;

      gSceneLayer.gpuMemoryUsageBytes += vcLayout_GetSize(pNode->pFeatureData[i].pGeometryLayout, (int)pNode->pFeatureData[i].geometryLayoutCount) * pNode->pGeometryData[i].vertCount;

      vcPolygonModel_CreateFromRawVertexData(&pGeometryData->pModel, pGeometryData->pData, (uint32_t)pGeometryData->vertCount, pFeatureData->pGeometryLayout, (int)pFeatureData->geometryLayoutCount);
      udFree(pGeometryData->pData);
      pGeometryData->loaded = true;
    }

    // don't always allow texture data to load
    if (gSceneLayer.gpuMemoryUsageBytes < gMaxAllowedGPUMemory)
    {
      for (size_t i = 0; i < pNode->textureDataCount; ++i)
      {
        vcSceneLayerNode::TextureData *pTextureData = &pNode->pTextureData[i];

        if (pTextureData->loaded)
          continue;

        if (pTextureData != nullptr && pTextureData->pData != nullptr)
        {
          gSceneLayer.gpuMemoryUsageBytes += (pNode->pTextureData[i].width * pNode->pTextureData[i].height * 4);

          vcTexture_Create(&pTextureData->pTexture, pTextureData->width, pTextureData->height, pTextureData->pData, vcTextureFormat_RGBA8, vcTFM_Linear);
          udFree(pTextureData->pData);
          pTextureData->loaded = true;
        }
      }

      pNode->loadState = vcSceneLayerNode::vcLS_Success;
      pNode->internalsLoadState = vcSceneLayerNode::vcILS_UploadedToGPU;
    }
  }

  // may be in a renderable state here depending on internal state
  return (pNode->internalsLoadState == vcSceneLayerNode::vcILS_NodeInternals) || (pNode->internalsLoadState == vcSceneLayerNode::vcILS_UploadedToGPU) || (pNode->internalsLoadState == vcSceneLayerNode::vcILS_FreedGPUResources);
}

bool vcSceneLayer_TouchNode(vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, const udDouble3 &cameraPosition)
{
  if (pNode->loadState == vcSceneLayerNode::vcLS_NotLoaded && pNode->internalsLoadState == vcSceneLayerNode::vcILS_None)
    vcSceneLayer_TryInsertNodeByDistance(pSceneLayer, pNode, cameraPosition);

  return pNode->internalsLoadState != vcSceneLayerNode::vcILS_None;
}

vcSceneLayerVertex *vcSceneLayer_GetVertex(vcSceneLayerNode::GeometryData *pGeometry, uint64_t index)
{
  if (pGeometry == nullptr || pGeometry->pData == nullptr || index >= pGeometry->vertCount)
    return nullptr;

  // TODO: (EVC-540) ASSUMPTIONS! (assumed a specific vertex layout!)
  return (vcSceneLayerVertex *)&pGeometry->pData[index * pGeometry->vertexStride];
}

udGeoZone *vcSceneLayer_GetPreferredZone(vcSceneLayer *pSceneLayer)
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
