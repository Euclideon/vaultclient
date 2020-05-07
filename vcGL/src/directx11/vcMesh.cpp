#include "vcMesh.h"
#include "vcD3D11.h"

udResult vcMeshInternal_RecreateBuffer(ID3D11Buffer **ppBuffer, D3D11_USAGE drawType, D3D11_BIND_FLAG bindFlag, uint32_t unitSize, int bufferSize, void *pBufferData = nullptr)
{
  if (ppBuffer == nullptr || unitSize == 0 || bufferSize == 0 || (drawType != D3D11_USAGE_DEFAULT && drawType != D3D11_USAGE_DYNAMIC))
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  ID3D11Buffer *pBuffer = *ppBuffer;

  if (pBuffer)
  {
    pBuffer->Release();
    pBuffer = nullptr;
  }

  D3D11_BUFFER_DESC desc;
  D3D11_SUBRESOURCE_DATA initialData;
  D3D11_SUBRESOURCE_DATA *pSubData = nullptr;

  memset(&desc, 0, sizeof(desc));
  desc.Usage = drawType;
  desc.ByteWidth = bufferSize * unitSize;
  desc.BindFlags = bindFlag;
  desc.CPUAccessFlags = (drawType == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
  desc.MiscFlags = 0;

  if (pBufferData != nullptr)
  {
    initialData.pSysMem = pBufferData;
    initialData.SysMemPitch = desc.ByteWidth;
    initialData.SysMemSlicePitch = 0;
    pSubData = &initialData;
  }

  UD_ERROR_IF(g_pd3dDevice->CreateBuffer(&desc, pSubData, &pBuffer) < 0, udR_MemoryAllocationFailure);

epilogue:
  *ppBuffer = pBuffer;

  return result;
}

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void* pVerts, uint32_t currentVerts, const void *pIndices, uint32_t currentIndices, vcMeshFlags flags/* = vcMF_None*/)
{
  bool invalidIndexSetup = ((flags & vcMF_NoIndexBuffer) == 0) && ((pIndices == nullptr && currentIndices > 0) || currentIndices == 0);
  if (ppMesh == nullptr || pMeshLayout == nullptr || totalTypes == 0 || currentVerts == 0 || invalidIndexSetup)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;
  vcMesh *pMesh = nullptr;

  pMesh = udAllocType(vcMesh, 1, udAF_Zero);
  UD_ERROR_NULL(pMesh, udR_MemoryAllocationFailure);

  pMesh->vertexSize = vcLayout_GetSize(pMeshLayout, totalTypes);
  pMesh->drawType = (flags & vcMF_Dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

  // Create vertex buffer
  UD_ERROR_CHECK(vcMeshInternal_RecreateBuffer(&pMesh->pVBO, pMesh->drawType, D3D11_BIND_VERTEX_BUFFER, pMesh->vertexSize, currentVerts, (void*)pVerts));

  if (currentIndices > 0 && ((flags & vcMF_NoIndexBuffer) == 0))
  {
    pMesh->indexBytes = (flags & vcMF_IndexShort) ? sizeof(short) : sizeof(int);

    // Create Index Buffer
    UD_ERROR_CHECK(vcMeshInternal_RecreateBuffer(&pMesh->pIBO, pMesh->drawType, D3D11_BIND_INDEX_BUFFER, pMesh->indexBytes, currentIndices, (void*)pIndices));
  }

  pMesh->vertexCount = currentVerts;
  pMesh->maxVertexCount = currentVerts;
  pMesh->indexCount = currentIndices;
  pMesh->maxIndexCount = currentIndices;
  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));

  *ppMesh = pMesh;
  result = udR_Success;
  pMesh = nullptr;

epilogue:
  if (pMesh != nullptr)
  {
    if (pMesh->pVBO != nullptr)
      pMesh->pVBO->Release();

    if (pMesh->pIBO != nullptr)
      pMesh->pIBO->Release();

    udFree(pMesh);
  }

  return result;
}

void vcMesh_Destroy(vcMesh **ppMesh)
{
  if (ppMesh == nullptr || *ppMesh == nullptr)
    return;

  vcMesh *pMesh = *ppMesh;
  *ppMesh = nullptr;

  if (pMesh->pVBO)
    pMesh->pVBO->Release();

  if (pMesh->pIBO)
    pMesh->pIBO->Release();

  udFree(pMesh);
}

