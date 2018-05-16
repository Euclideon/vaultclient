#ifndef vcTexture_h__
#define vcTexture_h__

#include "GL/glew.h"

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
vcFramebuffer vcCreateFramebuffer(vcTexture *pTexture, vcTexture *pDepth = nullptr, int level = 0);
vcTexture vcCreateDepthTexture(uint32_t width, uint32_t height, vcTextureFormat format = vcTextureFormat_D24, GLuint filterMode = GL_NEAREST);
void vcDestroyTexture(vcTexture *pTexture);
void vcDestroyFramebuffer(vcFramebuffer *pFramebuffer);

#endif//vcTexture_h__