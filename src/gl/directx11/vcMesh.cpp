#include "gl/vcMesh.h"
#include "vcD3D11.h"

bool vcMesh_CreateSimple(vcMesh **ppMesh, const vcSimpleVertex *pVerts, int totalVerts, const int *pIndices, int totalIndices)
{
  if (ppMesh == nullptr || pVerts == nullptr || pIndices == nullptr || totalVerts == 0 || totalIndices == 0)
    return false;

  bool result = false;
  vcMesh *pMesh = udAllocType(vcMesh, 1, udAF_Zero);

  D3D11_BUFFER_DESC desc;

  //TODO: This shouldn't be dynamic; immutable makes more sense overall

  // Create vertex buffer
  memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.ByteWidth = totalVerts * sizeof(vcSimpleVertex);
  desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  if (g_pd3dDevice->CreateBuffer(&desc, NULL, &pMesh->pVBO) < 0)
    goto epilogue;

  // Create Index Buffer
  memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.ByteWidth = totalIndices * sizeof(int);
  desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  if (g_pd3dDevice->CreateBuffer(&desc, NULL, &pMesh->pIBO) < 0)
    goto epilogue;

  // Copy and convert all vertices into a single contiguous buffer
  D3D11_MAPPED_SUBRESOURCE vertexResource, indexResource;
  if (g_pd3dDeviceContext->Map(pMesh->pVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexResource) != S_OK)
    goto epilogue;

  if (g_pd3dDeviceContext->Map(pMesh->pIBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexResource) != S_OK)
    goto epilogue;

  vcSimpleVertex* pGPUVerts = (vcSimpleVertex*)vertexResource.pData;
  int* pGPUIndices = (int*)indexResource.pData;

  memcpy(pGPUVerts, pVerts, totalVerts * sizeof(vcSimpleVertex));
  memcpy(pGPUIndices, pIndices, totalIndices * sizeof(int));

  g_pd3dDeviceContext->Unmap(pMesh->pVBO, 0);
  g_pd3dDeviceContext->Unmap(pMesh->pIBO, 0);

  pMesh->vertexCount = totalVerts;
  pMesh->indexCount = totalIndices;

  *ppMesh = pMesh;
  result = true;

epilogue:
  if (!result)
  {
    if (pMesh->pVBO)
      pMesh->pVBO->Release();

    if (pMesh->pIBO)
      pMesh->pIBO->Release();
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

bool vcMesh_RenderTriangles(vcMesh *pMesh, uint32_t numTriangles, uint32_t startIndex)
{
  if (pMesh == nullptr || numTriangles == 0 || pMesh->indexCount < (numTriangles + startIndex) * 3)
    return false;

  unsigned int stride = sizeof(vcSimpleVertex);
  unsigned int offset = 0;
  g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &pMesh->pVBO, &stride, &offset);
  g_pd3dDeviceContext->IASetIndexBuffer(pMesh->pIBO, DXGI_FORMAT_R32_UINT, 0);
  g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  g_pd3dDeviceContext->DrawIndexed(numTriangles * 3, startIndex * 3, 0);

  return true;
}
