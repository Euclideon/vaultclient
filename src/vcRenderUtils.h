#ifndef vcRenderUtils_h__
#define vcRenderUtils_h__

#include "udPlatform/udPlatform.h"
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

#include "vcTexture.h"
#include "udPlatform/udMath.h"

#ifndef VERIFY_GL
#ifdef _DEBUG
//#define VERIFY_GL() if (int glError = glGetError()) { /*cuDebugLog("GLError %d: (%s)\n", glError, msg);*/ __debugbreak(); }
#define VERIFY_GL()
#else
#define VERIFY_GL()
#endif
#endif //VERIFY_GL

struct vcSimpleVertex
{
  udFloat3 Position;
  udFloat2 UVs;
};

void vcCreateQuads(vcSimpleVertex *pVerts, int totalVerts, int *pIndices, int totalIndices, GLuint &vboID, GLuint &iboID, GLuint &vaoID);

GLint vcBuildShader(GLenum type, const GLchar *shaderCode);
GLint vcBuildProgram(GLint vertexShader, GLint fragmentShader);

#endif//vcRenderUtils_h__
