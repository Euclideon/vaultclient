#include "vcI3S.h"

#include <vector>

#include "vcSettings.h"
#include "vcPolygonModel.h"

#include "gl\vcTexture.h"

#include "udPlatform/udJSON.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udGeoZone.h"

static const char *pSceneLayerInfoFile = "3dSceneLayer.json";
static const char *pNodeInfoFile = "3dNodeIndexDocument.json";

struct vcIndexed3DSceneLayerNode
{
  enum vcLoadState
  {
    vcLS_NotLoaded,
    vcLS_Loading,
    vcLS_Loaded,

    vcLS_Failed,
  };

  vcLoadState loadState;

  uint32_t id;
  char href[vcMaxPathLength];
  udDouble4 mbs;
  std::vector<vcIndexed3DSceneLayerNode*> *pChildren;

  struct SharedResource
  {
    char href[vcMaxPathLength];

  } *pSharedResources;
  size_t sharedResourceCount;

  struct FeatureData
  {
    char href[vcMaxPathLength];

    udDouble3 position;
    udDouble3 pivotOffset;
    udDouble4 mbb;
  } *pFeatureData;
  size_t featureDataCount;

  struct GeometryData
  {
    char href[vcMaxPathLength];

    udDouble4x4 originMatrix;
    udDouble4x4 transformation;
    //int type;
    vcPolygonModel *pModel;

    // TODO: is this per node/geometry, or per scene layer?
    udGeoZone zone;
  } *pGeometryData;
  size_t geometryDataCount;

  struct TextureData
  {
    char href[vcMaxPathLength];

    vcTexture *pTexture;
  } *pTextureData;
  size_t textureDataCount;

  struct AttributeData
  {
    char href[vcMaxPathLength];

  } *pAttributeData;
  size_t attributeDataCount;
};

struct vcIndexed3DSceneLayer
{
  char sceneLayerURL[vcMaxPathLength];
  udDouble4 extent;
  vcIndexed3DSceneLayerNode *pRoot;

  size_t defaultGeometryLayoutCount;
  vcVertexLayoutTypes *pDefaultGeometryLayout;
};

const char *vcIndexed3DSceneLayer_AppendURL(const char *pRootURL, const char *pURL)
{
  if (udStrlen(pURL) >= 2 && pURL[0] == '.' && pURL[1] == '/')
    return udTempStr("%s/%s", pRootURL, pURL + 2);

  // TODO: What else?

  return udTempStr("%s/%s", pRootURL, pURL);
}

void vcIndexed3DSceneLayer_RecursiveDestroyNode(vcIndexed3DSceneLayerNode *pNode)
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

  if (pNode->pChildren)
  {
    for (size_t i = 0; i < pNode->pChildren->size(); ++i)
      vcIndexed3DSceneLayer_RecursiveDestroyNode(pNode->pChildren->at(i));

    delete pNode->pChildren;
    pNode->pChildren = nullptr;
  }

  udFree(pNode);
}

udResult vcIndexed3DSceneLayer_LoadNodeFeatureData(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  udJSON featuresJSON;
  char localFilename[vcMaxPathLength];

  for (size_t i = 0; i < pNode->featureDataCount; ++i)
  {
    udSprintf(localFilename, sizeof(localFilename), "%s.json", pNode->pFeatureData[i].href);
    UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pNode->href, localFilename), (void**)&pFileData, &fileLen));
    UD_ERROR_CHECK(featuresJSON.Parse(pFileData));

    // TODO: JIRA task add a udJSON.AsDouble2()
    pNode->pFeatureData[i].position.x = featuresJSON.Get("featureData[%zu].position[0]", i).AsDouble();
    pNode->pFeatureData[i].position.y = featuresJSON.Get("featureData[%zu].position[1]", i).AsDouble();
    pNode->pFeatureData[i].pivotOffset = featuresJSON.Get("featureData[%zu].pivotOffset", i).AsDouble3();
    pNode->pFeatureData[i].mbb = featuresJSON.Get("featureData[%zu].pivotOffset", i).AsDouble4();

    // TODO: Where does this go?
    ///pNode->geometries.transformation = featuresJSON.Get("geometryData[0].transformation").AsDouble4x4();

    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pFileData);
  return result;
}

