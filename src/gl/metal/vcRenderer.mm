#import <MetalKit/MetalKit.h>

#import "vcRenderer.h"
#import "vcMetal.h"
#include "imgui.h"
#include "vcGLState.h"

@implementation vcRenderer
{
  vcFramebuffer *pFramebuffers[BUFFER_COUNT];
  vcFramebuffer *pCurrFramebuffer;
  vcShader *pCurrShader;

  NSString *blend;
  vcGLStateBlendMode blendMode;
}

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view
{
  self = [super init];
  if (self)
  {
    blend = nullptr;

    _queue = [_device newCommandQueue];

    _blitBuffer = [_queue commandBuffer];
    _blitEncoder = [_blitBuffer blitCommandEncoder];

    view.paused = true;
  }
  return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
  // Finalise last frame

  // End encoding
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
    {
      [_encoders[i] endEncoding];
    }
  }
  [_blitEncoder endEncoding];

  [_commandBuffers[0] presentDrawable:view.currentDrawable];

  // Commit buffers
  [_blitBuffer commit];
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
    {
      [_commandBuffers[i] commit];
    }
  }

  // currentRenderPassDescriptor contains a drawable with a color texture attachment
  _renderPasses[0] = view.currentRenderPassDescriptor;

  if (_renderPasses[0] == nil)
  {
    _renderPasses[0] = [[MTLRenderPassDescriptor alloc] init];
    _renderPasses[0].colorAttachments[0].texture = _textures[[NSString stringWithUTF8String:pFramebuffers[0]->pColor->ID]];
  }
  _renderPasses[0].depthAttachment.texture = _textures[[NSString stringWithUTF8String:pFramebuffers[0]->pDepth->ID]];
  _renderPasses[0].stencilAttachment.texture = _textures[[NSString stringWithUTF8String:pFramebuffers[0]->pDepth->ID]];
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
    {
      _commandBuffers[i] = [_queue commandBuffer];
      [_encoders replaceObjectAtIndex:i withObject:[_commandBuffers[i] renderCommandEncoderWithDescriptor:_renderPasses[i]]];
    }
  }

  _blitBuffer = [_queue commandBuffer];
  _blitEncoder = [_blitBuffer blitCommandEncoder];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
  udUnused(view);
  if (pFramebuffers[0]->pDepth)
  {
    vcTexture *pIntermediate;
    vcTexture_Create(&pIntermediate, size.width, size.height, nullptr, pFramebuffers[0]->pDepth->format, vcTFM_Nearest, false, vcTWM_Repeat, vcTCF_RenderTarget, 0);

    vcTexture_Destroy(&pFramebuffers[0]->pDepth);

    pFramebuffers[0]->pDepth = pIntermediate;
    _viewCon.renderer.renderPasses[0].depthAttachment.texture = _viewCon.renderer.textures[[NSString stringWithUTF8String:pIntermediate->ID]];
    if (pIntermediate->format == vcTextureFormat_D24S8)
      _viewCon.renderer.renderPasses[0].stencilAttachment.texture = _viewCon.renderer.renderPasses[0].depthAttachment.texture;
  }
}

-(void)bindPipeline:(nonnull vcShader *)pShader
{
  pCurrShader = pShader;
  [self bindBlendState];
}

- (void)bindTexture:(nonnull struct vcTexture*)pTexture index:(NSInteger)tIndex
{
  NSString *currID = [NSString stringWithUTF8String:pTexture->ID];
  [_encoders[pCurrFramebuffer->ID] setFragmentTexture:_textures[currID] atIndex:tIndex];

  currID = [NSString stringWithUTF8String:pTexture->samplerID];
  [_encoders[pCurrFramebuffer->ID] setFragmentSamplerState:_samplers[currID] atIndex:tIndex];
}

- (void)bindSampler:(nonnull struct vcShaderSampler *)pSampler index:(NSInteger)samplerIndex
{
  NSString *currID = [NSString stringWithUTF8String:pSampler->name];
  [_encoders[pCurrFramebuffer->ID] setFragmentSamplerState:_samplers[currID] atIndex:samplerIndex];
}

- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable vcGLStencilSettings *)pStencil
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
    {
      [_encoders[i] setDepthStencilState:dsState];

      if (pStencil != nullptr)
        [_encoders[i] setStencilReferenceValue:(uint32_t)pStencil->compareValue];
    }
  }
}

- (void)setBlendMode:(vcGLStateBlendMode)newBlendMode
{
  if (!pCurrShader)
    return;

  blendMode = newBlendMode;

  NSString *pLookup;
  switch (blendMode)
  {
    case vcGLSBM_None:
      pLookup = nullptr;
      break;
    case vcGLSBM_Additive:
      pLookup = [NSString stringWithFormat:@"%dA",pCurrShader->ID];
      break;
    case vcGLSBM_Interpolative:
      pLookup = [NSString stringWithFormat:@"%dI",pCurrShader->ID];
      break;
    case vcGLSBM_Multiplicative:
      pLookup = [NSString stringWithFormat:@"%dM",pCurrShader->ID];
      break;
  }

  blend = pLookup;

  if (pLookup == nullptr || _blendPipelines[pLookup])
  {
    [self bindBlendState];
    return;
  }

  MTLRenderPipelineDescriptor *pDesc = [_pipeDescs[pCurrShader->ID] copy];

  if (blendMode == vcGLSBM_None)
  {
    pDesc.colorAttachments[0].blendingEnabled = false;
  }
  else
  {
    pDesc.alphaToCoverageEnabled = false;
    pDesc.colorAttachments[0].blendingEnabled = true;
    pDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
    pDesc.colorAttachments[0].writeMask = MTLColorWriteMaskAll;

    if (blendMode == vcGLSBM_Interpolative)
    {
      pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
      pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

      pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
    }
    else if (blendMode == vcGLSBM_Additive)
    {
      pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
      pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
    }
    else if (blendMode == vcGLSBM_Multiplicative)
    {
      pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorDestinationColor;
      pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorZero;
    }

    [_blendPipelines setObject:[_device newRenderPipelineStateWithDescriptor:pDesc error:nil] forKey:blend];
  }
}

