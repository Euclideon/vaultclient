#include "vcTerrainRenderer.h"
#include "vcTerrain.h"
#include "vcRenderUtils.h"
#include "vcRenderShaders.h"
#include "vcQuadTree.h"
#include "udPlatform/udPlatformUtil.h"

// temporary hard codeded
static const int vertResolution = 128;
static const int indexResolution = vertResolution - 1;
const char *blankTileTextureFilename = "E:\\Tiles\\UnknownTileTexture.png";
const char *tilesFilePath = "E:\\Tiles";

struct vcTile
{
  vcTexture texture;
  udDouble4x4 world;
};

struct vcTerrainRenderer
{
  vcTile *pTiles;
  int tileCount;

  GLuint vao;
  GLuint vbo;
  GLuint ibo;
  vcTexture blankTileTexture;

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

  pTerrainRenderer->presentShader.program = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, g_terrainTileVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, g_terrainTileFragmentShader));
  pTerrainRenderer->presentShader.uniform_viewProjection = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_viewProjection");
  pTerrainRenderer->presentShader.uniform_world = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_world");
  pTerrainRenderer->presentShader.uniform_texture = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_texture");
  pTerrainRenderer->presentShader.uniform_debugColour = glGetUniformLocation(pTerrainRenderer->presentShader.program, "u_debugColour");

  // build our vert/index list
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
    }
  }
  vcCreateQuads(verts, vertResolution * vertResolution, indices, indexResolution * indexResolution * 6, pTerrainRenderer->vbo, pTerrainRenderer->ibo, pTerrainRenderer->vao);
  
  pTerrainRenderer->blankTileTexture = vcLoadTextureFromDisk(blankTileTextureFilename, nullptr, nullptr, GL_LINEAR, false, 0, GL_CLAMP_TO_EDGE);

  (*ppTerrainRenderer) = pTerrainRenderer;
}

void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer)
{
  vcTerrainRenderer *pTerrainRenderer = (*ppTerrainRenderer);

  glDeleteProgram(pTerrainRenderer->presentShader.program);
  glDeleteBuffers(1, &pTerrainRenderer->vbo);
  glDeleteBuffers(1, &pTerrainRenderer->ibo);
  glDeleteVertexArrays(1, &pTerrainRenderer->vao);

  udFree(*ppTerrainRenderer);
}

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, const vcQuadTreeNode *pNodeList, int nodeCount, int leafNodeCount)
{
  char buff[256];

  // for now just throw everything out
  for (int i = 0; i < pTerrainRenderer->tileCount; ++i)
    vcDestroyTexture(&pTerrainRenderer->pTiles[i].texture);
  udFree(pTerrainRenderer->pTiles);

  pTerrainRenderer->pTiles = udAllocType(vcTile, leafNodeCount, udAF_Zero);
  pTerrainRenderer->tileCount = leafNodeCount;

  int tileIndex = 0;
  for (int i = 0; i < nodeCount; ++i)
  {
    const vcQuadTreeNode *pNode = &pNodeList[i];
    if (!vcQuadTree_IsLeafNode(pNode))
      continue;

    // just load static tile textures from disk for now
    int tileZ = pNode->level;
    int gridSize = 1 << tileZ;
    int tileX = (int)(pNode->position.x * gridSize);
    int tileY = (gridSize - 1) - (int)(pNode->position.y * gridSize);
    
    udSprintf(buff, sizeof(buff), "%s\\%d\\%d\\%d.png", tilesFilePath, tileZ, tileX, tileY);

    extern float gWorldScale;
    float nodeSize = pNode->childSize * 2.0f;
    pTerrainRenderer->pTiles[tileIndex].world = udDouble4x4::scaleUniform(gWorldScale) * udDouble4x4::translation(udDouble3::create(pNode->position.x, pNode->position.y, 0.0)) * udDouble4x4::scaleUniform(nodeSize);
    pTerrainRenderer->pTiles[tileIndex].texture = vcLoadTextureFromDisk(buff, nullptr, nullptr, GL_LINEAR, false, 0, GL_CLAMP_TO_EDGE);

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

    // randomize colours based on depth
    //srand((int)(pTerrainRenderer->pTiles[i].world.axis.x.x * 1000));
    //glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, 1.0f);
    glUniform3f(pTerrainRenderer->presentShader.uniform_debugColour, 1.0f, 1.0f, 1.0f);

    if (pTerrainRenderer->pTiles[i].texture.id != GL_INVALID_INDEX)
      glBindTexture(GL_TEXTURE_2D, pTerrainRenderer->pTiles[i].texture.id);
    else
      glBindTexture(GL_TEXTURE_2D, pTerrainRenderer->blankTileTexture.id);

    // TODO: instance me
    glDrawElements(GL_TRIANGLES, indexResolution * indexResolution * 6, GL_UNSIGNED_INT, 0);
    VERIFY_GL();
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
