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

#include "stb_image.h"

// temporary hard codeded
static const int vertResolution = 2;
static const int indexResolution = vertResolution - 1;

struct vcTile
{
  vcTexture texture;
  udDouble4x4 world;
};

struct vcCachedTexture
{
  udInt3 id; // each tile has a unique slippy {x,y,z}
  vcTexture texture;
  void *pData;
};

struct vcTerrainRenderer
{
  vcTile *pTiles;
  int tileCount;

  GLuint vao;
  GLuint vbo;
  GLuint ibo;

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
    GLint uniform_world;
    GLint uniform_viewProjection;
    GLint uniform_texture;
    GLint uniform_debugColour;

    GLint uniform_mapHeight;
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
      vcCachedTexture *pNextTexture;

      udLockMutex(pCache->pMutex);
      bool gotData = pCache->textureLoadList.PopFront(&pNextTexture);
      udReleaseMutex(pCache->pMutex);

      if (!gotData)
        continue;

      char buff[256];
      udSprintf(buff, sizeof(buff), "%s/%d/%d/%d.png", pRenderer->pSettings->maptiles.tileServerAddress, pNextTexture->id.z, pNextTexture->id.x, pNextTexture->id.y);

      void *pFileData;
      int64_t fileLen;
      int width, height, channelCount;

      if (udFile_Load(buff, &pFileData, &fileLen) != udR_Success)
        continue;

      uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);

      if (pData == nullptr)
        goto epilogue;

      pNextTexture->pData = udMemDup(pData, sizeof(uint32_t)*width*height, 0, udAF_None);

      pNextTexture->texture.width = width;
      pNextTexture->texture.height = height;
      pNextTexture->texture.format = vcTextureFormat_RGBA8;

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
  pTerrainRenderer->presentShader.uniform_viewProjection = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_viewProjection");
  pTerrainRenderer->presentShader.uniform_world = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_world");
  pTerrainRenderer->presentShader.uniform_texture = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_texture");
  pTerrainRenderer->presentShader.uniform_debugColour = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_debugColour");
  pTerrainRenderer->presentShader.uniform_mapHeight = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_mapHeight");
  pTerrainRenderer->presentShader.uniform_opacity = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_opacity");

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
      verts[index].Position.x = ((float)(x) / vertResolution) * normalizeVertexPositionScale;
      verts[index].Position.y = ((float)(y) / vertResolution) * normalizeVertexPositionScale;
      verts[index].Position.z = 0.0f;

      verts[index].UVs.x = verts[index].Position.x;
      verts[index].UVs.y = verts[index].Position.y;
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
    vcTextureDestroy(&pTerrainRenderer->cache.textures[i].texture);
  }

  pTerrainRenderer->cache.textures.Deinit();
  pTerrainRenderer->cache.textureLoadList.Deinit();

  glDeleteProgram(pTerrainRenderer->presentShader.program);
  glDeleteBuffers(1, &pTerrainRenderer->vbo);
  glDeleteBuffers(1, &pTerrainRenderer->ibo);
  glDeleteVertexArrays(1, &pTerrainRenderer->vao);

  udFree(*ppTerrainRenderer);
}

vcCachedTexture* FindTexture(udChunkedArray<vcCachedTexture> &textures, int x, int y, int z)
{
  for (size_t i = 0; i < textures.length; ++i)
  {
    vcCachedTexture *pTexture = &textures[i];
    if (pTexture->id.x == x && pTexture->id.y == y && pTexture->id.z == z)
    {
      return pTexture;
    }
  }
  return nullptr;
}

