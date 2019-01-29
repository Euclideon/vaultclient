#include "vcTerrainRenderer.h"
#include "vcTerrain.h"

#include "gl/vcGLState.h"
#include "gl/vcMeshUtils.h"
#include "gl/vcRenderShaders.h"
#include "gl/vcShader.h"
#include "gl/vcMesh.h"

#include "vcQuadTree.h"
#include "vcSettings.h"
#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"
#include "udPlatform/udFile.h"
#include "vcGIS.h"

#include "stb_image.h"

enum
{
  TileVertexResolution = 3, // This should match GPU struct size
  TileIndexResolution = (TileVertexResolution - 1),
  MaxSlippyLevel = 21,
};

// How many slippy levels we descend using TileVertexResolution as index
int gSlippyLayerDescendAmount[] =
{
  0, 0, 0, 1, 1, 2, 2, 2, 2, 3,
};

// Describes how to collapse the mesh's vertex positions
int gSubTilePositionIndexes[5][TileVertexResolution * TileVertexResolution] =
{
  // full tile
  {
    0, 1, 2,
    3, 4, 5,
    6, 7, 8
  },

  // bottom left (collapses top right corner)
  {
    0, 1, 1,
    3, 4, 4,
    4, 4, 4,
  },

  // bottom right (collapses top left corner)
  {
    1, 1, 2,
    4, 4, 5,
    5, 5, 5
  },

  // top left (collapses bottom right corner)
  {
    3, 3, 3,
    3, 4, 4,
    6, 7, 7
  },

  // top right (collapses bottom left corner)
  {
    4, 4, 4,
    4, 4, 5,
    7, 7, 8
  }
};

struct vcTileVertex
{
  udFloat2 uv;
  float index;
};
const vcVertexLayoutTypes vcTileVertexLayout[] = { vcVLT_Position3 };

struct vcTile
{
  bool isUsed;
  bool isLeaf;
  udInt3 slippyCoord;

  vcTexture *pTexture;
  udDouble3 worldPosition[TileVertexResolution * TileVertexResolution]; // each vertex has a unique world position

  vcTile *pParent;
  int childMaskInParent; // what region this tile occupies in its parent
  int waitingForChildLoadMask;
};

struct vcCachedTexture
{
  udInt3 id; // each tile has a unique slippy {x,y,z}
  vcTexture *pTexture;

  int32_t width, height;
  void *pData;

  double priority; // lower is better. (distance squared to camera)
  udDouble3 tilePosition;
  bool isVisible; // if associated tile is part of the current scene (latest quad tree data)
};

struct vcTerrainRenderer
{
  vcTile *pTiles;
  int tileCount;
  int tileCapacity;
  int activeTiles;

  udDouble4x4 latestViewProjectionMatrix;

  vcSettings *pSettings;
  vcMesh *pFullTileMesh;
  vcMesh *pSubTileMesh[4];

  vcTexture *pEmptyTileTexture;

  // cache textures
  struct vcTerrainCache
  {
    volatile bool keepLoading;
    udThread *pThreads[4];
    udSemaphore *pSemaphore;
    udMutex *pMutex;
    udChunkedArray<vcCachedTexture*> textureLoadList;
    udChunkedArray<vcCachedTexture> textures;
  } cache;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *pConstantBuffer;
    vcShaderSampler *uniform_texture;

    struct
    {
      udFloat4x4 projectionMatrix;
      udFloat4 eyePositions[TileVertexResolution * TileVertexResolution];
      udFloat4 colour; // Colour Multiply
    } everyObject;
  } presentShader;
};

