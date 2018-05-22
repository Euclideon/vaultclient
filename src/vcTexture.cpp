
#include "vcTexture.h"
#include "vcRenderUtils.h"

vcTexture vcCreateTexture(uint32_t width, uint32_t height, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, GLuint filterMode /*= GL_NEAREST*/, bool hasMipmaps /*= false*/, uint8_t *pPixels /*= nullptr*/, int32_t aniFilter /*= 0*/, int32_t wrapMode /*= GL_REPEAT*/)
{
  vcTexture tex;

  glGenTextures(1, &tex.id);
  glBindTexture(GL_TEXTURE_2D, tex.id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : filterMode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

  if (aniFilter > 0)
  {
    float aniso;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    aniso = udMin((float)aniFilter, aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
  }

  GLint internalFormat;
  switch (format)
  {
  case vcTextureFormat_RGBA8: // fall through
  default:
    internalFormat = GL_RGBA8;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pPixels);

  if (hasMipmaps)
    glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
  VERIFY_GL();

  tex.format = format;
  tex.width = width;
  tex.height = height;
  return tex;
}

vcTexture vcCreateDepthTexture(uint32_t width, uint32_t height, vcTextureFormat format /*= vcTextureFormat_D24*/, GLuint filterMode /*= GL_NEAREST*/)
{
  vcTexture tex;

  glGenTextures(1, &tex.id);
  glBindTexture(GL_TEXTURE_2D, tex.id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
  VERIFY_GL();

  GLint internalFormat;
  switch (format)
  {
  case vcTextureFormat_D24: // fall through
  default:
    internalFormat = GL_DEPTH_COMPONENT24;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); // Could use glTexStorage2D but OpenGL 4.2 only, GL_RED and GL_FLOAT are ignored because of NULL
  glBindTexture(GL_TEXTURE_2D, 0);
  VERIFY_GL();

  tex.format = format;
  tex.width = width;
  tex.height = height;
  return tex;
}

vcFramebuffer vcCreateFramebuffer(vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  vcFramebuffer fbo;
  GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

  glGenFramebuffers(1, &fbo.id);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.id);

  if (pDepth)
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pDepth->id, 0);

  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture->id, level);
  glDrawBuffers(1, DrawBuffers);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  VERIFY_GL();

  fbo.pAttachments[0] = pTexture;
  fbo.pDepth = pDepth;
  return fbo;
}

void vcDestroyTexture(vcTexture *pTexture)
{
  glDeleteTextures(1, &pTexture->id);
}

void vcDestroyFramebuffer(vcFramebuffer *pFramebuffer)
{
  glDeleteFramebuffers(1, &pFramebuffer->id);
}