#ifndef vcFramebuffer_h__
#define vcFramebuffer_h__

#include "udMath.h"
#include "vcTexture.h"

enum vcFramebufferClearOperation
{
  vcFramebufferClearOperation_None,

  vcFramebufferClearOperation_Colour,
  vcFramebufferClearOperation_DepthStencil,

  vcFramebufferClearOperation_All
};

struct vcFramebuffer;

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth = nullptr, uint32_t level = 0, vcTexture *pAttachment2 = nullptr, vcTexture *pAttachment3 = nullptr);
void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer);

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer, const vcFramebufferClearOperation clearOperation = vcFramebufferClearOperation_None, uint32_t clearColour = 0x0, const vcFramebufferClearOperation clearPreviousOperation = vcFramebufferClearOperation_None);

#endif//vcFramebuffer_h__
