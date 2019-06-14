#ifndef vcRenderer_h__
#define vcRenderer_h__

#import <MetalKit/MetalKit.h>
#import "gl/vcGLState.h"
#import "gl/vcTexture.h"
#import "gl/vcFramebuffer.h"
#import "vcMetal.h"

@interface vcRenderer : NSObject<MTKViewDelegate>

@property(nonatomic,strong,nonnull) id<MTLCommandQueue> queue;

// Blitting
@property(nonatomic,strong,nonnull) id<MTLCommandBuffer> blitBuffer;
@property(nonatomic,strong,nonnull) id<MTLBlitCommandEncoder> blitEncoder;

@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLRenderPipelineState>> *pipelines;

@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLBuffer>> *vertBuffers;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLBuffer>> *indexBuffers;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLTexture>> *textures;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLBuffer>> *constantBuffers;

// Permutables
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLDepthStencilState>> *depthStates;
@property(nonatomic,strong,nonnull) NSMutableDictionary<NSString*, id<MTLSamplerState>> *samplers;

// Per buffer objects
@property(nonatomic,strong,nonnull) NSMutableArray<MTLRenderPassDescriptor*> *renderPasses;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLCommandBuffer>> *commandBuffers;
@property(nonatomic,strong,nonnull) NSMutableArray<id<MTLRenderCommandEncoder>> *encoders;

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)mtkView;
- (void)mtkView:(nonnull MTKView*)view drawableSizeWillChange:(CGSize)size;
- (void)bindPipeline:(nonnull vcShader*)pShader;
- (void)bindTexture:(nonnull struct vcTexture*)pTexture index:(NSInteger)samplerIndex;
- (void)bindSampler:(nonnull struct vcShaderSampler*)pTexture index:(NSInteger)samplerIndex;
- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable vcGLStencilSettings *)pStencil;
- (void)bindBlendState:(vcGLStateBlendMode)blendMode;
- (void)bindConstantBuffer:(nonnull vcShaderConstantBuffer*)pBuffer index:(NSUInteger)index;
- (void)drawUnindexed:(nonnull id<MTLBuffer>)vertBuffer vertexStart:(NSUInteger)vStart vertexCount:(NSUInteger)vCount primitiveType:(MTLPrimitiveType)type;
- (void)drawIndexedTriangles:(nonnull id<MTLBuffer>)positionBuffer indexedBuffer:(nonnull id<MTLBuffer>)indexBuffer indexCount:(unsigned long)indexCount offset:(unsigned long)offset indexSize:(MTLIndexType)indexType primitiveType:(MTLPrimitiveType)type;
- (void)setCullMode:(MTLCullMode)mode;
- (void)setFillMode:(MTLTriangleFillMode)mode;
- (void)setWindingMode:(MTLWinding)mode;
- (void)setScissor:(MTLScissorRect)rect;
- (void)addFramebuffer:(nullable vcFramebuffer*)pFramebuffer;
- (void)setFramebuffer:(nullable vcFramebuffer*)pFramebuffer;
- (void)destroyFramebuffer:(nonnull vcFramebuffer*)pFramebuffer;
- (void)bindViewport:(MTLViewport)vp;
- (void)buildBlendPipelines:(nonnull MTLRenderPipelineDescriptor*)pDesc;
@end

#endif
