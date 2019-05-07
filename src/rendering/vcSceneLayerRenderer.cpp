#include "vcSceneLayerRenderer.h"

#include <vector>

#include "vcSceneLayerHelper.h"
#include "vcSettings.h"
#include "vcPolygonModel.h"

#include "gl/vcTexture.h"

#include "udPlatform/udJSON.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udGeoZone.h"

static const char *pSceneLayerInfoFile = "3dSceneLayer.json";
static const char *pNodeInfoFile = "3dNodeIndexDocument.json";

// Shared, reused dynamically allocated
static char *pSharedStringBuffer = nullptr;

enum
{
  // TODO: (EVC-541) Determine actual max URL length of I3S
  vcMaxURLLength = vcMaxPathLength
};

struct vcSceneLayerRendererNode
{
  enum vcLoadState
  {
    vcLS_NotLoaded,
    vcLS_Loading,
    vcLS_Success,

    vcLS_Failed,
  };

  vcLoadState loadState;

  uint32_t id;
  char url[vcMaxURLLength];
  udDouble4 minimumBoundingSphere;
  vcSceneLayerRendererNode *pChildren;
  size_t childrenCount;

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

    // TODO: (EVC-542) is this per node/geometry, or per scene layer?
    udGeoZone zone;
  } *pGeometryData;
  size_t geometryDataCount;

  struct TextureData
  {
    char url[vcMaxURLLength];

    vcTexture *pTexture;
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
  char sceneLayerURL[vcMaxURLLength];
  udJSON description;
  udDouble4 extent;
  vcSceneLayerRendererNode root;

  vcVertexLayoutTypes *pDefaultGeometryLayout;
  size_t defaultGeometryLayoutCount;
};

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

const char *vcSceneLayerRenderer_AppendRelativeURL(const char *pRootURL, const char *pURL)
{
  // TODO: (EVC-541) Overcome udTempStr()'s max size of 2048 (max size of URL is 4096)
  if (udStrlen(pURL) >= 2 && pURL[0] == '.' && pURL[1] == '/')
    return udTempStr("%s/%s", pRootURL, pURL + 2);

  // TODO: (EVC-541) What else?

  return udTempStr("%s/%s", pRootURL, pURL);
}

