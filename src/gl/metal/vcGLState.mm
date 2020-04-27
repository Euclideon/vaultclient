#import "vcMetal.h"

#import "SDL2/SDL.h"
#import "SDL2/SDL_syswm.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#import <UIKit/UIKit.h>
#elif UDPLATFORM_OSX
#import <Cocoa/Cocoa.h>
#else
# error "Unsupported platform!"
#endif

#import <Metal/Metal.h>

#include "vcStrings.h"

int32_t g_maxAnisotropy = 0;

id<MTLDevice> g_device;
id<MTLCommandQueue> g_queue;
vcFramebuffer *g_pDefaultFramebuffer = nullptr;
vcFramebuffer *g_pCurrFramebuffer = nullptr;
vcShader *g_pCurrShader = nullptr;
CAMetalLayer *g_pMetalLayer = nullptr;
id<CAMetalDrawable> g_currDrawable;
id<MTLCommandBuffer> g_blitBuffer;
id<MTLBlitCommandEncoder> g_blitEncoder;

vcGLState g_internalState = {};

MTLStencilOperation mapStencilOperation[] =
{
  MTLStencilOperationKeep,
  MTLStencilOperationZero,
  MTLStencilOperationReplace,
  MTLStencilOperationIncrementClamp,
  MTLStencilOperationDecrementClamp
  // Or Inc/Dec Wrap?
};

MTLCompareFunction mapStencilFunction[] =
{
  MTLCompareFunctionAlways,
  MTLCompareFunctionNever,
  MTLCompareFunctionLess,
  MTLCompareFunctionLessEqual,
  MTLCompareFunctionGreater,
  MTLCompareFunctionGreaterEqual,
  MTLCompareFunctionEqual,
  MTLCompareFunctionNotEqual
};

MTLCompareFunction mapDepthMode[] =
{
  MTLCompareFunctionNever,
  MTLCompareFunctionLess,
  MTLCompareFunctionLessEqual,
  MTLCompareFunctionEqual,
  MTLCompareFunctionGreaterEqual,
  MTLCompareFunctionGreater,
  MTLCompareFunctionNotEqual,
  MTLCompareFunctionAlways
};

static id<MTLDepthStencilState> s_depthStates[vcGLSDM_Total * 2];

void vcGLState_BuildDepthStates()
{
  MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc]init];
  id<MTLDepthStencilState> dsState;

  for (int i = 0 ; i < vcGLSDM_Total; ++i)
  {
    int j = i * 2;
    depthStencilDesc.depthWriteEnabled = false;
    depthStencilDesc.depthCompareFunction = mapDepthMode[i];
    dsState = [g_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    s_depthStates[j] = dsState;

    depthStencilDesc.depthWriteEnabled = true;
    dsState = [g_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    s_depthStates[j+1] = dsState;
  }
}

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
  g_device = MTLCreateSystemDefaultDevice();

  if (g_device == nullptr)
  {
    NSLog(@"Metal is not supported on this device");
    return false;
  }

  g_queue = [g_device newCommandQueue];
  vcGLState_BuildDepthStates();

  SDL_SysWMinfo wminfo;
  SDL_VERSION(&wminfo.version);
  SDL_GetWindowWMInfo(pWindow, &wminfo);

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  UIView *sdlview = wminfo.info.uikit.window;
#elif UDPLATFORM_OSX
  NSView *sdlview = wminfo.info.cocoa.window.contentView;
#else
# error "Unsupported platform!"
#endif

  sdlview.autoresizesSubviews = true;
  
  g_pMetalLayer = [CAMetalLayer layer];
  g_pMetalLayer.device = g_device;

  g_pMetalLayer.autoresizingMask = kCALayerHeightSizable | kCALayerWidthSizable;
  g_pMetalLayer.needsDisplayOnBoundsChange = YES;
  g_pMetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  g_pMetalLayer.allowsNextDrawableTimeout = NO;
  g_pMetalLayer.drawableSize = CGSizeMake(sdlview.frame.size.width, sdlview.frame.size.height);
  sdlview.layer = g_pMetalLayer;
  g_blitBuffer = [g_queue commandBuffer];
  g_blitEncoder = [g_blitBuffer blitCommandEncoder];

  vcTexture *defaultTexture, *defaultDepth;
  vcTexture_Create(&defaultTexture, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, vcTCF_RenderTarget);
  vcTexture_Create(&defaultDepth, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_D32F, vcTFM_Nearest, vcTCF_RenderTarget);
  
  vcFramebuffer_Create(&g_pDefaultFramebuffer, defaultTexture, defaultDepth);
  vcFramebuffer_Bind(g_pDefaultFramebuffer);

  *ppDefaultFramebuffer = g_pDefaultFramebuffer;

  vcGLState_ResetState(true);

  return true;
}