vcTexture* AssignTileTexture(vcTerrainRenderer *pTerrainRenderer, int tileX, int tileY, int tileZ)
{
  vcTexture *pResultTexture = nullptr;

  vcCachedTexture *pCachedTexture = FindTexture(pTerrainRenderer->cache.textures, tileX, tileY, tileZ);
  if (!pCachedTexture)
  {
    udLockMutex(pTerrainRenderer->cache.pMutex);

    // Texture needs to be loaded, cache it
    pTerrainRenderer->cache.textures.PushBack(&pCachedTexture);

    pCachedTexture->pData = nullptr;
    pCachedTexture->id.x = tileX;
    pCachedTexture->id.y = tileY;
    pCachedTexture->id.z = tileZ;

    pCachedTexture->texture = vcTextureCreate(1, 1, vcTextureFormat_RGBA8, GL_LINEAR, false, nullptr, 0, GL_CLAMP_TO_EDGE);

    pTerrainRenderer->cache.textureLoadList.PushBack(pCachedTexture);
    udIncrementSemaphore(pTerrainRenderer->cache.pSemaphore);

    udReleaseMutex(pTerrainRenderer->cache.pMutex);
  }
  else
  {
    pResultTexture = &pCachedTexture->texture;

    // texture is already in cache but might not be loaded yet
    if (pCachedTexture->pData != nullptr || pCachedTexture->texture.width == 1 || pCachedTexture->texture.height == 1)
    {
      if (pCachedTexture->pData != nullptr)
      {
        vcTextureUploadPixels(&pCachedTexture->texture, pCachedTexture->pData, pCachedTexture->texture.width, pCachedTexture->texture.height);
        udFree(pCachedTexture->pData);
      }
      else
      {
        pResultTexture = nullptr;
      }
    }
  }

  return pResultTexture;
}

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const vcQuadTreeNode *pNodeList, int nodeCount, int visibleNodeCount)
{
  udDouble2 worldScale = udDouble2::create(worldCorners[3].x - worldCorners[0].x, worldCorners[0].y - worldCorners[3].y);

  // TODO: for now just throw everything out every frame
  udFree(pTerrainRenderer->pTiles);

  pTerrainRenderer->pTiles = udAllocType(vcTile, visibleNodeCount, udAF_Zero);
  pTerrainRenderer->tileCount = visibleNodeCount;

  int rootGridSize = 1 << slippyCoords.z;

  // brute force DFS 
  int tileIndex = 0;
  for (int d = 21; d >= 0; --d)
  {
    for (int i = nodeCount - 1; i >= 0; --i)
    {
      const vcQuadTreeNode *pNode = &pNodeList[i];
      if (!vcQuadTree_IsLeafNode(pNode) || !pNode->isVisible || pNode->level != d)
        continue;

      int offsetX = slippyCoords.x << pNode->level;
      int offsetY = ((rootGridSize - 1) - slippyCoords.y) << pNode->level; // invert

      int tileZ = pNode->level + slippyCoords.z;
      int gridSizeAtLevel = 1 << pNode->level;
      int totalGridSize = 1 << tileZ;
      int tileX = (int)(pNode->position.x * gridSizeAtLevel) + offsetX;
      int tileY = (totalGridSize - 1) - ((int)(pNode->position.y * gridSizeAtLevel) + offsetY); // invert

      vcTexture *pTexture = AssignTileTexture(pTerrainRenderer, tileX, tileY, tileZ);

      if (pTexture == nullptr)
        continue;

      pTerrainRenderer->pTiles[tileIndex].texture = *pTexture;

      float nodeSize = pNode->childSize * 2.0f;
      pTerrainRenderer->pTiles[tileIndex].world = udDouble4x4::translation(worldCorners[0].x, worldCorners[3].y, 0) * udDouble4x4::scaleNonUniform(worldScale.x, worldScale.y, 1.0) * udDouble4x4::translation(udDouble3::create(pNode->position.x, pNode->position.y, 0.0)) * udDouble4x4::scaleUniform(nodeSize);
      ++tileIndex;

      if (tileIndex >= visibleNodeCount) // stop
      {
        d = 0;
        break;
      }
    }
  }
}

void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &viewProjection)
{
  if (pTerrainRenderer->tileCount == 0)
    return;

  glDisable(GL_CULL_FACE);
  glUseProgram(pTerrainRenderer->presentShader.program);

  udFloat4x4 viewProjectionF = udFloat4x4::create(viewProjection);
  glUniformMatrix4fv(pTerrainRenderer->presentShader.uniform_viewProjection, 1, GL_FALSE, viewProjectionF.a);

  glBindVertexArray(pTerrainRenderer->vao);
  glBindBuffer(GL_ARRAY_BUFFER, pTerrainRenderer->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainRenderer->ibo);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(pTerrainRenderer->presentShader.uniform_texture, 0);

  glUniform1f(pTerrainRenderer->presentShader.uniform_mapHeight, pTerrainRenderer->pSettings->maptiles.mapHeight);
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
    udFloat4x4 tileWorldF = udFloat4x4::create(pTerrainRenderer->pTiles[i].world);
    glUniformMatrix4fv(pTerrainRenderer->presentShader.uniform_world, 1, GL_FALSE, tileWorldF.a);


    glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, 1.0f, 1.0f, 1.0f);

    // uncomment these two lines for tile rendering debug view
    //srand(i);
    //glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX);

    glBindTexture(GL_TEXTURE_2D, pTerrainRenderer->pTiles[i].texture.id);

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
    vcTextureDestroy(&pTerrainRenderer->cache.textures[i].texture);
  }

  udFree(pTerrainRenderer->pTiles);
  pTerrainRenderer->tileCount = 0;
}