void vcSceneLayerRenderer_RecursiveDestroyNode(vcSceneLayerRendererNode *pNode)
{
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
    vcPolygonModel_Destroy(&pNode->pGeometryData[i].pModel);

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
    vcTexture_Destroy(&pNode->pTextureData[i].pTexture);

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
    UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendRelativeURL(pNode->url, pSharedStringBuffer), (void**)&pFileData, &fileLen));
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
  char *pVerts = nullptr;
  int sridCode = 0;
  udDouble3 originLatLong = {};
  uint64_t vertCount = 0;
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
  udFloat2 heightBounds = {};

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(pSharedStringBuffer, vcMaxURLLength, "%s.bin", pNode->pGeometryData[i].url);
    UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendRelativeURL(pNode->url, pSharedStringBuffer), (void**)&pFileData, &fileLen));

    pCurrentFile = pFileData;

    // Assuming theres an equivalent feature element at this position
    originLatLong = udDouble3::create(pNode->pFeatureData[i].position.x, pNode->pFeatureData[i].position.y, 0.0);
    udGeoZone_FindSRID(&sridCode, originLatLong, true);
    udGeoZone_SetFromSRID(&pNode->pGeometryData[i].zone, sridCode);

    // debugging
    pointCartesian = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong, true);
    printf("Origin: %f, %f\n", pointCartesian.x, pointCartesian.y);
    printf("SRID: %d\n", sridCode);

    /// Header
    headerElementCount = pSceneLayer->description.Get("store.defaultGeometrySchema.header").ArrayLength();
    for (size_t h = 0; h < headerElementCount; ++h)
    {
      pPropertyName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].property", h).AsString();
      pTypeName = pSceneLayer->description.Get("store.defaultGeometrySchema.header[%zu].type", h).AsString();
      if (udStrEqual(pPropertyName, "vertexCount"))
        pCurrentFile += vcSceneLayerHelper_ReadSceneLayerType<uint64_t>(&vertCount, pCurrentFile, pTypeName);
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
    UD_ERROR_IF(vertCount * vertexSize == 0, udR_ParseError);

    pVerts = udAllocType(char, vertCount * vertexSize, udAF_Zero);

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
        pointCartesian = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong + udDouble3::create(vertI3S.x, vertI3S.y, 0.0), true);
        originCartesian = udDouble3::create(pointCartesian.x, pointCartesian.y, vertI3S.z + pNode->pFeatureData[i].position.z);
        pNode->pGeometryData[i].originMatrix = udDouble4x4::translation(originCartesian);

        for (uint64_t v = 0; v < vertCount; ++v)
        {
          memcpy(&vertI3S, pCurrentFile, sizeof(vertI3S));
          pointCartesian = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong + udDouble3::create(vertI3S.x, vertI3S.y, 0.0), true);
          pointCartesian.z = vertI3S.z; // Elevation (the z component of the vertex position) is specified in meters
          finalVertPosition = udFloat3::create(pointCartesian - originCartesian);

          memcpy(pVerts + vertexOffset + (v * vertexSize), &finalVertPosition, attributeSize);
          pCurrentFile += attributeSize;

          heightBounds.x = udMin(finalVertPosition.z, heightBounds.x);
          heightBounds.y = udMax(finalVertPosition.z, heightBounds.y);
        }

        // TODO: (EVC-543) The heights appear to be normalized? So I did this
        // to line it up so that the minimum vertex height is at 0.0. I don't know
        // if this is correct, I've probably missed something in the model format.
        pNode->pGeometryData[i].originMatrix.axis.t.z += (heightBounds.y - heightBounds.x) * 0.5;
      }
      else
      {
        for (uint64_t v = 0; v < vertCount; ++v)
        {
          memcpy(pVerts + vertexOffset + (v * vertexSize), pCurrentFile, attributeSize);
          pCurrentFile += attributeSize;
        }
      }
      vertexOffset += attributeSize;
    }

    //uint64_t featureAttributeId = *(uint64_t*)(pCurrentFile);
    //uint32_t featureAttributeFaceRangeMin = *(uint32_t*)(pCurrentFile + 8);
    //uint32_t featureAttributeFaceRangeMax = *(uint32_t*)(pCurrentFile + 12);

    UD_ERROR_IF(vertCount > UINT16_MAX, udR_Failure_);
    UD_ERROR_CHECK(vcPolygonModel_CreateFromData(&pNode->pGeometryData[i].pModel, pVerts, (uint16_t)vertCount, pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount));

    udFree(pVerts);
    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pVerts);
  udFree(pFileData);
  return result;
}

// Assumed vcSceneLayerRenderer_LoadNodeFeatures() already called
udResult vcSceneLayerRenderer_LoadNodeTextureData(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result = udR_Success;
  const char *pExtension = nullptr;

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    // TODO: (EVC-542) other formats (this information is in sharedResource.json)
    pExtension = "jpg"; // temp
    udSprintf(pSharedStringBuffer, vcMaxURLLength, "%s.%s", pNode->pTextureData[i].url, pExtension);
    if (!vcTexture_CreateFromFilename(&pNode->pTextureData[i].pTexture, vcSceneLayerRenderer_AppendRelativeURL(pNode->url, pSharedStringBuffer)))
      result = udR_Failure_;
  }

//epilogue:
  return result;
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


