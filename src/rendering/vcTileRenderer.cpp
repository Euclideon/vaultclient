#include "vcTileRenderer.h"
#include "vcQuadTree.h"
#include "vcGIS.h"
#include "vcSettings.h"

#include "gl/vcGLState.h"
#include "gl/vcShader.h"
#include "gl/vcMesh.h"

#include "udThread.h"
#include "udFile.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"
#include "udStringUtil.h"

#include "stb_image.h"

// Debug tiles with colour information
#define VISUALIZE_DEBUG_TILES 0

// TODO: This is a temporary solution, where we know the dem data stops at level 13.
#define HACK_DEM_LEVEL 11

enum
{
  TileVertexControlPointRes = 3, // Change with caution : 'vcQuadTreeNode::worldBounds[]' and GPU structs need to match
  TileVertexResolution = 31,
  TileIndexResolution = (TileVertexResolution - 1),

  MaxTextureUploadsPerFrame = 3,
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

struct vcTileRenderer
{
  float frameDeltaTime;
  float totalTime;

  vcSettings *pSettings;
  vcQuadTree quadTree;

  vcMesh *pTileMeshes[udLengthOf(MeshConfigurations)];
  vcTexture *pEmptyTileTexture;
  vcTexture *pEmptyDemTileTexture;

  udDouble3 cameraPosition;

  // cache textures
  struct vcTileCache
  {
    volatile bool keepLoading;
    udThread *pThreads[8];
    udSemaphore *pSemaphore;
    udMutex *pMutex;
    udChunkedArray<vcQuadTreeNode*> tileLoadList;
    udChunkedArray<vcQuadTreeNode*> tileTimeoutList;
  } cache;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *pConstantBuffer;
    vcShaderSampler *uniform_texture;
    vcShaderSampler *uniform_dem;

    struct
    {
      udFloat4x4 projectionMatrix;
      udFloat4x4 viewMatrix;
      udFloat4 eyePositions[TileVertexControlPointRes * TileVertexControlPointRes];
      udFloat4 colour;
      udFloat4 uvOffsetScale;
      udFloat4 demUVOffsetScale;
      udFloat4 tileNormal;
    } everyObject;
  } presentShader;
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

