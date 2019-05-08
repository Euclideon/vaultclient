#ifndef vcMetal_h__
#define vcMetal_h__

#import "udPlatform/udPlatform.h"

#import "gl/vcGLState.h"
#import "gl/vcTexture.h"
#import "gl/vcMesh.h"
#import "gl/vcFramebuffer.h"

#import <Metal/Metal.h>
#import "vcRenderer.h"
#import "vcViewCon.h"

#define BUFFER_COUNT 2
#define METAL_DEBUG

extern vcViewCon *_viewCon;
extern id<MTLDevice> _device;
extern id<MTLLibrary> _library;

struct vcTexture
{
  char ID[32];
  char samplerID[32];
  uint32_t width;
  uint32_t height;
  vcTextureFormat format;
};

struct vcFramebuffer
{
  uint32_t ID;
  vcTexture *pColor;
  vcTexture *pDepth;
  bool drawMe;
};

struct vcShaderConstantBuffer
{
  uint32_t ID;
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

  vcShaderConstantBuffer bufferObjects[16];
  int numBufferObjects;
  vcShaderSampler samplerIndexes[16];
  int numSamplerIndexes;
};

struct vcMesh
{
  char vBufferIndex[32];
  char iBufferIndex[32];
  uint32_t vertexBytes;

  uint32_t vertexCount;
  MTLIndexType indexType;
  uint32_t indexCount;
  uint32_t indexBytes;
};

#endif // vcMetal_h__
