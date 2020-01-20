#include "gl/vcFramebuffer.h"
#include "vcOpenGL.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, uint32_t level /*= 0*/)
{
  if (ppFramebuffer == nullptr || pTexture == nullptr)
    return false;

  udResult result = udR_Success;
  static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };

  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  UD_ERROR_NULL(pFramebuffer, udR_MemoryAllocationFailure);

  glGenFramebuffers(1, &pFramebuffer->id);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pFramebuffer->id);

  if (pDepth)
  {
    GLenum attachment = GL_DEPTH_ATTACHMENT;
#if VCGL_HASSTENCIL
    if (pDepth->format == vcTextureFormat_D24S8)
      attachment = GL_DEPTH_STENCIL_ATTACHMENT;
#endif

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, pDepth->id, 0);
  }

  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture->id, level);
  glDrawBuffers(1, DrawBuffers);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  pFramebuffer->pAttachments[0] = pTexture;
  pFramebuffer->pDepth = pDepth;

  *ppFramebuffer = pFramebuffer;
  pFramebuffer = nullptr;

epilogue:
  if (pFramebuffer != nullptr)
    vcFramebuffer_Destroy(&pFramebuffer);

  VERIFY_GL();
  return result == udR_Success;
}

void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer)
{
  if (ppFramebuffer == nullptr || *ppFramebuffer == nullptr)
    return;

  glDeleteFramebuffers(1, &(*ppFramebuffer)->id);
  udFree(*ppFramebuffer);

  VERIFY_GL();
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer, const vcFramebufferClearOperation clearOperation /*= vcFramebufferClearOperation_None*/, uint32_t clearColour /*= 0x0*/, const vcFramebufferClearOperation clearPreviousOperation /*= vcFramebufferClearOperation_None*/)
{
  if (pFramebuffer == nullptr)
    return false;

  // OpenGL ES only
  // glInvalidateFramebuffer() only available above our min spec OpenGL version, so disabled
#if UDPLATFORM_ANDROID || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  static const GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
  switch (clearPreviousOperation)
  {
  case vcFramebufferClearOperation_None:
    // No invalidation
    break;
  case vcFramebufferClearOperation_Colour:
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &attachments[0]);
    break;
  case vcFramebufferClearOperation_DepthStencil:
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, &attachments[1]);
    break;
  case vcFramebufferClearOperation_All:
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 3, &attachments[0]);
    break;
  }
#else
  udUnused(clearPreviousOperation);
#endif

  if (pFramebuffer->id == GL_INVALID_INDEX)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  else
    glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->id);


  if (clearOperation == vcFramebufferClearOperation_Colour || clearOperation == vcFramebufferClearOperation_All)
    glClearColor(((clearColour >> 16) & 0xFF) / 255.f, ((clearColour >> 8) & 0xFF) / 255.f, (clearColour & 0xFF) / 255.f, ((clearColour >> 24) & 0xFF) / 255.f);

  switch (clearOperation)
  {
  case vcFramebufferClearOperation_None:
    // Clear nothing
    break;
  case vcFramebufferClearOperation_Colour:
    glClear(GL_COLOR_BUFFER_BIT);
    break;
  case vcFramebufferClearOperation_DepthStencil:
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    break;
  case vcFramebufferClearOperation_All:
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    break;
  }

  VERIFY_GL();
  return true;
}