        if (!pNode->renderInfo.tryLoad)
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
            betterNode = !pNode->rendered && pBestNode->rendered;
            if (pNode->rendered == pBestNode->rendered)
            {
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

      udResult result = udR_Failure_;
      udMutex *pMutexCopy = pCache->pMutex;

      void *pFileData = nullptr;
      int64_t fileLen = -1;
      int width = 0;
      int height = 0;
      int channelCount = 0;
      uint8_t *pData = nullptr;

      char localFileName[vcMaxPathLength] = {};
      char serverAddress[vcMaxPathLength] = {};
      bool downloadingFromServer = false;

      vcQuadTreeNode *pBestNode = pCache->tileLoadList[best];
      pCache->tileLoadList.RemoveSwapLast(best);

      // TODO: Processing a dem request has been stuck into this block. This has been rushed
      // and will be looked at later.

      // process either a colour request, or a DEM request
      if (pBestNode->renderInfo.loadStatus == vcNodeRenderInfo::vcTLS_InQueue)
      {
        // do colour request
        pBestNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_Downloading;

        udSprintf(localFileName, "%s/%s/%d/%d/%d.%s", pRenderer->pSettings->cacheAssetPath, udUUID_GetAsString(pRenderer->pSettings->maptiles.tileServerAddressUUID), pBestNode->slippyPosition.z, pBestNode->slippyPosition.x, pBestNode->slippyPosition.y, pRenderer->pSettings->maptiles.tileServerExtension);
        udSprintf(serverAddress, "%s/%d/%d/%d.%s", pRenderer->pSettings->maptiles.tileServerAddress, pBestNode->slippyPosition.z, pBestNode->slippyPosition.x, pBestNode->slippyPosition.y, pRenderer->pSettings->maptiles.tileServerExtension);
        udReleaseMutex(pMutexCopy);
        pMutexCopy = nullptr;

        char *pTileURL = localFileName;
        if (udFileExists(pTileURL) != udR_Success)
        {
          pTileURL = serverAddress;
          downloadingFromServer = true;
        }

        UD_ERROR_CHECK(udFile_Load(pTileURL, &pFileData, &fileLen));
        UD_ERROR_IF(fileLen == 0, udR_InternalError);

        // Node has been invalidated since download started
        if (!pBestNode->touched)
        {
          // TODO: Put into LRU texture cache (but for now just throw it out)
          pBestNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_None;

          // Even though the node is now invalid - since we've done the expensive part and downloaded data,
          // it may be put into local disk cache.
          UD_ERROR_SET(udR_Success);
        }

        pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
        UD_ERROR_NULL(pData, udR_InternalError);

        pBestNode->renderInfo.colourData.width = width;
        pBestNode->renderInfo.colourData.height = height;
        pBestNode->renderInfo.colourData.pData = udMemDup(pData, sizeof(uint32_t) * width * height, 0, udAF_None);

        pBestNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_Downloaded;

        stbi_image_free(pData);
      }
      else if (pBestNode->renderInfo.demLoadStatus == vcNodeRenderInfo::vcTLS_InQueue)
      {
        // do DEM request
        pBestNode->renderInfo.demLoadStatus = vcNodeRenderInfo::vcTLS_Failed;

        char demFileName[vcMaxPathLength] = {};
        udSprintf(demFileName, "https://az.vault.euclideon.com/dem/%d/%d/%d.png", pBestNode->slippyPosition.z, pBestNode->slippyPosition.x, pBestNode->slippyPosition.y);
        udReleaseMutex(pMutexCopy);
        pMutexCopy = nullptr;

        if (udFile_Load(demFileName, &pFileData, &fileLen) == udR_Success)
        {
          pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
          if (pData != nullptr)
          {
            pBestNode->renderInfo.demData.width = width;
            pBestNode->renderInfo.demData.height = height;
            pBestNode->renderInfo.demData.pData = udMemDup(pData, sizeof(uint32_t) * width * height, 0, udAF_None);
            stbi_image_free(pData);

            pBestNode->renderInfo.demLoadStatus = vcNodeRenderInfo::vcTLS_Downloaded;
          }

          udFree(pFileData);
          pData = nullptr;
        }

        // force the 'epilogue' code to not fire
        result = udR_Success;
      }

epilogue:
      if (pMutexCopy != nullptr)
        udReleaseMutex(pMutexCopy);

      if (result != udR_Success)
      {
        pBestNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_Failed;
        if (result == udR_Pending)
        {
          pBestNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_InQueue;

          udLockMutex(pCache->pMutex);
          if (pBestNode->slippyPosition.z <= 10)
          {
            // TODO: server prioritizes these tiles, so will be available much sooner. Requeue immediately
            pCache->tileLoadList.PushBack(pBestNode);
          }
          else
          {
            pBestNode->renderInfo.timeoutTime = pRenderer->totalTime + 15.0f; // 15 seconds
            pCache->tileTimeoutList.PushBack(pBestNode); // timeout it, inserted last
          }
          udReleaseMutex(pCache->pMutex);
        }
      }

      // This functionality here for now until the cache module is implemented
      if (pFileData && fileLen > 0 && downloadingFromServer && pCache->keepLoading)
      {
        if (!vcTileRenderer_TryWriteTile(localFileName, pFileData, fileLen))
        {
          size_t index = 0;
          char localFolderPath[vcMaxPathLength];
          udSprintf(localFolderPath, "%s", localFileName);
          if (udStrrchr(localFileName, "\\/", &index) != nullptr)
            localFolderPath[index] = '\0';

          if (vcTileRenderer_CreateDirRecursive(localFolderPath) == udR_Success)
            vcTileRenderer_TryWriteTile(localFileName, pFileData, fileLen);
        }
      }

      udFree(pFileData);
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

  float normalizeVertexPositionScale = float(TileVertexResolution) / (TileVertexResolution - 1); // ensure verts are [0, 1]
  for (int y = 0; y < TileVertexResolution; ++y)
  {
    for (int x = 0; x < TileVertexResolution; ++x)
    {
      uint32_t index = y * TileVertexResolution + x;
      float normX = ((float)(x) / TileVertexResolution) * normalizeVertexPositionScale;
      float normY = ((float)(y) / TileVertexResolution) * normalizeVertexPositionScale;
      pVerts[index].position.x = minUV.x + normX * (maxUV.x - minUV.x);
      pVerts[index].position.y = minUV.y + normY * (maxUV.y - minUV.y);
      pVerts[index].position.z = (float)index;
      
    }
  }
}

udResult vcTileRenderer_Create(vcTileRenderer **ppTileRenderer, vcSettings *pSettings)
{
  udResult result;
  vcTileRenderer *pTileRenderer = nullptr;
  vcP3Vertex verts[TileVertexResolution * TileVertexResolution] = {};
  int indicies[TileIndexResolution * TileIndexResolution * 6] = {};
  uint32_t greyPixel = 0xf3f3f3ff;
  uint16_t blackPixel = 0x0;
  UD_ERROR_NULL(ppTileRenderer, udR_InvalidParameter_);

  pTileRenderer = udAllocType(vcTileRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pTileRenderer, udR_MemoryAllocationFailure);

  vcQuadTree_Create(&pTileRenderer->quadTree, pSettings);

  pTileRenderer->pSettings = pSettings;

  pTileRenderer->cache.pSemaphore = udCreateSemaphore();
  pTileRenderer->cache.pMutex = udCreateMutex();
  pTileRenderer->cache.keepLoading = true;
  pTileRenderer->cache.tileLoadList.Init(128);
  pTileRenderer->cache.tileTimeoutList.Init(128);

  for (size_t i = 0; i < udLengthOf(pTileRenderer->cache.pThreads); ++i)
    UD_ERROR_CHECK(udThread_Create(&pTileRenderer->cache.pThreads[i], vcTileRenderer_LoadThread, pTileRenderer));

  UD_ERROR_IF(!vcShader_CreateFromFile(&pTileRenderer->presentShader.pProgram, "asset://assets/shaders/tileVertexShader", "asset://assets/shaders/tileFragmentShader", vcP3VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pTileRenderer->presentShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pTileRenderer->presentShader.pConstantBuffer, pTileRenderer->presentShader.pProgram, "u_EveryObject", sizeof(pTileRenderer->presentShader.everyObject)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pTileRenderer->presentShader.uniform_texture, pTileRenderer->presentShader.pProgram, "u_texture"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pTileRenderer->presentShader.uniform_dem, pTileRenderer->presentShader.pProgram, "u_dem"), udR_InternalError);

  // build mesh variants
  for (size_t i = 0; i < udLengthOf(MeshConfigurations); ++i)
  {
    vcTileRenderer_BuildMeshVertices(verts, indicies, udFloat2::create(0.0f, 0.0f), udFloat2::create(1.0f, 1.0f), MeshConfigurations[i]);
    vcMesh_Create(&pTileRenderer->pTileMeshes[i], vcP3VertexLayout, (int)udLengthOf(vcP3VertexLayout), verts, TileVertexResolution * TileVertexResolution, indicies, TileIndexResolution * TileIndexResolution * 6);
  }

  UD_ERROR_CHECK(vcTexture_Create(&pTileRenderer->pEmptyTileTexture, 1, 1, &greyPixel));
  UD_ERROR_CHECK(vcTexture_Create(&pTileRenderer->pEmptyDemTileTexture, 1, 1, &blackPixel, vcTextureFormat_R16));

  *ppTileRenderer = pTileRenderer;
  pTileRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pTileRenderer)
    vcTileRenderer_Destroy(&pTileRenderer);

