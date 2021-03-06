#include "vcTileRenderer.h"
#include "vcQuadTree.h"
#include "vcGIS.h"
#include "vcSettings.h"
#include "vcPolygonModel.h"
#include "vcStringFormat.h"

#include "vcGLState.h"
#include "vcShader.h"
#include "vcMesh.h"

#include "udThread.h"
#include "udFile.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"
#include "udStringUtil.h"

#include "stb_image.h"
#include "Lerc1Decode/CntZImage.h"

// Debug tiles with colour information
#define VISUALIZE_DEBUG_TILES 0

// Time to wait before retrying a failed tile load
static const float TileTimeoutRetrySec = 5.0f;
static const int TileFailedRetryCount = 5;

// Frequency of how often to update quad tree
static const float QuadTreeUpdateFrequencySec = 0.5f;

// TODO: This is a temporary solution, where we know the dem data stops at level X.
const int MaxDemLevels = 16;
const char *pDemTileServerAddress = "https://elevation3d.arcgis.com/arcgis/rest/services/WorldElevation3D/Terrain3D/ImageServer/tile/%d/%d/%d";
udUUID DemTileServerAddresUUID= {};

enum
{
  TileVertexResolution = 31 + 2, // +2 for skirt
  TileIndexResolution = (TileVertexResolution - 1),
};


// Mesh stitching configurations
int MeshUp = 1 << 0;
int MeshRight = 1 << 1;
int MeshDown = 1 << 2;
int MeshLeft = 1 << 3;

// build meshes
int MeshConfigurations[] =
{
  0,

  MeshUp,                              //  ^
  MeshRight,                           //   >
  MeshDown,                            //  .
  MeshLeft,                            // <

  MeshUp | MeshRight,                  //  ^>
  MeshUp | MeshDown,                   //  ^.
  MeshUp | MeshLeft,                   // <^

  MeshRight | MeshDown,                //  .>
  MeshRight | MeshLeft,                // < >

  MeshDown | MeshLeft,                 // <.

  MeshUp | MeshRight | MeshDown,       //  ^.>
  MeshUp | MeshLeft | MeshRight,       // <^>
  MeshUp | MeshLeft | MeshDown,        // <^.

  MeshDown | MeshLeft | MeshRight,     // <.>

  MeshUp | MeshLeft | MeshRight | MeshDown // <^.>
};

struct vcTileShader
{
  vcShader *pProgram;
  vcShaderConstantBuffer *pConstantBuffer;
  vcShaderSampler *uniform_texture;
  vcShaderSampler *uniform_dem;
  vcShaderSampler *uniform_normal;

  struct
  {
    udFloat4x4 projectionMatrix;
    udFloat4x4 viewMatrix;
    udFloat4 eyePositions[vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution];
    udFloat4 colour;
    udFloat4 objectInfo; // objectId.x, tileSkirtLength
    udFloat4 uvOffsetScale;
    udFloat4 demUVOffsetScale;
    udFloat4 worldNormals[vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution];
    udFloat4 worldBitangents[vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution];
  } everyObject;
};

struct vcTileRenderer
{
  float frameDeltaTime;
  float totalTime;
  float generateTreeUpdateTimer;

  vcSettings *pSettings;
  vcQuadTree quadTree;

  vcMesh *pTileMeshes[udLengthOf(MeshConfigurations)];
  vcTexture *pEmptyTileTexture;
  vcTexture *pEmptyDemTileTexture;
  vcTexture *pEmptyNormalTexture;

  udDouble3 cameraPosition;
  double cameraDistanceToSurface;
  bool cameraIsUnderMapSurface;

  struct vcSortedNode
  {
    vcQuadTreeNode *pNode;
    double distToCameraSqr;
  };
  udChunkedArray<vcSortedNode> renderLists[vcMaxTileLayerCount];

  // cache textures
  struct vcTileCache
  {
    volatile bool keepLoading;
    udThread *pThreads[4];
    udSemaphore *pSemaphore;
    udMutex *pMutex;
    udChunkedArray<vcQuadTreeNode*> tileLoadList;
  } cache;

  vcTileShader presentShaders[2]; // base layer, sub layers
};

// This functionality here for now until the cache module is implemented
bool vcTileRenderer_TryWriteTile(const char *filename, void *pFileData, size_t fileLen)
{
  udFile *pFile = nullptr;
  if (udFile_Open(&pFile, filename, udFOF_Create | udFOF_Write) == udR_Success)
  {
    udFile_Write(pFile, pFileData, fileLen);
    udFile_Close(&pFile);
    return true;
  }

  return false;
}

// This functionality here for now. In the future will be migrated to udPlatformUtils.
udResult vcTileRenderer_CreateDirRecursive(const char *pFolderPath)
{
  udResult result = udR_Success;
  char *pMutableDirectoryPath = nullptr;

  UD_ERROR_NULL(pFolderPath, udR_InvalidParameter_);

  pMutableDirectoryPath = udStrdup(pFolderPath);
  UD_ERROR_NULL(pMutableDirectoryPath, udR_MemoryAllocationFailure);

  for (uint32_t i = 0;; ++i)
  {
    if (pMutableDirectoryPath[i] == '\0' || pMutableDirectoryPath[i] == '/' || pMutableDirectoryPath[i] == '\\')
    {
      pMutableDirectoryPath[i] = '\0';
      result = udCreateDir(pMutableDirectoryPath);

      // TODO: handle directories already existing
      // TODO: handle path not found
      //if (result != udR_Success)
      //  UD_ERROR_HANDLE();

      pMutableDirectoryPath[i] = pFolderPath[i];

      if (pMutableDirectoryPath[i] == '\0')
        break;
    }
  }

epilogue:
  udFree(pMutableDirectoryPath);

  return result;
}

// This functionality here for now until the cache module is implemented
bool vcTileRenderer_CacheHasData(const char *pLocalURL)
{
#if UDPLATFORM_EMSCRIPTEN
  udUnused(pLocalURL);
  return false;
#else
  return udFileExists(pLocalURL) == udR_Success;
#endif
}

void vcTileRenderer_CacheDataToDisk(const char *pFilename, void *pData, int64_t dataLength)
{
#if UDPLATFORM_EMSCRIPTEN
  udUnused(pFilename);
  udUnused(pData);
  udUnused(dataLength);
#else
  if (pData == nullptr || dataLength == 0)
    return;

  if (!vcTileRenderer_TryWriteTile(pFilename, pData, dataLength))
  {
    size_t index = 0;
    char localFolderPath[vcMaxPathLength] = {};
    udSprintf(localFolderPath, "%s", pFilename);
    if (udStrrchr(pFilename, "\\/", &index) != nullptr)
      localFolderPath[index] = '\0';

    if (vcTileRenderer_CreateDirRecursive(localFolderPath) == udR_Success)
      vcTileRenderer_TryWriteTile(pFilename, pData, dataLength);
  }
#endif
}

udResult vcTileRenderer_HandleTileDownload(vcNodeRenderInfo *pRenderNodeInfo, const char *pRemoteURL, const char *pLocalURL)
{
  udResult result;

  bool downloadingFromServer = false;
  void *pFileData = nullptr;
  int64_t fileLen = -1;
  int width = 0;
  int height = 0;
  int channelCount = 0;
  uint8_t *pData = nullptr;

  // check for file in local cache
  const char *pTileURL = pLocalURL;
  if (!vcTileRenderer_CacheHasData(pTileURL))
  {
    pTileURL = pRemoteURL;
    downloadingFromServer = true;
  }

  UD_ERROR_CHECK(udFile_Load(pTileURL, &pFileData, &fileLen));
  UD_ERROR_IF(fileLen == 0, udR_InternalError);

  if (udStrBeginsWithi((const char*)pFileData, "CntZImage"))
  {
    // 'null' tile
    UD_ERROR_IF(fileLen <= 67, udR_ObjectNotFound);

    LercNS::CntZImage image;
    LercNS::Byte *pBytes = (LercNS::Byte*)pFileData;

    UD_ERROR_IF(!image.read(&pBytes, 9999.0), udR_ImageLoadFailure);
    UD_ERROR_IF(image.getPixel(0, 0).cnt == 0, udR_ImageLoadFailure);

    width = image.getWidth();
    height = image.getHeight();

    float *pDataF = udAllocType(float, width * height, udAF_Zero);
    UD_ERROR_NULL(pDataF, udR_MemoryAllocationFailure);

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        LercNS::CntZ pixel =  image.getPixel(y, x);
        float demHeight = pixel.z * pixel.cnt;
        pDataF[y * width + x] = demHeight;
      }
    }

    pRenderNodeInfo->data.pData = pDataF;
  }
  else // assume its a standard image format
  {
    pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
    UD_ERROR_NULL(pData, udR_InternalError);

    pRenderNodeInfo->data.pData = udMemDup(pData, sizeof(uint32_t) * width * height, 0, udAF_None);
    stbi_image_free(pData);
  }

  pRenderNodeInfo->data.width = width;
  pRenderNodeInfo->data.height = height;

  result = udR_Success;