void vcGLState_Deinit()
{
  @autoreleasepool {
    vcTexture_Destroy(&g_pDefaultFramebuffer->pColor);
    vcTexture_Destroy(&g_pDefaultFramebuffer->pDepth);
    vcFramebuffer_Destroy(&g_pDefaultFramebuffer);
    g_device = nil;
    g_queue = nil;
    g_pMetalLayer = nil;
    g_currDrawable = nil;

    [g_blitEncoder endEncoding];
    [g_blitBuffer commit];
    [g_blitBuffer waitUntilCompleted];
    g_blitBuffer = nil;
    g_blitEncoder = nil;

    for (size_t i = 0; i < udLengthOf(s_depthStates); ++i)
      s_depthStates[i] = nil;
    return;
  }
}

bool vcGLState_ApplyState(vcGLState *pState)
{
  bool success = true;

  success &= vcGLState_SetFaceMode(pState->fillMode, pState->cullMode, pState->isFrontCCW);
  success &= vcGLState_SetBlendMode(pState->blendMode);
  success &= vcGLState_SetDepthStencilMode(pState->depthReadMode, pState->doDepthWrite, &pState->stencil);

  return success;
}

bool vcGLState_ResetState(bool force /*= false*/)
{
  bool success = true;

  success &= vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back, true, force);
  success &= vcGLState_SetBlendMode(vcGLSBM_None, force);
  success &= vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true, nullptr, force);

  return success;
}

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW /*= true*/, bool force /*= false*/)
{
  if (force || fillMode != g_internalState.fillMode || cullMode != g_internalState.cullMode || isFrontCCW != g_internalState.isFrontCCW)
  {
    switch(fillMode)
    {
      case vcGLSFM_Solid:
        [g_pCurrFramebuffer->encoder setTriangleFillMode:MTLTriangleFillModeFill];
        break;
      case vcGLSFM_Wireframe:
        [g_pCurrFramebuffer->encoder setTriangleFillMode:MTLTriangleFillModeFill];
        break;
    }

    switch(cullMode)
    {
      case vcGLSCM_None:
        [g_pCurrFramebuffer->encoder setCullMode:MTLCullModeNone];
        break;
      case vcGLSCM_Front:
        [g_pCurrFramebuffer->encoder setCullMode:MTLCullModeFront];
        break;
      case vcGLSCM_Back:
        [g_pCurrFramebuffer->encoder setCullMode:MTLCullModeBack];
        break;
    }

    if (isFrontCCW)
      [g_pCurrFramebuffer->encoder setFrontFacingWinding:MTLWindingCounterClockwise];
    else
      [g_pCurrFramebuffer->encoder setFrontFacingWinding:MTLWindingClockwise];

    g_internalState.fillMode = fillMode;
    g_internalState.cullMode = cullMode;
    g_internalState.isFrontCCW = isFrontCCW;
  }
  return true;
}

bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force /*= false*/)
{
  if (force || blendMode != g_internalState.blendMode)
  {
    g_internalState.blendMode = blendMode;
    if (g_pCurrShader != nullptr && g_pCurrFramebuffer != nullptr)
    {
      vcShader_UpdatePipeline(g_pCurrShader);
      [g_pCurrFramebuffer->encoder setRenderPipelineState:g_pCurrShader->pipeline];
    }
  }
  return true;
}

bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil /* = nullptr */, bool force /*= false*/)
{
  bool enableStencil = pStencil != nullptr;

  if ((g_internalState.depthReadMode != depthReadMode) || (g_internalState.doDepthWrite != doDepthWrite) || force || (g_internalState.stencil.enabled != enableStencil) ||
      (enableStencil && ((g_internalState.stencil.onStencilFail != pStencil->onStencilFail) || (g_internalState.stencil.onDepthFail != pStencil->onDepthFail) || (g_internalState.stencil.onStencilAndDepthPass != pStencil->onStencilAndDepthPass) || (g_internalState.stencil.compareFunc != pStencil->compareFunc) || (g_internalState.stencil.compareMask != pStencil->compareMask) || (g_internalState.stencil.writeMask != pStencil->writeMask))))
  {
    g_internalState.depthReadMode = depthReadMode;
    g_internalState.doDepthWrite = doDepthWrite;

    g_internalState.stencil.enabled = enableStencil;

    @autoreleasepool
    {
      if (!enableStencil)
      {
        if (g_pCurrFramebuffer != nullptr)
          [g_pCurrFramebuffer->encoder setDepthStencilState:s_depthStates[((int)depthReadMode) * 2 + doDepthWrite]];

        return true;
      }

      MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc]init];

      depthStencilDesc.depthCompareFunction = mapDepthMode[depthReadMode];
      depthStencilDesc.depthWriteEnabled = doDepthWrite;

#if UDPLATFORM_OSX
      MTLStencilDescriptor *stencilDesc = [[MTLStencilDescriptor alloc] init];

      stencilDesc.readMask = (uint32)pStencil->compareMask;
      stencilDesc.writeMask = (uint32)pStencil->writeMask;
      stencilDesc.stencilCompareFunction = mapStencilFunction[pStencil->compareFunc];
      stencilDesc.stencilFailureOperation = mapStencilOperation[pStencil->onStencilFail];
      stencilDesc.depthFailureOperation = mapStencilOperation[pStencil->onDepthFail];
      stencilDesc.depthStencilPassOperation = mapStencilOperation[pStencil->onStencilAndDepthPass];

      depthStencilDesc.frontFaceStencil = stencilDesc;
      depthStencilDesc.backFaceStencil = stencilDesc;
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
      return false;
#else
# error "Unknown platform!"
#endif
      g_internalState.stencil.writeMask = pStencil->writeMask;
      g_internalState.stencil.compareFunc = pStencil->compareFunc;
      g_internalState.stencil.compareValue = pStencil->compareValue;
      g_internalState.stencil.compareMask = pStencil->compareMask;
      g_internalState.stencil.onStencilFail = pStencil->onStencilFail;
      g_internalState.stencil.onDepthFail = pStencil->onDepthFail;
      g_internalState.stencil.onStencilAndDepthPass = pStencil->onStencilAndDepthPass;

      if (g_pCurrFramebuffer != nullptr)
      {
        [g_pCurrFramebuffer->encoder setDepthStencilState:[g_device newDepthStencilStateWithDescriptor:depthStencilDesc]];
        [g_pCurrFramebuffer->encoder setStencilReferenceValue:(uint32_t)pStencil->compareValue];
      }
    }
  }

  return true;
}

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth /*= 0.f*/, float maxDepth /*= 1.f*/)
{
  if (x < 0 || y < 0 || width < 1 || height < 1)
    return false;

  g_internalState.viewportZone = udInt4::create(x, y, width, height);
  g_internalState.depthRange = udFloat2::create(minDepth, maxDepth);

  [g_pCurrFramebuffer->encoder setViewport:{
    .originX = double(g_internalState.viewportZone.x),
    .originY = double(g_internalState.viewportZone.y),
    .width = double(g_internalState.viewportZone.z),
    .height = double(g_internalState.viewportZone.w),
    .znear = g_internalState.depthRange.x,
    .zfar = g_internalState.depthRange.y
  }];
  
  vcGLState_Scissor(x, y, x + width, y + height);

  return true;
}

bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth)
{
  return vcGLState_SetViewport(g_internalState.viewportZone.x, g_internalState.viewportZone.y, g_internalState.viewportZone.z, g_internalState.viewportZone.w, minDepth, maxDepth);
}

bool vcGLState_Present(SDL_Window *pWindow)
{
  if (pWindow == nullptr)
    return false;

  @autoreleasepool {
    vcGLState_FlushBlit();

    [g_pDefaultFramebuffer->encoder endEncoding];

    if (g_currDrawable)
      [g_pDefaultFramebuffer->commandBuffer presentDrawable:g_currDrawable];
    
    [g_pDefaultFramebuffer->commandBuffer commit];
    
    g_currDrawable = [g_pMetalLayer nextDrawable];
    if (!g_currDrawable)
      NSLog(@"Drawable unavailable");

    g_pDefaultFramebuffer->pRenderPass.colorAttachments[0].texture = g_currDrawable.texture;
    
    g_pDefaultFramebuffer->commandBuffer = [g_queue commandBuffer];
    g_pDefaultFramebuffer->encoder = [g_pDefaultFramebuffer->commandBuffer renderCommandEncoderWithDescriptor:g_pDefaultFramebuffer->pRenderPass];
  }

  memset(&g_internalState.frameInfo, 0, sizeof(g_internalState.frameInfo));
  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  if (((int)width != (int)g_pDefaultFramebuffer->pRenderPass.colorAttachments[0].texture.width || (int)height != (int)g_pDefaultFramebuffer->pRenderPass.colorAttachments[0].texture.height))
  {
    g_pMetalLayer.drawableSize = CGSizeMake(width, height);
    g_pDefaultFramebuffer->pColor->width = width;
    g_pDefaultFramebuffer->pColor->height = height;
    g_pDefaultFramebuffer->actions |= vcRFA_Resize;

    if (g_pDefaultFramebuffer->pDepth)
    {
      vcTexture *pIntermediate;
      vcTexture_Create(&pIntermediate, width, height, nullptr, g_pDefaultFramebuffer->pDepth->format, vcTFM_Nearest, vcTCF_RenderTarget);

      vcTexture_Destroy(&g_pDefaultFramebuffer->pDepth);

      g_pDefaultFramebuffer->pDepth = pIntermediate;
      g_pDefaultFramebuffer->pRenderPass.depthAttachment.texture = pIntermediate->texture;
    }
  }

  return true;
}

void vcGLState_Scissor(int left, int top, int right, int bottom, bool force /*= false*/)
{
  udUnused(force);

  MTLScissorRect rect = {
      .x = (NSUInteger)left,
      .y = (NSUInteger)top,
      .width = (NSUInteger)right - left,
      .height = (NSUInteger)bottom - top
  };

  if (g_pCurrFramebuffer == g_pDefaultFramebuffer && (g_pCurrFramebuffer->actions & vcRFA_Resize) != vcRFA_Resize)
    [g_pCurrFramebuffer->encoder setScissorRect:rect];
}

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel)
{
  return udMin(desiredAniLevel, g_maxAnisotropy);
}

void vcGLState_ReportGPUWork(size_t drawCount, size_t triCount, size_t uploadBytesCount)
{
  g_internalState.frameInfo.drawCount += drawCount;
  g_internalState.frameInfo.triCount += triCount;
  g_internalState.frameInfo.uploadBytesCount += uploadBytesCount;
}

bool vcGLState_IsGPUDataUploadAllowed()
{
  return (g_internalState.frameInfo.uploadBytesCount < vcGLState_MaxUploadBytesPerFrame);
}

void vcGLState_FlushBlit()
{
  @autoreleasepool {
    [g_blitEncoder endEncoding];
    [g_blitBuffer commit];
    [g_blitBuffer waitUntilCompleted];
    g_blitBuffer = [g_queue commandBuffer];
    g_blitEncoder = [g_blitBuffer blitCommandEncoder];
  }
}