  return result;
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
  pTileRenderer->cache.tileTimeoutList.Deinit();

  vcShader_ReleaseConstantBuffer(pTileRenderer->presentShader.pProgram, pTileRenderer->presentShader.pConstantBuffer);
  vcShader_DestroyShader(&(pTileRenderer->presentShader.pProgram));
  for (size_t i = 0; i < udLengthOf(MeshConfigurations); ++i)
    vcMesh_Destroy(&pTileRenderer->pTileMeshes[i]);
  vcTexture_Destroy(&pTileRenderer->pEmptyTileTexture);
  vcTexture_Destroy(&pTileRenderer->pEmptyDemTileTexture);

  vcQuadTree_Destroy(&(*ppTileRenderer)->quadTree);
  udFree(*ppTileRenderer);
  *ppTileRenderer = nullptr;

  return udR_Success;
}

void vcTileRenderer_RecursiveUpdateNodeAABB(vcQuadTree *pQuadTree, vcQuadTreeNode *pParentNode, vcQuadTreeNode *pNode)
{
  if (pParentNode != nullptr && !vcQuadTree_HasDemData(pNode))
    pNode->demMinMax = pParentNode->demMinMax; // inherit

  vcQuadTree_CalculateNodeAABB(pNode);

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
      vcTileRenderer_RecursiveUpdateNodeAABB(pQuadTree, pNode , &pQuadTree->nodes.pPool[pNode->childBlockIndex + c]);
  }
}

