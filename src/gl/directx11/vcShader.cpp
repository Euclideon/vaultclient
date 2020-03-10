#include "gl/vcShader.h"
#include "vcD3D11.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include <d3dcompiler.h>
#include <D3D11Shader.h>

bool vsShader_InternalReflectShaderConstantBuffers(ID3D10Blob *pBlob, int type, vcShaderConstantBuffer *pBuffers, int *pNextBuffer)
{
  if (pBuffers == nullptr || pNextBuffer == nullptr)
    return false;

  ID3D11ShaderReflection *pReflection = NULL;
  D3DReflect(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflection);

  D3D11_SHADER_DESC desc;
  pReflection->GetDesc(&desc);

  // Find Uniform Buffers
  for (uint32_t i = 0; i < desc.ConstantBuffers; ++i)
  {
    uint32_t register_index = 0;
    ID3D11ShaderReflectionConstantBuffer *pBuffer = NULL;
    pBuffer = pReflection->GetConstantBufferByIndex(i);

    D3D11_SHADER_BUFFER_DESC bdesc;
    pBuffer->GetDesc(&bdesc);

    for (uint32_t k = 0; k < desc.BoundResources; ++k)
    {
      D3D11_SHADER_INPUT_BIND_DESC ibdesc;
      pReflection->GetResourceBindingDesc(k, &ibdesc);

      if (!udStrcmp(ibdesc.Name, bdesc.Name))
        register_index = ibdesc.BindPoint;
    }

    int index = (*pNextBuffer)++;

    //register_index, bdesc.Name, pBuffer, &bdesc;
    //mShaderBuffers.push_back(shaderbuffer);

    udStrcpy(pBuffers[index].bufferName, bdesc.Name);
    pBuffers[index].expectedSize = bdesc.Size;
    pBuffers[index].type = type;
    pBuffers[index].registerSlot = register_index;

    D3D11_BUFFER_DESC bufferdesc;
    bufferdesc.ByteWidth = bdesc.Size;
    bufferdesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferdesc.MiscFlags = 0;
    g_pd3dDevice->CreateBuffer(&bufferdesc, NULL, &pBuffers[index].pBuffer);
  }

  pReflection->Release();

  return true;
}

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs)
{
  if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr || pInputTypes == nullptr)
    return false;

  vcShader *pShader = udAllocType(vcShader, 1, udAF_Zero);
  ID3D10Blob *pVSBlob = nullptr;
  ID3D10Blob *pPSBlob = nullptr;
  ID3D10Blob *pErrorBlob = nullptr;

  // Vertex Shader
  D3DCompile(pVertexShader, udStrlen(pVertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &pVSBlob, &pErrorBlob);

  if (pVSBlob == nullptr)
  {
    udDebugPrintf("%s", pErrorBlob->GetBufferPointer());
    pErrorBlob->Release();

    return false;
  }

  if (g_pd3dDevice->CreateVertexShader((DWORD*)pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &pShader->pVertexShader) != S_OK)
    return false;

  // Create the pixel shader
  D3DCompile(pFragmentShader, udStrlen(pFragmentShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &pPSBlob, &pErrorBlob);

  if (pPSBlob == nullptr)
  {
    udDebugPrintf("%s", pErrorBlob->GetBufferPointer());
    pErrorBlob->Release();

    return false;
  }

  if (g_pd3dDevice->CreatePixelShader((DWORD*)pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pShader->pPixelShader) != S_OK)
    return false;

  // Create the input layout
  D3D11_INPUT_ELEMENT_DESC *pVertexLayout = udAllocStack(D3D11_INPUT_ELEMENT_DESC, totalInputs, udAF_Zero);
  uint32_t accumlatedOffset = 0;

  for (size_t i = 0; i < totalInputs; ++i)
  {
    switch (pInputTypes[i])
    {
    case vcVLT_Position2:
      pVertexLayout[i] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 8;
      break;
    case vcVLT_Position3:
      pVertexLayout[i] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 12;
      break;
    case vcVLT_Position4:
      pVertexLayout[i] = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 16;
      break;
    case vcVLT_TextureCoords2:
      pVertexLayout[i] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 8;
      break;
    case vcVLT_RibbonInfo4:
      pVertexLayout[i] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 16;
      break;
    case vcVLT_ColourBGRA:
      pVertexLayout[i] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 4;
      break;
    case vcVLT_Normal3:
      pVertexLayout[i] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 12;
      break;
    case vcVLT_QuadCorner:
      pVertexLayout[i] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 8;
      break;
    }
  }

  if (g_pd3dDevice->CreateInputLayout(pVertexLayout, totalInputs, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pShader->pLayout) != S_OK)
    return false;

  vsShader_InternalReflectShaderConstantBuffers(pVSBlob, 0, pShader->bufferObjects, &pShader->numBufferObjects);
  vsShader_InternalReflectShaderConstantBuffers(pPSBlob, 1, pShader->bufferObjects, &pShader->numBufferObjects);

  pVSBlob->Release();
  pPSBlob->Release();

  *ppShader = pShader;

  return (pShader != nullptr);
}

void vcShader_DestroyShader(vcShader **ppShader)
{
  if (ppShader == nullptr || *ppShader == nullptr)
    return;

  for (int i = 0; i < (*ppShader)->numBufferObjects; ++i)
    (*ppShader)->bufferObjects[i].pBuffer->Release();

  (*ppShader)->pLayout->Release();
  (*ppShader)->pPixelShader->Release();
  (*ppShader)->pVertexShader->Release();

  udFree(*ppShader);
}

bool vcShader_Bind(vcShader *pShader)
{
  if (pShader != nullptr)
  {
    g_pd3dDeviceContext->IASetInputLayout(pShader->pLayout);

    g_pd3dDeviceContext->VSSetShader(pShader->pVertexShader, NULL, 0);
    g_pd3dDeviceContext->PSSetShader(pShader->pPixelShader, NULL, 0);
  }

  return true;
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSamplerUniform /*= nullptr*/)
{
  udUnused(pShader);
  udUnused(pSamplerUniform);

  if (pTexture == nullptr)
  {
    ID3D11ShaderResourceView *nullView[] = { nullptr };
    g_pd3dDeviceContext->PSSetShaderResources(samplerIndex, 1, nullView);
  }
  else
  {
    g_pd3dDeviceContext->PSSetShaderResources(samplerIndex, 1, &pTexture->pTextureView);
    g_pd3dDeviceContext->PSSetSamplers(samplerIndex, 1, &pTexture->pSampler);
  }

  return true;
}

bool vcShader_GetConstantBuffer(vcShaderConstantBuffer **ppBuffer, vcShader *pShader, const char *pBufferName, const size_t bufferSize)
{
  if (ppBuffer == nullptr || pShader == nullptr || pBufferName == nullptr || bufferSize == 0)
    return false;

  *ppBuffer = nullptr;

  for (int i = 0; i < pShader->numBufferObjects; ++i)
  {
    if (udStrEqual(pShader->bufferObjects[i].bufferName, pBufferName) && bufferSize == pShader->bufferObjects[i].expectedSize)
    {
      *ppBuffer = &pShader->bufferObjects[i];
      break;
    }
  }

  return (*ppBuffer != nullptr);
}

bool vcShader_BindConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer, const void *pData, const size_t bufferSize)
{
  if (pShader == nullptr || pBuffer == nullptr || pData == nullptr || bufferSize == 0)
    return false;

  D3D11_MAPPED_SUBRESOURCE mapped_resource;
  if (g_pd3dDeviceContext->Map(pBuffer->pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
    return false;

  if (mapped_resource.RowPitch == bufferSize)
    memcpy(mapped_resource.pData, pData, bufferSize);
#if UD_DEBUG
  else
    __debugbreak(); // If this is hit please confirm your upload sizes
#endif

  g_pd3dDeviceContext->Unmap(pBuffer->pBuffer, 0);

  if (pBuffer->type == 0)
    g_pd3dDeviceContext->VSSetConstantBuffers(pBuffer->registerSlot, 1, &pBuffer->pBuffer);
  else
    g_pd3dDeviceContext->PSSetConstantBuffers(pBuffer->registerSlot, 1, &pBuffer->pBuffer);

  return true;
}

bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer)
{
  if (pShader == nullptr || pBuffer == nullptr)
    return false;

  //TODO
  return true;
}


bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName)
{
  udUnused(ppSampler);
  udUnused(pShader);
  udUnused(pSamplerName);

  return true;
}
