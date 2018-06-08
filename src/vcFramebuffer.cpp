#include "vcFramebuffer.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

  glGenFramebuffers(1, &pFramebuffer->id);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pFramebuffer->id);

  if (pDepth)
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pDepth->id, 0);

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
