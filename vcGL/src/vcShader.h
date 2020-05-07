#ifndef vcShader_h__
#define vcShader_h__

#include "udMath.h"
#include "vcLayout.h"

#include "vcGLState.h"

struct vcShader;
struct vcShaderSampler;
struct vcShaderConstantBuffer;
struct vcTexture;

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs);
template <size_t N> inline bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes (&inputTypes)[N]) { return vcShader_CreateFromText(ppShader, pVertexShader, pFragmentShader, inputTypes, (uint32_t)N); }
bool vcShader_CreateFromFile(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs);
template <size_t N> inline bool vcShader_CreateFromFile(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes (&inputTypes)[N]) { return vcShader_CreateFromFile(ppShader, pVertexShader, pFragmentShader, inputTypes, (uint32_t)N); }
void vcShader_DestroyShader(vcShader **ppShader);

bool vcShader_Bind(vcShader *pShader); // nullptr to unbind shader

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler = nullptr, vcGLSamplerShaderStage samplerStage = vcGLSamplerShaderStage_Fragment);

bool vcShader_GetConstantBuffer(vcShaderConstantBuffer **ppBuffer, vcShader *pShader, const char *pBufferName, const size_t bufferSize);
bool vcShader_BindConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer, const void *pData, const size_t bufferSize);
bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer);

bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName);

#endif
