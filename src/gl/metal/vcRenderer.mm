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
  
  vcGLStateBlendMode blendMode;
}

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view
{
  self = [super init];
  if (self)
  {
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
  [_blitEncoder endEncoding];
  [_blitBuffer commit];
  [_blitBuffer waitUntilScheduled];
  for (size_t i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i] != nullptr && pFramebuffers[i]->render)
    {
      [_encoders[i] endEncoding];
      if (i != 0)
      {
        [_commandBuffers[i] commit];
        [_commandBuffers[i] waitUntilScheduled];
      }
    }
  }
  
  if (view.currentDrawable)
    [_commandBuffers[0] presentDrawable:view.currentDrawable];

  [_commandBuffers[0] commit];
  
  // currentRenderPassDescriptor contains a drawable with a color texture attachment
  if (view.currentRenderPassDescriptor)
  {
    _renderPasses[0] = view.currentRenderPassDescriptor;
  }
  else
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
      if (pFramebuffers[i]->render)
      {
        _commandBuffers[i] = [_queue commandBuffer];
        [_encoders replaceObjectAtIndex:i withObject:[_commandBuffers[i] renderCommandEncoderWithDescriptor:_renderPasses[i]]];
      }
      pFramebuffers[i]->render = false;
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
  [self bindBlendState:blendMode];
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

- (void)bindConstantBuffer:(nonnull vcShaderConstantBuffer *)pBuffer index:(NSUInteger)index
{
  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:[_constantBuffers objectAtIndex:pBuffer->ID] offset:0 atIndex:index];
  [_encoders[pCurrFramebuffer->ID] setFragmentBuffer:[_constantBuffers objectAtIndex:pBuffer->ID] offset:0 atIndex:index];
}

- (void)bindBlendState:(vcGLStateBlendMode)newMode
{
  blendMode = newMode;
  if (pCurrShader != nullptr && pCurrFramebuffer != nullptr)
    [_encoders[pCurrFramebuffer->ID] setRenderPipelineState:_pipelines[(pCurrShader->ID * vcGLSBM_Count) + newMode]];
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
  rect.width = udMin(rect.width, _renderPasses[0].colorAttachments[0].texture.width - rect.x);
  rect.height = udMin(rect.height, _renderPasses[0].colorAttachments[0].texture.height - rect.y);

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
      pFramebuffer->render = true;
      
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
  pFramebuffer->render = true;
}

- (void)destroyFramebuffer:(vcFramebuffer*)pFramebuffer
{
  [_encoders[pFramebuffer->ID] endEncoding];
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

- (void)buildBlendPipelines:(nonnull MTLRenderPipelineDescriptor*)pDesc
{
  NSError *err = nil;
  [_pipelines addObject:[_device newRenderPipelineStateWithDescriptor:pDesc error:&err]];
#ifdef METAL_DEBUG
  if (err != nil)
    NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif
  pDesc.colorAttachments[0].blendingEnabled = YES;
  pDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
  
  for (int i = vcGLSBM_Interpolative; i < vcGLSBM_Count; ++i)
  {
    switch ((vcGLStateBlendMode)i)
    {
      case vcGLSBM_None:
        break;
      case vcGLSBM_Interpolative:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
        break;
      case vcGLSBM_Additive:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        break;
      case vcGLSBM_Multiplicative:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorDestinationColor;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorZero;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        break;
      case vcGLSBM_Count:
        break;
    }
    
    [_pipelines addObject:[_device newRenderPipelineStateWithDescriptor:pDesc error:&err]];
#ifdef METAL_DEBUG
    if (err != nil)
      NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif
  }
}
@end
