#import <MetalKit/MetalKit.h>

#import "Renderer.h"
#import "vcMetal.h"
#include "imgui.h"
#include "vcGLState.h"

@implementation Renderer
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

  do {
    _renderPasses[0] = view.currentRenderPassDescriptor;
  } while (_renderPasses[0] == nil);
  
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
  vcGLState_SetViewport(0, 0, size.width, size.height);
}

- (void)defaultPipelines
{
  id<MTLFunction> vert = [_library newFunctionWithName:@"vertex_main"];
  id<MTLFunction> frag = [_library newFunctionWithName:@"fragment_main"];

  MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
  vertexDescriptor.attributes[0].offset = sizeof(float) * 2;
  vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
  vertexDescriptor.attributes[0].bufferIndex = 0;
  vertexDescriptor.attributes[1].offset = sizeof(float) * 2;
  vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
  vertexDescriptor.attributes[1].bufferIndex = 0;
  vertexDescriptor.attributes[2].offset = sizeof(char) * 4;
  vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4; // color
  vertexDescriptor.attributes[2].bufferIndex = 0;
  vertexDescriptor.layouts[0].stepRate = 1;
  vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
  vertexDescriptor.layouts[0].stride = (sizeof(float) * 4) + sizeof(char) * 4;

  MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDescriptor.vertexFunction = vert;
  pipelineDescriptor.fragmentFunction = frag;
  pipelineDescriptor.vertexDescriptor = vertexDescriptor;
  pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
  pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
  pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
  pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float;
#else
  pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
  pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
#endif
    
  [_pipelines addObject:[_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:nil]];
  [_pipeDescs addObject:pipelineDescriptor];
}

-(void)bindPipeline:(nonnull vcShader *)shader
{
  pCurrShader = shader;
  [self bindBlendState];
}

- (void)bindTexture:(nonnull struct vcTexture*)texture index:(NSInteger)tIndex
{
  NSString *currID = [NSString stringWithUTF8String:texture->ID];
  [_encoders[pCurrFramebuffer->ID] setFragmentTexture:_textures[currID] atIndex:tIndex];
  
  currID = [NSString stringWithUTF8String:texture->samplerID];
  [_encoders[pCurrFramebuffer->ID] setFragmentSamplerState:_samplers[currID] atIndex:tIndex];
}

- (void)bindSampler:(nonnull struct vcShaderSampler *)sampler index:(NSInteger)samplerIndex
{
  NSString *currID = [NSString stringWithUTF8String:sampler->name];
  [_encoders[pCurrFramebuffer->ID] setFragmentSamplerState:_samplers[currID] atIndex:samplerIndex];
}

- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable vcGLStencilSettings *)stencilSettings
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (pFramebuffers[i])
    {
      [_encoders[i] setDepthStencilState:dsState];
    
      if (stencilSettings)
        [_encoders[i] setStencilReferenceValue:(uint32_t)stencilSettings->compareValue];
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
    default:
      NSLog(@"Error generating blend mode lookup string");
      break;
  }

  blend = pLookup;

  if (!pLookup || _blendPipelines[pLookup])
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
  if (pCurrShader && pCurrFramebuffer)
  {
    id<MTLRenderPipelineState> pipe;
    
    if (!blend)
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

- (void)drawUnindexed:(id<MTLBuffer>)vertBuffer
          vertexStart:(NSUInteger)vStart
          vertexCount:(NSUInteger)vCount
        primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindConstantBuffer:&pCurrShader->bufferObjects[i] index:i+1];
  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:vertBuffer
                                            offset:vStart
                                           atIndex:0];
  [_encoders[pCurrFramebuffer->ID] drawPrimitives:type
                                      vertexStart:vStart
                                      vertexCount:vCount];
}
- (void)drawIndexedTriangles:(id<MTLBuffer>)positionBuffer
               indexedBuffer:(id<MTLBuffer>)indexBuffer
                  indexCount:(unsigned long)indexCount
                   indexSize:(MTLIndexType)indexType
               primitiveType:(MTLPrimitiveType)type
{
  for (int i = 0; i < pCurrShader->numBufferObjects; ++i)
    [self bindConstantBuffer:&pCurrShader->bufferObjects[i] index:i+1];
  [_encoders[pCurrFramebuffer->ID] setVertexBuffer:positionBuffer
                             offset:0
                            atIndex:0];
  [_encoders[pCurrFramebuffer->ID] drawIndexedPrimitives:type
                               indexCount:indexCount
                                indexType:indexType
                              indexBuffer:indexBuffer
                        indexBufferOffset:0
                            instanceCount:1];
}

- (void)cullMode:(MTLCullMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
    if (pFramebuffers[i])
    {
      [_encoders[i] setCullMode:mode];
    }
}
- (void)fillMode:(MTLTriangleFillMode)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
    if (pFramebuffers[i])
      [_encoders[i] setTriangleFillMode:mode];
}
- (void)windingMode:(MTLWinding)mode
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
    if (pFramebuffers[i])
      [_encoders[i] setFrontFacingWinding:mode];
}

- (void)addFramebuffer:(nullable vcFramebuffer*)framebuffer
{
  for (int i = 0; i < BUFFER_COUNT; ++i)
  {
    if (!pFramebuffers[i])
    {
      MTLRenderPassDescriptor *pass = [[MTLRenderPassDescriptor alloc] init];
      
      pass.colorAttachments[0].loadAction = MTLLoadActionClear;
      pass.colorAttachments[0].storeAction = MTLStoreActionStore;
      pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
      if (framebuffer->pColor)
        pass.colorAttachments[0].texture = _textures[[NSString stringWithUTF8String:framebuffer->pColor->ID]];
      
      if (framebuffer->pDepth)
      {
        pass.depthAttachment.texture = _textures[[NSString stringWithUTF8String:framebuffer->pDepth->ID]];
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionStore;
        pass.depthAttachment.clearDepth = 1.0;
        if (framebuffer->pDepth->format == vcTextureFormat_D24S8)
        {
          pass.stencilAttachment.texture = _textures[[NSString stringWithUTF8String:framebuffer->pDepth->ID]];
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
      
      pFramebuffers[i] = framebuffer;
      framebuffer->ID = i;
      
      [_renderPasses setObject:pass atIndexedSubscript:i];
      [_commandBuffers setObject:[_queue commandBuffer] atIndexedSubscript:i];
      [_encoders setObject:[_commandBuffers[i] renderCommandEncoderWithDescriptor:pass] atIndexedSubscript:i];

      return;
    }
  }
}
- (void)setFramebuffer:(vcFramebuffer*)framebuffer
{
  pCurrFramebuffer = framebuffer;
}
- (void)destroyFramebuffer:(vcFramebuffer*)framebuffer
{
  [_encoders[framebuffer->ID] endEncoding];
  [_commandBuffers[framebuffer->ID] commit];
  pFramebuffers[framebuffer->ID] = nullptr;
}
- (void)bindViewport:(MTLViewport)vp
{
#ifdef METAL_DEBUG
  if (vp.znear < 0)
    NSLog(@"invalid viewport");
#endif
  for (int i = 0; i < BUFFER_COUNT; ++i)
    if (pFramebuffers[i])
      [_encoders[i] setViewport:vp];
}
- (id<MTLCommandBuffer>)mainBuffer
{
  return _commandBuffers[0];
}
- (id<MTLRenderCommandEncoder>)mainEncoder
{
  return _encoders[0];
}
- (MTLRenderPassDescriptor*)mainRenderPass
{
  return _renderPasses[0];
}

@end