epilogue:

  if (downloadingFromServer && result == udR_Success)
    vcTileRenderer_CacheDataToDisk(pLocalURL, pFileData, fileLen);

  udFree(pFileData);
  return result;
}

template <typename T>
float vcTileRenderer_BilinearSample(T *pPixelData, const udFloat2 &sampleUV, int32_t width, int32_t height)
{
  static float HalfPixelOffset = -0.5f;
  udFloat2 uv = { (sampleUV[0] + HalfPixelOffset / width) * width,
                  (sampleUV[1] + HalfPixelOffset / height) * height };

  udFloat2 whole = udFloat2::create(udFloor(uv.x), udFloor(uv.y));
  udFloat2 rem = udFloat2::create(uv.x - whole.x, uv.y - whole.y);

  float maxWidth = width - 1.0f;
  float maxHeight = height - 1.0f;

  udFloat2 uvBL = udFloat2::create(udClamp(whole.x + 0.0f, 0.0f, maxWidth), udClamp(whole.y + 0.0f, 0.0f, maxHeight));
  udFloat2 uvBR = udFloat2::create(udClamp(whole.x + 1, 0.0f, maxWidth), udClamp(whole.y + 0, 0.0f, maxHeight));
  udFloat2 uvTL = udFloat2::create(udClamp(whole.x + 0, 0.0f, maxWidth), udClamp(whole.y + 1, 0.0f, maxHeight));
  udFloat2 uvTR = udFloat2::create(udClamp(whole.x + 1, 0.0f, maxWidth), udClamp(whole.y + 1, 0.0f, maxHeight));

  float pColourBL = (float)pPixelData[(int)(uvBL.x + uvBL.y * width)];
  float pColourBR = (float)pPixelData[(int)(uvBR.x + uvBR.y * width)];
  float pColourTL = (float)pPixelData[(int)(uvTL.x + uvTL.y * width)];
  float pColourTR = (float)pPixelData[(int)(uvTR.x + uvTR.y * width)];

  float colourT = udLerp(pColourTL, pColourTR, rem.x);
  float colourB = udLerp(pColourBL, pColourBR, rem.x);
  return udLerp(colourB, colourT, rem.y);
}

void vcTileRenderer_GenerateDEMAndNormalMaps(vcQuadTreeNode *pNode, void *pRawDemPixels)
{
  pNode->demMinMax[0] = 32767;
  pNode->demMinMax[1] = -32768;

  pNode->demHeightsCopySize.x = pNode->demInfo.data.width;
  pNode->demHeightsCopySize.y = pNode->demInfo.data.height;

  pNode->pDemHeightsCopy = udAllocType(float, pNode->demInfo.data.width * pNode->demInfo.data.height, udAF_None);
  for (int h = 0; h < pNode->demInfo.data.height; ++h)
  {
    for (int w = 0; w < pNode->demInfo.data.width; ++w)
    {
      int index = h * pNode->demInfo.data.width + w;
      float height = ((float*)pRawDemPixels)[index];

      pNode->demMinMax[0] = udMin(pNode->demMinMax.x, height);
      pNode->demMinMax[1] = udMax(pNode->demMinMax.y, height);
      pNode->pDemHeightsCopy[index] = height;
    }
  }

  // And now generate normals
  pNode->normalInfo.data.width = pNode->demInfo.data.width;
  pNode->normalInfo.data.height = pNode->demInfo.data.height;

  // Calculate the horizontal region distances covered by this node
  udFloat2 tileWorldSize = udFloat2::create((float)udMag3(pNode->worldBounds[vcQuadTreeNodeVertexResolution - 1] - pNode->worldBounds[0]), (float)udMag3(pNode->worldBounds[vcQuadTreeNodeVertexResolution * (vcQuadTreeNodeVertexResolution - 1)] - pNode->worldBounds[0]));

  float stepSize = 1.0f;
  udFloat2 stepOffset = udFloat2::create(stepSize / pNode->normalInfo.data.width, stepSize / pNode->normalInfo.data.height);

  udFloat2 offsets[] =
  {
    udFloat2::create( stepOffset.x, 0),
    udFloat2::create(0,             stepOffset.y),
    udFloat2::create(-stepOffset.x, 0),
    udFloat2::create(0,            -stepOffset.y),
  };

  uint32_t *pNormalPixels = udAllocType(uint32_t, pNode->normalInfo.data.width * pNode->normalInfo.data.height, udAF_Zero);
  for (int h = 0; h < pNode->normalInfo.data.height; ++h)
  {
    for (int w = 0; w < pNode->normalInfo.data.width; ++w)
    {
      int i0 = h * pNode->normalInfo.data.width + w;
      udFloat2 uv = udFloat2::create(float(w) / pNode->normalInfo.data.width, float(h) / pNode->demInfo.data.height);

      float h0 = vcTileRenderer_BilinearSample(pNode->pDemHeightsCopy, uv, pNode->normalInfo.data.width, pNode->normalInfo.data.height);
      udFloat3 p0 = udFloat3::create(0.0f, 0.0f, h0);

      udFloat3 n = udFloat3::zero();
      for (int e = 0; e < 2; ++e)
      {
        int e0 = e * 2;
        float h1 = vcTileRenderer_BilinearSample(pNode->pDemHeightsCopy, uv + offsets[e0], pNode->normalInfo.data.width, pNode->normalInfo.data.height);
        udFloat3 p1 = udFloat3::create(tileWorldSize.x * offsets[e0].x, tileWorldSize.y * offsets[e0].y, h1);

        int e1 = e0 + 1;
        float h2 = vcTileRenderer_BilinearSample(pNode->pDemHeightsCopy, uv + offsets[e1], pNode->normalInfo.data.width, pNode->normalInfo.data.height);
        udFloat3 p2 = udFloat3::create(tileWorldSize.x * offsets[e1].x, tileWorldSize.y * offsets[e1].y, h2);

        n += udNormalize3(udCross(p1 - p0, p2 - p0));
      }
      n = udNormalize3(n);

      int nx = (int)(((n.x * 0.5f) + 0.5f) * 255);
      int ny = (int)(((n.y * 0.5f) + 0.5f) * 255);
      int nz = (int)(((n.z * 0.5f) + 0.5f) * 255);
      pNormalPixels[i0] = nx | (ny << 8) | (nz << 16) | (0xff000000);
    }
  }

  // TODO: (AB#1751)
  pNode->demInfo.data.pData = pRawDemPixels;
  pNode->normalInfo.data.pData = pNormalPixels;
}