- (void)bindConstantBuffer:(nonnull vcShaderConstantBuffer *)pBuffer index:(NSUInteger)index
{
  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:[_constantBuffers objectAtIndex:pBuffer->ID] offset:0 atIndex:index];
  [_encoders[pCurrFramebuffer->ID] setFragmentBuffer:[_constantBuffers objectAtIndex:pBuffer->ID] offset:0 atIndex:index];
}

- (void)bindBlendState
{
  if (pCurrShader != nullptr && pCurrFramebuffer != nullptr)
  {
    id<MTLRenderPipelineState> pipe;

    if (blend == nullptr)
    {
      pipe = [_pipelines objectAtIndex:pCurrShader->ID];
    }
    else
    {
      if (![_blendPipelines objectForKey:blend] || ![blend hasPrefix:[NSString stringWithFormat:@"%d",pCurrShader->ID]])
        [self setBlendMode:blendMode];

      pipe = [_blendPipelines objectForKey:blend];
    }

    [_encoders[pCurrFramebuffer->ID] setRenderPipelineState:pipe];
  }
}

- (void)drawUnindexed:(id<MTLBuffer>)vertBuffer vertexStart:(NSUInteger)vStart vertexCount:(NSUInteger)vCount primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindConstantBuffer:&pCurrShader->bufferObjects[i] index:i+1];

  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:vertBuffer offset:vStart atIndex:0];
  [_encoders[pCurrFramebuffer->ID] drawPrimitives:type vertexStart:vStart vertexCount:vCount];
}
- (void)drawIndexedTriangles:(id<MTLBuffer>)vertBuffer indexedBuffer:(id<MTLBuffer>)indexBuffer indexCount:(unsigned long)indexCount offset:(unsigned long)offset indexSize:(MTLIndexType)indexType primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindConstantBuffer:&pCurrShader->bufferObjects[i] index:i+1];

  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:vertBuffer offset:0 atIndex:0];
  [_encoders[pCurrFramebuffer->ID] drawIndexedPrimitives:type indexCount:indexCount indexType:indexType indexBuffer:indexBuffer indexBufferOffset:offset];
}

- (void)setCullMode:(MTLCullMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [_encoders[i] setCullMode:mode];
  }
}
- (void)setFillMode:(MTLTriangleFillMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [_encoders[i] setTriangleFillMode:mode];
  }
}
- (void)setWindingMode:(MTLWinding)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr)
      [_encoders[i] setFrontFacingWinding:mode];
  }
}

- (void)setScissor:(MTLScissorRect)rect
{
  rect.width = udMin(rect.width, _renderPasses[0].colorAttachments[0].texture.width);
  rect.height = udMin(rect.height, _renderPasses[0].colorAttachments[0].texture.height);
  [_encoders[pCurrFramebuffer->ID] setScissorRect:rect];

}

- (void)addFramebuffer:(nullable vcFramebuffer*)pFramebuffer
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] == nullptr)
    {
      MTLRenderPassDescriptor *pass = [[MTLRenderPassDescriptor alloc] init];

      pass.colorAttachments[0].loadAction = MTLLoadActionClear;
      pass.colorAttachments[0].storeAction = MTLStoreActionStore;
      pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
      if (pFramebuffer->pColor != nullptr)
        pass.colorAttachments[0].texture = _textures[[NSString stringWithUTF8String:pFramebuffer->pColor->ID]];

      if (pFramebuffer->pDepth != nullptr)
      {
        pass.depthAttachment.texture = _textures[[NSString stringWithUTF8String:pFramebuffer->pDepth->ID]];
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionStore;
        pass.depthAttachment.clearDepth = 1.0;
        if (pFramebuffer->pDepth->format == vcTextureFormat_D24S8)
        {
          pass.stencilAttachment.texture = _textures[[NSString stringWithUTF8String:pFramebuffer->pDepth->ID]];
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

      [_renderPasses setObject:pass atIndexedSubscript:i];
      [_commandBuffers setObject:[_queue commandBuffer] atIndexedSubscript:i];
      [_encoders setObject:[_commandBuffers[i] renderCommandEncoderWithDescriptor:pass] atIndexedSubscript:i];

      return;
    }
  }
}
- (void)setFramebuffer:(vcFramebuffer*)pFramebuffer
{
  pCurrFramebuffer = pFramebuffer;
}
- (void)destroyFramebuffer:(vcFramebuffer*)pFramebuffer
{
  [_encoders[pFramebuffer->ID] endEncoding];
  [_commandBuffers[pFramebuffer->ID] commit];
  pFramebuffers[pFramebuffer->ID] = nullptr;
}
- (void)bindViewport:(MTLViewport)vp
{
#ifdef METAL_DEBUG
  if (vp.znear < 0)
    NSLog(@"invalid viewport");
#endif
  for (int i = 0; i < BUFFER_COUNT; ++i)
    if (pFramebuffers[i] != nullptr)
      [_encoders[i] setViewport:vp];
}

@end
