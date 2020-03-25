#ifndef vcRenderer_h__
#define vcRenderer_h__

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import "vcMetal.h"

#import "gl/vcGLState.h"
#import "gl/vcTexture.h"
#import "gl/vcFramebuffer.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#define PlatformView UIView
#elif UDPLATFORM_OSX
#define PlatformView NSView
#else
# error "Unsupported platform!"
#endif

@interface vcRenderer : NSObject

// Blitting
@property(nonatomic,strong,nonnull) id<MTLCommandBuffer> blitBuffer;
@property(nonatomic,strong,nonnull) id<MTLBlitCommandEncoder> blitEncoder;

- (void)initWithView:(nonnull PlatformView*)view;
- (nullable CAMetalLayer*)makeBackingLayer;
- (void)setFrameSize:(NSSize)newSize;
- (void)draw;
- (void)flush:(uint32_t)i;
- (void)flushBlit;
- (void)bindPipeline:(nonnull struct vcShader*)pShader;
- (void)bindTexture:(nonnull struct vcTexture*)pTexture index:(NSInteger)samplerIndex;
- (void)bindDepthStencil:(nonnull id<MTLDepthStencilState>)dsState settings:(nullable struct vcGLStencilSettings *)pStencil;
- (void)bindBlendState:(vcGLStateBlendMode)blendMode;
- (void)drawUnindexed:(nonnull id<MTLBuffer>)vertBuffer vertexStart:(NSUInteger)vStart vertexCount:(NSUInteger)vCount primitiveType:(MTLPrimitiveType)type;
- (void)drawIndexed:(nonnull id<MTLBuffer>)positionBuffer indexedBuffer:(nonnull id<MTLBuffer>)indexBuffer indexCount:(unsigned long)indexCount offset:(unsigned long)offset indexSize:(MTLIndexType)indexType primitiveType:(MTLPrimitiveType)type;
- (void)setCullMode:(MTLCullMode)mode;
- (void)setFillMode:(MTLTriangleFillMode)mode;
- (void)setWindingMode:(MTLWinding)mode;
- (void)setScissor:(MTLScissorRect)rect;
- (void)addFramebuffer:(nullable struct vcFramebuffer*)pFramebuffer;
- (void)setFramebuffer:(nullable struct vcFramebuffer*)pFramebuffer;
- (void)destroyFramebuffer:(nonnull struct vcFramebuffer*)pFramebuffer;
- (void)bindViewport:(MTLViewport)vp;
- (void)bindVB:(nonnull vcShaderConstantBuffer*)pBuffer index:(uint8_t)index;
@end

extern vcRenderer* _Nullable _renderer;

#endif