uint32_t vcTileRenderer_LoadThread(void *pThreadData)
{
  vcTileRenderer *pRenderer = (vcTileRenderer*)pThreadData;
  vcTileRenderer::vcTileCache *pCache = &pRenderer->cache;

  while (pCache->keepLoading)
  {
    int loadStatus = udWaitSemaphore(pCache->pSemaphore, 1000);

    if (loadStatus != 0 && pCache->tileLoadList.length == 0)
      continue;

    while (pCache->tileLoadList.length > 0 && pCache->keepLoading)
    {
      udLockMutex(pCache->pMutex);

      // TODO: Store in priority order and recalculate on insert/delete
      int best = -1;
      vcQuadTreeNode *pNode = nullptr;
      double bestDistancePrioritySqr = FLT_MAX;

      for (int i = 0; i < (int)pCache->tileLoadList.length; ++i)
      {
        pNode = pCache->tileLoadList[i];

        bool tryLoadPayload = false;
        for (int j = 0; j < pRenderer->pSettings->maptiles.activeLayerCount; ++j)
          tryLoadPayload = tryLoadPayload || pNode->pPayloads[j].tryLoad;

        if ((!tryLoadPayload && !pNode->demInfo.tryLoad) || !pNode->touched)
          continue;

        double distanceToCameraSqr = udMagSq3(pNode->tileCenter - pRenderer->cameraPosition);

        bool betterNode = true;
        if (best != -1)
        {
          vcQuadTreeNode *pBestNode = pCache->tileLoadList[best];

          // priorities: visibility > failed to render visible area > distance
          betterNode = pNode->visible && !pBestNode->visible;
          if (pNode->visible == pBestNode->visible)
          {
            betterNode = !pNode->completeRender && pBestNode->completeRender;
            if (pNode->completeRender == pBestNode->completeRender)
            {
              // TODO: Factor in incomplete DEM renders
              betterNode = distanceToCameraSqr < bestDistancePrioritySqr;
            }
          }
        }

        if (betterNode)
        {
          bestDistancePrioritySqr = distanceToCameraSqr;
          best = int(i);
        }
      }

      if (best == -1)
      {
        udReleaseMutex(pCache->pMutex);
        break;
      }

      vcQuadTreeNode *pBestNode = pCache->tileLoadList[best];
      pCache->tileLoadList.RemoveSwapLast(best);

      // Perform these checks safely
      bool doDemRequest = pBestNode->demInfo.loadStatus.TestAndSet(vcNodeRenderInfo::vcTLS_Downloading, vcNodeRenderInfo::vcTLS_InQueue);
      bool doColourRequest[vcMaxTileLayerCount] = {};
      for (int mapLayer = 0; mapLayer < pRenderer->pSettings->maptiles.activeLayerCount; ++mapLayer)
        doColourRequest[mapLayer] = pBestNode->pPayloads[mapLayer].loadStatus.TestAndSet(vcNodeRenderInfo::vcTLS_Downloading, vcNodeRenderInfo::vcTLS_InQueue);

      udReleaseMutex(pCache->pMutex);

      char localURL[vcMaxPathLength] = {};
      char serverURL[vcMaxPathLength] = {};

      char xSlippyStr[16] = {};
      char ySlippyStr[16] = {};
      char zSlippyStr[16] = {};
      const char *pSlippyStrs[] = { zSlippyStr, xSlippyStr, ySlippyStr };

      // process dem and/or colour request
      if (doDemRequest)
      {
        // note: z, y, x for this server
        udSprintf(localURL, "%s/%s/%d/%d/%d.png", pRenderer->pSettings->cacheAssetPath, udUUID_GetAsString(DemTileServerAddresUUID), pBestNode->slippyPosition.z, pBestNode->slippyPosition.y, pBestNode->slippyPosition.x);
        udSprintf(serverURL, pDemTileServerAddress, pBestNode->slippyPosition.z, pBestNode->slippyPosition.y, pBestNode->slippyPosition.x);

        // allow continue on failure
        udResult demResult = vcTileRenderer_HandleTileDownload(&pBestNode->demInfo, serverURL, localURL);

        // Decode DEM and generate normals
        if (demResult == udR_Success)
        {
          vcTileRenderer_GenerateDEMAndNormalMaps(pBestNode, pBestNode->demInfo.data.pData);
          pBestNode->demInfo.loadStatus.Set(vcNodeRenderInfo::vcTLS_Downloaded);
        }
        else
        {
          pBestNode->demInfo.loadStatus.Set((demResult == udR_ObjectNotFound) ? vcNodeRenderInfo::vcTLS_NotAvailable : vcNodeRenderInfo::vcTLS_Failed);
          pBestNode->demInfo.timeoutTime = 0.0f;
          ++pBestNode->demInfo.loadRetryCount;
        }
      }

      for (int mapLayer = 0; mapLayer < pRenderer->pSettings->maptiles.activeLayerCount; ++mapLayer)
      {
        if (!doColourRequest[mapLayer])
          continue;

        udSprintf(localURL, "%s/%s/%d/%d/%d.png", pRenderer->pSettings->cacheAssetPath, udUUID_GetAsString(pRenderer->pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddressUUID), pBestNode->slippyPosition.z, pBestNode->slippyPosition.x, pBestNode->slippyPosition.y);

        udStrItoa(xSlippyStr, pBestNode->slippyPosition.x);
        udStrItoa(ySlippyStr, pBestNode->slippyPosition.y);
        udStrItoa(zSlippyStr, pBestNode->slippyPosition.z);

        vcStringFormat(serverURL, udLengthOf(serverURL), pRenderer->pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddress, pSlippyStrs, udLengthOf(pSlippyStrs));

        // allow continue on failure
        udResult colourResult = vcTileRenderer_HandleTileDownload(&pBestNode->pPayloads[mapLayer], serverURL, localURL);
        if (colourResult == udR_Success)
        {
          pBestNode->pPayloads[mapLayer].loadStatus.Set(vcNodeRenderInfo::vcTLS_Downloaded);
        }
        else
        {
          pBestNode->pPayloads[mapLayer].loadStatus.Set(vcNodeRenderInfo::vcTLS_Failed);
          pBestNode->pPayloads[mapLayer].timeoutTime = 0.0f;
          ++pBestNode->pPayloads[mapLayer].loadRetryCount;
        }
      }
    }
  }

  return 0;
}

void vcTileRenderer_BuildMeshVertices(vcP3Vertex *pVerts, int *pIndicies, udFloat2 minUV, udFloat2 maxUV, int collapseEdgeMask)
{
  for (int y = 0; y < TileIndexResolution; ++y)
  {
    for (int x = 0; x < TileIndexResolution; ++x)
    {
      int index = y * TileIndexResolution + x;
      int vertIndex = y * TileVertexResolution + x;

      pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution;
      pIndicies[index * 6 + 1] = vertIndex + 1;
      pIndicies[index * 6 + 2] = vertIndex;

      pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution;
      pIndicies[index * 6 + 4] = vertIndex + TileVertexResolution + 1;
      pIndicies[index * 6 + 5] = vertIndex + 1;

      // corner cases
      if ((collapseEdgeMask & MeshDown) && (collapseEdgeMask & MeshRight) && x >= (TileIndexResolution - 2) && y >= (TileIndexResolution - 2))
      {
        if (x == TileIndexResolution - 2 && y == TileIndexResolution - 2)
        {
          // do nothing
        }
        else if (x == TileIndexResolution - 1 && y == TileIndexResolution - 2)
        {
          // collapse
          pIndicies[index * 6 + 3] = vertIndex;
          pIndicies[index * 6 + 4] = vertIndex;
          pIndicies[index * 6 + 5] = vertIndex;
        }
        else if (x == TileIndexResolution - 2 && y == TileIndexResolution - 1)
        {
          // collapse
          pIndicies[index * 6 + 3] = vertIndex;
          pIndicies[index * 6 + 4] = vertIndex;
          pIndicies[index * 6 + 5] = vertIndex;
        }
        else // x == 1 && y == 1
        {
          // collapse both
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution - 1;
          pIndicies[index * 6 + 1] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 2] = vertIndex;

          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 4] = vertIndex - TileVertexResolution + 1;
          pIndicies[index * 6 + 5] = vertIndex;
        }
      }
      else if ((collapseEdgeMask & MeshDown) && (collapseEdgeMask & MeshLeft) && x <= 1 && y >= (TileIndexResolution - 2))
      {
        if (x == 0 && y == TileIndexResolution - 2)
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex + 1;

          // re-orient
          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 4] = vertIndex + 1;
          pIndicies[index * 6 + 5] = vertIndex;
        }
        else if (x == 1 && y == TileIndexResolution - 2)
        {
          // do nothing
        }
        else if (x == 0 && y == TileIndexResolution - 1)
        {
          // re-orient
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 1] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex - TileVertexResolution;

          // collapse
          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 4] = vertIndex + TileVertexResolution + 2;
          pIndicies[index * 6 + 5] = vertIndex + 1;
        }
        else // x == 1 && y == 1
        {
          // collapse
          pIndicies[index * 6 + 1] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 2] = vertIndex + TileVertexResolution;

          // re-orient
          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 4] = vertIndex + 1;
          pIndicies[index * 6 + 5] = vertIndex;
        }
      }
      else if ((collapseEdgeMask & MeshUp) && (collapseEdgeMask & MeshRight) && x >= (TileIndexResolution - 2) && y <= 1)
      {
        if (x == TileIndexResolution - 2 && y == 0)
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex;
          pIndicies[index * 6 + 1] = vertIndex;
          pIndicies[index * 6 + 2] = vertIndex;

          // re-orient triangle
          pIndicies[index * 6 + 5] = vertIndex;
        }
        else if (x == TileIndexResolution - 1 && y == 0)
        {
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 1] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex - 1;

          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 4] = vertIndex + TileVertexResolution + TileVertexResolution + 1;
          pIndicies[index * 6 + 5] = vertIndex + 1;
        }
        else if (x == TileIndexResolution - 2 && y == 1)
        {
          // do nothing
        }
        else // x == TileIndexResolution -1 && y == 1
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex;
          pIndicies[index * 6 + 1] = vertIndex;
          //pIndicies[index * 6 + 2] = vertIndex;

          // re-orient triangle
          pIndicies[index * 6 + 5] = vertIndex;
        }
      }
      else if ((collapseEdgeMask & MeshUp) && (collapseEdgeMask & MeshLeft) && x <= 1 && y <= 1)
      {
        if (x == 0 && y == 0)
        {
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 1] = vertIndex + 2;
          pIndicies[index * 6 + 2] = vertIndex;

          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution + TileVertexResolution;
          pIndicies[index * 6 + 4] = vertIndex + TileVertexResolution + 1;
          pIndicies[index * 6 + 5] = vertIndex;
        }
        else if (x == 1 && y == 0)
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex + 1;
        }
        else if (x == 0 && y == 1)
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex + 1;
        }
        else // x==1, y == 1
        {
          // do nothing
        }
      }
      else if (y == 0 && (collapseEdgeMask & MeshUp))
      {
        if ((x & 1) == 0)
        {
          pIndicies[index * 6 + 1] = vertIndex + 2;

          pIndicies[index * 6 + 5] = vertIndex + 2;
        }
        else
        {
          // collapse
          pIndicies[index * 6 + 0] = vertIndex + 1;
          pIndicies[index * 6 + 2] = vertIndex + 1;
        }
      }
      else if (y == TileIndexResolution - 1 && (collapseEdgeMask & MeshDown))
      {
        if ((x & 1) == 0)
        {
          // collapse
          pIndicies[index * 6 + 4] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 5] = vertIndex + TileVertexResolution;
        }
        else
        {
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution - 1;

          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution - 1;
        }
      }
      else if (x == TileIndexResolution - 1 && (collapseEdgeMask & MeshRight))
      {
        if ((y & 1) == 0)
        {
          // collapse
          pIndicies[index * 6 + 3] = vertIndex + 1;
          pIndicies[index * 6 + 4] = vertIndex + 1;
        }
        else
        {
          pIndicies[index * 6 + 1] = vertIndex - TileVertexResolution + 1;

          pIndicies[index * 6 + 5] = vertIndex - TileVertexResolution + 1;
        }
      }
      else if (x == 0 && (collapseEdgeMask & MeshLeft))
      {
        if ((y & 1) == 0)
        {
          pIndicies[index * 6 + 0] = vertIndex + TileVertexResolution + TileVertexResolution;

          pIndicies[index * 6 + 3] = vertIndex + TileVertexResolution + TileVertexResolution;
        }
        else
        {
          // collapse
          pIndicies[index * 6 + 1] = vertIndex + TileVertexResolution;
          pIndicies[index * 6 + 2] = vertIndex + TileVertexResolution;
        }
      }
    }
  }

  // account for 'skirt'
  float normalizeVertexPositionScale = float((TileVertexResolution - 2)) / ((TileVertexResolution - 2) -1); // ensure verts are [0, 1]

  for (int y = 0; y < TileVertexResolution; ++y)
  {
    for (int x = 0; x < TileVertexResolution; ++x)
    {
      uint32_t index = y * TileVertexResolution + x;
      pVerts[index].position.z = 0.0f;

      // account for 'skirt'
      float normX = ((float)udMax(0, x - 1) / (TileVertexResolution - 2)) * normalizeVertexPositionScale;
      float normY = ((float)udMax(0, y - 1) / (TileVertexResolution - 2)) * normalizeVertexPositionScale;

      // handle skirts
      if (x == 0 || x == TileVertexResolution - 1)
      {
        pVerts[index].position.z = -1;
        normX = (x == 0) ? 0.0f : 1.0f;
      }

      if (y == 0 || y == TileVertexResolution - 1)
      {
        pVerts[index].position.z = -1;
        normY = (y == 0) ? 0.0f : 1.0f;
      }

      pVerts[index].position.x = minUV.x + normX * (maxUV.x - minUV.x);
      pVerts[index].position.y = minUV.y + normY * (maxUV.y - minUV.y);
    }
  }
}

