#include "vcRenderer.h"
#include "imgui.h"
#include "vcGLState.h"

@implementation vcRenderer
{
  vcFramebuffer *pFramebuffers[BUFFER_COUNT];
  vcFramebuffer *pCurrFramebuffer;
  vcShader *pCurrShader;

  vcGLStateBlendMode blendMode;

  id<CAMetalDrawable> currDrawable;
  CAMetalLayer *pMetalLayer;

  MTLViewport viewport;
}

- (void)initWithView:(nonnull PlatformView*)view
{
  _blitBuffer = [_queue commandBuffer];
  _blitEncoder = [_blitBuffer blitCommandEncoder];

  pMetalLayer = [self makeBackingLayer];
  pMetalLayer.drawableSize = CGSizeMake(view.frame.size.width, view.frame.size.height);
  view.layer = pMetalLayer;
}

- (void)draw
{
  // Finalise last frame

  // End encoding
  [self flushBlit];

  for (size_t i = 1; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [self flush:i];
  }

  [pFramebuffers[0]->encoder endEncoding];

  if (currDrawable)
    [pFramebuffers[0]->commandBuffer presentDrawable:currDrawable];

  [pFramebuffers[0]->commandBuffer commit];

  currDrawable = [pMetalLayer nextDrawable];
  if (!currDrawable)
    NSLog(@"Drawable unavailable");

  pFramebuffers[0]->pRenderPass.colorAttachments[0].texture = currDrawable.texture;

  pFramebuffers[0]->commandBuffer = [_queue commandBuffer];
  pFramebuffers[0]->encoder = [pFramebuffers[0]->commandBuffer renderCommandEncoderWithDescriptor:pFramebuffers[0]->pRenderPass];
}

- (nullable CAMetalLayer*)makeBackingLayer
{
  CAMetalLayer* pLayer = [CAMetalLayer layer];
  pLayer.device = _device;

  pLayer.autoresizingMask = kCALayerHeightSizable | kCALayerWidthSizable;
  pLayer.needsDisplayOnBoundsChange = YES;
  pLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  pLayer.allowsNextDrawableTimeout = NO;

  return pLayer;
}

- (void)setFrameSize:(NSSize)newSize
{
  if (pCurrFramebuffer->ID == 0 && ((int)newSize.width != (int)pFramebuffers[0]->pRenderPass.colorAttachments[0].texture.width || (int)newSize.height != (int)pFramebuffers[0]->pRenderPass.colorAttachments[0].texture.height))
  {
    pMetalLayer.drawableSize = CGSizeMake(newSize.width, newSize.height);
    pCurrFramebuffer->pColor->width = newSize.width;
    pCurrFramebuffer->pColor->height = newSize.height;
    pCurrFramebuffer->actions |= vcRFA_Resize;

    if (pFramebuffers[0]->pDepth)
    {
      vcTexture *pIntermediate;
      vcTexture_Create(&pIntermediate, newSize.width, newSize.height, nullptr, pFramebuffers[0]->pDepth->format, vcTFM_Nearest, vcTCF_RenderTarget);

      vcTexture_Destroy(&pFramebuffers[0]->pDepth);

      pFramebuffers[0]->pDepth = pIntermediate;
      pFramebuffers[0]->pRenderPass.depthAttachment.texture = pIntermediate->texture;
      if (pIntermediate->format == vcTextureFormat_D24S8)
        pFramebuffers[0]->pRenderPass.stencilAttachment.texture = pFramebuffers[0]->pRenderPass.depthAttachment.texture;
    }
  }
}

- (void)bindPipeline:(nonnull vcShader *)pShader
{
  pCurrShader = pShader;

  [self bindBlendState:blendMode];
}

- (void)bindTexture:(nonnull struct vcTexture *)pTexture index:(NSInteger)tIndex
{
  [pCurrFramebuffer->encoder setFragmentTexture:pTexture->texture atIndex:tIndex];
  [pCurrFramebuffer->encoder setFragmentSamplerState:pTexture->sampler atIndex:tIndex];
}

- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable vcGLStencilSettings *)pStencil
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
    {
      [pFramebuffers[i]->encoder setDepthStencilState:dsState];

      if (pStencil != nullptr)
        [pFramebuffers[i]->encoder setStencilReferenceValue:(uint32_t)pStencil->compareValue];
    }
  }
}

- (void)bindBlendState:(vcGLStateBlendMode)newMode
{
  blendMode = newMode;
  if (pCurrShader != nullptr && pCurrFramebuffer != nullptr)
    [pCurrFramebuffer->encoder setRenderPipelineState:pCurrShader->pipelines[newMode]];
}

- (void)flush:(uint32_t)i
{
  if ((pFramebuffers[i]->actions & vcRFA_Draw) == vcRFA_Draw)
  {
    [pFramebuffers[i]->encoder endEncoding];
    [pFramebuffers[i]->commandBuffer commit];
    [pFramebuffers[i]->commandBuffer waitUntilCompleted];
  }
  
  if ((pFramebuffers[i]->actions & vcRFA_Renew) == vcRFA_Renew)
  {
    pFramebuffers[i]->commandBuffer = [_queue commandBuffer];
    pFramebuffers[i]->encoder = [pFramebuffers[i]->commandBuffer renderCommandEncoderWithDescriptor:pFramebuffers[i]->pRenderPass];
  }
  
  pFramebuffers[i]->actions = pFramebuffers[i]->actions & vcRFA_Blit;
}

- (void)flushBlit
{
  [_blitEncoder endEncoding];
  [_blitBuffer commit];
  [_blitBuffer waitUntilCompleted];
  _blitBuffer = [_queue commandBuffer];
  _blitEncoder = [_blitBuffer blitCommandEncoder];
}

