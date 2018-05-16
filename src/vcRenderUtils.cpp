
#include "vcRenderUtils.h"

void vcCreateQuads(vcSimpleVertex *pVerts, int totalVerts, int *pIndices, int totalIndices, GLuint &vboID, GLuint &iboID, GLuint &vaoID)
{
  const size_t BufferSize = sizeof(vcSimpleVertex) * totalVerts;
  const size_t VertexSize = sizeof(vcSimpleVertex);

  glGenVertexArrays(1, &vaoID);
  glBindVertexArray(vaoID);

  glGenBuffers(1, &vboID);
  glBindBuffer(GL_ARRAY_BUFFER, vboID);
  glBufferData(GL_ARRAY_BUFFER, BufferSize, pVerts, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &iboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * totalIndices, pIndices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // Unbind
  glBindVertexArray(0);
  VERIFY_GL();
}

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