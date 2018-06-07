#include "vcTerrainRenderer.h"
#include "vcTerrain.h"
#include "vcRenderUtils.h"
#include "vcRenderShaders.h"
#include "vcQuadTree.h"
#include "vcSettings.h"
#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"
#include "udPlatform/udFile.h"
#include "vcGIS.h"

#include "stb_image.h"

// temporary hard codeded
static const int vertResolution = 2;
static const int indexResolution = vertResolution - 1;

struct vcTile
{
  vcTexture pTexture;
  udDouble4x4 world[4]; // each tile corner has different world transform
};

struct vcCachedTexture
{
  udInt3 id; // each tile has a unique slippy {x,y,z}
  vcTexture *pTexture;
  void *pData;

  double priority; // lower is better. (distanceSqr to camera)
  udDouble3 tilePosition;
  bool isVisible; // if associated tile is part of the current scene (latest quad tree data)
};

struct vcTerrainRenderer
{
  vcTile *pTiles;
  int tileCount;

  GLuint vao;
  GLuint vbo;
  GLuint ibo;

  udDouble4x4 latestViewProjectionMatrix;

  vcSettings *pSettings;

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
    GLuint program;
    GLint uniform_worldViewProjection[4];
    GLint uniform_texture;
    GLint uniform_debugColour;

    GLint uniform_opacity;
  } presentShader;
};

void vcTerrainRenderer_LoadThread(void *pThreadData)
{
  vcTerrainRenderer *pRenderer = (vcTerrainRenderer*)pThreadData;
  vcTerrainRenderer::vcTerrainCache *pCache = &pRenderer->cache;

  while (pCache->keepLoading)
  {
    int loadStatus = udWaitSemaphore(pCache->pSemaphore, 1000);

    if (loadStatus != 0)
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
      udSprintf(buff, sizeof(buff), "%s/%d/%d/%d.png", pRenderer->pSettings->maptiles.tileServerAddress, pNextTexture->id.z, pNextTexture->id.x, pNextTexture->id.y);
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

      pNextTexture->pTexture->width = width;
      pNextTexture->pTexture->height = height;
      pNextTexture->pTexture->format = vcTextureFormat_RGBA8;

epilogue:
      udFree(pFileData);
      stbi_image_free(pData);
    }
  }
}

void vcTerrainRenderer_Init(vcTerrainRenderer **ppTerrainRenderer, vcSettings *pSettings)
{
  vcTerrainRenderer *pTerrainRenderer = udAllocType(vcTerrainRenderer, 1, udAF_Zero);

  pTerrainRenderer->pSettings = pSettings;

  pTerrainRenderer->cache.pSemaphore = udCreateSemaphore();
  pTerrainRenderer->cache.pMutex = udCreateMutex();
  pTerrainRenderer->cache.keepLoading = true;
  pTerrainRenderer->cache.textures.Init(128);
  pTerrainRenderer->cache.textureLoadList.Init(16);

  for (size_t i = 0; i < UDARRAYSIZE(vcTerrainRenderer::vcTerrainCache::pThreads); ++i)
    udThread_Create(&pTerrainRenderer->cache.pThreads[i], (udThreadStart*)vcTerrainRenderer_LoadThread, pTerrainRenderer);

  pTerrainRenderer->presentShader.program = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_terrainTileVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_terrainTileFragmentShader));
  pTerrainRenderer->presentShader.uniform_texture = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_texture");
  pTerrainRenderer->presentShader.uniform_debugColour = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_debugColour");
  pTerrainRenderer->presentShader.uniform_opacity = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_opacity");

  pTerrainRenderer->presentShader.uniform_worldViewProjection[0] = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_worldViewProjection0");
  pTerrainRenderer->presentShader.uniform_worldViewProjection[1] = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_worldViewProjection1");
  pTerrainRenderer->presentShader.uniform_worldViewProjection[2] = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_worldViewProjection2");
  pTerrainRenderer->presentShader.uniform_worldViewProjection[3] = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_worldViewProjection3");

  // build our vertex/index list
  vcSimpleVertex verts[vertResolution * vertResolution];
  int indices[indexResolution* indexResolution * 6];
  for (int y = 0; y < indexResolution; ++y)
  {
    for (int x = 0; x < indexResolution; ++x)
    {
      int index = y * indexResolution + x;
      int vertIndex = y * vertResolution + x;
      indices[index * 6 + 0] = vertIndex + vertResolution;
      indices[index * 6 + 1] = vertIndex + 1;
      indices[index * 6 + 2] = vertIndex;

      indices[index * 6 + 3] = vertIndex + vertResolution;
      indices[index * 6 + 4] = vertIndex + vertResolution + 1;
      indices[index * 6 + 5] = vertIndex + 1;
    }
  }

  float normalizeVertexPositionScale = float(vertResolution) / (vertResolution - 1); // ensure verts are [0, 1]
  for (int y = 0; y < vertResolution; ++y)
  {
    for (int x = 0; x < vertResolution; ++x)
    {
      int index = y * vertResolution + x;
      verts[index].Position.x = float(index ? (1 << (index-1)) : 0);
      verts[index].Position.y = 0.0f;
      verts[index].Position.z = 0.0f;

      verts[index].UVs.x = ((float)(x) / vertResolution) * normalizeVertexPositionScale;
      verts[index].UVs.y = ((float)(y) / vertResolution) * normalizeVertexPositionScale;
    }
  }

  vcCreateQuads(verts, vertResolution * vertResolution, indices, indexResolution * indexResolution * 6, pTerrainRenderer->vbo, pTerrainRenderer->ibo, pTerrainRenderer->vao);
  (*ppTerrainRenderer) = pTerrainRenderer;
}