// Assumed vcIndexed3DSceneLayer_LoadNodeFeatures() already called
udResult vcIndexed3DSceneLayer_LoadNodeGeometryData(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  udResult result;
  char *pFileData = nullptr;
  int64_t fileLen = 0;
  char *pVerts = nullptr;
  int sridCode = 0;
  udDouble3 originLatLong = {};
  uint32_t vertCount = 0;
  uint32_t featureCount = 0;
  char *pCurrentFile = nullptr;
  uint32_t vertexSize = 0;
  uint32_t vertexOffset = 0;
  uint32_t attributeSize = 0;
  char localFilename[vcMaxPathLength];

  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
  {
    udSprintf(localFilename, sizeof(localFilename), "%s.bin", pNode->pGeometryData[i].href);
    UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pNode->href, localFilename), (void**)&pFileData, &fileLen));

    // Assuming theres an equivalent feature element
    originLatLong = udDouble3::create(pNode->pFeatureData[i].position.x, pNode->pFeatureData[i].position.y, 0.0);
    udGeoZone_FindSRID(&sridCode, originLatLong, true);
    udGeoZone_SetFromSRID(&pNode->pGeometryData[i].zone, sridCode);

    // debug testing
    udDouble3 originCartXY = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong, true);
    printf("Origin: %f, %f\n", originCartXY.x, originCartXY.y);
    printf("SRID: %d\n", sridCode);

    pCurrentFile = pFileData;

    // TODO: READ HEADER INFO instead of hard coding these bad boys
    vertCount = *(uint32_t*)(pCurrentFile + 0);
    featureCount = *(uint32_t*)(pCurrentFile + 4);
    pCurrentFile += 8;

    vertexSize = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);
    pVerts = udAllocType(char, vertCount * vertexSize, udAF_Zero);

    // vertices are non-interleaved. Is that what "Topology: PerAttributeArray" signals?
    vertexOffset = 0;
    for (int attributeIndex = 0; attributeIndex < pSceneLayer->defaultGeometryLayoutCount; ++attributeIndex)
    {
      attributeSize = vcLayout_GetSize(pSceneLayer->pDefaultGeometryLayout[attributeIndex]);

      if (pSceneLayer->pDefaultGeometryLayout[attributeIndex] == vcVLT_Position3)
      {
        // positions require additional processing

        // Get the first position, and use that as an origin matrix (floating point precision issue)
        udFloat3 originVertXYZ = (*(udFloat3*)(pCurrentFile));
        udDouble3 cartesianOrigin = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong + udDouble3::create(originVertXYZ.x, originVertXYZ.y, 0.0), true);
        udDouble3 vertOriginPosition = udDouble3::create(cartesianOrigin.x, cartesianOrigin.y, originVertXYZ.z);

        pNode->pGeometryData[i].originMatrix = udDouble4x4::translation(vertOriginPosition);

        for (uint32_t v = 0; v < vertCount; ++v)
        {
          udFloat3 vertXYZ = (*(udFloat3*)(pCurrentFile));
          udDouble3 cartesian = udGeoZone_ToCartesian(pNode->pGeometryData[i].zone, originLatLong + udDouble3::create(vertXYZ.x, vertXYZ.y, 0.0), true);
          cartesian.z = vertXYZ.z; // Elevation (the z component of the vertex position) is specified in meters
          udFloat3 vertPosition = udFloat3::create(cartesian - vertOriginPosition);

          memcpy(pVerts + vertexOffset + (v * vertexSize), &vertPosition, attributeSize);
          pCurrentFile += attributeSize;
        }
      }
      else
      {
        for (uint32_t v = 0; v < vertCount; ++v)
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

    vcPolygonModel_CreateFromData(&pNode->pGeometryData[i].pModel, pVerts, (uint16_t)vertCount, pSceneLayer->pDefaultGeometryLayout, (int)pSceneLayer->defaultGeometryLayoutCount);

    udFree(pVerts);
    udFree(pFileData);
  }

  result = udR_Success;

epilogue:
  udFree(pFileData);
  return result;
}

// Assumed vcIndexed3DSceneLayer_LoadNodeFeatures() already called
udResult vcIndexed3DSceneLayer_LoadNodeTextureData(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  udUnused(pSceneLayer);

  udResult result;
  char localFilename[vcMaxPathLength];

  for (size_t i = 0; i < pNode->textureDataCount; ++i)
  {
    // TODO: other formats
    // TODO: (this is in sharedResource.json)
    const char *pExtension = "jpg";
    udSprintf(localFilename, sizeof(localFilename), "%s.%s", pNode->pTextureData[i].href, pExtension);
    if (!vcTexture_CreateFromFilename(&pNode->pTextureData[i].pTexture, vcIndexed3DSceneLayer_AppendURL(pNode->href, localFilename)))
      result = udR_Failure_;
  }

  result = udR_Success;

//epilogue:
  return result;
}