- (void)drawUnindexed:(id<MTLBuffer>)vertBuffer vertexStart:(NSUInteger)vStart vertexCount:(NSUInteger)vCount primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindVB:&pCurrShader->bufferObjects[i] index:i+1];
  
  [pCurrFramebuffer->encoder setVertexBuffer:vertBuffer offset:vStart atIndex:0];
  [pCurrFramebuffer->encoder drawPrimitives:type vertexStart:vStart vertexCount:vCount];

  pCurrFramebuffer->actions |= vcRFA_Draw | vcRFA_Renew;

  switch(pCurrShader->flush)
  {
  case vcRFO_None:
    break;
  case vcRFO_Blit:
    [self flushBlit];
    break;
  case vcRFO_Flush:
    [self flush:pCurrFramebuffer->ID];
    break;
  }
}

- (void)drawIndexed:(id<MTLBuffer>)vertBuffer indexedBuffer:(id<MTLBuffer>)indexBuffer indexCount:(unsigned long)indexCount offset:(unsigned long)offset indexSize:(MTLIndexType)indexType primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindVB:&pCurrShader->bufferObjects[i] index:i+1];
  
  [pCurrFramebuffer->encoder setVertexBuffer:vertBuffer offset:0 atIndex:30];
  [pCurrFramebuffer->encoder drawIndexedPrimitives:type indexCount:indexCount indexType:indexType indexBuffer:indexBuffer indexBufferOffset:offset];

  pCurrFramebuffer->actions |= vcRFA_Draw | vcRFA_Renew;

  switch(pCurrShader->flush)
  {
  case vcRFO_None:
    break;
  case vcRFO_Blit:
    [self flushBlit];
    break;
  case vcRFO_Flush:
    [self flush:pCurrFramebuffer->ID];
    break;
  }
}

- (void)setCullMode:(MTLCullMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [pFramebuffers[i]->encoder setCullMode:mode];
  }
}

- (void)setFillMode:(MTLTriangleFillMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [pFramebuffers[i]->encoder setTriangleFillMode:mode];
  }
}

- (void)setWindingMode:(MTLWinding)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [pFramebuffers[i]->encoder setFrontFacingWinding:mode];
  }
}

- (void)setScissor:(MTLScissorRect)rect
{
  if (pCurrFramebuffer->ID == 0 && (pCurrFramebuffer->actions & vcRFA_Resize) != vcRFA_Resize)
    [pCurrFramebuffer->encoder setScissorRect:rect];
}

- (void)addFramebuffer:(nullable vcFramebuffer*)pFramebuffer
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] == nullptr)
    {
      MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];

      pass.colorAttachments[0].loadAction = MTLLoadActionClear;
      pass.colorAttachments[0].storeAction = MTLStoreActionStore;
      pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
      if (pFramebuffer->pColor != nullptr)
        pass.colorAttachments[0].texture = pFramebuffer->pColor->texture;

      if (pFramebuffer->pDepth != nullptr)
      {
        pass.depthAttachment.texture = pFramebuffer->pDepth->texture;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionStore;
        pass.depthAttachment.clearDepth = 1.0;
        if (pFramebuffer->pDepth->format == vcTextureFormat_D24S8)
        {
          pass.stencilAttachment.texture = pFramebuffer->pDepth->texture;
          pass.stencilAttachment.loadAction = MTLLoadActionClear;
          pass.stencilAttachment.storeAction = MTLStoreActionStore;
          pass.stencilAttachment.clearStencil = 0;
        }
        else
        {
          pass.stencilAttachment.texture = nil;
        }
      }
      else
      {
        pass.depthAttachment.texture = nil;
        pass.stencilAttachment.texture = nil;
      }

      pFramebuffers[i] = pFramebuffer;
      pFramebuffer->ID = i;
      pFramebuffer->actions = 0;

      pFramebuffer->pRenderPass = pass;
      pFramebuffer->commandBuffer = [_queue commandBuffer];
      pFramebuffer->encoder = [pFramebuffer->commandBuffer renderCommandEncoderWithDescriptor:pass];
    
      return;
    }
  }
  NSLog(@"Increase BUFFER_COUNT");
}

- (void)setFramebuffer:(vcFramebuffer*)pFramebuffer
{
  pCurrFramebuffer = pFramebuffer;

  [self bindViewport:viewport];
}

- (void)destroyFramebuffer:(vcFramebuffer*)pFramebuffer
{
  if (pCurrFramebuffer != nullptr && pCurrFramebuffer->ID == pFramebuffer->ID)
    pCurrFramebuffer = nullptr;

  [pFramebuffer->encoder endEncoding];
  pFramebuffers[pFramebuffer->ID] = nullptr;
}

- (void)bindViewport:(MTLViewport)vp
{
#ifdef METAL_DEBUG
  if (vp.znear < 0)
    NSLog(@"invalid viewport");
#endif
  viewport = vp;
  [pCurrFramebuffer->encoder setViewport:vp];
}

- (void)bindVB:(nonnull vcShaderConstantBuffer*)pBuffer index:(uint8_t)index
{
  if (pBuffer->buffers[0].index != -1)
    [pCurrFramebuffer->encoder setVertexBytes:pBuffer->buffers[0].pCB length:pBuffer->expectedSize atIndex:pBuffer->buffers[0].index];

  if (pBuffer->buffers[1].index != -1)
    [pCurrFramebuffer->encoder setFragmentBytes:pBuffer->buffers[1].pCB length:pBuffer->expectedSize atIndex:pBuffer->buffers[1].index];
}
@end
