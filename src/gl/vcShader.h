#ifndef vcShader_h__
#define vcShader_h__

#include "udPlatform/udMath.h"

struct vcShader;
struct vcShaderSampler;
struct vcShaderConstantBuffer;
struct vcTexture;

enum vcShaderVertexInputTypes
{
  vcSVIT_Position2, //Vec2
  vcSVIT_Position3, //Vec3
  vcSVIT_TextureCoords2, //Vec2
  vcSVIT_ColourBGRA, //uint32_t

  vcSVIT_TotalTypes
};

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcShaderVertexInputTypes *pInputTypes, uint32_t totalInputs);
void vcShader_DestroyShader(vcShader **ppShader);

bool vcShader_Bind(vcShader *pShader); // nullptr to unbind shader

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler = nullptr);

bool vcShader_GetConstantBuffer(vcShaderConstantBuffer **ppBuffer, vcShader *pShader, const char *pBufferName, const size_t bufferSize);
bool vcShader_BindConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer, const void *pData, const size_t bufferSize);
bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer);

bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName);

#endif