udResult vcIndexed3DSceneLayer_LoadNode(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  udResult result;
  udJSON nodeJSON;
  char *pFileData = nullptr;
  int64_t fileLen = 0;

  pNode->loadState = vcIndexed3DSceneLayerNode::vcLS_Loading;

  // Load the nodes info
  UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pNode->href, pNodeInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

  pNode->sharedResourceCount = nodeJSON.Get("sharedResource").ArrayLength();
  pNode->featureDataCount = nodeJSON.Get("featureData").ArrayLength();
  pNode->geometryDataCount = nodeJSON.Get("geometryData").ArrayLength();
  pNode->textureDataCount = nodeJSON.Get("textureData").ArrayLength();
  pNode->attributeDataCount = nodeJSON.Get("attributeData").ArrayLength();

  pNode->pSharedResources = udAllocType(vcIndexed3DSceneLayerNode::SharedResource, pNode->sharedResourceCount, udAF_Zero);
  pNode->pFeatureData = udAllocType(vcIndexed3DSceneLayerNode::FeatureData, pNode->featureDataCount, udAF_Zero);
  pNode->pGeometryData = udAllocType(vcIndexed3DSceneLayerNode::GeometryData, pNode->geometryDataCount, udAF_Zero);
  pNode->pTextureData = udAllocType(vcIndexed3DSceneLayerNode::TextureData, pNode->textureDataCount, udAF_Zero);
  pNode->pAttributeData = udAllocType(vcIndexed3DSceneLayerNode::AttributeData, pNode->attributeDataCount, udAF_Zero);

  for (size_t i = 0; i < pNode->sharedResourceCount; ++i)
    udStrcpy(pNode->pSharedResources[i].href, sizeof(pNode->pSharedResources[i].href), nodeJSON.Get("sharedResource[%zu].href", i).AsString());
  for (size_t i = 0; i < pNode->featureDataCount; ++i)
    udStrcpy(pNode->pFeatureData[i].href, sizeof(pNode->pFeatureData[i].href), nodeJSON.Get("featureData[%zu].href", i).AsString());
  for (size_t i = 0; i < pNode->geometryDataCount; ++i)
    udStrcpy(pNode->pGeometryData[i].href, sizeof(pNode->pGeometryData[i].href), nodeJSON.Get("geometryData[%zu].href", i).AsString());
  for (size_t i = 0; i < pNode->textureDataCount; ++i)
    udStrcpy(pNode->pTextureData[i].href, sizeof(pNode->pTextureData[i].href), nodeJSON.Get("textureData[%zu].href", i).AsString());
  for (size_t i = 0; i < pNode->attributeDataCount; ++i)
    udStrcpy(pNode->pAttributeData[i].href, sizeof(pNode->pAttributeData[i].href), nodeJSON.Get("attributeData[%zu].href", i).AsString());

  pNode->pChildren = new std::vector<vcIndexed3DSceneLayerNode*>();

  size_t numChildren = nodeJSON.Get("children").ArrayLength();
  for (size_t i = 0; i < numChildren; ++i)
  {
    // TODO: handle this memory allocation failure
    vcIndexed3DSceneLayerNode *pChildNode = udAllocType(vcIndexed3DSceneLayerNode, 1, udAF_Zero);
    pChildNode->loadState = vcIndexed3DSceneLayerNode::vcLS_NotLoaded;
    pChildNode->id = nodeJSON.Get("children[%zu].id", i).AsInt();
    pChildNode->mbs = nodeJSON.Get("children[%zu].mbs", i).AsDouble4();

    const char *pChildNodeURL = nodeJSON.Get("children[%zu].href", i).AsString();
    udStrcpy(pChildNode->href, sizeof(pChildNode->href), vcIndexed3DSceneLayer_AppendURL(pNode->href, pChildNodeURL));
    pNode->pChildren->push_back(pChildNode);
  }

  vcIndexed3DSceneLayer_LoadNodeFeatureData(pSceneLayer, pNode);
  vcIndexed3DSceneLayer_LoadNodeGeometryData(pSceneLayer, pNode);
  vcIndexed3DSceneLayer_LoadNodeTextureData(pSceneLayer, pNode);

  result = udR_Success;
  pNode->loadState = vcIndexed3DSceneLayerNode::vcLS_Loaded;

epilogue:
  if (pNode->loadState != vcIndexed3DSceneLayerNode::vcLS_Loaded)
    pNode->loadState = vcIndexed3DSceneLayerNode::vcLS_Failed;

  udFree(pFileData);
  return result;
}

