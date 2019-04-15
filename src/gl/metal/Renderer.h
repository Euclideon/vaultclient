#ifndef Renderer_h__
#define Renderer_h__

#import <MetalKit/MetalKit.h>
#import "gl/vcGLState.h"
#import "gl/vcTexture.h"
#import "gl/vcFramebuffer.h"
#import "vcMetal.h"

@interface Renderer : NSObject<MTKViewDelegate>

@property(nonatomic,strong,nonnull) id<MTLCommandQueue> queue;

// Blit stuff
@property(nonatomic,strong,nonnull) id<MTLCommandBuffer> blitBuffer;
@property(nonatomic,strong,nonnull) id<MTLBlitCommandEncoder> blitEncoder;

@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLRenderPipelineState>> *pipelines;
@property(nonatomic,strong,nonnull) NSMutableArray<MTLRenderPipelineDescriptor*> *pipeDescs;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLBuffer>> *constantBuffers;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLBuffer>> *indexBuffers;

@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLBuffer>> *vertBuffers;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLTexture>> *textures;

// Permutables
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLDepthStencilState>> *depthStates;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLSamplerState>> *samplers;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLRenderPipelineState>> *blendPipelines;

// Per buffer objects
@property(nonatomic,strong,nonnull) NSMutableArray<MTLRenderPassDescriptor*> *renderPasses;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLCommandBuffer>> *commandBuffers;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLRenderCommandEncoder>> *encoders;

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)mtkView;
- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size;
- (void)defaultPipelines;
- (void)bindPipeline:(nonnull vcShader*)shader;
- (void)bindTexture:(nonnull struct vcTexture*)texture index:(NSInteger)samplerIndex;
- (void)bindSampler:(nonnull struct vcShaderSampler*)texture index:(NSInteger)samplerIndex;
- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable vcGLStencilSettings *)stencilSettings;
- (void)setBlendMode:(vcGLStateBlendMode)blendMode;
- (void)bindConstantBuffer:(nonnull vcShaderConstantBuffer*)pBuffer index:(NSUInteger)index;
- (void)drawUnindexed:(nonnull id<MTLBuffer>)vertBuffer
          vertexStart:(NSUInteger)vStart
          vertexCount:(NSUInteger)vCount
        primitiveType:(MTLPrimitiveType)type;
- (void)drawIndexedTriangles:(nonnull id<MTLBuffer>)positionBuffer
               indexedBuffer:(nonnull id<MTLBuffer>)indexBuffer
                  indexCount:(unsigned long)indexCount
                   indexSize:(MTLIndexType)indexType
               primitiveType:(MTLPrimitiveType)type;
- (void)cullMode:(MTLCullMode)mode;
- (void)fillMode:(MTLTriangleFillMode)mode;
- (void)windingMode:(MTLWinding)mode;
- (void)addFramebuffer:(nullable vcFramebuffer*)framebuffer;
- (void)setFramebuffer:(nullable vcFramebuffer*)framebuffer;
- (void)destroyFramebuffer:(nonnull vcFramebuffer*)framebuffer;
- (void)bindViewport:(MTLViewport)vp;

- (nonnull id<MTLCommandBuffer>)mainBuffer;
- (nonnull id<MTLRenderCommandEncoder>)mainEncoder;
- (nonnull MTLRenderPassDescriptor*)mainRenderPass;

@end

#endif
