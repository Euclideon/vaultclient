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