udResult vcIndexed3DSceneLayer_Create(vcIndexed3DSceneLayer **ppSceneLayer, const char *pSceneLayerURL)
{
  udResult result;
  vcIndexed3DSceneLayer *pSceneLayer = nullptr;
  const char *pFileData = nullptr;
  int64_t fileLen = 0;
  udJSON sceneLayerJSON;

  pSceneLayer = udAllocType(vcIndexed3DSceneLayer, 1, udAF_Zero);
  udStrcpy(pSceneLayer->sceneLayerURL, sizeof(pSceneLayer->sceneLayerURL), pSceneLayerURL);

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform moves
  UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pSceneLayer->sceneLayerURL, pSceneLayerInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(sceneLayerJSON.Parse(pFileData));

  // Load sceneLayer info
  pSceneLayer->defaultGeometryLayoutCount = sceneLayerJSON.Get("store.defaultGeometrySchema.ordering").ArrayLength();
  pSceneLayer->pDefaultGeometryLayout = udAllocType(vcVertexLayoutTypes, pSceneLayer->defaultGeometryLayoutCount, udAF_Zero);
  for (size_t i = 0; i < pSceneLayer->defaultGeometryLayoutCount; ++i)
  {
    const char *pAttributeName = sceneLayerJSON.Get("store.defaultGeometrySchema.ordering[%zu]", i).AsString();

    // TODO: Probably need to read vertex attribute info.
    // E.g. Is anything other than 3 x Float32 for position?
    if (udStrcmp(pAttributeName, "position") == 0)
    {
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_Position3;
    }
    else if (udStrcmp(pAttributeName, "normal") == 0)
    {
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_Normal3;
    }
    else if (udStrcmp(pAttributeName, "uv0") == 0)
    {
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_TextureCoords2;
    }
    else if (udStrcmp(pAttributeName, "uv1") == 0)
    {
      //pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_TextureCoords2;
    }
    else if (udStrcmp(pAttributeName, "color") == 0)
    {
      pSceneLayer->pDefaultGeometryLayout[i] = vcVLT_ColourBGRA;
    }
    else
    {
      // TODO: Unknown attribute type
    }
  }

  // Initialize the root
  pSceneLayer->pRoot = udAllocType(vcIndexed3DSceneLayerNode, 1, udAF_Zero);
  const char *pRootURL = vcIndexed3DSceneLayer_AppendURL(pSceneLayer->sceneLayerURL, sceneLayerJSON.Get("store.rootNode").AsString());
  udStrcpy(pSceneLayer->pRoot->href, sizeof(pSceneLayer->pRoot->href), pRootURL);
  vcIndexed3DSceneLayer_LoadNode(pSceneLayer, pSceneLayer->pRoot);

  // FOR NOW, build first layer of tree
  for (size_t i = 0; i < udMin(10ull, pSceneLayer->pRoot->pChildren->size()); ++i)// pSceneLayer->root.pChildren->size(); ++i)
  {
    vcIndexed3DSceneLayer_LoadNode(pSceneLayer, pSceneLayer->pRoot->pChildren->at(i));
  }

  *ppSceneLayer = pSceneLayer;
  result = udR_Success;

epilogue:
  udFree(pFileData);
  return result;
}

udResult vcIndexed3DSceneLayer_Destroy(vcIndexed3DSceneLayer **ppSceneLayer)
{
  udResult result;
  vcIndexed3DSceneLayer *pSceneLayer = nullptr;

  UD_ERROR_NULL(ppSceneLayer, udR_InvalidParameter_);

  pSceneLayer = *ppSceneLayer;
  *ppSceneLayer = nullptr;

  vcIndexed3DSceneLayer_RecursiveDestroyNode(pSceneLayer->pRoot);

  udFree(pSceneLayer->pDefaultGeometryLayout);
  udFree(pSceneLayer);
  result = udR_Success;

epilogue:
  return result;
}

bool vcIndexed3DSceneLayer_RecursiveRender(vcIndexed3DSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix)
{
  for (int i = 0; i < udMin(10ull, pNode->pChildren->size()); ++i) // hard coded 10 atm cause they arent unzipped!
  {
    vcIndexed3DSceneLayerNode *pChildNode = pNode->pChildren->at(i);
    for (int geometry = 0; geometry < pChildNode->geometryDataCount; ++geometry)
    {
      vcTexture *pDrawTexture = nullptr;
      if (pChildNode->textureDataCount > 0)
        pDrawTexture = pChildNode->pTextureData[0].pTexture; // todo: pretty sure i need to read the material for this...
      vcPolygonModel_Render(pChildNode->pGeometryData[geometry].pModel, pChildNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, pDrawTexture);

      // TODO: other geometry types
    }
  }

  return true;
}

bool vcIndexed3DSceneLayer_Render(vcIndexed3DSceneLayer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix)
{
  vcIndexed3DSceneLayer_RecursiveRender(pSceneLayer->pRoot, viewProjectionMatrix);
  return true;
}
