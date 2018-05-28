#ifndef vcTexture_h__
#define vcTexture_h__

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

#include "udPlatform/udMath.h"

enum vcTextureFormat
{
  vcTextureFormat_Unknown,

  vcTextureFormat_RGBA8,
  vcTextureFormat_D24,

  vcTextureFormat_Count
};

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

vcTexture vcCreateTexture(uint32_t width, uint32_t height, vcTextureFormat format = vcTextureFormat_RGBA8, GLuint filterMode = GL_NEAREST, bool hasMipmaps = false, uint8_t *pPixels = nullptr, int32_t aniFilter = 0, int32_t wrapMode = GL_REPEAT);
vcTexture vcCreateDepthTexture(uint32_t width, uint32_t height, vcTextureFormat format = vcTextureFormat_D24, GLuint filterMode = GL_NEAREST);
vcFramebuffer vcCreateFramebuffer(vcTexture *pTexture, vcTexture *pDepth = nullptr, int level = 0);

vcTexture vcLoadTextureFromDisk(const char *filename, uint32_t *pWidth = nullptr, uint32_t *pHeight = nullptr, int32_t filterMode = GL_LINEAR, bool hasMipmaps = false, int32_t aniFilter = 0, int32_t wrapMode = GL_REPEAT);

void vcDestroyTexture(vcTexture *pTexture);
void vcDestroyFramebuffer(vcFramebuffer *pFramebuffer);

#endif//vcTexture_h__
