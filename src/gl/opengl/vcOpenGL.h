#ifndef vcOpenGL_h__
#define vcOpenGL_h__

#include "udPlatform.h"

#if UDPLATFORM_OSX
# include "OpenGL/gl3.h"
# include "OpenGL/gl3ext.h"
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# include "OpenGLES/ES3/gl.h"
# include "OpenGLES/ES3/glext.h"
#elif UDPLATFORM_LINUX
# define GL_GLEXT_PROTOTYPES 1
# include "GL/gl.h"
# include "GL/glu.h"
# include "GL/glext.h"
#else
# include "GL/glew.h"
#endif

#ifndef VERIFY_GL
#ifdef _DEBUG
#define VERIFY_GL() if (int glError = glGetError()) { udDebugPrintf("GLError %d\n", glError); __debugbreak(); }
#else
#define VERIFY_GL()
#endif
#endif //VERIFY_GL

#include "gl/vcGLState.h"
#include "gl/vcTexture.h"
#include "gl/vcMesh.h"

struct vcTexture
{
  GLuint id;

  vcTextureFormat format;
  GLuint width, height;
};

struct vcFramebuffer
{
  GLuint id;

  vcTexture *pAttachments[1];
  vcTexture *pDepth; // optional
};

struct vcShaderConstantBuffer
{
  GLuint id;
  GLuint bindPoint;
  char name[32];
};

struct vcShaderSampler
{
  GLuint id;
};

struct vcShader
{
  GLuint programID;
  vcShaderConstantBuffer bufferObjects[16];
  int numBufferObjects;
  vcShaderSampler samplerIndexes[16];
  int numSamplerIndexes;
};

struct vcMesh
{
  GLuint vao;
  GLuint vbo;
  GLuint ibo;

  uint32_t vertexCount;
  uint32_t indexCount;

  GLenum drawType;
  GLenum indexType;
  int indexBytes;
  uint32_t vertexSize;
};

static const GLuint vcTFMToGL[] = { GL_NEAREST, GL_LINEAR }; // Filter Mode
UDCOMPILEASSERT(UDARRAYSIZE(vcTFMToGL) == vcTFM_Total, "TextureFilterModes not equal size");

static const GLuint vcTWMToGL[] = { GL_REPEAT, GL_CLAMP_TO_EDGE }; // Wrap Mode
UDCOMPILEASSERT(UDARRAYSIZE(vcTWMToGL) == vcTFM_Total, "TextureFilterModes not equal size");


#endif // vcOpenGL_h__
