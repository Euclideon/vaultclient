#include "vcTerrainRenderer.h"
#include "vcTerrain.h"
#include "vcRenderUtils.h"
#include "vcRenderShaders.h"
#include "vcQuadTree.h"
#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udChunkedArray.h"

// temporary hard codeded
static const int vertResolution = 2;
static const int indexResolution = vertResolution - 1;
const char *blankTileTextureFilename = "UnknownTileTexture.png";
const char *tilesFilePath = "T:\\4PeterAdams\\Tiles";

struct vcTile
{
  vcTexture texture;
  udDouble4x4 world;
};

struct vcCachedTexture
{
  udInt3 id; // each tile has a unique slippy {x,y,z}
  vcTexture texture;
};

struct vcTerrainRenderer
{
  vcTile *pTiles;
  int tileCount;

  GLuint vao;
  GLuint vbo;
  GLuint ibo;
  vcTexture blankTileTexture;

  // cache textures
  udChunkedArray<vcCachedTexture> cachedTextures;

  struct
  {
    GLuint program;
    GLint uniform_world;
    GLint uniform_viewProjection;
    GLint uniform_texture;
    GLint uniform_debugColour;
  } presentShader;
};

void vcTerrainRenderer_Init(vcTerrainRenderer **ppTerrainRenderer)
{
  vcTerrainRenderer *pTerrainRenderer = udAllocType(vcTerrainRenderer, 1, udAF_Zero);

  pTerrainRenderer->cachedTextures.Init(128);

  pTerrainRenderer->presentShader.program = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_terrainTileVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_terrainTileFragmentShader));
  pTerrainRenderer->presentShader.uniform_viewProjection = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_viewProjection");
  pTerrainRenderer->presentShader.uniform_world = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_world");
  pTerrainRenderer->presentShader.uniform_texture = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_texture");
  pTerrainRenderer->presentShader.uniform_debugColour = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_debugColour");

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
  
  pTerrainRenderer->blankTileTexture = vcTextureLoadFromDisk(blankTileTextureFilename, nullptr, nullptr, GL_LINEAR, false, 0, GL_CLAMP_TO_EDGE);
  (*ppTerrainRenderer) = pTerrainRenderer;
}

void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer)
{
  vcTerrainRenderer *pTerrainRenderer = (*ppTerrainRenderer);

  pTerrainRenderer->cachedTextures.Deinit();

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

vcTexture AssignTileTexture(vcTerrainRenderer *pTerrainRenderer, int tileX, int tileY, int tileZ)
{
  vcTexture resultTexture = pTerrainRenderer->blankTileTexture;

  static char buff[256];
  vcCachedTexture *pCachedTexture = FindTexture(pTerrainRenderer->cachedTextures, tileX, tileY, tileZ);
  if (!pCachedTexture)
  {
    udSprintf(buff, sizeof(buff), "%s\\%d\\%d\\%d.png", tilesFilePath, tileZ, tileX, tileY);
    vcTexture loadedTexture = vcTextureLoadFromDisk(buff, nullptr, nullptr, GL_LINEAR, false, 0, GL_CLAMP_TO_EDGE);
    if (loadedTexture.id != GL_INVALID_INDEX)
    {
      // Texture load successful, cache it
      pTerrainRenderer->cachedTextures.PushBack(&pCachedTexture);
      pCachedTexture->id.x = tileX;
      pCachedTexture->id.y = tileY;
      pCachedTexture->id.z = tileZ;
      pCachedTexture->texture = loadedTexture;
      resultTexture = loadedTexture;
    }
  }
  else
  {
    // texture is already in cache
    resultTexture = pCachedTexture->texture;
  }

  return resultTexture;
}

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, const udDouble2 worldCorners[4], const udInt3 &slippyCoords, const vcQuadTreeNode *pNodeList, int nodeCount, int leafNodeCount)
{
  udDouble2 worldScale = udDouble2::create(worldCorners[3].x - worldCorners[0].x, worldCorners[0].y - worldCorners[3].y);

  // TODO: for now just throw everything out every frame
  udFree(pTerrainRenderer->pTiles);

  pTerrainRenderer->pTiles = udAllocType(vcTile, leafNodeCount, udAF_Zero);
  pTerrainRenderer->tileCount = leafNodeCount;

  int rootGridSize = 1 << slippyCoords.z;

  int tileIndex = 0;
  for (int i = 0; i < nodeCount; ++i)
  {
    const vcQuadTreeNode *pNode = &pNodeList[i];
    if (!vcQuadTree_IsLeafNode(pNode))
      continue;

    int offsetX = slippyCoords.x << pNode->level;
    int offsetY = ((rootGridSize-1) - slippyCoords.y) << pNode->level; // invert

    int tileZ = pNode->level + slippyCoords.z;
    int gridSizeAtLevel = 1 << pNode->level;
    int totalGridSize = 1 << tileZ;
    int tileX = (int)(pNode->position.x * gridSizeAtLevel) + offsetX;
    int tileY = (totalGridSize - 1) - ((int)(pNode->position.y * gridSizeAtLevel) + offsetY); // invert

    pTerrainRenderer->pTiles[tileIndex].texture = AssignTileTexture(pTerrainRenderer, tileX, tileY, tileZ);

    float nodeSize = pNode->childSize * 2.0f;
    pTerrainRenderer->pTiles[tileIndex].world = udDouble4x4::translation(worldCorners[0].x, worldCorners[3].y, 0) * udDouble4x4::scaleNonUniform(worldScale.x, worldScale.y, 1.0) * udDouble4x4::translation(udDouble3::create(pNode->position.x, pNode->position.y, 0.0)) * udDouble4x4::scaleUniform(nodeSize);
    ++tileIndex;
  }
}

void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &viewProjection)
{
  if (pTerrainRenderer->tileCount == 0)
    return;

  glUseProgram(pTerrainRenderer->presentShader.program);

  udFloat4x4 viewProjectionF = udFloat4x4::create(viewProjection);
  glUniformMatrix4fv(pTerrainRenderer->presentShader.uniform_viewProjection, 1, GL_FALSE, viewProjectionF.a);

  glBindVertexArray(pTerrainRenderer->vao);
  glBindBuffer(GL_ARRAY_BUFFER, pTerrainRenderer->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pTerrainRenderer->ibo);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(pTerrainRenderer->presentShader.uniform_texture, 0);

  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
  {
    udFloat4x4 tileWorldF = udFloat4x4::create(pTerrainRenderer->pTiles[i].world);
    glUniformMatrix4fv(pTerrainRenderer->presentShader.uniform_world, 1, GL_FALSE, tileWorldF.a);

    // Unused colour at the moment.
    glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, 1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, pTerrainRenderer->pTiles[i].texture.id);

    // TODO: instance me
    glDrawElements(GL_TRIANGLES, indexResolution * indexResolution * 6, GL_UNSIGNED_INT, 0);
    VERIFY_GL();
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
