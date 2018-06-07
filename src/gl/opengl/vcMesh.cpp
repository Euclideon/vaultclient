#include "gl/vcMesh.h"
#include "vcOpenGL.h"

bool vcMesh_CreateSimple(vcMesh **ppMesh, const vcSimpleVertex *pVerts, int totalVerts, const int *pIndices, int totalIndices)
{
  if (ppMesh == nullptr || pVerts == nullptr || pIndices == nullptr || totalVerts == 0 || totalIndices == 0)
    return false;

  vcMesh *pMesh = udAllocType(vcMesh, 1, udAF_Zero);

  const size_t BufferSize = sizeof(vcSimpleVertex) * totalVerts;
  const size_t VertexSize = sizeof(vcSimpleVertex);

  glGenVertexArrays(1, &pMesh->vao);
  glBindVertexArray(pMesh->vao);

  glGenBuffers(1, &pMesh->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, BufferSize, pVerts, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &pMesh->ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * totalIndices, pIndices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // Unbind
  glBindVertexArray(0);
  VERIFY_GL();

  pMesh->vertexCount = totalVerts;
  pMesh->indexCount = totalIndices;

  *ppMesh = pMesh;
  return true;
}

void vcMesh_Destroy(vcMesh **ppMesh)
{
  glDeleteBuffers(1, &(*ppMesh)->vbo);
  glDeleteBuffers(1, &(*ppMesh)->ibo);
  glDeleteVertexArrays(1, &(*ppMesh)->vao);

  udFree(*ppMesh);
}

bool vcMesh_RenderTriangles(vcMesh *pMesh, uint32_t numTriangles, uint32_t startIndex)
{
  if (pMesh == nullptr || numTriangles == 0 || pMesh->indexCount < (numTriangles + startIndex) * 3)
    return false;

  glBindVertexArray(pMesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);

  glDrawElements(GL_TRIANGLES, numTriangles * 3, GL_UNSIGNED_INT, (void*)(startIndex * 3 * sizeof(int)));
  VERIFY_GL();

  glBindVertexArray(0);

  return true;
}