udResult vcMesh_UploadData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0 || pMesh->drawType != D3D11_USAGE_DYNAMIC || (pMesh->indexBytes != 0 && (pIndices == nullptr || totalIndices == 0)))
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  D3D11_MAPPED_SUBRESOURCE vertexResource, indexResource;

  pMesh->vertexSize = vcLayout_GetSize(pLayout, totalTypes);

  if (pMesh->maxVertexCount < (uint32_t)totalVerts)
  {
    UD_ERROR_CHECK(vcMeshInternal_RecreateBuffer(&pMesh->pVBO, pMesh->drawType, D3D11_BIND_VERTEX_BUFFER, pMesh->vertexSize, totalVerts));
    pMesh->maxVertexCount = totalVerts;
  }

  UD_ERROR_IF(g_pd3dDeviceContext->Map(pMesh->pVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexResource) != S_OK, udR_MemoryAllocationFailure);
  memcpy(vertexResource.pData, pVerts, totalVerts * pMesh->vertexSize);
  g_pd3dDeviceContext->Unmap(pMesh->pVBO, 0);

  pMesh->vertexCount = totalVerts;

  if (pMesh->indexBytes != 0)
  {
    if (pMesh->maxIndexCount < (uint32_t)totalIndices)
    {
      UD_ERROR_CHECK(vcMeshInternal_RecreateBuffer(&pMesh->pIBO, pMesh->drawType, D3D11_BIND_INDEX_BUFFER, pMesh->indexBytes, totalIndices));
      pMesh->maxIndexCount = totalIndices;
    }

    UD_ERROR_IF(g_pd3dDeviceContext->Map(pMesh->pIBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexResource) != S_OK, udR_MemoryAllocationFailure);
    memcpy(indexResource.pData, pIndices, totalIndices * pMesh->indexBytes);
    g_pd3dDeviceContext->Unmap(pMesh->pIBO, 0);

    pMesh->indexCount = totalIndices;
  }

  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));
  result = udR_Success;

epilogue:
  return result;
}

udResult vcMesh_UploadSubData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, int startVertex, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{

  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0 || pMesh->drawType != D3D11_USAGE_DYNAMIC)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

  D3D11_MAPPED_SUBRESOURCE vertexResource, indexResource;

  UD_ERROR_IF(g_pd3dDeviceContext->Map(pMesh->pVBO, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &vertexResource) != S_OK, udR_MemoryAllocationFailure);
  memcpy((void*)((uint8_t*)vertexResource.pData + startVertex * pMesh->vertexSize), pVerts, totalVerts * pMesh->vertexSize);
  g_pd3dDeviceContext->Unmap(pMesh->pVBO, 0);

  if (pMesh->indexBytes != 0)
  {
    UD_ERROR_IF(g_pd3dDeviceContext->Map(pMesh->pIBO, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &indexResource) != S_OK, udR_MemoryAllocationFailure);
    memcpy(indexResource.pData, pIndices, totalIndices * pMesh->indexBytes);
    g_pd3dDeviceContext->Unmap(pMesh->pIBO, 0);

    pMesh->indexCount = totalIndices;
  }

  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexSize) + (pMesh->indexCount * pMesh->indexBytes));
  result = udR_Success;

epilogue:
  return result;
}

bool vcMesh_Render(vcMesh *pMesh, uint32_t elementCount /* = 0*/, uint32_t startElement /* = 0*/, vcMeshRenderMode renderMode /*= vcMRM_Triangles*/)
{
  if (pMesh == nullptr || (pMesh->indexBytes > 0 && pMesh->indexCount < (elementCount + startElement) * 3) || (elementCount == 0 && startElement != 0))
    return false;

  if (elementCount == 0)
    elementCount = (pMesh->indexCount ? pMesh->indexCount : pMesh->vertexCount) / 3;

  unsigned int stride = pMesh->vertexSize;
  unsigned int offset = 0;
  g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &pMesh->pVBO, &stride, &offset);

  if (pMesh->indexBytes == 4)
    g_pd3dDeviceContext->IASetIndexBuffer(pMesh->pIBO, DXGI_FORMAT_R32_UINT, 0);
  else if (pMesh->indexBytes == 2)
    g_pd3dDeviceContext->IASetIndexBuffer(pMesh->pIBO, DXGI_FORMAT_R16_UINT, 0);

  D3D11_PRIMITIVE_TOPOLOGY d3dRenderMode = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  int elementsPerPrimitive = 3;
  switch (renderMode)
  {
  case vcMRM_TriangleStrip:
    d3dRenderMode = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Points:
    d3dRenderMode = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Triangles: // fall through
  default:
    d3dRenderMode = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    elementsPerPrimitive = 3;
    break;
  }

  g_pd3dDeviceContext->IASetPrimitiveTopology(d3dRenderMode);

  if (pMesh->indexCount == 0)
    g_pd3dDeviceContext->Draw(elementCount * elementsPerPrimitive, startElement * elementsPerPrimitive);
  else
    g_pd3dDeviceContext->DrawIndexed(elementCount * elementsPerPrimitive, startElement * elementsPerPrimitive, 0);

  vcGLState_ReportGPUWork(1, elementCount * elementsPerPrimitive, 0);
  return true;
}