void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer)
{
  vcTerrainRenderer *pTerrainRenderer = (*ppTerrainRenderer);

  pTerrainRenderer->cache.keepLoading = false;
  udIncrementSemaphore(pTerrainRenderer->cache.pSemaphore);

  for (size_t i = 0; i < UDARRAYSIZE(vcTerrainRenderer::vcTerrainCache::pThreads); ++i)
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

  glDeleteProgram(pTerrainRenderer->presentShader.program);
  glDeleteBuffers(1, &pTerrainRenderer->vbo);
  glDeleteBuffers(1, &pTerrainRenderer->ibo);
  glDeleteVertexArrays(1, &pTerrainRenderer->vao);

  udFree(*ppTerrainRenderer);
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

vcTexture* AssignTileTexture(vcTerrainRenderer *pTerrainRenderer, const udInt3 &slippyCoord, udDouble3 &tileCenterPosition, double distToCameraSqr)
{
  vcTexture *pResultTexture = nullptr;

  vcCachedTexture *pCachedTexture = FindTexture(pTerrainRenderer->cache.textures, slippyCoord);
  if (!pCachedTexture)
  {
    // Texture needs to be loaded, cache it
    pTerrainRenderer->cache.textures.PushBack(&pCachedTexture);

    pCachedTexture->pData = nullptr;
    pCachedTexture->id = slippyCoord;

    vcTexture_Create(&pCachedTexture->pTexture, 1, 1, vcTextureFormat_RGBA8, GL_LINEAR, false, nullptr, 0, GL_CLAMP_TO_EDGE);

    pCachedTexture->priority = distToCameraSqr;
    pCachedTexture->tilePosition = tileCenterPosition;
    pTerrainRenderer->cache.textureLoadList.PushBack(pCachedTexture);
    udIncrementSemaphore(pTerrainRenderer->cache.pSemaphore);
  }
  else
  {
    pResultTexture = pCachedTexture->pTexture;

    // texture is already in cache but might not be loaded yet
    if (pCachedTexture->pData != nullptr || pCachedTexture->pTexture->width == 1 || pCachedTexture->pTexture->height == 1)
    {
      if (pCachedTexture->pData != nullptr)
      {
        vcTexture_UploadPixels(pCachedTexture->pTexture, pCachedTexture->pData, pCachedTexture->pTexture->width, pCachedTexture->pTexture->height);
        udFree(pCachedTexture->pData);
      }
      else
      {
        pResultTexture = nullptr;
      }
    }
  }
  pCachedTexture->isVisible = true;

  return pResultTexture;
}

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, int16_t srid, const udInt3 &slippyCoords, const udDouble3 &cameraLocalPosition, const vcQuadTreeNode *pNodeList, int nodeCount, int visibleNodeCount)
{
  // TODO: for now just throw everything out every frame
  udFree(pTerrainRenderer->pTiles);
  pTerrainRenderer->pTiles = udAllocType(vcTile, visibleNodeCount, udAF_Zero);
  pTerrainRenderer->tileCount = visibleNodeCount;

  // invalidate all streaming textures
  udLockMutex(pTerrainRenderer->cache.pMutex);
  for (size_t i = 0; i < pTerrainRenderer->cache.textureLoadList.length; ++i)
    pTerrainRenderer->cache.textureLoadList[i]->isVisible = false;

  int rootGridSize = 1 << slippyCoords.z;

  int tileIndex = 0;
  for (int i = 0; i <= nodeCount; ++i)
  {
    const vcQuadTreeNode *pNode = &pNodeList[i];
    if (!vcQuadTree_IsLeafNode(pNode) || !pNode->isVisible)
      continue;

    int gridSizeAtLevel = 1 << pNode->level;

    udInt2 offset = udInt2::create(slippyCoords.x << pNode->level, ((rootGridSize - 1) - slippyCoords.y) << pNode->level); // y-inverted
    udInt3 slippyTileCoord = udInt3::create(0, 0, pNode->level + slippyCoords.z);
    int totalGridSize = 1 << slippyTileCoord.z;
    slippyTileCoord.x = (int)(pNode->position.x * gridSizeAtLevel) + offset.x;
    slippyTileCoord.y = (totalGridSize - 1) - ((int)(pNode->position.y * gridSizeAtLevel) + offset.y); // y-inverted

    udDouble3 localCorners[4]; // nw, ne, sw, se
    for (int t = 0; t < 4; ++t)
    {
      vcGIS_SlippyToLocal(srid, &localCorners[t], udInt2::create(slippyTileCoord.x + (t % 2), slippyTileCoord.y + (t / 2)), slippyTileCoord.z);
      pTerrainRenderer->pTiles[tileIndex].world[t] = udDouble4x4::translation(localCorners[t].x, localCorners[t].y, 0.0);
    }
    udDouble3 tileCenterPosition = (localCorners[1] + localCorners[2]) * 0.5;

    double distToCameraSqr = udMagSq2(cameraLocalPosition.toVector2() - tileCenterPosition.toVector2());
    vcTexture *pTexture = AssignTileTexture(pTerrainRenderer, slippyTileCoord, tileCenterPosition, distToCameraSqr);

    pTerrainRenderer->pTiles[tileIndex].pTexture.id = GL_INVALID_INDEX;
    if (pTexture != nullptr)
      pTerrainRenderer->pTiles[tileIndex].pTexture = *pTexture;

    ++tileIndex;
  }
  udReleaseMutex(pTerrainRenderer->cache.pMutex);
}