void vcTileRenderer_UpdateTileDEMTexture(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode)
{
  vcTileRenderer::vcTileCache *pTileCache = &pTileRenderer->cache;
  if (pNode->renderInfo.demLoadStatus == vcNodeRenderInfo::vcTLS_None)
  {
    pNode->renderInfo.demLoadStatus = vcNodeRenderInfo::vcTLS_InQueue;

    pNode->renderInfo.demData.pData = nullptr;
    pNode->renderInfo.demData.pTexture = nullptr;

    pTileCache->tileLoadList.PushBack(pNode);
    udIncrementSemaphore(pTileCache->pSemaphore);
  }

  pNode->renderInfo.tryLoad = true;

  if (pNode->renderInfo.demLoadStatus == vcNodeRenderInfo::vcTLS_Downloaded)
  {
    pNode->renderInfo.demLoadStatus = vcNodeRenderInfo::vcTLS_Loaded;
    pNode->renderInfo.tryLoad = false;

    pNode->demMinMax[0] = 32767;
    pNode->demMinMax[1] = -32768;

    int16_t *pShortPixels = udAllocType(int16_t, pNode->renderInfo.demData.width * pNode->renderInfo.demData.height, udAF_Zero);
    for (int h = 0; h < pNode->renderInfo.demData.height; ++h)
    {
      for (int w = 0; w < pNode->renderInfo.demData.width; ++w)
      {
        int index = h * pNode->renderInfo.demData.width + w;
        uint32_t p = ((uint32_t*)pNode->renderInfo.demData.pData)[index];
        uint8_t r = uint8_t((p & 0xff000000) >> 24);
        uint8_t g = uint8_t((p & 0x00ff0000) >> 16);

        int16_t height = r | (g << 8);

        if (height == -32768) // TODO: invalid sentinel value
          height = 0;

        pNode->demMinMax[0] = udMin(pNode->demMinMax.x, (int32_t)height);
        pNode->demMinMax[1] = udMax(pNode->demMinMax.y, (int32_t)height);
        pShortPixels[index] = height;
      }
    }
    vcTexture_CreateAdv(&pNode->renderInfo.demData.pTexture, vcTextureType_Texture2D, pNode->renderInfo.demData.width, pNode->renderInfo.demData.height, 1, pShortPixels, vcTextureFormat_R16, vcTFM_Linear, false, vcTWM_Clamp);
    udFree(pShortPixels);
    udFree(pNode->renderInfo.demData.pData);

    // update descendent AABB
    vcTileRenderer_RecursiveUpdateNodeAABB(&pTileRenderer->quadTree, nullptr, pNode);
  }
}

bool vcTileRenderer_UpdateTileTexture(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode)
{
  vcTileRenderer::vcTileCache *pTileCache = &pTileRenderer->cache;
  if (pNode->renderInfo.loadStatus == vcNodeRenderInfo::vcTLS_None)
  {
    pNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_InQueue;

    pNode->renderInfo.colourData.pData = nullptr;
    pNode->renderInfo.colourData.pTexture = nullptr;
    pNode->renderInfo.timeoutTime = pTileRenderer->totalTime;

    pTileCache->tileLoadList.PushBack(pNode);
    udIncrementSemaphore(pTileCache->pSemaphore);
  }

  pNode->renderInfo.tryLoad = true;

  if (pNode->renderInfo.loadStatus == vcNodeRenderInfo::vcTLS_Downloaded)
  {
    pNode->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_Loaded;
    pNode->renderInfo.tryLoad = false;

    vcTexture_CreateAdv(&pNode->renderInfo.colourData.pTexture, vcTextureType_Texture2D, pNode->renderInfo.colourData.width, pNode->renderInfo.colourData.height, 1, pNode->renderInfo.colourData.pData, vcTextureFormat_RGBA8, vcTFM_Linear, true, vcTWM_Clamp, vcTCF_None, 16);
    udFree(pNode->renderInfo.colourData.pData);
    return true;
  }

  return false;
}

