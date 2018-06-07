#include "gl/vcShader.h"
#include "vcD3D11.h"
#include "udPlatform/udPlatformUtil.h"

#include <d3dcompiler.h>
#include <D3D11Shader.h>

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcShaderVertexInputTypes *pInputTypes, uint32_t totalInputs)
{
  if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr || pInputTypes == nullptr)
    return false;

  vcShader *pShader = udAllocType(vcShader, 1, udAF_Zero);
  ID3D10Blob *pVSBlob, *pPSBlob;
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
    case vcSVIT_Position2:
      pVertexLayout[i] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 8;
      break;
    case vcSVIT_Position3:
      pVertexLayout[i] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 12;
      break;

    case vcSVIT_TextureCoords2:
      pVertexLayout[i] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 8;
      break;

    case vcSVIT_ColourBGRA:
      pVertexLayout[i] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, accumlatedOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
      accumlatedOffset += 4;
      break;
    }
  }

  if (g_pd3dDevice->CreateInputLayout(pVertexLayout, totalInputs, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pShader->pLayout) != S_OK)
    return false;

  ID3D11ShaderReflection *pReflection = NULL;
  D3DReflect(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflection);

  D3D11_SHADER_DESC desc;
  pReflection->GetDesc(&desc);

  // Find Uniform Buffers
  for (uint32_t i = 0; i < desc.ConstantBuffers; ++i)
  {
    unsigned int register_index = 0;
    ID3D11ShaderReflectionConstantBuffer *pBuffer = NULL;
    pBuffer = pReflection->GetConstantBufferByIndex(i);

    D3D11_SHADER_BUFFER_DESC bdesc;
    pBuffer->GetDesc(&bdesc);

    for (unsigned int k = 0; k < desc.BoundResources; ++k)
    {
      D3D11_SHADER_INPUT_BIND_DESC ibdesc;
      pReflection->GetResourceBindingDesc(k, &ibdesc);

      if (!udStrcmp(ibdesc.Name, bdesc.Name))
        register_index = ibdesc.BindPoint;
    }

    //register_index, bdesc.Name, pBuffer, &bdesc;
    //mShaderBuffers.push_back(shaderbuffer);
  }

  // Create the constant buffer
  //{
  //  D3D11_BUFFER_DESC desc;
  //  desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
  //  desc.Usage = D3D11_USAGE_DYNAMIC;
  //  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  //  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  //  desc.MiscFlags = 0;
  //  g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pVertexConstantBuffer);
  //}

  pVSBlob->Release();
  pPSBlob->Release();

  *ppShader = pShader;

  return (pShader != nullptr);
}

void vcShader_DestroyShader(vcShader **ppShader)
{
  if (ppShader == nullptr || *ppShader == nullptr)
    return;

  (*ppShader)->pLayout->Release();
  (*ppShader)->pPixelShader->Release();
  (*ppShader)->pVertexShader->Release();

  //glDeleteProgram((*ppShader)->programID);
  udFree(*ppShader);
}

bool vcShader_GetUniformIndex(vcShaderUniform **ppUniform, vcShader *pShader, const char *pUniformName)
{
  if (ppUniform == nullptr || pShader == nullptr || pUniformName == nullptr)
    return false;

  //GLuint uID = glGetUniformLocation(pShader->programID, pUniformName);
  //
  //if (uID == GL_INVALID_INDEX)
  //  return false;

  vcShaderUniform *pUniform = udAllocType(vcShaderUniform, 1, udAF_Zero);
  //pUniform->id = uID;

  *ppUniform = pUniform;

  return true;
}

bool vcShader_Bind(vcShader *pShader)
{
  if (pShader == nullptr)
    return false;

  g_pd3dDeviceContext->IASetInputLayout(pShader->pLayout);

  g_pd3dDeviceContext->VSSetShader(pShader->pVertexShader, NULL, 0);
  //g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_pVertexConstantBuffer);
  g_pd3dDeviceContext->PSSetShader(pShader->pPixelShader, NULL, 0);

  return true;
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderUniform *pSamplerUniform /*= nullptr*/)
{
  udUnused(pShader);
  udUnused(pSamplerUniform);

  g_pd3dDeviceContext->PSSetSamplers(samplerIndex, 1, &pTexture->pSampler);

  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat3 vector)
{
  //glUniform3f(pShaderUniform->id, vector.x, vector.y, vector.z);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat4x4 matrix)
{
  //glUniformMatrix4fv(pShaderUniform->id, 1, GL_FALSE, matrix.a);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, float floatVal)
{
  //glUniform1f(pShaderUniform->id, floatVal);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, int32_t intVal)
{
  //glUniform1i(pShaderUniform->id, intVal);
  return true;
}
