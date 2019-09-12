#import "gl/vcFramebuffer.h"
#import "vcMetal.h"

uint32_t g_blitTargetID = 0;

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  if (ppFramebuffer == nullptr)
    return false;

  udUnused(level);
  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);

  pFramebuffer->pColor = pTexture;
  pFramebuffer->pDepth = pDepth;
  pFramebuffer->clear = 0;
  
  @autoreleasepool
  {
    [_renderer addFramebuffer:pFramebuffer];
  }
  
  *ppFramebuffer = pFramebuffer;
  pFramebuffer = nullptr;

  return true;
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

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer)
{
  [_renderer setFramebuffer:pFramebuffer];

  return true;
}

bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour)
{
  if (pFramebuffer == nullptr)
    return false;

  if (pFramebuffer->clear != colour)
  {
    udFloat4 col = udFloat4::create(((colour >> 16) & 0xFF) / 255.f, ((colour >> 8) & 0xFF) / 255.f, (colour & 0xFF) / 255.f, ((colour >> 24) & 0xFF) / 255.f);

    _renderer.renderPasses[pFramebuffer->ID].colorAttachments[0].clearColor = MTLClearColorMake(col.x,col.y,col.z,col.w);
    _renderer.renderPasses[pFramebuffer->ID].depthAttachment.clearDepth = 1.0;
  }

  return true;
}

bool vcFramebuffer_BeginReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  udUnused(pPixels);
  
  if (pFramebuffer == nullptr || pAttachment == nullptr || (x + width) > pAttachment->width || (y + height) > pAttachment->height)
    return false;
  
  @autoreleasepool
  {
    if (pAttachment->blitTarget == 0)
    {
      int pixelBytes = 4;
      [_renderer.blitBuffers addObject:[_device newBufferWithLength:pixelBytes * pAttachment->width * pAttachment->height options:MTLResourceStorageModeShared]];
      
      pAttachment->blitTarget = ++g_blitTargetID;
      pFramebuffer->actions |= vcRFA_Blit;
    }
    else if (_renderer.blitBuffers[pAttachment->blitTarget - 1].length < 4 * pAttachment->width * pAttachment->height)
    {
      [_renderer.blitBuffers replaceObjectAtIndex:pAttachment->blitTarget - 1 withObject:[_device newBufferWithLength:4 * pAttachment->width * pAttachment->height options:MTLResourceStorageModeShared]];
    }
    
    [_renderer.blitEncoder copyFromTexture:_renderer.textures[[NSString stringWithFormat:@"%u",pAttachment->ID]] sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(pAttachment->width, pAttachment->height, 1) toBuffer:_renderer.blitBuffers[pAttachment->blitTarget - 1] destinationOffset:0 destinationBytesPerRow:4 * pAttachment->width destinationBytesPerImage:4 * pAttachment->width * pAttachment->height];
  
    [_renderer flushBlit];
  }
  
  return true;
}

bool vcFramebuffer_EndReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || (x + width) > pAttachment->width || (y + height) > pAttachment->height)
    return false;
  
  if (pAttachment->blitTarget == 0)
    return true;
  
  int pixelBytes = 4; // assumed

  uint32_t *pSource = (uint32_t*)[_renderer.blitBuffers[pAttachment->blitTarget - 1] contents];
  pSource += ((y * pAttachment->width) + x);
  
  uint32_t rowBytes = pixelBytes * width;
  
  for (uint32_t i = 0; i < height; ++i)
  {
    for (uint32_t j = 0; j < width; ++j)
      *((uint32_t*)pPixels + (i * rowBytes)) = *(pSource + (i * pAttachment->width) + j);
  }
  
  return true;
}