void vcTileRenderer_UpdateTextureQueuesRecursive(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, int &tileUploadCount)
{
  if (tileUploadCount >= MaxTextureUploadsPerFrame)
    return;

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
      vcTileRenderer_UpdateTextureQueuesRecursive(pTileRenderer, &pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c], tileUploadCount);
  }

  if (pNode->renderInfo.loadStatus != vcNodeRenderInfo::vcTLS_Loaded && vcQuadTree_IsVisibleLeafNode(&pTileRenderer->quadTree, pNode))
  {
    if (vcTileRenderer_UpdateTileTexture(pTileRenderer, pNode))
      ++tileUploadCount;
  }

  // hacky - looking for specific DEM levels
  if (pNode->slippyPosition.z <= HACK_DEM_LEVEL)
  {
    bool demLeaf = vcQuadTree_IsLeafNode(pNode) || (!vcQuadTree_IsLeafNode(pNode) && pNode->slippyPosition.z == HACK_DEM_LEVEL);
    if (demLeaf && pNode->renderInfo.demLoadStatus != vcNodeRenderInfo::vcTLS_Loaded)
    {
      vcTileRenderer_UpdateTileDEMTexture(pTileRenderer, pNode);
    }
  }
}

void vcTileRenderer_UpdateTextureQueues(vcTileRenderer *pTileRenderer)
{
  // invalidate current queue
  for (size_t i = 0; i < pTileRenderer->cache.tileLoadList.length; ++i)
    pTileRenderer->cache.tileLoadList[i]->renderInfo.tryLoad = false;

  // Limit the max number of tiles uploaded per frame
  // TODO: use timings instead
  int tileUploadCount = 0;

  // update visible tiles textures
  vcTileRenderer_UpdateTextureQueuesRecursive(pTileRenderer, &pTileRenderer->quadTree.nodes.pPool[pTileRenderer->quadTree.rootIndex], tileUploadCount);

  // remove from the queue any tiles that are invalid
  for (int i = 0; i < (int)pTileRenderer->cache.tileLoadList.length; ++i)
  {
    if (!pTileRenderer->cache.tileLoadList[i]->renderInfo.tryLoad)
    {
      // TODO: Bug causing this data to be allocated, free just in case
      if (pTileRenderer->cache.tileLoadList[i]->renderInfo.loadStatus != vcNodeRenderInfo::vcTLS_None)
      {
        udFree(pTileRenderer->cache.tileLoadList[i]->renderInfo.colourData.pData);
        vcTexture_Destroy(&pTileRenderer->cache.tileLoadList[i]->renderInfo.colourData.pTexture);
      }

      // TODO: Bug causing this data to rarely leak, free just in case
      if (pTileRenderer->cache.tileLoadList[i]->renderInfo.demLoadStatus != vcNodeRenderInfo::vcTLS_None)
      {
        udFree(pTileRenderer->cache.tileLoadList[i]->renderInfo.demData.pData);
        vcTexture_Destroy(&pTileRenderer->cache.tileLoadList[i]->renderInfo.demData.pTexture);
      }

      pTileRenderer->cache.tileLoadList[i]->renderInfo.loadStatus = vcNodeRenderInfo::vcTLS_None;
      pTileRenderer->cache.tileLoadList[i]->renderInfo.demLoadStatus = vcNodeRenderInfo::vcTLS_None;
      pTileRenderer->cache.tileLoadList.RemoveSwapLast(i);
      --i;
    }
  }

  // Note: this is a FIFO queue, so only need to check the head
  while (pTileRenderer->cache.tileTimeoutList.length > 0)
  {
    vcQuadTreeNode *pNode = pTileRenderer->cache.tileTimeoutList[0];
    if (pNode->renderInfo.tryLoad && (pNode->renderInfo.timeoutTime - pTileRenderer->totalTime) > 0.0f)
      break;

    pTileRenderer->cache.tileLoadList.PushBack(pNode);
    pTileRenderer->cache.tileTimeoutList.RemoveSwapLast(0);
  }

  // TODO: For each tile in cache, LRU destroy
}