void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &viewProj)
{
  if (pTerrainRenderer->tileCount == 0)
    return;

  pTerrainRenderer->latestViewProjectionMatrix = viewProj;

  // for debugging
#if 0
  int visibleCount = 0;
  for (size_t i = 0; i < pTerrainRenderer->cache.textureLoadList.length; ++i)
    visibleCount += pTerrainRenderer->cache.textureLoadList[i]->isVisible ? 1 : 0;

  printf("visibleTiles=%d: totalLoad=%zu\n", visibleCount, pTerrainRenderer->cache.textureLoadList.length);
#endif

  udDouble4x4 mapHeightTranslation = udDouble4x4::translation(0, 0, pTerrainRenderer->pSettings->maptiles.mapHeight);

  glDisable(GL_CULL_FACE);
  glUseProgram(pTerrainRenderer->presentShader.program);

  glBindVertexArray(pTerrainRenderer->vao);
  glBindBuffer(GL_ARRAY_BUFFER, pTerrainRenderer->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainRenderer->ibo);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(pTerrainRenderer->presentShader.uniform_texture, 0);

  glUniform1f(pTerrainRenderer->presentShader.uniform_opacity, pTerrainRenderer->pSettings->maptiles.transparency);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Overlay)
  {
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
  }

  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    if (pTerrainRenderer->pTiles[i].pTexture.id == GL_INVALID_INDEX)
      continue;

    for (int t = 0; t < 4; ++t)
    {
      udFloat4x4 tileWV = udFloat4x4::create(viewProj * mapHeightTranslation * pTerrainRenderer->pTiles[i].world[t]);
      glUniformMatrix4fv(pTerrainRenderer->presentShader.uniform_worldViewProjection[t], 1, GL_FALSE, tileWV.a);
    }

#if 0
    srand(i);
    glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX);
#else
    glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, 1.0f, 1.0f, 1.0f);
#endif

    glBindTexture(GL_TEXTURE_2D, pTerrainRenderer->pTiles[i].pTexture.id);

    // TODO: instance me
    glDrawElements(GL_TRIANGLES, indexResolution * indexResolution * 6, GL_UNSIGNED_INT, 0);
    VERIFY_GL();
  }

  if (pTerrainRenderer->pSettings->maptiles.blendMode == vcMTBM_Overlay)
  {
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
  }

  glDisable(GL_BLEND);

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}


void vcTerrainRenderer_ClearCache(vcTerrainRenderer *pTerrainRenderer)
{
  // TODO: pfox confirm this will work
  udLockMutex(pTerrainRenderer->cache.pMutex);
  pTerrainRenderer->cache.textureLoadList.Clear();
  udReleaseMutex(pTerrainRenderer->cache.pMutex);

  for (size_t i = 0; i < pTerrainRenderer->cache.textures.length; ++i)
  {
    udFree(pTerrainRenderer->cache.textures[i].pData);
    vcTexture_Destroy(&pTerrainRenderer->cache.textures[i].pTexture);
  }

  udFree(pTerrainRenderer->pTiles);
  pTerrainRenderer->tileCount = 0;
}