void vcTerrainRenderer_LoadThread(void *pThreadData)
{
  vcTerrainRenderer *pRenderer = (vcTerrainRenderer*)pThreadData;
  vcTerrainRenderer::vcTerrainCache *pCache = &pRenderer->cache;

  while (pCache->keepLoading)
  {
    int loadStatus = udWaitSemaphore(pCache->pSemaphore, 1000);

    if (loadStatus != 0 && pCache->textureLoadList.length == 0)
      continue;

    while (pCache->textureLoadList.length > 0 && pCache->keepLoading)
    {
      udLockMutex(pCache->pMutex);

      // Choose the next tile to load based on 'priority':
      // 1. visibility (in frustum)
      // 2. distance based
      vcCachedTexture *pNextTexture = nullptr;
      size_t best = (size_t)-1;
      bool hasFrustumPriority = false;
      for (size_t i = 0; i < pCache->textureLoadList.length; ++i)
      {
        if (!pCache->textureLoadList[i]->isVisible)
          continue;

        // TODO: frustum plane approach for correct AABB frustum detection
        udDouble4 tileCenter = pRenderer->latestViewProjectionMatrix * udDouble4::create(pCache->textureLoadList[i]->tilePosition.x, pCache->textureLoadList[i]->tilePosition.y, 0.0, 1.0);
        bool hasHigherPriority = pNextTexture ? (pCache->textureLoadList[i]->priority <= pNextTexture->priority) : true;
        bool isInFrustum = tileCenter.x >= -tileCenter.w && tileCenter.x <= tileCenter.w &&
          tileCenter.y >= -tileCenter.w && tileCenter.y <= tileCenter.w &&
          tileCenter.z >= -tileCenter.w && tileCenter.z <= tileCenter.w;

        // as soon as we detect a tile inside the frustum, only consider other tiles
        // also in that frustum.
        bool isBetter = isInFrustum ? (!hasFrustumPriority || hasHigherPriority) : (!hasFrustumPriority && hasHigherPriority);
        if (isBetter)
        {
          best = i;
          pNextTexture = pCache->textureLoadList[i];
        }
        hasFrustumPriority |= isInFrustum;
      }

      if (best == (size_t)-1)
      {
        udReleaseMutex(pCache->pMutex);
        break;
      }

      pCache->textureLoadList.RemoveSwapLast(best);

      char buff[256];
      udSprintf(buff, sizeof(buff), "%s/%d/%d/%d.%s", pRenderer->pSettings->maptiles.tileServerAddress, pNextTexture->id.z, pNextTexture->id.x, pNextTexture->id.y, pRenderer->pSettings->maptiles.tileServerExtension);
      udReleaseMutex(pCache->pMutex);

      void *pFileData;
      int64_t fileLen;
      int width, height, channelCount;

      if (udFile_Load(buff, &pFileData, &fileLen) != udR_Success)
        continue;

      uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      if (pData == nullptr)
      {
        // TODO: if this occurs, put back in the queue?
        goto epilogue;
      }

      pNextTexture->pData = udMemDup(pData, sizeof(uint32_t)*width*height, 0, udAF_None);

      pNextTexture->width = width;
      pNextTexture->height = height;

epilogue:
      udFree(pFileData);
      stbi_image_free(pData);
    }
  }
}


void vcTerrainRenderer_BuildMeshVertices(vcTileVertex *pVerts, int *pIndicies, udFloat2 minUV, udFloat2 maxUV)
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
    }
  }

  float normalizeVertexPositionScale = float(TileVertexResolution) / (TileVertexResolution - 1); // ensure verts are [0, 1]
  for (int y = 0; y < TileVertexResolution; ++y)
  {
    for (int x = 0; x < TileVertexResolution; ++x)
    {
      uint32_t index = y * TileVertexResolution + x;
      pVerts[index].index = (float)index;

      float normX = ((float)(x) / TileVertexResolution) * normalizeVertexPositionScale;
      float normY = ((float)(y) / TileVertexResolution) * normalizeVertexPositionScale;
      pVerts[index].uv.x = minUV.x + normX * (maxUV.x - minUV.x);
      pVerts[index].uv.y = minUV.y + normY * (maxUV.y - minUV.y);

    }
  }
}


