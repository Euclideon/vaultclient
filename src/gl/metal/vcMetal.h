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

extern vcGLState g_internalState;
extern id<MTLDevice> g_device;
extern id<MTLCommandQueue> g_queue;
extern vcFramebuffer *g_pDefaultFramebuffer;
extern vcFramebuffer *g_pCurrFramebuffer;
extern vcShader *g_pCurrShader;
extern id<MTLBlitCommandEncoder> g_blitEncoder;

void vcGLState_FlushBlit();

enum vcRendererFramebufferActions
{
  vcRFA_Resize = 1
};

struct vcTexture
{
  id<MTLTexture> texture;
  id<MTLSamplerState> sampler;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  vcTextureFormat format;
  vcTextureCreationFlags flags;
  id<MTLBuffer> blitBuffer;
};

struct vcFramebuffer
{
  vcTexture *pColor;
  vcTexture *pDepth;
  uint32_t clear;
  int actions;
  MTLRenderPassDescriptor *pRenderPass;
  id<MTLCommandBuffer> commandBuffer;
  id<MTLRenderCommandEncoder> encoder;
};

struct vcShaderConstantBuffer
{
  char name[32];
  size_t expectedSize;
  struct
  {
    int index;
    const void *pCB;
  } buffers[2]; // 0 for Vertex shader, 1 for Fragment shader
};

struct vcShaderSampler
{
  char name[32];
};

struct vcShader
{
  bool inititalised;
  id<MTLLibrary> vertexLibrary;
  id<MTLLibrary> fragmentLibrary;
  id<MTLRenderPipelineState> pipelines[vcGLSBM_Count * 2];

  vcShaderConstantBuffer bufferObjects[16];
  int numBufferObjects;

  struct
  {
    float cameraNearPlane;
    float cameraFarPlane;
    float clipZNear;
    float clipZFar;
  } cameraPlane;
  vcShaderConstantBuffer *pCameraPlaneParams;
};

struct vcMesh
{
  id<MTLBuffer> vBuffer;
  uint32_t vertexCount;
  uint32_t vertexBytes;

  id<MTLBuffer> iBuffer;
  MTLIndexType indexType;
  uint32_t indexCount;
  uint32_t indexBytes;
};

#endif // vcMetal_h__
