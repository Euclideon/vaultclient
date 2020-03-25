#import "gl/vcFramebuffer.h"
#import "vcMetal.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, uint32_t level /*= 0*/)
{
  udUnused(level);

  if (ppFramebuffer == nullptr || pTexture == nullptr)
    return false;

  udResult result = udR_Success;

  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  UD_ERROR_NULL(pFramebuffer, udR_MemoryAllocationFailure);

  pFramebuffer->pColor = pTexture;
  pFramebuffer->pDepth = pDepth;
  pFramebuffer->clear = 0;
  
  @autoreleasepool
  {
    [_renderer addFramebuffer:pFramebuffer];
  }
  
  *ppFramebuffer = pFramebuffer;
  pFramebuffer = nullptr;

epilogue:
  if (pFramebuffer != nullptr)
    vcFramebuffer_Destroy(&pFramebuffer);

  return result == udR_Success;
}

void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer)
{
  if (ppFramebuffer == nullptr || *ppFramebuffer == nullptr)
    return;

  @autoreleasepool
  {
    [_renderer destroyFramebuffer:*ppFramebuffer];
  }
  
  udFree(*ppFramebuffer);
  *ppFramebuffer = nullptr;
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer, const vcFramebufferClearOperation clearOperation /*= vcFramebufferClearOperation_None*/, uint32_t clearColour /*= 0x0*/, const vcFramebufferClearOperation clearPreviousOperation /*= vcFramebufferClearOperation_None*/)
{
  udUnused(clearOperation);
  udUnused(clearPreviousOperation);

  if (pFramebuffer == nullptr)
    return false;

  [_renderer setFramebuffer:pFramebuffer];

  if (pFramebuffer->clear != clearColour)
  {
    udFloat4 col = udFloat4::create(((clearColour >> 16) & 0xFF) / 255.f, ((clearColour >> 8) & 0xFF) / 255.f, (clearColour & 0xFF) / 255.f, ((clearColour >> 24) & 0xFF) / 255.f);

    pFramebuffer->pRenderPass.colorAttachments[0].clearColor = MTLClearColorMake(col.x,col.y,col.z,col.w);
    pFramebuffer->pRenderPass.depthAttachment.clearDepth = 1.0;
  }

  return true;
}
