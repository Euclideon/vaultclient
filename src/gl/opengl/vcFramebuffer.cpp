#include "gl/vcFramebuffer.h"
#include "vcOpenGL.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };

  glGenFramebuffers(1, &pFramebuffer->id);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pFramebuffer->id);

  if (pDepth)
  {
    GLenum attachment = GL_DEPTH_ATTACHMENT;
    if (pDepth->format == vcTextureFormat_D24S8)
      attachment = GL_DEPTH_STENCIL_ATTACHMENT;

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, pDepth->id, 0);
  }

  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture->id, level);
  glDrawBuffers(1, DrawBuffers);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  pFramebuffer->pAttachments[0] = pTexture;
  pFramebuffer->pDepth = pDepth;

  *ppFramebuffer = pFramebuffer;
  return true;
}

void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer)
{
  if (ppFramebuffer == nullptr || *ppFramebuffer == nullptr)
    return;

  glDeleteFramebuffers(1, &(*ppFramebuffer)->id);
  udFree(*ppFramebuffer);
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer)
{
  if (pFramebuffer == nullptr || pFramebuffer->id == GL_INVALID_INDEX)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  else
    glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->id);

  return true;
}

bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour)
{
  GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
  if (pFramebuffer->pDepth && pFramebuffer->pDepth->format == vcTextureFormat_D24S8)
    clearMask |= GL_STENCIL_BUFFER_BIT;

  glClearColor(((colour >> 16) & 0xFF) / 255.f, ((colour >> 8) & 0xFF) / 255.f, (colour & 0xFF) / 255.f, ((colour >> 24) & 0xFF) / 255.f);
  glClear(clearMask);

  return true;
}

bool vcFramebuffer_BeginReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || int(x + width) > pAttachment->width || int(y + height) > pAttachment->height)
    return false;

  bool result = true;
  void *pPixelBuffer = pPixels;
  glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->id);

  if ((pAttachment->flags & vcTCF_AsynchronousRead) == vcTCF_AsynchronousRead)
  {
    // Copy to PBO
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pAttachment->pbos[pAttachment->pboIndex]);
    pPixelBuffer = nullptr;
  }

  switch (pAttachment->format)
  {
  case vcTextureFormat_RGBA8:
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pPixelBuffer);
    break;
  case vcTextureFormat_BGRA8:
    glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pPixelBuffer);
    break;
  case vcTextureFormat_D24S8:
    glReadPixels(x, y, width, height, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, pPixelBuffer);
    break;
  case vcTextureFormat_D32F:
    glReadPixels(x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, pPixelBuffer);
    break;
  case vcTextureFormat_Unknown: // fall through
  case vcTextureFormat_Cubemap: // fall through
  case vcTextureFormat_Count:
    result = false;
  }

  if ((pAttachment->flags & vcTCF_AsynchronousRead) == vcTCF_AsynchronousRead)
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  
  return result;
}

bool vcFramebuffer_EndReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || int(x + width) > pAttachment->width || int(y + height) > pAttachment->height || (pAttachment->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead)
    return false;

  int pixelBytes = 4; // assumptions

  // Read previous PBO back to CPU
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pAttachment->pbos[pAttachment->pboIndex]);
  pAttachment->pboIndex = (pAttachment->pboIndex + 1) & 1;

  uint8_t *pData = (uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width * height * pixelBytes, GL_MAP_READ_BIT);
  if (pData != nullptr)
  {
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    memcpy(pPixels, pData, width * height * pixelBytes);
  } 

  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  return true;
}