udResult vcTileRenderer_Create(vcTileRenderer **ppTileRenderer, udWorkerPool *pWorkerPool, vcSettings *pSettings)
{
  udResult result;
  vcTileRenderer *pTileRenderer = nullptr;
  vcP3Vertex verts[TileVertexResolution * TileVertexResolution] = {};
  int indicies[TileIndexResolution * TileIndexResolution * 6] = {};
  uint32_t greyPixel = 0x00f3f3f3; // abgr
  uint16_t flatDemPixel = 0x8000;
  uint32_t flatNormalPixel = 0xffff7f7f;
  UD_ERROR_NULL(ppTileRenderer, udR_InvalidParameter_);

  pTileRenderer = udAllocType(vcTileRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pTileRenderer, udR_MemoryAllocationFailure);

  pTileRenderer->generateTreeUpdateTimer = QuadTreeUpdateFrequencySec;

  UD_ERROR_CHECK(udUUID_GenerateFromString(&DemTileServerAddresUUID, pDemTileServerAddress));

  vcQuadTree_Create(&pTileRenderer->quadTree, pSettings);

  pTileRenderer->pSettings = pSettings;

  pTileRenderer->cache.pSemaphore = udCreateSemaphore();
  pTileRenderer->cache.pMutex = udCreateMutex();
  pTileRenderer->cache.keepLoading = true;
  pTileRenderer->cache.tileLoadList.Init(256);

  for (int i = 0; i < vcMaxTileLayerCount; ++i)
    pTileRenderer->renderLists[i].Init(128);

  for (size_t i = 0; i < udLengthOf(pTileRenderer->cache.pThreads); ++i)
    UD_ERROR_CHECK(udThread_Create(&pTileRenderer->cache.pThreads[i], vcTileRenderer_LoadThread, pTileRenderer));

  UD_ERROR_CHECK(vcTileRenderer_ReloadShaders(pTileRenderer, pWorkerPool));

  // build mesh variants
  for (size_t i = 0; i < udLengthOf(MeshConfigurations); ++i)
  {
    vcTileRenderer_BuildMeshVertices(verts, indicies, udFloat2::create(0.0f, 0.0f), udFloat2::create(1.0f, 1.0f), MeshConfigurations[i]);
    vcMesh_Create(&pTileRenderer->pTileMeshes[i], vcP3VertexLayout, (int)udLengthOf(vcP3VertexLayout), verts, TileVertexResolution * TileVertexResolution, indicies, TileIndexResolution * TileIndexResolution * 6);
  }

  UD_ERROR_CHECK(vcTexture_Create(&pTileRenderer->pEmptyTileTexture, 1, 1, &greyPixel));
  UD_ERROR_CHECK(vcTexture_Create(&pTileRenderer->pEmptyDemTileTexture, 1, 1, &flatDemPixel, vcTextureFormat_RG8));
  UD_ERROR_CHECK(vcTexture_Create(&pTileRenderer->pEmptyNormalTexture, 1, 1, &flatNormalPixel));

  *ppTileRenderer = pTileRenderer;
  pTileRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pTileRenderer)
    vcTileRenderer_Destroy(&pTileRenderer);

  return result;
}

void vcTileRenderer_DestroyShaders(vcTileRenderer *pTileRenderer)
{
  for (size_t i = 0; i < udLengthOf(pTileRenderer->presentShaders); ++i)
  {
    vcShader_ReleaseConstantBuffer(pTileRenderer->presentShaders[i].pProgram, pTileRenderer->presentShaders[i].pConstantBuffer);
    vcShader_DestroyShader(&(pTileRenderer->presentShaders[i].pProgram));
  }
}

udResult vcTileRenderer_Destroy(vcTileRenderer **ppTileRenderer)
{
  if (ppTileRenderer == nullptr || *ppTileRenderer == nullptr)
    return udR_InvalidParameter_;

  vcTileRenderer *pTileRenderer = *ppTileRenderer;

  pTileRenderer->cache.keepLoading = false;

  for (size_t i = 0; i < udLengthOf(pTileRenderer->cache.pThreads); ++i)
    udIncrementSemaphore(pTileRenderer->cache.pSemaphore);

  for (size_t i = 0; i < udLengthOf(pTileRenderer->cache.pThreads); ++i)
  {
    udThread_Join(pTileRenderer->cache.pThreads[i]);
    udThread_Destroy(&pTileRenderer->cache.pThreads[i]);
  }

  udDestroyMutex(&pTileRenderer->cache.pMutex);
  udDestroySemaphore(&pTileRenderer->cache.pSemaphore);

  pTileRenderer->cache.tileLoadList.Deinit();

  vcTileRenderer_DestroyShaders(pTileRenderer);

  for (size_t i = 0; i < udLengthOf(MeshConfigurations); ++i)
    vcMesh_Destroy(&pTileRenderer->pTileMeshes[i]);
  vcTexture_Destroy(&pTileRenderer->pEmptyTileTexture);
  vcTexture_Destroy(&pTileRenderer->pEmptyDemTileTexture);
  vcTexture_Destroy(&pTileRenderer->pEmptyNormalTexture);

  for (int i = 0; i < vcMaxTileLayerCount; ++i)
    pTileRenderer->renderLists[i].Deinit();

  vcQuadTree_Destroy(&(*ppTileRenderer)->quadTree);
  udFree(*ppTileRenderer);
  *ppTileRenderer = nullptr;

  return udR_Success;
}

