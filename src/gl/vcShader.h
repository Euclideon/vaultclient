#ifndef vcShader_h__
#define vcShader_h__

#include "udPlatform/udMath.h"

struct vcShader;
struct vcShaderUniform;
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

bool vcShader_GetUniformIndex(vcShaderUniform **ppUniform, vcShader *pShader, const char *pUniformName);

bool vcShader_Bind(vcShader *pShader); // nullptr to unbind shader
bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderUniform *pSamplerUniform = nullptr);

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat3 vector);
bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat4x4 matrix);
bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, float floatVal);
bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, int32_t intVal);

#endif
