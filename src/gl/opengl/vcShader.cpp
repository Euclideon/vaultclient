#include "gl/vcShader.h"
#include "vcOpenGL.h"

GLint vcBuildShader(GLenum type, const GLchar *shaderCode)
{
  GLint compiled;
  GLint shaderCodeLen = (GLint)strlen(shaderCode);
  GLint shaderObject = glCreateShader(type);
  glShaderSource(shaderObject, 1, &shaderCode, &shaderCodeLen);
  glCompileShader(shaderObject);
  glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    GLint blen = 1024;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &blen);
    if (blen > 1)
    {
      GLchar* compiler_log = (GLchar*)udAlloc(blen);
      glGetShaderInfoLog(shaderObject, blen, &slen, compiler_log);
      udFree(compiler_log);
    }
    return -1;
  }

  return shaderObject;
}

GLint vcBuildProgram(GLint vertexShader, GLint fragmentShader)
{
  GLint programObject = glCreateProgram();
  if (vertexShader != -1)
    glAttachShader(programObject, vertexShader);
  if (fragmentShader != -1)
    glAttachShader(programObject, fragmentShader);

  glLinkProgram(programObject);
  GLint linked;
  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
  if (!linked)
  {
    GLint blen = 1024;
    GLsizei slen = 0;
    glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &blen);
    if (blen > 1)
    {
      GLchar* linker_log = (GLchar*)udAlloc(blen);
      glGetProgramInfoLog(programObject, blen, &slen, linker_log);
      udFree(linker_log);
    }
    return -1;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return programObject;
}

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcShaderVertexInputTypes * /*pInputTypes*/, uint32_t /*totalInputs*/)
{
  if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr)
    return false;

  vcShader *pShader = udAllocType(vcShader, 1, udAF_Zero);

  pShader->programID = vcBuildProgram(vcBuildShader(GL_VERTEX_SHADER, pVertexShader), vcBuildShader(GL_FRAGMENT_SHADER, pFragmentShader));

  if (pShader->programID == GL_INVALID_INDEX)
    udFree(pShader);

  *ppShader = pShader;

  return (pShader != nullptr);
}

void vcShader_DestroyShader(vcShader **ppShader)
{
  if (ppShader == nullptr || *ppShader == nullptr)
    return;

  glDeleteProgram((*ppShader)->programID);
  udFree(*ppShader);
}

bool vcShader_GetUniformIndex(vcShaderUniform **ppUniform, vcShader *pShader, const char *pUniformName)
{
  if (ppUniform == nullptr || pShader == nullptr || pUniformName == nullptr || pShader->programID == GL_INVALID_INDEX)
    return false;

  GLuint uID = glGetUniformLocation(pShader->programID, pUniformName);

  if (uID == GL_INVALID_INDEX)
    return false;

  vcShaderUniform *pUniform = udAllocType(vcShaderUniform, 1, udAF_Zero);
  pUniform->id = uID;

  *ppUniform = pUniform;

  return true;
}

bool vcShader_Bind(vcShader *pShader)
{
  if (pShader != nullptr && pShader->programID == GL_INVALID_INDEX)
    return false;

  if (pShader == nullptr)
    glUseProgram(0);
  else
    glUseProgram(pShader->programID);

  VERIFY_GL();

  return true;
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderUniform *pSamplerUniform /*= nullptr*/)
{
  if (pTexture == nullptr || pTexture->id == GL_INVALID_INDEX)
    return false;

  udUnused(pShader);

  glActiveTexture(GL_TEXTURE0 + samplerIndex);
  VERIFY_GL();

  if (pTexture->format == vcTextureFormat_Cubemap)
    glBindTexture(GL_TEXTURE_CUBE_MAP, pTexture->id);
  else
    glBindTexture(GL_TEXTURE_2D, pTexture->id);
  VERIFY_GL();

  if (pSamplerUniform != nullptr)
    vcShader_SetUniform(pSamplerUniform, samplerIndex);

  VERIFY_GL();

  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat3 vector)
{
  glUniform3f(pShaderUniform->id, vector.x, vector.y, vector.z);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, udFloat4x4 matrix)
{
  glUniformMatrix4fv(pShaderUniform->id, 1, GL_FALSE, matrix.a);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, float floatVal)
{
  glUniform1f(pShaderUniform->id, floatVal);
  return true;
}

bool vcShader_SetUniform(vcShaderUniform *pShaderUniform, int32_t intVal)
{
  glUniform1i(pShaderUniform->id, intVal);
  return true;
}