udResult vcTileRenderer_ReloadShaders(vcTileRenderer *pTileRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcTileRenderer_DestroyShaders(pTileRenderer);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pTileRenderer->presentShaders[0].pProgram, pWorkerPool, "asset://assets/shaders/tileVertexShader", "asset://assets/shaders/tileFragmentShader", vcP3VertexLayout,
    [pTileRenderer](void *)
    {
      vcShader_Bind(pTileRenderer->presentShaders[0].pProgram);
      vcShader_GetConstantBuffer(&pTileRenderer->presentShaders[0].pConstantBuffer, pTileRenderer->presentShaders[0].pProgram, "u_EveryObject", sizeof(pTileRenderer->presentShaders[0].everyObject));
      vcShader_GetSamplerIndex(&pTileRenderer->presentShaders[0].uniform_texture, pTileRenderer->presentShaders[0].pProgram, "colour");
      vcShader_GetSamplerIndex(&pTileRenderer->presentShaders[0].uniform_normal, pTileRenderer->presentShaders[0].pProgram, "normal");
      vcShader_GetSamplerIndex(&pTileRenderer->presentShaders[0].uniform_dem, pTileRenderer->presentShaders[0].pProgram, "dem");
    }
  ), udR_InternalError);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pTileRenderer->presentShaders[1].pProgram, pWorkerPool, "asset://assets/shaders/tileVertexShader", "asset://assets/shaders/tileLayerFragmentShader", vcP3VertexLayout,
    [pTileRenderer](void *)
    {
      vcShader_Bind(pTileRenderer->presentShaders[1].pProgram);
      vcShader_GetConstantBuffer(&pTileRenderer->presentShaders[1].pConstantBuffer, pTileRenderer->presentShaders[1].pProgram, "u_EveryObject", sizeof(pTileRenderer->presentShaders[1].everyObject));
      vcShader_GetSamplerIndex(&pTileRenderer->presentShaders[1].uniform_texture, pTileRenderer->presentShaders[1].pProgram, "colour");
      vcShader_GetSamplerIndex(&pTileRenderer->presentShaders[1].uniform_dem, pTileRenderer->presentShaders[1].pProgram, "dem");
    }
  ), udR_InternalError);

  result = udR_Success;
epilogue:

  return result;
}

bool vcTileRenderer_IsNodeInQueue(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode)
{
  for (int layer = 0; layer < pTileRenderer->pSettings->maptiles.activeLayerCount; ++layer)
  {
    if (pNode->pPayloads[layer].loadStatus.Get() == vcNodeRenderInfo::vcTLS_InQueue)
      return true;
  }

  return pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_InQueue;
}

void vcTileRenderer_UpdateDemState(vcQuadTree *pQuadTree, vcQuadTreeNode *pNode, const vcQuadTreeNode *pInheritNode)
{
  if (pNode->demBoundsState == vcQuadTreeNode::vcDemBoundsState_Absolute)
    return;

  if (pNode->demBoundsState == vcQuadTreeNode::vcDemBoundsState_None)
    pNode->demMinMax = { 32767, -32768 };

  pNode->demBoundsState = vcQuadTreeNode::vcDemBoundsState_Inherited;
  pNode->demMinMax[0] = udMin(pNode->demMinMax[0], pInheritNode->demMinMax[0]);
  pNode->demMinMax[1] = udMax(pNode->demMinMax[1], pInheritNode->demMinMax[1]);

  vcQuadTree_CalculateNodeAABB(pQuadTree, pNode);
}

void vcTileRenderer_RecursiveDownUpdateNodeAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pParentNode, vcQuadTreeNode *pChildNode)
{
  if (pParentNode != nullptr)
    vcTileRenderer_UpdateDemState(pQuadTree, pChildNode, pParentNode);

  if (!vcQuadTree_IsLeafNode(pChildNode))
  {
    for (int c = 0; c < 4; ++c)
      vcTileRenderer_RecursiveDownUpdateNodeAABB(pQuadTree, pChildNode, &pQuadTree->nodes.pPool[pChildNode->childBlockIndex + c]);
  }
}

void vcTileRenderer_RecursiveUpUpdateNodeAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pChildNode)
{
  if (pChildNode->parentIndex == INVALID_NODE_INDEX)
    return;

  vcQuadTreeNode *pParentNode = &pQuadTree->nodes.pPool[pChildNode->parentIndex];
  vcTileRenderer_UpdateDemState(pQuadTree, pParentNode, pChildNode);
  vcTileRenderer_RecursiveUpUpdateNodeAABB(pQuadTree, pParentNode);
}

void vcTileRenderer_UpdateTileDEMTexture(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, float *pUploadBudgetRemainingMS)
{
  vcTileRenderer::vcTileCache *pTileCache = &pTileRenderer->cache;
  bool queueTile = (pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_None);
  if (pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_Failed)
  {
    if (pNode->demInfo.loadRetryCount < TileFailedRetryCount)
    {
      pNode->demInfo.timeoutTime += pTileRenderer->frameDeltaTime;
      if (pNode->demInfo.timeoutTime >= TileTimeoutRetrySec)
        queueTile = true;
    }
    else
    {
      // flag it to allow parent attempt at getting it
      pNode->demInfo.loadStatus.Set(vcNodeRenderInfo::vcTLS_NotAvailable);
    }
  }

  if (queueTile)
  {
    // add if not already in queue
    if (!vcTileRenderer_IsNodeInQueue(pTileRenderer, pNode))
    {
      pTileCache->tileLoadList.PushBack(pNode);
      udIncrementSemaphore(pTileCache->pSemaphore);
    }

    pNode->demInfo.loadStatus.Set(vcNodeRenderInfo::vcTLS_InQueue);
    pNode->demInfo.data.pData = nullptr;
    pNode->demInfo.data.pTexture = nullptr;
  }

  pNode->demInfo.tryLoad = true;
  if (pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_Downloaded && *pUploadBudgetRemainingMS > 0.0f)
  {
    uint64_t uploadStartTime = udPerfCounterStart();

    pNode->demInfo.tryLoad = false;

    vcTexture_CreateAdv(&pNode->demInfo.data.pTexture, vcTextureType_Texture2D, pNode->demInfo.data.width, pNode->demInfo.data.height, 1, pNode->demInfo.data.pData, vcTextureFormat_R32F, vcTFM_Linear, false, vcTWM_Clamp);
    udFree(pNode->demInfo.data.pData);

    vcTexture_CreateAdv(&pNode->normalInfo.data.pTexture, vcTextureType_Texture2D, pNode->normalInfo.data.width, pNode->normalInfo.data.height, 1, pNode->normalInfo.data.pData, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Clamp);
    udFree(pNode->normalInfo.data.pData);

    pNode->demBoundsState = vcQuadTreeNode::vcDemBoundsState_Absolute;
    vcQuadTree_CalculateNodeAABB(&pTileRenderer->quadTree, pNode);

    // conditonal update AABBs of tree (up and down)
    vcTileRenderer_RecursiveDownUpdateNodeAABB(&pTileRenderer->quadTree, nullptr, pNode);
    vcTileRenderer_RecursiveUpUpdateNodeAABB(&pTileRenderer->quadTree, pNode);

    pNode->demInfo.loadStatus.Set(vcNodeRenderInfo::vcTLS_Loaded);

    *pUploadBudgetRemainingMS -= udPerfCounterMilliseconds(uploadStartTime);
  }
}

bool vcTileRenderer_UpdateTileTexture(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, float *pUploadBudgetRemainingMS)
{
  vcTileRenderer::vcTileCache *pTileCache = &pTileRenderer->cache;
  for (int layer = 0; layer < pTileRenderer->pSettings->maptiles.activeLayerCount; ++layer)
  {
    int32_t loadStatus = pNode->pPayloads[layer].loadStatus.Get();
    if (loadStatus == vcNodeRenderInfo::vcTLS_Loaded || pNode->slippyPosition.z >= pTileRenderer->pSettings->maptiles.layers[layer].maxDepth)
      continue;

    bool queueTile = (loadStatus == vcNodeRenderInfo::vcTLS_None);
    if (loadStatus == vcNodeRenderInfo::vcTLS_Failed && pNode->pPayloads[layer].loadRetryCount < TileFailedRetryCount)
    {
      pNode->pPayloads[layer].timeoutTime += pTileRenderer->frameDeltaTime;
      if (pNode->pPayloads[layer].timeoutTime >= TileTimeoutRetrySec)
        queueTile = true;
    }

    if (queueTile)
    {
      // add if not already in queue
      if (!vcTileRenderer_IsNodeInQueue(pTileRenderer, pNode))
      {
        pTileCache->tileLoadList.PushBack(pNode);
        udIncrementSemaphore(pTileCache->pSemaphore);
      }

      pNode->pPayloads[layer].loadStatus.Set(vcNodeRenderInfo::vcTLS_InQueue);

      pNode->pPayloads[layer].data.pData = nullptr;
      pNode->pPayloads[layer].data.pTexture = nullptr;
    }

    pNode->pPayloads[layer].tryLoad = true;
    if (loadStatus == vcNodeRenderInfo::vcTLS_Downloaded && *pUploadBudgetRemainingMS > 0.0f)
    {
      uint64_t uploadStartTime = udPerfCounterStart();

      pNode->pPayloads[layer].tryLoad = false;

      vcTexture_CreateAdv(&pNode->pPayloads[layer].data.pTexture, vcTextureType_Texture2D, pNode->pPayloads[layer].data.width, pNode->pPayloads[layer].data.height, 1, pNode->pPayloads[layer].data.pData, vcTextureFormat_RGBA8, vcTFM_Linear, true, vcTWM_Clamp, vcTCF_None, 16);
      udFree(pNode->pPayloads[layer].data.pData);

      pNode->pPayloads[layer].loadStatus.Set(vcNodeRenderInfo::vcTLS_Loaded);

      *pUploadBudgetRemainingMS -= udPerfCounterMilliseconds(uploadStartTime);
      return true;
    }
  }

  return false;
}