void vcTileRenderer_Update(vcTileRenderer *pTileRenderer, const double deltaTime, vcGISSpace *pSpace, const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos, const udDouble3& cameraZeroAltitude, const udDouble4x4 &viewProjectionMatrix)
{
  pTileRenderer->frameDeltaTime = (float)deltaTime;
  pTileRenderer->totalTime += pTileRenderer->frameDeltaTime;
  pTileRenderer->cameraPosition = cameraWorldPos;

  vcQuadTreeViewInfo viewInfo =
  {
    pSpace,
    slippyCoords,
    cameraWorldPos,
    cameraZeroAltitude,
    pTileRenderer->pSettings->maptiles.mapHeight,
    viewProjectionMatrix,
    MaxVisibleTileLevel
  };

  uint64_t startTime = udPerfCounterStart();

  vcQuadTree_Update(&pTileRenderer->quadTree, viewInfo);

  udLockMutex(pTileRenderer->cache.pMutex);
  vcTileRenderer_UpdateTextureQueues(pTileRenderer);
  udReleaseMutex(pTileRenderer->cache.pMutex);

  pTileRenderer->quadTree.metaData.generateTimeMs = udPerfCounterMilliseconds(startTime);
}

bool vcTileRenderer_DrawNode(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, vcMesh *pMesh, const udDouble4x4 &view)
{
  vcTexture *pTexture = pNode->renderInfo.colourDrawInfo.pTexture;
  if (pTexture == nullptr)
    pTexture = pTileRenderer->pEmptyTileTexture;

  vcTexture *pDemTexture = pNode->renderInfo.demDrawInfo.pTexture;
  if (pDemTexture == nullptr)
    pDemTexture = pTileRenderer->pEmptyDemTileTexture;

  for (int t = 0; t < TileVertexControlPointRes * TileVertexControlPointRes; ++t)
  {
    udFloat4 eyeSpaceVertexPosition = udFloat4::create(view * udDouble4::create(pNode->worldBounds[t], 1.0));
    pTileRenderer->presentShader.everyObject.eyePositions[t] = eyeSpaceVertexPosition;
  }

  udFloat2 size = pNode->renderInfo.colourDrawInfo.uvEnd - pNode->renderInfo.colourDrawInfo.uvStart;
  pTileRenderer->presentShader.everyObject.uvOffsetScale = udFloat4::create(pNode->renderInfo.colourDrawInfo.uvStart, size.x, size.y);

  udFloat2 demSize = pNode->renderInfo.demDrawInfo.uvEnd - pNode->renderInfo.demDrawInfo.uvStart;
  pTileRenderer->presentShader.everyObject.demUVOffsetScale = udFloat4::create(pNode->renderInfo.demDrawInfo.uvStart, demSize.x, demSize.y);

  pTileRenderer->presentShader.everyObject.tileNormal = udFloat4::create(udFloat3::create(pNode->worldNormal), 0.0f);

  vcShader_BindTexture(pTileRenderer->presentShader.pProgram, pTexture, 0, pTileRenderer->presentShader.uniform_texture);

  // TODO: This is a hack, needs to be fixed for both graphics APIs
  uint16_t samplerIndex = 0;
#if GRAPHICS_API_OPENGL
  samplerIndex = 1;
#endif
  vcShader_BindTexture(pTileRenderer->presentShader.pProgram, pDemTexture, samplerIndex, pTileRenderer->presentShader.uniform_dem, vcGLSamplerShaderStage_Vertex);

  vcShader_BindConstantBuffer(pTileRenderer->presentShader.pProgram, pTileRenderer->presentShader.pConstantBuffer, &pTileRenderer->presentShader.everyObject, sizeof(pTileRenderer->presentShader.everyObject));
  vcMesh_Render(pMesh, TileIndexResolution * TileIndexResolution * 2); // 2 tris per quad

  pNode->rendered = true;
  ++pTileRenderer->quadTree.metaData.nodeRenderCount;

  return true;
}

void vcTileRenderer_RecursiveSetRendered(vcTileRenderer *pTileRenderer, vcQuadTreeNode *pNode, bool rendered)
{
  pNode->rendered = pNode->rendered || rendered;
  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
      vcTileRenderer_RecursiveSetRendered(pTileRenderer, &pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c], pNode->rendered);
  }
}