void vcTerrainRenderer_Init(vcTerrainRenderer **ppTerrainRenderer, vcSettings *pSettings)
{
  vcTerrainRenderer *pTerrainRenderer = udAllocType(vcTerrainRenderer, 1, udAF_Zero);

  pTerrainRenderer->tileCapacity = 200; // generally capacity doesn't exceed this
  pTerrainRenderer->pTiles = udAllocType(vcTile, pTerrainRenderer->tileCapacity, udAF_Zero);

  pTerrainRenderer->pSettings = pSettings;

  pTerrainRenderer->cache.pSemaphore = udCreateSemaphore();
  pTerrainRenderer->cache.pMutex = udCreateMutex();
  pTerrainRenderer->cache.keepLoading = true;
  pTerrainRenderer->cache.textures.Init(128);
  pTerrainRenderer->cache.textureLoadList.Init(16);

  for (size_t i = 0; i < udLengthOf(pTerrainRenderer->cache.pThreads); ++i)
    udThread_Create(&pTerrainRenderer->cache.pThreads[i], (udThreadStart*)vcTerrainRenderer_LoadThread, pTerrainRenderer);

  vcShader_CreateFromText(&pTerrainRenderer->presentShader.pProgram, g_terrainTileVertexShader, g_terrainTileFragmentShader, vcSimpleVertexLayout, (int)udLengthOf(vcSimpleVertexLayout));
  vcShader_GetConstantBuffer(&pTerrainRenderer->presentShader.pConstantBuffer, pTerrainRenderer->presentShader.pProgram, "u_EveryObject", sizeof(pTerrainRenderer->presentShader.everyObject));
  vcShader_GetSamplerIndex(&pTerrainRenderer->presentShader.uniform_texture, pTerrainRenderer->presentShader.pProgram, "u_texture");

  // build meshes
  vcTileVertex verts[TileVertexResolution * TileVertexResolution];
  int indicies[TileIndexResolution * TileIndexResolution * 6];
  vcTerrainRenderer_BuildMeshVertices(verts, indicies, udFloat2::create(0.0f, 0.0f), udFloat2::create(1.0f, 1.0f));

  vcMesh_Create(&pTerrainRenderer->pFullTileMesh, vcTileVertexLayout, (int)udLengthOf(vcTileVertexLayout), verts, TileVertexResolution * TileVertexResolution, indicies, TileIndexResolution * TileIndexResolution * 6);
  for (int i = 0; i < 4; ++i)
    vcMesh_Create(&pTerrainRenderer->pSubTileMesh[i], vcTileVertexLayout, (int)udLengthOf(vcTileVertexLayout), verts, TileVertexResolution * TileVertexResolution, indicies, TileIndexResolution * TileIndexResolution * 6);

  uint32_t grayPixel = 0xf3f3f3ff;
  vcTexture_Create(&pTerrainRenderer->pEmptyTileTexture, 1, 1, &grayPixel);

  (*ppTerrainRenderer) = pTerrainRenderer;
}


void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer)
{
  if (ppTerrainRenderer == nullptr || *ppTerrainRenderer == nullptr)
    return;

  vcTerrainRenderer *pTerrainRenderer = (*ppTerrainRenderer);

  pTerrainRenderer->cache.keepLoading = false;
  udIncrementSemaphore(pTerrainRenderer->cache.pSemaphore);

  for (size_t i = 0; i < udLengthOf(pTerrainRenderer->cache.pThreads); ++i)
  {
    udThread_Join(pTerrainRenderer->cache.pThreads[i]);
    udThread_Destroy(&pTerrainRenderer->cache.pThreads[i]);
  }

  udDestroyMutex(&pTerrainRenderer->cache.pMutex);
  udDestroySemaphore(&pTerrainRenderer->cache.pSemaphore);

  for (size_t i = 0; i < pTerrainRenderer->cache.textures.length; ++i)
  {
    udFree(pTerrainRenderer->cache.textures[i].pData);
    vcTexture_Destroy(&pTerrainRenderer->cache.textures[i].pTexture);
  }

  pTerrainRenderer->cache.textures.Deinit();
  pTerrainRenderer->cache.textureLoadList.Deinit();

  udFree(pTerrainRenderer->pTiles);

  //TODO: Cleanup uniforms
  vcShader_DestroyShader(&pTerrainRenderer->presentShader.pProgram);

  vcMesh_Destroy(&pTerrainRenderer->pFullTileMesh);
  for (int i = 0; i < 4; ++i)
    vcMesh_Destroy(&pTerrainRenderer->pSubTileMesh[i]);

  vcTexture_Destroy(&pTerrainRenderer->pEmptyTileTexture);
  udFree(*ppTerrainRenderer);
}