void vcTileRenderer_UpdateTextureQueuesRecursive(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, float *pUploadBudgetRemainingMS)
{
  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
      vcTileRenderer_UpdateTextureQueuesRecursive(pTileRenderer, &pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c], pUploadBudgetRemainingMS);
  }

  if (vcQuadTree_IsVisibleLeafNode(&pTileRenderer->quadTree, pNode))
  {
    for (int j = 0; j < pTileRenderer->pSettings->maptiles.activeLayerCount; ++j)
    {
      if (pNode->pPayloads[j].loadStatus.Get() != vcNodeRenderInfo::vcTLS_Loaded)
      {
        vcTileRenderer_UpdateTileTexture(pTileRenderer, pNode, pUploadBudgetRemainingMS);
        break;
      }
    }
  }

  if (pNode->demInfo.loadStatus.Get() != vcNodeRenderInfo::vcTLS_Loaded && pNode->demInfo.loadStatus.Get() != vcNodeRenderInfo::vcTLS_NotAvailable &&
      pTileRenderer->pSettings->maptiles.demEnabled && pNode->slippyPosition.z <= MaxDemLevels)
  {
    // If child, or if we're at the DEM limit
    bool loadDem = vcQuadTree_IsVisibleLeafNode(&pTileRenderer->quadTree, pNode) || (pNode->slippyPosition.z == MaxDemLevels);

    // However we may still want to load this tiles DEM - if a child was 'not available'.
    if (!loadDem && !vcQuadTree_IsLeafNode(pNode))
    {
      for (int c = 0; c < 4; ++c)
      {
        if (pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c].demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_NotAvailable)
        {
          loadDem = true;
          break;
        }
      }
    }

    if (loadDem)
      vcTileRenderer_UpdateTileDEMTexture(pTileRenderer, pNode, pUploadBudgetRemainingMS);

  }
}

void vcTileRenderer_UpdateTextureQueues(vcTileRenderer *pTileRenderer, bool *pIsLoading)
{
  // invalidate current queue
  for (size_t i = 0; i < pTileRenderer->cache.tileLoadList.length; ++i)
  {
    vcQuadTreeNode *pNode = pTileRenderer->cache.tileLoadList[i];
    for (int j = 0; j < pTileRenderer->pSettings->maptiles.activeLayerCount; ++j)
      pNode->pPayloads[j].tryLoad = false;

    pTileRenderer->cache.tileLoadList[i]->demInfo.tryLoad = false;
  }

  // Limit the frame time taken up by texture uploads
  float totalUploadTimeRemainingMS = 1.0f; // note: it can go above this

  // update visible tiles textures
  vcTileRenderer_UpdateTextureQueuesRecursive(pTileRenderer, &pTileRenderer->quadTree.nodes.pPool[pTileRenderer->quadTree.rootIndex], &totalUploadTimeRemainingMS);

  // remove from the queue any tiles that are invalid
  for (int i = 0; i < (int)pTileRenderer->cache.tileLoadList.length; ++i)
  {
    vcQuadTreeNode *pNode = pTileRenderer->cache.tileLoadList[i];

    *pIsLoading |= pNode->visible;

    bool tryLoad = false;
    for (int j = 0; j < pTileRenderer->pSettings->maptiles.activeLayerCount; ++j)
      tryLoad = tryLoad || pNode->pPayloads[j].tryLoad;

    if (!pNode->touched || (!tryLoad && !pNode->demInfo.tryLoad))
    {
      for (int j = 0; j < pTileRenderer->pSettings->maptiles.activeLayerCount; ++j)
        pNode->pPayloads[j].loadStatus.TestAndSet(vcNodeRenderInfo::vcTLS_None, vcNodeRenderInfo::vcTLS_InQueue);

      pNode->demInfo.loadStatus.TestAndSet(vcNodeRenderInfo::vcTLS_None, vcNodeRenderInfo::vcTLS_InQueue);
      pTileRenderer->cache.tileLoadList.RemoveSwapLast(i);
      --i;
    }
  }
}

void vcTileRenderer_Update(vcTileRenderer *pTileRenderer, const double deltaTime, udGeoZone *pGeozone, const udInt3 &slippyCoords, const vcCamera *pCamera, const udDouble4x4 &viewProjectionMatrix, bool *pIsLoading)
{
  pTileRenderer->frameDeltaTime = (float)deltaTime;
  pTileRenderer->totalTime += pTileRenderer->frameDeltaTime;
  pTileRenderer->cameraPosition = pCamera->position;
  pTileRenderer->cameraIsUnderMapSurface = pCamera->cameraIsUnderSurface;
  pTileRenderer->cameraDistanceToSurface = pCamera->heightAboveEarthSurface;

  vcQuadTreeViewInfo viewInfo =
  {
    pGeozone,
    slippyCoords,
    pCamera->position,
    pCamera->positionZeroAltitude,
    viewProjectionMatrix
  };

  vcQuadTree_UpdateView(&pTileRenderer->quadTree, pCamera, viewInfo.viewProjectionMatrix);

  pTileRenderer->generateTreeUpdateTimer += pTileRenderer->frameDeltaTime;
  if (pTileRenderer->generateTreeUpdateTimer >= QuadTreeUpdateFrequencySec || pTileRenderer->quadTree.geozone.srid != pGeozone->srid)
  {
    pTileRenderer->generateTreeUpdateTimer = 0.0;

    uint64_t startTime = udPerfCounterStart();
    vcQuadTree_Update(&pTileRenderer->quadTree, viewInfo);
    pTileRenderer->quadTree.metaData.generateTimeMs = udPerfCounterMilliseconds(startTime);
  }

  udLockMutex(pTileRenderer->cache.pMutex);
  vcTileRenderer_UpdateTextureQueues(pTileRenderer, pIsLoading);
  udReleaseMutex(pTileRenderer->cache.pMutex);
}

void vcTileRenderer_DrawNode(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, vcTileShader *pShader, const udDouble4x4 &view, int layer, vcMesh *pMesh)
{
  vcNodeRenderInfo *pDrawInfo = &pNode->pPayloads[layer];
  vcTexture *pTexture = pDrawInfo->drawInfo.pTexture;
  if (pTexture == nullptr)
  {
    pNode->completeRender = false;
    pTexture = pTileRenderer->pEmptyTileTexture;
  }

  vcTexture *pDemTexture = pNode->demInfo.drawInfo.pTexture;
  vcTexture *pNormalTexture = pNode->normalInfo.drawInfo.pTexture;
  if (pDemTexture == nullptr || !pTileRenderer->pSettings->maptiles.demEnabled)
  {
    // TODO: completeRender = false?
    pDemTexture = pTileRenderer->pEmptyDemTileTexture;
    pNormalTexture = pTileRenderer->pEmptyNormalTexture;
  }

  for (int t = 0; t < vcQuadTreeNodeVertexResolution * vcQuadTreeNodeVertexResolution; ++t)
  {
    udDouble3 mapHeightOffset = pNode->worldNormals[t] * udDouble3::create(pTileRenderer->pSettings->maptiles.layers[layer].mapHeight);
    udDouble3 worldPos = pNode->worldBounds[t] + mapHeightOffset;
    udDouble4 eyePosition = view * udDouble4::create(worldPos, 1.0);
    udDouble3 normal = pNode->worldNormals[t];
    if (pTileRenderer->quadTree.geozone.projection == udGZPT_ECEF)
      normal = udNormalize(worldPos);

    pShader->everyObject.eyePositions[t] = udFloat4::create(eyePosition);

    // store the distance in the here (this will be used to generate the globe)
    pShader->everyObject.eyePositions[t].w = (float)udMag3(worldPos);

    pShader->everyObject.worldNormals[t] = udFloat4::create(udFloat3::create(normal), 0.0f);
    pShader->everyObject.worldBitangents[t] = udFloat4::create(udFloat3::create(pNode->worldBitangents[t]), 0.0f);
  }

  udFloat2 size = pDrawInfo->drawInfo.uvEnd - pDrawInfo->drawInfo.uvStart;
  pShader->everyObject.uvOffsetScale = udFloat4::create(pDrawInfo->drawInfo.uvStart, size.x, size.y);

  udFloat2 demSize = pNode->demInfo.drawInfo.uvEnd - pNode->demInfo.drawInfo.uvStart;
  pShader->everyObject.demUVOffsetScale = udFloat4::create(pNode->demInfo.drawInfo.uvStart, demSize.x, demSize.y);

  uint16_t samplerIndex = 0;
  vcShader_BindTexture(pShader->pProgram, pTexture, samplerIndex++, pShader->uniform_texture);
  if (layer == 0)
    vcShader_BindTexture(pShader->pProgram, pNormalTexture, samplerIndex++, pShader->uniform_normal);

  // TODO: This is a hack, needs to be fixed for both graphics APIs
#if !GRAPHICS_API_OPENGL
  samplerIndex = 0;
#endif
  vcShader_BindTexture(pShader->pProgram, pDemTexture, samplerIndex, pShader->uniform_dem, vcGLSamplerShaderStage_Vertex);

  vcShader_BindConstantBuffer(pShader->pProgram, pShader->pConstantBuffer, &pShader->everyObject, sizeof(pShader->everyObject));

  vcMesh_Render(pMesh, TileIndexResolution * TileIndexResolution * 2); // 2 tris per quad

  //pNode->rendered = true;
  ++pTileRenderer->quadTree.metaData.nodeRenderCount;
}