udResult vcSceneLayerRenderer_LoadNode(vcSceneLayerRenderer *pSceneLayer, vcSceneLayerRendererNode *pNode)
{
  udResult result;
  udJSON nodeJSON;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  vcSceneLayerRendererNode *pChildNode = nullptr;
  const char *pChildNodeURL = nullptr;

  pNode->loadState = vcSceneLayerRendererNode::vcLS_Loading;

  // Load the nodes info
  UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendRelativeURL(pNode->url, pNodeInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pSharedResources, &pNode->sharedResourceCount, nodeJSON, "sharedResource"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pFeatureData, &pNode->featureDataCount, nodeJSON, "featureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pGeometryData, &pNode->geometryDataCount, nodeJSON, "geometryData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pTextureData, &pNode->textureDataCount, nodeJSON, "textureData"));
  UD_ERROR_CHECK(vcSceneLayerRenderer_AllocateNodeData(&pNode->pAttributeData, &pNode->attributeDataCount, nodeJSON, "attributeData"));

  pNode->childrenCount = nodeJSON.Get("children").ArrayLength();
  if (pNode->childrenCount > 0)
  {
    pNode->pChildren = udAllocType(vcSceneLayerRendererNode, pNode->childrenCount, udAF_Zero);
    UD_ERROR_NULL(pNode->pChildren, udR_MemoryAllocationFailure);

    for (size_t i = 0; i < pNode->childrenCount; ++i)
    {
      pChildNode = &pNode->pChildren[i];
      pChildNode->loadState = vcSceneLayerRendererNode::vcLS_NotLoaded;
      pChildNode->id = nodeJSON.Get("children[%zu].id", i).AsInt();
      pChildNode->minimumBoundingSphere = nodeJSON.Get("children[%zu].mbs", i).AsDouble4();

      pChildNodeURL = nodeJSON.Get("children[%zu].href", i).AsString();
      udStrcpy(pChildNode->url, sizeof(pChildNode->url), vcSceneLayerRenderer_AppendRelativeURL(pNode->url, pChildNodeURL));
    }
  }

  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeFeatureData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeGeometryData(pSceneLayer, pNode));
  UD_ERROR_CHECK(vcSceneLayerRenderer_LoadNodeTextureData(pSceneLayer, pNode));

  result = udR_Success;
  pNode->loadState = vcSceneLayerRendererNode::vcLS_Success;

epilogue:
  if (pNode->loadState != vcSceneLayerRendererNode::vcLS_Success)
    pNode->loadState = vcSceneLayerRendererNode::vcLS_Failed;

  udFree(pFileData);
  return result;
}

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayer, const char *pSceneLayerURL)
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

  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform moves
  UD_ERROR_CHECK(udFile_Load(vcSceneLayerRenderer_AppendRelativeURL(pSceneLayer->sceneLayerURL, pSceneLayerInfoFile), (void**)&pFileData, &fileLen));
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
  pRootURL = vcSceneLayerRenderer_AppendRelativeURL(pSceneLayer->sceneLayerURL, pSceneLayer->description.Get("store.rootNode").AsString());
  udStrcpy(pSceneLayer->root.url, sizeof(pSceneLayer->root.url), pRootURL);
  vcSceneLayerRenderer_LoadNode(pSceneLayer, &pSceneLayer->root);

  // FOR NOW, build first layer of tree
  for (size_t i = 0; i < udMin(10ull, pSceneLayer->root.childrenCount); ++i)
  {
    vcSceneLayerRenderer_LoadNode(pSceneLayer, &pSceneLayer->root.pChildren[i]);
  }

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

// TODO: Visibility culling
// Notes about implementation: https://docs.opengeospatial.org/cs/17-014r5/17-014r5.html
//TraverseNodeTree(node)
//{
//  if (node’s mbs is not visible) // see 1)
//                                 // do nothing
//  else if (node has no children or ScreenSize(mbs) < maxScreenThreshold) //see 2)
//                                                                         // render the node // see 3)
//  else
//    for each child in children(node) // see 4)
//      TraverseNodeTree(child);
//}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRendererNode *pNode, const udDouble4x4 &viewProjectionMatrix)
{
  for (size_t i = 0; i < udMin(10ull, pNode->childrenCount); ++i) // hard coded 10 atm cause they arent unzipped!
  {
    vcSceneLayerRendererNode *pChildNode = &pNode->pChildren[i];
    for (size_t geometry = 0; geometry < pChildNode->geometryDataCount; ++geometry)
    {
      vcTexture *pDrawTexture = nullptr;
      if (pChildNode->textureDataCount > 0)
        pDrawTexture = pChildNode->pTextureData[0].pTexture; // TODO: pretty sure i need to read the material for this...
      vcPolygonModel_Render(pChildNode->pGeometryData[geometry].pModel, pChildNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, pDrawTexture);

      // TODO: (EVC-542) other geometry types
    }
  }
  return true;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix)
{
  vcSceneLayerRenderer_RecursiveRender(&pSceneLayer->root, viewProjectionMatrix);
  return true;
}