vcTile *vcTerrainRenderer_FindTile(vcTerrainRenderer *pTerrainRenderer, const udInt3 &slippy)
{
  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    if (pTerrainRenderer->pTiles[i].isUsed && pTerrainRenderer->pTiles[i].slippyCoord == slippy)
      return &pTerrainRenderer->pTiles[i];
  }

  return nullptr;
}


vcTile *vcTerrainRenderer_FindParentOfTile(vcTerrainRenderer *pTerrainRenderer, const vcTile *pTile)
{
  udInt3 parentSlippy = udInt3::create(pTile->slippyCoord.x >> 1, pTile->slippyCoord.y >> 1, pTile->slippyCoord.z - 1);
  return vcTerrainRenderer_FindTile(pTerrainRenderer, parentSlippy);
}


vcTile *vcTerrainRenderer_FindChildOfTile(vcTerrainRenderer *pTerrainRenderer, const vcTile *pTile, const udInt2 &childOffset)
{
  udInt3 childSlippy = udInt3::create((pTile->slippyCoord.x << 1) + childOffset.x, (pTile->slippyCoord.y << 1) + childOffset.y, pTile->slippyCoord.z + 1);
  return vcTerrainRenderer_FindTile(pTerrainRenderer, childSlippy);
}


vcTile *vcTerrainRenderer_FindEmptyTileSlot(vcTerrainRenderer *pTerrainRenderer)
{
  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    if (!pTerrainRenderer->pTiles[i].isUsed)
      return &pTerrainRenderer->pTiles[i];
  }

  return nullptr;
}


vcTile *vcTerrainRenderer_FindOrCreateNewTile(vcTerrainRenderer *pTerrainRenderer, const udInt3 &slippy)
{
  vcTile *pTile = vcTerrainRenderer_FindTile(pTerrainRenderer, slippy);
  if (!pTile)
  {
    pTile = vcTerrainRenderer_FindEmptyTileSlot(pTerrainRenderer);
    if (pTile)
      memset(pTile, 0, sizeof(vcTile));
  }

  if (!pTile)
  {
    pTerrainRenderer->tileCount++;
    if (pTerrainRenderer->tileCount >= pTerrainRenderer->tileCapacity)
    {
      pTerrainRenderer->tileCapacity *= 2;
      pTerrainRenderer->pTiles = (vcTile*)udRealloc(pTerrainRenderer->pTiles, sizeof(vcTile) * pTerrainRenderer->tileCapacity);

      // udRealloc does not zero the remaining memory
      memset(pTerrainRenderer->pTiles + pTerrainRenderer->tileCount, 0, sizeof(vcTile) * (pTerrainRenderer->tileCapacity - pTerrainRenderer->tileCount));
    }

    pTile = &pTerrainRenderer->pTiles[pTerrainRenderer->tileCount - 1];
  }

  pTile->slippyCoord = slippy;
  return pTile;
}