void vcTileRenderer_DrapeColour(vcQuadTreeNode *pChild, vcQuadTreeNode *pAncestor)
{
  pChild->renderInfo.colourDrawInfo.uvStart = udFloat2::zero();
  pChild->renderInfo.colourDrawInfo.uvEnd = udFloat2::one();
  if (pAncestor != nullptr && pAncestor != pChild)
  {
    // calculate what portion of ancestors colour to display at this tile
    pChild->renderInfo.colourDrawInfo.pTexture = pAncestor->renderInfo.colourDrawInfo.pTexture;
    int depthDiff = pChild->slippyPosition.z - pAncestor->slippyPosition.z;
    int slippyRange = (int)udPow(2.0f, (float)depthDiff);
    udFloat2 boundsRange = udFloat2::create((float)slippyRange);

    // top-left, and bottom-right corners
    udInt2 slippy0 = pChild->slippyPosition.toVector2();
    udInt2 slippy1 = slippy0 + udInt2::one();

    udInt2 ancestorSlippyLocal0 = udInt2::create(pAncestor->slippyPosition.x * slippyRange, pAncestor->slippyPosition.y * slippyRange);
    udInt2 ancestorSlippyLocal1 = ancestorSlippyLocal0 + udInt2::create(slippyRange);

    pChild->renderInfo.colourDrawInfo.uvStart = udFloat2::create(slippy0 - ancestorSlippyLocal0) / boundsRange;
    pChild->renderInfo.colourDrawInfo.uvEnd = udFloat2::one() - (udFloat2::create(ancestorSlippyLocal1 - slippy1) / boundsRange);
  }
}

void vcTileRenderer_DrapeDEM(vcQuadTreeNode *pChild, vcQuadTreeNode *pAncestor)
{
  pChild->renderInfo.demDrawInfo.uvStart = udFloat2::zero();
  pChild->renderInfo.demDrawInfo.uvEnd = udFloat2::one();
  if (pAncestor != nullptr && pAncestor != pChild)
  {
    // calculate what portion of ancestors DEM to display at this tile
    pChild->renderInfo.demDrawInfo.pTexture = pAncestor->renderInfo.demDrawInfo.pTexture;
    int depthDiff = pChild->slippyPosition.z - pAncestor->slippyPosition.z;
    int slippyRange = (int)udPow(2.0f, (float)depthDiff);
    udFloat2 boundsRange = udFloat2::create((float)slippyRange);

    // top-left, and bottom-right corners
    udInt2 slippy0 = pChild->slippyPosition.toVector2();
    udInt2 slippy1 = slippy0 + udInt2::one();

    udInt2 ancestorSlippyLocal0 = udInt2::create(pAncestor->slippyPosition.x * slippyRange, pAncestor->slippyPosition.y * slippyRange);
    udInt2 ancestorSlippyLocal1 = ancestorSlippyLocal0 + udInt2::create(slippyRange);

    pChild->renderInfo.demDrawInfo.uvStart = udFloat2::create(slippy0 - ancestorSlippyLocal0) / boundsRange;
    pChild->renderInfo.demDrawInfo.uvEnd = udFloat2::one() - (udFloat2::create(ancestorSlippyLocal1 - slippy1) / boundsRange);
  }
}

