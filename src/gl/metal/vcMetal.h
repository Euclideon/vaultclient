#ifndef vcMetal_h__
#define vcMetal_h__

#import "udPlatform.h"

#import "gl/vcGLState.h"
#import "gl/vcTexture.h"
#import "gl/vcMesh.h"
#import "gl/vcFramebuffer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import "vcRenderer.h"

#define BUFFER_COUNT 6
#define DRAWABLES 1

#define ForceUnwrap(type, nullableExpression) ^type _Nonnull () { \
type _Nullable maybeValue___ = nullableExpression; \
if (maybeValue___) { \
return (type _Nonnull) maybeValue___; \
} else { \
NSLog(@"Attempted to force unwrap a null: " #nullableExpression); \
abort(); \
} \
}()

extern id<MTLDevice> _device;
extern id<MTLLibrary> _library;

enum vcRendererFramebufferActions
{
  vcRFA_Draw = 1,
  vcRFA_Renew = 2,
  vcRFA_Blit = 4,
  vcRFA_Resize = 8
};

enum vcRendererFlushOption
{
  vcRFO_None,
  vcRFO_Flush,
  vcRFO_Blit
};

struct vcTexture
{
  uint32_t ID;
  char samplerID[32];
  uint32_t width;
  uint32_t height;
  vcTextureFormat format;
  vcTextureCreationFlags flags;
  uint32_t blitTarget;
};

struct vcFramebuffer
{
  uint32_t ID;
  vcTexture *pColor;
  vcTexture *pDepth;
  uint32_t clear;
  int actions;
};

struct vcShaderConstantBuffer
{
  const void *pCB;
  char name[32];
  size_t expectedSize;
};

struct vcShaderSampler
{
  char name[32];
};

struct vcShader
{
  uint32_t ID;

  //uint16_t gID;
  //uint16_t geom;

  vcShaderConstantBuffer bufferObjects[16];
  int numBufferObjects;
  vcShaderSampler samplerIndexes[16];
  int numSamplerIndexes;

  vcRendererFlushOption flush;
};

struct vcMesh
{
  char vBufferIndex[32];
  uint32_t vertexCount;
  uint32_t vertexBytes;

  char iBufferIndex[32];
  MTLIndexType indexType;
  uint32_t indexCount;
  uint32_t indexBytes;
};

#endif // vcMetal_h__