vcCachedTexture* FindTexture(udChunkedArray<vcCachedTexture> &textures, const udInt3 &slippyCoord)
{
  for (size_t i = 0; i < textures.length; ++i)
  {
    vcCachedTexture *pTexture = &textures[i];
    if (pTexture->id.x == slippyCoord.x && pTexture->id.y == slippyCoord.y && pTexture->id.z == slippyCoord.z)
    {
      return pTexture;
    }
  }

  return nullptr;
}


vcTexture* AssignTileTexture(vcTerrainRenderer *pTerrainRenderer, vcTile *pTile, udDouble3 &tileCenterPosition, double distToCameraSqr)
{
  vcTexture *pResultTexture = nullptr;
  vcCachedTexture *pCachedTexture = FindTexture(pTerrainRenderer->cache.textures, pTile->slippyCoord);

  if (pCachedTexture == nullptr)
  {
    // Texture needs to be loaded, cache it
    pTerrainRenderer->cache.textures.PushBack(&pCachedTexture);

    pCachedTexture->pData = nullptr;
    pCachedTexture->id = pTile->slippyCoord;
    pCachedTexture->pTexture = nullptr;

    pCachedTexture->priority = distToCameraSqr;
    pCachedTexture->tilePosition = tileCenterPosition;
    pTerrainRenderer->cache.textureLoadList.PushBack(pCachedTexture);
    udIncrementSemaphore(pTerrainRenderer->cache.pSemaphore);
  }
  else if (pCachedTexture->pData != nullptr) // texture is already in cache but might not be loaded yet
  {
    vcTexture_Create(&pCachedTexture->pTexture, pCachedTexture->width, pCachedTexture->height, pCachedTexture->pData, vcTextureFormat_RGBA8, vcTFM_Linear, true, vcTWM_Clamp, vcTCF_None, 16);
    udFree(pCachedTexture->pData);

    // inform parent
    if (pTile->pParent)
    {
      pTile->pParent->waitingForChildLoadMask &= ~pTile->childMaskInParent;
    }
  }

  if (pCachedTexture != nullptr && pCachedTexture->pTexture != nullptr)
    pResultTexture = pCachedTexture->pTexture;

  pCachedTexture->isVisible = true;

  return pResultTexture;
}


void vcTerrainRenderer_CleanUpEmptyTiles(vcTerrainRenderer *pTerrainRenderer)
{
  for (int i = pTerrainRenderer->tileCount - 1; i >= 0; --i)
  {
    vcTile *pTile = &pTerrainRenderer->pTiles[i];
    if (pTile->isUsed)
      return;

    pTerrainRenderer->tileCount--;
  }
}


void vcTerrainRenderer_FreeUnusedTiles(vcTerrainRenderer *pTerrainRenderer)
{
  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    vcTile *pTile = &pTerrainRenderer->pTiles[i];
    if (pTile->isUsed && !pTile->isLeaf && !pTile->waitingForChildLoadMask) // not a child, and no children depend on this
    {
      pTile->isUsed = false;

      // inform children that their parent has been removed
      for (int c = 0; c < 4; ++c)
      {
        vcTile *pChild = vcTerrainRenderer_FindChildOfTile(pTerrainRenderer, pTile, udInt2::create(c % 2, c / 2));
        if (pChild)
          pChild->pParent = nullptr;
      }
    }
  }
}


