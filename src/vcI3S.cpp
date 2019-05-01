#include "vcI3S.h"

#include <vector>

#include "vcSettings.h"
#include "vcPolygonModel.h"

#include "udPlatform/udJSON.h"
#include "udPlatform/udFile.h"

static const char *pSceneLayerInfoFile = "3dSceneLayer.json";
static const char *pNodeInfoFile = "3dNodeIndexDocument.json";
static const char *pNodeGeometriesURL = "geometries/0.bin";

//temp
#include "gl\vcTexture.h"
vcTexture *pTempTexture = nullptr;

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


  vcPolygonModel *pModel;
};

struct vcIndexed3DSceneLayer
{
  char sceneLayerURL[vcMaxPathLength];
  udDouble4 extent;
  vcIndexed3DSceneLayerNode root;
};

const char *vcIndexed3DSceneLayer_AppendURL(const char *pRootURL, const char *pURL)
{
  if (udStrlen(pURL) >= 2 && pURL[0] == '.' && pURL[1] == '/')
    return udTempStr("%s/%s", pRootURL, pURL + 2);

  // TODO: What else?

  return udTempStr("%s/%s", pRootURL, pURL);
}

void vcIndexed3DSceneLayer_DestroyNode(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  // TODO: recursively destroy?

  (pSceneLayer);
  (pNode);
  //for (size_t i = 0; i < pNode->pChildren->size(); ++i)
  //  udFree(pNode->pChildren[i]);
  //
  //delete pNode->pChildren;
  //pNode->pChildren = nullptr;
  //udFree(pNode)
}

udResult vcIndexed3DSceneLayer_LoadNodeGeometries(vcIndexed3DSceneLayerNode *pNode)
{
  udResult result;
  unsigned char *pFileData = nullptr;
  int64_t fileLen = 0;

  // TODO: node.geometryData
  UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pNode->href, pNodeGeometriesURL), (void**)&pFileData, &fileLen));
  //UD_ERROR_CHECK(udFile_Load("E:/Vault Datasets/I3S/tilt/tilt/nodes/0-1-0/geometries/0.bin", (void**)&pFileData, &fileLen));


  unsigned char *pCurrentFile = pFileData;

  // TODO: read default geometry schema
  uint32_t vertCount = *(uint32_t*)(pCurrentFile + 0);
  uint32_t featureCount = *(uint32_t*)(pCurrentFile + 4);
  pCurrentFile += 8;

  // position, normal, uv0, color (non interleaved?)
  vcPolygonModelVertex *pVerts = udAllocType(vcPolygonModelVertex, vertCount, udAF_Zero);

  size_t size = sizeof(vcPolygonModelVertex);

  // Topology: PerAttributeArray
  for (int i = 0; i < vertCount; ++i)
  {
    float f = *(float*)pCurrentFile;
    pVerts[i].pos = (*(udFloat3*)(pCurrentFile)) * udFloat3::create(800.0f, 800.0f, 1.0f);
    pCurrentFile += 12;

    printf("v: %f, %f, %f\n", pVerts[i].pos.x, pVerts[i].pos.y, pVerts[i].pos.z);
    //unsigned char a = *(pCurrentFile + 0);
    //unsigned char b = *(pCurrentFile + 1);
    //unsigned char c = *(pCurrentFile + 2);
    //unsigned char d = *(pCurrentFile + 3);
    //
    //uint32_t p = ((*(pCurrentFile + 0)) << 0) | ((*(pCurrentFile + 1)) << 8) | ((*(pCurrentFile + 2)) << 16) | ((*(pCurrentFile + 3)) << 24);
    //float x = *((float*)&p);
    //pCurrentFile += 4;
    //p = ((*(pCurrentFile + 0)) << 0) | ((*(pCurrentFile + 1)) << 8) | ((*(pCurrentFile + 2)) << 16) | ((*(pCurrentFile + 3)) << 24);
    //float y = *((float*)&p);
    //pCurrentFile += 4;
    //p = ((*(pCurrentFile + 0)) << 0) | ((*(pCurrentFile + 1)) << 8) | ((*(pCurrentFile + 2)) << 16) | ((*(pCurrentFile + 3)) << 24);
    //float z = *((float*)&p);
    //pCurrentFile += 4;
  }

  for (int i = 0; i < vertCount; ++i)
  {
    pVerts[i].normal = *(udFloat3*)(pCurrentFile);
    pCurrentFile += 12;
    printf("n: %f, %f, %f\n", pVerts[i].normal.x, pVerts[i].normal.y, pVerts[i].normal.z);
  }

  for (int i = 0; i < vertCount; ++i)
  {
    pVerts[i].uv = *(udFloat2*)(pCurrentFile);
    pCurrentFile += 8;
    printf("uv: %f, %f\n", pVerts[i].uv.x, pVerts[i].uv.y);
  }

  for (int i = 0; i < vertCount; ++i)
  {
    pCurrentFile += 4;
  }

  //pVerts[0].pos = udFloat3::create(0, 0, 0);
  //pVerts[1].pos = udFloat3::create(1, 0, 0);
  //pVerts[2].pos = udFloat3::create(1, 1, 0);
  //pVerts[3].pos = udFloat3::create(1, 1, 0);
  //pVerts[4].pos = udFloat3::create(0, 1, 0);
  //pVerts[5].pos = udFloat3::create(0, 0, 1);
  //pVerts[6].pos = udFloat3::create(0, 0, 1);
  //pVerts[7].pos = udFloat3::create(1, 0, 1);
  //pVerts[8].pos = udFloat3::create(1, 1, 1);
  //pVerts[9].pos = udFloat3::create(1, 1, 1);
  //pVerts[10].pos = udFloat3::create(0, 1, 1);
  //pVerts[11].pos = udFloat3::create(0, 0, 1);

  uint64_t featureAttributeId = *(uint64_t*)(pCurrentFile);
  uint32_t featureAttributeFaceRangeMin = *(uint32_t*)(pCurrentFile + 8);
  uint32_t featureAttributeFaceRangeMax = *(uint32_t*)(pCurrentFile + 12);

  vcPolygonModel_CreateFromData(&pNode->pModel, pVerts, vertCount);