void vcTileRenderer_DrapeColour(vcQuadTreeNode *pChild, vcQuadTreeNode *pAncestor, int layer)
{
  vcNodeRenderInfo *pDrawInfo = &pChild->pPayloads[layer];
  pDrawInfo->drawInfo.uvStart = udFloat2::zero();
  pDrawInfo->drawInfo.uvEnd = udFloat2::one();
  if (pAncestor != nullptr && pAncestor != pChild)
  {
    // calculate what portion of ancestors colour to display at this tile
    pDrawInfo->drawInfo.pTexture = pAncestor->pPayloads[layer].drawInfo.pTexture;
    int depthDiff = pChild->slippyPosition.z - pAncestor->slippyPosition.z;
    int slippyRange = (int)udPow(2.0f, (float)depthDiff);
    udFloat2 boundsRange = udFloat2::create((float)slippyRange);

    // top-left, and bottom-right corners
    udInt2 slippy0 = pChild->slippyPosition.toVector2();
    udInt2 slippy1 = slippy0 + udInt2::one();

    udInt2 ancestorSlippyLocal0 = udInt2::create(pAncestor->slippyPosition.x * slippyRange, pAncestor->slippyPosition.y * slippyRange);
    udInt2 ancestorSlippyLocal1 = ancestorSlippyLocal0 + udInt2::create(slippyRange);

    pDrawInfo->drawInfo.uvStart = udFloat2::create(slippy0 - ancestorSlippyLocal0) / boundsRange;
    pDrawInfo->drawInfo.uvEnd = udFloat2::one() - (udFloat2::create(ancestorSlippyLocal1 - slippy1) / boundsRange);
  }
}

void vcTileRenderer_DrapeDEM(vcQuadTreeNode *pChild, vcQuadTreeNode *pAncestor)
{
  pChild->demInfo.drawInfo.uvStart = udFloat2::zero();
  pChild->demInfo.drawInfo.uvEnd = udFloat2::one();
  if (pAncestor != nullptr && pAncestor != pChild)
  {
    // calculate what portion of ancestors DEM to display at this tile
    pChild->demInfo.drawInfo.pTexture = pAncestor->demInfo.drawInfo.pTexture;
    pChild->normalInfo.drawInfo.pTexture = pAncestor->normalInfo.drawInfo.pTexture;
    int depthDiff = pChild->slippyPosition.z - pAncestor->slippyPosition.z;
    int slippyRange = (int)udPow(2.0f, (float)depthDiff);
    udFloat2 boundsRange = udFloat2::create((float)slippyRange);

    // top-left, and bottom-right corners
    udInt2 slippy0 = pChild->slippyPosition.toVector2();
    udInt2 slippy1 = slippy0 + udInt2::one();

    udInt2 ancestorSlippyLocal0 = udInt2::create(pAncestor->slippyPosition.x * slippyRange, pAncestor->slippyPosition.y * slippyRange);
    udInt2 ancestorSlippyLocal1 = ancestorSlippyLocal0 + udInt2::create(slippyRange);

    pChild->demInfo.drawInfo.uvStart = udFloat2::create(slippy0 - ancestorSlippyLocal0) / boundsRange;
    pChild->demInfo.drawInfo.uvEnd = udFloat2::one() - (udFloat2::create(ancestorSlippyLocal1 - slippy1) / boundsRange);
  }
}

// Sorted by distance to camera
void vcTileRenderer_InsertRenderListSorted(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, int layer)
{
  double distToCameraSqr = udMagSq3(pNode->tileCenter - pTileRenderer->cameraPosition);
  vcTileRenderer::vcSortedNode queuedNode = { pNode, distToCameraSqr };

  udChunkedArray<vcTileRenderer::vcSortedNode> *pRenderList = &pTileRenderer->renderLists[layer];

  size_t insertIndex = 0;
  for (; insertIndex < pRenderList->length; ++insertIndex)
  {
    if (distToCameraSqr < pRenderList->GetElement(insertIndex)->distToCameraSqr)
    {
      pRenderList->Insert(insertIndex, &queuedNode);
      break;
    }
  }

  if (insertIndex == pRenderList->length)
    pRenderList->PushBack(queuedNode);
}

void vcTileRenderer_RecursiveBuildRenderList(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, int layer, vcQuadTreeNode *pBestTexturedAncestor, vcQuadTreeNode *pBestDemAncestor)
{
  if (layer == 0)
  {
    pNode->completeRender = true;

    // recalculate visibility here
    pNode->visible = vcQuadTree_IsNodeVisible(&pTileRenderer->quadTree, pNode);

    pNode->demInfo.drawInfo.pTexture = nullptr;
    pNode->normalInfo.drawInfo.pTexture = nullptr;
  }

  if (!pNode->visible && pNode->slippyPosition.z >= vcQuadTree_MinimumDescendLayer)
    return;

  pTileRenderer->quadTree.metaData.visibleNodeCount++;

  // Progressively get the closest ancestors available data for draping (if own data doesn't exist)
  pNode->pPayloads[layer].drawInfo.pTexture = nullptr;
  if (pNode->pPayloads[layer].data.pTexture != nullptr)
  {
    pNode->pPayloads[layer].drawInfo.pTexture = pNode->pPayloads[layer].data.pTexture;
    pBestTexturedAncestor = pNode;
  }

  if (layer == 0 && pNode->demInfo.data.pTexture != nullptr)
  {
    pNode->demInfo.drawInfo.pTexture = pNode->demInfo.data.pTexture;
    pNode->normalInfo.drawInfo.pTexture = pNode->normalInfo.data.pTexture;
    pBestDemAncestor = pNode;
  }

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
    {
      vcQuadTreeNode *pChildNode = &pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c];
      vcTileRenderer_RecursiveBuildRenderList(pTileRenderer, pChildNode, layer, pBestTexturedAncestor, pBestDemAncestor);

      pNode->completeRender = pNode->completeRender && pChildNode->completeRender;
    }

    // only draw leaves
    return;
  }

  vcTileRenderer_DrapeColour(pNode, pBestTexturedAncestor, layer);

  if (layer == 0)
    vcTileRenderer_DrapeDEM(pNode, pBestDemAncestor);

  vcTileRenderer_InsertRenderListSorted(pTileRenderer, pNode, layer);
}

void vcTileRenderer_DrawRenderList(vcTileRenderer *pTileRenderer, vcTileShader *pShader, const udDouble4x4 &view, int layer)
{
  udChunkedArray<vcTileRenderer::vcSortedNode> *pRenderList = &pTileRenderer->renderLists[layer];
  for (size_t i = 0; i < pRenderList->length; ++i)
  {
    vcQuadTreeNode *pNode = pRenderList->GetElement(i)->pNode;

    // Lookup mesh variant for rendering
    size_t meshIndex = 0;
    for (size_t mc = 0; mc < udLengthOf(MeshConfigurations); ++mc)
    {
      if (MeshConfigurations[mc] == pNode->neighbours)
      {
        meshIndex = mc;
        break;
      }
    }

    vcTileRenderer_DrawNode(pTileRenderer, pNode, pShader, view, layer, pTileRenderer->pTileMeshes[meshIndex]);
  }
}

