#include "gl/vcMesh.h"
#include "vcOpenGL.h"

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void* pVerts, int currentVerts, const void *pIndices, int currentIndices, vcMeshFlags flags/* = vcMF_None*/)
{
  if (ppMesh == nullptr || pMeshLayout == nullptr || totalTypes == 0 || pVerts == nullptr || currentVerts == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  vcMesh *pMesh = udAllocType(vcMesh, 1, udAF_Zero);

  uint32_t vertexSize = vcLayout_GetSize(pMeshLayout, totalTypes);

  if (flags & vcMF_Dynamic)
    pMesh->drawType = GL_STREAM_DRAW;
  else
    pMesh->drawType = GL_STATIC_DRAW;


  glGenVertexArrays(1, &pMesh->vao);
  glBindVertexArray(pMesh->vao);

  glGenBuffers(1, &pMesh->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, vertexSize * currentVerts, pVerts, pMesh->drawType);

  if ((flags & vcMF_NoIndexBuffer) == 0)
  {
    if (flags & vcMF_IndexShort)
    {
      pMesh->indexType = GL_UNSIGNED_SHORT;
      pMesh->indexBytes = sizeof(short);
    }
    else
    {
      pMesh->indexType = GL_UNSIGNED_INT;
      pMesh->indexBytes = sizeof(int);
    }

    glGenBuffers(1, &pMesh->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexBytes * currentIndices, pIndices, pMesh->drawType);
  }

  ptrdiff_t accumulatedOffset = 0;
  for (int i = 0; i < totalTypes; ++i)
  {
    if (pMeshLayout[i] >= vcVLT_TotalTypes)
      UD_ERROR_SET(udR_InvalidParameter_);

    switch (pMeshLayout[i])
    {
    case vcVLT_Position2:
      glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_Position3:
      glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 3 * sizeof(float);
      break;
    case vcVLT_TextureCoords2:
      glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_ColourBGRA:
      glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 1 * sizeof(uint32_t);
      break;
    case vcVLT_TotalTypes:
      break; // never reaches here due to error set above
    }
    glEnableVertexAttribArray(i);

  }

  pMesh->vertexCount = currentVerts;
  pMesh->indexCount = currentIndices;

epilogue:
  glBindVertexArray(0);
  VERIFY_GL();

  if (result != udR_Success && pMesh != nullptr)
    vcMesh_Destroy(&pMesh);

  *ppMesh = pMesh;

  return result;
}

void vcMesh_Destroy(vcMesh **ppMesh)
{
  glDeleteBuffers(1, &(*ppMesh)->vbo);
  glDeleteBuffers(1, &(*ppMesh)->ibo);
  glDeleteVertexArrays(1, &(*ppMesh)->vao);

  udFree(*ppMesh);
}

udResult vcMesh_UploadData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0 || pMesh->drawType == GL_STATIC_DRAW)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  uint32_t vertexSize = vcLayout_GetSize(pLayout, totalTypes);

  pMesh->vertexCount = totalVerts;
  pMesh->indexCount = totalIndices;

  glBindVertexArray(pMesh->vao);

  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, vertexSize * totalVerts, pVerts, pMesh->drawType);

  if (pMesh->indexType != GL_NONE)
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexBytes * totalIndices, pIndices, pMesh->drawType);
  }

  return result;
}

bool vcMesh_RenderTriangles(vcMesh *pMesh, uint32_t numTriangles, uint32_t startIndex)
{
  if (pMesh == nullptr || numTriangles == 0 || pMesh->indexCount < (numTriangles + startIndex) * 3)
    return false;

  VERIFY_GL();

  glBindVertexArray(pMesh->vao);
  VERIFY_GL();
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  VERIFY_GL();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
  VERIFY_GL();

  if(pMesh->indexType != GL_NONE)
    glDrawElements(GL_TRIANGLES, numTriangles * 3, pMesh->indexType, (void*)(size_t)(startIndex * 3 * pMesh->indexBytes));
  else
    glDrawArrays(GL_TRIANGLES, startIndex * 3, numTriangles * 3);

  VERIFY_GL();

  glBindVertexArray(0);

  return true;
}