epilogue:
  udFree(pVerts);
  udFree(pFileData);
  return result;
}

udResult vcIndexed3DSceneLayer_BuildNode(vcIndexed3DSceneLayer *pSceneLayer, vcIndexed3DSceneLayerNode *pNode)
{
  udResult result;
  udJSON nodeJSON;
  char *pFileData = nullptr;
  int64_t fileLen = 0;

  pNode->loadState = vcIndexed3DSceneLayerNode::vcLS_Loading;

  // Load the nodes info
  UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pNode->href, pNodeInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(nodeJSON.Parse(pFileData));

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


  if (&pSceneLayer->root != pNode)
  {
    // TODO: like all of it

    vcIndexed3DSceneLayer_LoadNodeGeometries(pNode);
  }

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

  vcTexture_CreateFromFilename(&pTempTexture, "E:/Vault Datasets/I3S/mesh4/nodes/9/textures/0_0.jpg");

  // TODO: Actually unzip. For now I'm manually unzipping until udPlatform moves
  //UD_ERROR_CHECK(udFile_Load("E:/Vault Datasets/I3S/mesh4/mesh4.slpk", (void**)&pFileData, &fileLen));

  UD_ERROR_CHECK(udFile_Load(vcIndexed3DSceneLayer_AppendURL(pSceneLayer->sceneLayerURL, pSceneLayerInfoFile), (void**)&pFileData, &fileLen));
  UD_ERROR_CHECK(sceneLayerJSON.Parse(pFileData));

  // Initialize the root
  const char *pRootURL = vcIndexed3DSceneLayer_AppendURL(pSceneLayer->sceneLayerURL, sceneLayerJSON.Get("store.rootNode").AsString());
  udStrcpy(pSceneLayer->root.href, sizeof(pSceneLayer->root.href), pRootURL);
  vcIndexed3DSceneLayer_BuildNode(pSceneLayer, &pSceneLayer->root);

  // FOR NOW, build first layer of tree
  for (size_t i = 9; i < 10; ++i)// pSceneLayer->root.pChildren->size(); ++i)
  {
    vcIndexed3DSceneLayer_BuildNode(pSceneLayer, pSceneLayer->root.pChildren->at(i));
  }

  //UD_ERROR_CHECK(udFile_Load(udTempStr("E:/Vault Datasets/I3S/mesh4/%s.json", rootNodePath), (void**)&pFileData, &fileLen));

  //pIndexed3DSceneLayer->root.

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

  udFree(pSceneLayer);
  result = udR_Success;

epilogue:
  return result;
}

bool vcIndexed3DSceneLayer_Render(vcIndexed3DSceneLayer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix)
{
  udDouble4x4 modelMatrix = udDouble4x4::identity();
  for (int i = 9; i < 10; ++i)
  {
    vcPolygonModel_Render(pSceneLayer->root.pChildren->at(i)->pModel, modelMatrix, viewProjectionMatrix, pTempTexture);
  }
  return true;
}