void vcTileRenderer_Render(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, const udDouble4x4 &proj, const bool cameraInsideGround, const float encodedObjectId)
{
  vcQuadTreeNode *pRootNode = &pTileRenderer->quadTree.nodes.pPool[pTileRenderer->quadTree.rootIndex];
  if (!pRootNode->touched) // can occur on failed re-roots
    return;

  pTileRenderer->quadTree.metaData.visibleNodeCount = 0;
  pTileRenderer->quadTree.metaData.nodeRenderCount = 0;

  // Build render list
  for (int layer = 0; layer < pTileRenderer->pSettings->maptiles.activeLayerCount; ++layer)
  {
    pTileRenderer->renderLists[layer].Clear();
    vcTileRenderer_RecursiveBuildRenderList(pTileRenderer, pRootNode, layer, nullptr, nullptr);
  }

  // Variable skirt length to handle logZ when close (tile logZ is done in vertex shader)
  float halfEarthRadius = 0.5f * (float)pTileRenderer->quadTree.geozone.semiMinorAxis;
  float tileSkirtLength = udClamp((float)pTileRenderer->cameraDistanceToSurface, 8000.0f, halfEarthRadius);

  for (int layer = 0; layer < pTileRenderer->pSettings->maptiles.activeLayerCount; ++layer)
  {
    vcTileShader *pShader = &pTileRenderer->presentShaders[udMin(layer, 1)];
    vcShader_Bind(pShader->pProgram);
    pShader->everyObject.projectionMatrix = udFloat4x4::create(proj);
    pShader->everyObject.viewMatrix = udFloat4x4::create(view);

    vcGLState_SetBlendMode(vcGLSBM_None);
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
    vcGLState_SetViewportDepthRange(0.0f, 1.0f);

    if (layer == 0) // base pass
    {
      vcGLStateCullMode cullMode = vcGLSCM_Back;
      if (cameraInsideGround)
        cullMode = vcGLSCM_Front;

      vcGLState_SetFaceMode(vcGLSFM_Solid, cullMode);

      if (pTileRenderer->pSettings->maptiles.layers[layer].transparency < 1.0f)
      {
        vcGLState_SetBlendMode(vcGLSBM_Interpolative);
        tileSkirtLength = 0.0f;
      }

      if (pTileRenderer->pSettings->maptiles.layers[layer].blendMode == vcMTBM_Overlay)
      {
        vcGLState_SetDepthStencilMode(vcGLSDM_Always, true);
        tileSkirtLength = 0.0f;
      }
      else if (pTileRenderer->pSettings->maptiles.layers[layer].blendMode == vcMTBM_Underlay)
      {
        // almost at the back, so it can still 'occlude' the skybox
        vcGLState_SetViewportDepthRange(0.999f, 0.999f);
        tileSkirtLength = 0.0f;
      }
    }
    else
    {
      vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
      vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, false);
      vcGLState_SetBlendMode(vcGLSBM_Interpolative);
      tileSkirtLength = 0.0f;
    }

    // Because of limitations of float32, morph the terrain into a globe shape, using height as a T (this avoids 'wobbly' terrain)
    static const double minMorphHeightMeters = 8000.0;
    static const double maxMorphHeightMeters = 30000.0;
    float globeMorphDelta = 0.0f;
    if (pTileRenderer->quadTree.geozone.projection == udGZPT_ECEF)
      globeMorphDelta = (float)udClamp((pTileRenderer->cameraDistanceToSurface - minMorphHeightMeters) / (maxMorphHeightMeters - minMorphHeightMeters), 0.0, 1.0);

    pShader->everyObject.objectInfo = udFloat4::create(encodedObjectId, (pTileRenderer->cameraIsUnderMapSurface ? -1.0f : 1.0f) * tileSkirtLength, globeMorphDelta, 0);
    pShader->everyObject.colour = udFloat4::create(1.f, 1.f, 1.f, pTileRenderer->pSettings->maptiles.layers[layer].transparency);

    // render nodes
    vcTileRenderer_DrawRenderList(pTileRenderer, pShader, view, layer);
  }

  vcGLState_SetViewportDepthRange(0.0f, 1.0f);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true, nullptr);
  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
  vcShader_Bind(nullptr);

#if 0
  printf("touched=%d, visible=%d, rendered=%d, leaves=%d, build=%f, loadList=%zu...used=%d\n", pTileRenderer->quadTree.metaData.nodeTouchedCount, pTileRenderer->quadTree.metaData.visibleNodeCount, pTileRenderer->quadTree.metaData.nodeRenderCount, pTileRenderer->quadTree.metaData.leafNodeCount, pTileRenderer->quadTree.metaData.generateTimeMs, pTileRenderer->cache.tileLoadList.length, pTileRenderer->quadTree.nodes.used);
#endif
}

void vcTileRenderer_ClearTiles(vcTileRenderer *pTileRenderer)
{
  pTileRenderer->generateTreeUpdateTimer = QuadTreeUpdateFrequencySec;
  
  udLockMutex(pTileRenderer->cache.pMutex);
  
  pTileRenderer->cache.tileLoadList.Clear();
  vcQuadTree_Reset(&pTileRenderer->quadTree);
  
  udReleaseMutex(pTileRenderer->cache.pMutex);
}

udDouble3 vcTileRenderer_QueryMapHeightAtCartesian(vcTileRenderer *pTileRenderer, const udDouble3 &worldUp, const udDouble3 &point)
{
  vcQuadTreeNode *pNode = vcQuadTree_GetLeafNodeFromCartesian(&pTileRenderer->quadTree, point);

  udDouble3 latLonAltZero = udGeoZone_CartesianToLatLong(pTileRenderer->quadTree.geozone, point);
  latLonAltZero.z = 0;
  udDouble3 surfacePosition = udGeoZone_LatLongToCartesian(pTileRenderer->quadTree.geozone, latLonAltZero);

  float resultMapHeight = pTileRenderer->pSettings->maptiles.layers[0].mapHeight;

  if (pTileRenderer->pSettings->maptiles.demEnabled && pNode != nullptr && pNode->demBoundsState != vcQuadTreeNode::vcDemBoundsState_None)
  {
    if (pNode->demBoundsState == vcQuadTreeNode::vcDemBoundsState_Inherited)
    {
      // search for fine DEM data up the tree
      while (pNode->parentIndex != INVALID_NODE_INDEX)
      {
        if (pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_Loaded)
          break;

        pNode = &pTileRenderer->quadTree.nodes.pPool[pNode->parentIndex];
      }
    }

    // `pDemHeights` can be null if inherited tile was ancestor, which has since been pruned
    if (pNode->demInfo.loadStatus.Get() == vcNodeRenderInfo::vcTLS_Loaded)
    {
      udDouble2 demUV = {};
      vcGIS_LatLongToSlippy(&demUV, latLonAltZero, pNode->slippyPosition.z);

      udFloat2 sampleUV = udFloat2::create((float)(demUV.x - udFloor(demUV.x)), (float)(demUV.y - udFloor(demUV.y)));
      resultMapHeight += vcTileRenderer_BilinearSample(pNode->pDemHeightsCopy, sampleUV, pNode->demHeightsCopySize.x, pNode->demHeightsCopySize.y);
    }
  }

  return surfacePosition + worldUp * resultMapHeight;
}

udDouble3 vcTileRenderer_QueryMapAtCartesian(vcTileRenderer *pTileRenderer, const udDouble3 &point, const udDouble3 *pWorldUp, udDouble3 *pNormal)
{
  // Assumption that the world up will not change significantly enough to recalculate per offset, so reuse
  udDouble3 worldUp = (pWorldUp ? *pWorldUp : vcGIS_GetWorldLocalUp(pTileRenderer->quadTree.geozone, point));

  udDouble3 p0 = vcTileRenderer_QueryMapHeightAtCartesian(pTileRenderer, worldUp, point);
  if (pNormal != nullptr)
  {
    static const double SampleOffsetAmountMeters = 4.0f;

    udDouble3 forward = vcGIS_GetWorldLocalNorth(pTileRenderer->quadTree.geozone, point);
    udDouble3 right = udNormalize3(udCross3(forward, worldUp));

    udDouble3 p1 = vcTileRenderer_QueryMapHeightAtCartesian(pTileRenderer, worldUp, point + (SampleOffsetAmountMeters * right));
    udDouble3 p2 = vcTileRenderer_QueryMapHeightAtCartesian(pTileRenderer, worldUp, point + (SampleOffsetAmountMeters * forward));

    udDouble3 n0 = udCross3(udNormalize3(p1 - p0), udNormalize3(p2 - p0));
    *pNormal = udNormalize3(n0);
  }

  return p0;
}