void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, vcGISSpace *pSpace, const udInt3 &slippyCoords, const udDouble3 &cameraLocalPosition, const vcQuadTreeNode *pNodeList, int nodeCount)
{
  // Invalidate tiles
  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    pTerrainRenderer->pTiles[i].isLeaf = false;
    pTerrainRenderer->pTiles[i].waitingForChildLoadMask = 0;
  }
  pTerrainRenderer->activeTiles = 0;

  // invalidate all streaming textures
  udLockMutex(pTerrainRenderer->cache.pMutex);
  for (size_t i = 0; i < pTerrainRenderer->cache.textureLoadList.length; ++i)
    pTerrainRenderer->cache.textureLoadList[i]->isVisible = false;

  // Build our visible tile list
  // Iterate the list in-order so parents *should* be loaded before their children
  for (int i = 0; i < nodeCount; ++i)
  {
    const vcQuadTreeNode *pNode = &pNodeList[i];
    if (!vcQuadTree_IsLeafNode(pNode) || !pNode->isVisible)
      continue;
    ++pTerrainRenderer->activeTiles;

    udInt3 slippyTileCoord = udInt3::create(pNode->slippyPosition.x, pNode->slippyPosition.y, pNode->level + slippyCoords.z);

    vcTile *pTile = vcTerrainRenderer_FindOrCreateNewTile(pTerrainRenderer, slippyTileCoord);
    pTile->childMaskInParent = pNode->childMaskInParent;
    pTile->isUsed = true;

    // Ensure we never go past level MAX_SLIPPY_LEVEL
    int slippyLayerDescendAmount = udMin((MaxSlippyLevel - slippyTileCoord.z), gSlippyLayerDescendAmount[TileVertexResolution]);

    udDouble3 localCorners[TileVertexResolution * TileVertexResolution];
    for (int t = 0; t < TileVertexResolution * TileVertexResolution; ++t)
    {
      udInt2 slippySampleCoord = udInt2::create((slippyTileCoord.x * (1 << slippyLayerDescendAmount)) + (t % TileVertexResolution),
        (slippyTileCoord.y * (1 << slippyLayerDescendAmount)) + (t / TileVertexResolution));
      vcGIS_SlippyToLocal(pSpace, &localCorners[t], slippySampleCoord, slippyTileCoord.z + slippyLayerDescendAmount);
      pTile->worldPosition[t] = udDouble3::create(localCorners[t].x, localCorners[t].y, 0.0);
    }

    udDouble3 tileCenterPosition = (localCorners[1] + localCorners[2]) * 0.5;
    double distToCameraSqr = udMagSq2(cameraLocalPosition.toVector2() - tileCenterPosition.toVector2());
    pTile->pTexture = AssignTileTexture(pTerrainRenderer, pTile, tileCenterPosition, distToCameraSqr);

    // Inform parent to use it's texture if this tile has none
    pTile->pParent = vcTerrainRenderer_FindParentOfTile(pTerrainRenderer, pTile);
    if ((pTile->pTexture == nullptr) && pTile->pParent && pTile->pParent->pTexture)
      pTile->pParent->waitingForChildLoadMask |= pNode->childMaskInParent;

    pTile->isLeaf = true;
  }

  udReleaseMutex(pTerrainRenderer->cache.pMutex);

  vcTerrainRenderer_FreeUnusedTiles(pTerrainRenderer);
  vcTerrainRenderer_CleanUpEmptyTiles(pTerrainRenderer);
}


void vcTerrainRenderer_DrawTile(vcTerrainRenderer *pTerrainRenderer, vcTile *pTile, vcMesh *pMesh, const udDouble4x4 &view, const int subTileIndexes[])
{
  for (int t = 0; t < TileVertexResolution * TileVertexResolution; ++t)
  {
    udFloat4 eyeSpaceVertexPosition = udFloat4::create(view * udDouble4::create(pTile->worldPosition[subTileIndexes[t]], 1.0));
    pTerrainRenderer->presentShader.everyObject.eyePositions[t] = eyeSpaceVertexPosition;
  }

  vcShader_BindConstantBuffer(pTerrainRenderer->presentShader.pProgram, pTerrainRenderer->presentShader.pConstantBuffer, &pTerrainRenderer->presentShader.everyObject, sizeof(pTerrainRenderer->presentShader.everyObject));
  vcMesh_Render(pMesh, TileIndexResolution * TileIndexResolution * 2); // 2 tris per quad
}


