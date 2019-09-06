#import "gl/vcFramebuffer.h"
#import "vcMetal.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  if (ppFramebuffer == nullptr)
    return false;

  udUnused(level);
  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);

  pFramebuffer->pColor = pTexture;
  pFramebuffer->pDepth = pDepth;
  pFramebuffer->clear = 0;

  [_renderer addFramebuffer:pFramebuffer];

  *ppFramebuffer = pFramebuffer;
  pFramebuffer = nullptr;

  return true;
}

void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer)
{
  if (ppFramebuffer == nullptr || *ppFramebuffer == nullptr)
    return;

  [_renderer destroyFramebuffer:*ppFramebuffer];

  udFree(*ppFramebuffer);
  *ppFramebuffer = nullptr;
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer)
{
  [_renderer setFramebuffer:pFramebuffer];

  return true;
}

// Changing clear colour might not be necessary in release, as it doesn't commit any commandbuffers that haven't had their corresponding framebuffer bound (and presumably rendered to) in a frame
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
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || (x + width) > pAttachment->width || (y + height) > pAttachment->height)
    return false;

  int pixelBytes = 4; // assumed
  MTLRegion region = MTLRegionMake2D(x, y, width, height);
  [_viewCon.renderer.textures[[NSString stringWithUTF8String:pAttachment->ID]] getBytes:pPixels bytesPerRow:pAttachment->width * pixelBytes fromRegion:region mipmapLevel:0];

  return true;
}

bool vcFramebuffer_EndReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  udUnused(pFramebuffer);
  udUnused(pAttachment);
  udUnused(x);
  udUnused(y);
  udUnused(width);
  udUnused(height);
  udUnused(pPixels);

  // Unnecessary for metal
  return false;
}