// 'true' indicates the node was able to render itself (or it didn't want to render itself).
// 'false' indicates that the nodes ancestor needs to be rendered.
bool vcTileRenderer_RecursiveRenderNodes(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, vcQuadTreeNode *pNode, vcQuadTreeNode *pBestTexturedAncestor, vcQuadTreeNode *pBestDemAncestor)
{
  if (!pNode->touched)
  {
    // re-test visibility
    pNode->visible = vcQuadTree_IsNodeVisible(&pTileRenderer->quadTree, pNode);
  }

  if (!pNode->visible)
    return false;

  // Progressively get the closest ancestors available data for draping (if own data doesn't exist)
  pNode->renderInfo.colourDrawInfo.pTexture = nullptr;
  pNode->renderInfo.demDrawInfo.pTexture = nullptr;
  if (pNode->renderInfo.colourData.pTexture != nullptr)
  {
    pNode->renderInfo.colourDrawInfo.pTexture = pNode->renderInfo.colourData.pTexture;
    pBestTexturedAncestor = pNode;
  }

  if (pNode->renderInfo.demData.pTexture != nullptr)
  {
    pNode->renderInfo.demDrawInfo.pTexture = pNode->renderInfo.demData.pTexture;
    pBestDemAncestor = pNode;
  }

  if (!vcQuadTree_IsLeafNode(pNode))
  {
    for (int c = 0; c < 4; ++c)
    {
      vcQuadTreeNode *pChildNode = &pTileRenderer->quadTree.nodes.pPool[pNode->childBlockIndex + c];
      vcTileRenderer_RecursiveRenderNodes(pTileRenderer, view, pChildNode, pBestTexturedAncestor, pBestDemAncestor);
    }

    // only draw leaves
    return true;
  }

  vcTileRenderer_DrapeColour(pNode, pBestTexturedAncestor);
  vcTileRenderer_DrapeDEM(pNode, pBestDemAncestor);

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

  vcTileRenderer_DrawNode(pTileRenderer, pNode, pTileRenderer->pTileMeshes[meshIndex], view);

  // This child doesn't need parent to draw itself
  return true;
}

void vcTileRenderer_Render(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, const udDouble4x4 &proj, const bool cameraInsideGround)
{
  vcQuadTreeNode *pRootNode = &pTileRenderer->quadTree.nodes.pPool[pTileRenderer->quadTree.rootIndex];
  if (!pRootNode->touched) // can occur on failed re-roots
    return;

  udDouble4x4 viewWithMapTranslation = view * udDouble4x4::translation(0, 0, pTileRenderer->pSettings->maptiles.mapHeight);

  vcGLStateCullMode cullMode = vcGLSCM_Back;
  if (cameraInsideGround)
    cullMode = vcGLSCM_Front;

  vcGLState_SetFaceMode(vcGLSFM_Solid, cullMode);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);

  if (pTileRenderer->pSettings->maptiles.transparency >= 1.0f)
    vcGLState_SetBlendMode(vcGLSBM_None);
  else
    vcGLState_SetBlendMode(vcGLSBM_Interpolative);

  if (pTileRenderer->pSettings->maptiles.blendMode == vcMTBM_Overlay)
  {
    vcGLState_SetViewportDepthRange(0.0f, 0.0f);
    vcGLState_SetDepthStencilMode(vcGLSDM_Always, true);
  }
  else if (pTileRenderer->pSettings->maptiles.blendMode == vcMTBM_Underlay)
  {
    // almost at the back, so it can still 'occlude' the skybox
    vcGLState_SetViewportDepthRange(0.999f, 0.999f);
  }

  vcShader_Bind(pTileRenderer->presentShader.pProgram);
  pTileRenderer->presentShader.everyObject.projectionMatrix = udFloat4x4::create(proj);
  pTileRenderer->presentShader.everyObject.viewMatrix = udFloat4x4::create(view);
  pTileRenderer->presentShader.everyObject.colour = udFloat4::create(1.f, 1.f, 1.f, pTileRenderer->pSettings->maptiles.transparency);

  vcTileRenderer_RecursiveRenderNodes(pTileRenderer, viewWithMapTranslation, pRootNode, nullptr, nullptr);

  vcGLState_SetViewportDepthRange(0.0f, 1.0f);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true, nullptr);
  vcGLState_SetBlendMode(vcGLSBM_None);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back);
  vcShader_Bind(nullptr);

#if 0
  printf("touched=%d, visible=%d, rendered=%d, leaves=%d, build=%f, loadList=%zu\n", pTileRenderer->quadTree.metaData.nodeTouchedCount, pTileRenderer->quadTree.metaData.visibleNodeCount, pTileRenderer->quadTree.metaData.nodeRenderCount, pTileRenderer->quadTree.metaData.leafNodeCount, pTileRenderer->quadTree.metaData.generateTimeMs, pTileRenderer->cache.tileLoadList.length);
#endif
}

void vcTileRenderer_ClearTiles(vcTileRenderer *pTileRenderer)
{
  udLockMutex(pTileRenderer->cache.pMutex);

  pTileRenderer->cache.tileLoadList.Clear();
  vcQuadTree_Reset(&pTileRenderer->quadTree);

  udReleaseMutex(pTileRenderer->cache.pMutex);
}