void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &view, const udDouble4x4 &proj)
{
  if (pTerrainRenderer->tileCount == 0)
    return;

  pTerrainRenderer->latestViewProjectionMatrix = proj * view;

  // for debugging
#if 0
  int visibleCount = 0;
  for (size_t i = 0; i < pTerrainRenderer->cache.textureLoadList.length; ++i)
    visibleCount += pTerrainRenderer->cache.textureLoadList[i]->isVisible ? 1 : 0;

  printf("Active: %d, Total: %d, Capacity: %d\n", pTerrainRenderer->activeTiles, pTerrainRenderer->tileCount, pTerrainRenderer->tileCapacity);
  printf("visibleTiles=%d: totalLoad=%zu\n", visibleCount, pTerrainRenderer->cache.textureLoadList.length);
#endif

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

  vcShader_Bind(pTerrainRenderer->presentShader.pProgram);

  vcGLState_SetBlendMode(vcGLSBM_AdditiveSrcInterpolativeDst);

  if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Overlay)
    vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);
  else if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Underlay)
    vcGLState_SetViewportDepthRange(1.0f, 1.0f);

  pTerrainRenderer->presentShader.everyObject.projectionMatrix = udFloat4x4::create(proj);
  pTerrainRenderer->presentShader.everyObject.colour = udFloat4::create(1.f, 1.f, 1.f, pTerrainRenderer->pSettings->maptiles.transparency);

  udDouble4x4 mapHeightTranslation = udDouble4x4::translation(0, 0, pTerrainRenderer->pSettings->maptiles.mapHeight);
  udDouble4x4 viewWithMapTranslation = view * mapHeightTranslation;

  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    vcTile *pTile = &pTerrainRenderer->pTiles[i];

    if (!pTile->isUsed)
      continue;

    // This tiles texture is not loaded, however it's parent is
    if (!pTile->pTexture && pTile->pParent && pTile->pParent->pTexture)
      continue;

#if UD_DEBUG
    srand(i);
    pTerrainRenderer->presentShader.everyObject.colour = udFloat4::create(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, pTerrainRenderer->pSettings->maptiles.transparency);
#endif

    vcTexture *pTexture = pTile->pTexture ? pTile->pTexture : pTerrainRenderer->pEmptyTileTexture;
    vcShader_BindTexture(pTerrainRenderer->presentShader.pProgram, pTexture, 0);

    // Split tile and draw sub-regions
    if (pTile->waitingForChildLoadMask != 0)
    {
      for (int c = 0; c < 4; ++c)
      {
        if (pTile->waitingForChildLoadMask & (1 << c))
        {
          vcTerrainRenderer_DrawTile(pTerrainRenderer, pTile, pTerrainRenderer->pSubTileMesh[c], viewWithMapTranslation, gSubTilePositionIndexes[c + 1]);
        }
      }
    }
    else
    {
      // draw the full tile
      vcTerrainRenderer_DrawTile(pTerrainRenderer, pTile, pTerrainRenderer->pFullTileMesh, viewWithMapTranslation, gSubTilePositionIndexes[0]);
    }
  }

  if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Overlay)
    vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
  else if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Underlay)
    vcGLState_SetViewportDepthRange(0.0f, 1.0f);

  vcGLState_SetBlendMode(vcGLSBM_None);

  vcShader_Bind(nullptr);
}


void vcTerrainRenderer_ClearCache(vcTerrainRenderer *pTerrainRenderer)
{
  udLockMutex(pTerrainRenderer->cache.pMutex);
  pTerrainRenderer->cache.textureLoadList.Clear();
  udReleaseMutex(pTerrainRenderer->cache.pMutex);

  for (size_t i = 0; i < pTerrainRenderer->cache.textures.length; ++i)
  {
    udFree(pTerrainRenderer->cache.textures[i].pData);
    vcTexture_Destroy(&pTerrainRenderer->cache.textures[i].pTexture);
  }

  memset(pTerrainRenderer->pTiles, 0, sizeof(vcTile) * pTerrainRenderer->tileCapacity);
  pTerrainRenderer->tileCount = 0;
}
