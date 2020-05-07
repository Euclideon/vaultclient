#include "vcMesh.h"
#include "vcOpenGL.h"

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void* pVerts, uint32_t currentVerts, const void *pIndices, uint32_t currentIndices, vcMeshFlags flags/* = vcMF_None*/)
{
  bool invalidIndexSetup = ((flags & vcMF_NoIndexBuffer) == 0) && ((pIndices == nullptr && currentIndices > 0) || currentIndices == 0);
  if (ppMesh == nullptr || pMeshLayout == nullptr || totalTypes == 0 || currentVerts == 0 || invalidIndexSetup)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  ptrdiff_t accumulatedOffset = 0;
  vcMesh *pMesh = nullptr;

  pMesh = udAllocType(vcMesh, 1, udAF_Zero);
  UD_ERROR_NULL(pMesh, udR_MemoryAllocationFailure);

  pMesh->vertexSize = vcLayout_GetSize(pMeshLayout, totalTypes);

  if (flags & vcMF_Dynamic)
    pMesh->drawType = GL_STREAM_DRAW;
  else
    pMesh->drawType = GL_STATIC_DRAW;

  glGenVertexArrays(1, &pMesh->vao);
  glBindVertexArray(pMesh->vao);

  glGenBuffers(1, &pMesh->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, pMesh->vertexSize * currentVerts, pVerts, pMesh->drawType);

  if (currentIndices > 0 && ((flags & vcMF_NoIndexBuffer) == 0))
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

  for (int i = 0; i < totalTypes; ++i)
  {
    if (pMeshLayout[i] >= vcVLT_TotalTypes)
      UD_ERROR_SET(udR_InvalidParameter_);
    switch (pMeshLayout[i])
    {
    case vcVLT_Position2:
      glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_Position3:
      glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 3 * sizeof(float);
      break;
    case vcVLT_Position4:
      glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 4 * sizeof(float);
      break;
    case vcVLT_TextureCoords2:
      glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_ColourBGRA:
      glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, GL_TRUE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 1 * sizeof(uint32_t);
      break;
    case vcVLT_Normal3:
      glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 3 * sizeof(float);
      break;
    case vcVLT_QuadCorner:
      glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_Color0:
      glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 4 * sizeof(float);
      break;
    case vcVLT_Color1:
      glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, pMesh->vertexSize, (GLvoid*)accumulatedOffset);
      accumulatedOffset += 4 * sizeof(float);
      break;
    case vcVLT_Unsupported: // TODO: (EVC-641) Handle unsupported attributes interleaved with supported attributes
      continue; // NOTE continue
    case vcVLT_TotalTypes:
      break; // never reaches here due to error set above
    }
    glEnableVertexAttribArray(i);

  }

  pMesh->vertexCount = currentVerts;
  pMesh->indexCount = currentIndices;
  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));

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
  if (ppMesh == nullptr || *ppMesh == nullptr)
    return;

  vcMesh *pMesh = *ppMesh;
  *ppMesh = nullptr;

  glDeleteBuffers(1, &pMesh->vbo);
  glDeleteBuffers(1, &pMesh->ibo);
  glDeleteVertexArrays(1, &pMesh->vao);

  udFree(pMesh);
}

udResult vcMesh_UploadData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0 || pMesh->drawType == GL_STATIC_DRAW)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  pMesh->vertexSize = vcLayout_GetSize(pLayout, totalTypes);

  pMesh->vertexCount = totalVerts;
  pMesh->indexCount = totalIndices;

  glBindVertexArray(pMesh->vao);

  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, pMesh->vertexSize * totalVerts, pVerts, pMesh->drawType);

  if (pMesh->indexType != GL_NONE)
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexBytes * totalIndices, pIndices, pMesh->drawType);
  }

  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));
  return result;
}

udResult vcMesh_UploadSubData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, int startVertex, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0 || pMesh->drawType == GL_STATIC_DRAW)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  pMesh->vertexSize = vcLayout_GetSize(pLayout, totalTypes);

  glBindVertexArray(pMesh->vao);

  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBufferSubData(GL_ARRAY_BUFFER, startVertex * pMesh->vertexSize, totalVerts * pMesh->vertexSize, pVerts);

  if (pIndices != nullptr && totalIndices > 0 && pMesh->indexType != GL_NONE)
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexBytes * totalIndices, pIndices, pMesh->drawType);
  }

  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));
  return result;
}

bool vcMesh_Render(vcMesh *pMesh, uint32_t elementCount /* = 0*/, uint32_t startElement /* = 0*/, vcMeshRenderMode renderMode /*= vcMRM_Triangles*/)
{
  if (pMesh == nullptr || (pMesh->indexBytes > 0 && pMesh->indexCount < (elementCount + startElement) * 3) || (elementCount == 0 && startElement != 0))
    return false;

  if (elementCount == 0)
    elementCount = (pMesh->indexCount ? pMesh->indexCount : pMesh->vertexCount) / 3;

  glBindVertexArray(pMesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, pMesh->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->ibo);

  GLenum glRenderMode = GL_TRIANGLES;
  int elementsPerPrimitive = 3;
  switch (renderMode)
  {
  case vcMRM_TriangleStrip:
    glRenderMode = GL_TRIANGLE_STRIP;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Points:
    glRenderMode = GL_POINTS;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Triangles: // fall through
  default:
    glRenderMode = GL_TRIANGLES;
    elementsPerPrimitive = 3;
    break;
  }

  if (pMesh->indexCount == 0)
    glDrawArrays(glRenderMode, startElement * elementsPerPrimitive, elementCount * elementsPerPrimitive);
  else
    glDrawElements(glRenderMode, elementCount * elementsPerPrimitive, pMesh->indexType, (void*)(size_t)(startElement * elementsPerPrimitive * pMesh->indexBytes));

  VERIFY_GL();

  glBindVertexArray(0);

  vcGLState_ReportGPUWork(1, elementCount * elementsPerPrimitive, 0);
  return true;
}
