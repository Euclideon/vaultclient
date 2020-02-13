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
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_ex/imgui_impl_metal.h"

int32_t g_maxAnisotropy = 0;
vcRenderer *_renderer;

id<MTLDevice> _device;
id<MTLLibrary> _library;
id<MTLCommandQueue> _queue;

static vcGLState s_internalState;

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

void vcGLState_BuildDepthStates()
{
  MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc]init];
  id<MTLDepthStencilState> dsState;

  for (int i = 0 ; i <= vcGLSDM_Always; ++i)
  {
    int j = i * 2;
    depthStencilDesc.depthWriteEnabled = false;
    depthStencilDesc.depthCompareFunction = mapDepthMode[i];
    dsState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    _renderer.depthStates[j] = dsState;

    depthStencilDesc.depthWriteEnabled = true;
    dsState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    _renderer.depthStates[j+1] = dsState;
  }
}

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
  _device = MTLCreateSystemDefaultDevice();
  _library = [_device newLibraryWithFile:[[NSBundle mainBundle] pathForResource:@"shaders" ofType:@"metallib" ] error:nil];

  if (_device == nullptr)
  {
    NSLog(@"Metal is not supported on this device");
    return false;
  }
  else if (_library == nullptr)
  {
    NSLog(@"Shader library couldn't be created");
    return false;
  }

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
  
  _renderer = [vcRenderer alloc];
  [_renderer initWithView:sdlview];

  vcTexture *defaultTexture, *defaultDepth;
  vcTexture_Create(&defaultTexture, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_BGRA8, vcTFM_Nearest, vcTCF_RenderTarget);
  vcTexture_Create(&defaultDepth, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, vcTCF_RenderTarget);
  
  vcFramebuffer *pFramebuffer;
  vcFramebuffer_Create(&pFramebuffer, defaultTexture, defaultDepth);
  vcFramebuffer_Bind(pFramebuffer);

  vcGLState_BuildDepthStates();

  ImGui_ImplSDL2_InitForOpenGL(pWindow);

  *ppDefaultFramebuffer = pFramebuffer;

  vcGLState_ResetState(true);

  return true;
}

void vcGLState_Deinit()
{
  return;
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
  if (force || fillMode != s_internalState.fillMode || cullMode != s_internalState.cullMode || isFrontCCW != s_internalState.isFrontCCW)
  {
    switch(fillMode)
    {
      case vcGLSFM_Solid:
        [_renderer setFillMode:MTLTriangleFillModeFill];
        break;
      case vcGLSFM_Wireframe:
        [_renderer setFillMode:MTLTriangleFillModeLines];
        break;
    }

    switch(cullMode)
    {
      case vcGLSCM_None:
        [_renderer setCullMode:MTLCullModeNone];
        break;
      case vcGLSCM_Front:
        [_renderer setCullMode:MTLCullModeFront];
        break;
      case vcGLSCM_Back:
        [_renderer setCullMode:MTLCullModeBack];
        break;
    }

    if (isFrontCCW)
      [_renderer setWindingMode:MTLWindingCounterClockwise];
    else
      [_renderer setWindingMode:MTLWindingClockwise];

    s_internalState.fillMode = fillMode;
    s_internalState.cullMode = cullMode;
    s_internalState.isFrontCCW = isFrontCCW;
  }
  return true;
}

bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force /*= false*/)
{
  if (force || blendMode != s_internalState.blendMode)
  {
    s_internalState.blendMode = blendMode;
    [_renderer bindBlendState:blendMode];
  }
  return true;
}

bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil /* = nullptr */, bool force /*= false*/)
{
  bool enableStencil = pStencil != nullptr;

  if ((s_internalState.depthReadMode != depthReadMode) || (s_internalState.doDepthWrite != doDepthWrite) || force || (s_internalState.stencil.enabled != enableStencil) ||
      (enableStencil && ((s_internalState.stencil.onStencilFail != pStencil->onStencilFail) || (s_internalState.stencil.onDepthFail != pStencil->onDepthFail) || (s_internalState.stencil.onStencilAndDepthPass != pStencil->onStencilAndDepthPass) || (s_internalState.stencil.compareFunc != pStencil->compareFunc) || (s_internalState.stencil.compareMask != pStencil->compareMask) || (s_internalState.stencil.writeMask != pStencil->writeMask))))
  {
    s_internalState.depthReadMode = depthReadMode;
    s_internalState.doDepthWrite = doDepthWrite;

    s_internalState.stencil.enabled = enableStencil;

    @autoreleasepool
    {
      if (!enableStencil)
      {
        [_renderer bindDepthStencil:_renderer.depthStates[((int)depthReadMode) * 2 + doDepthWrite] settings:nullptr];
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
      s_internalState.stencil.writeMask = pStencil->writeMask;
      s_internalState.stencil.compareFunc = pStencil->compareFunc;
      s_internalState.stencil.compareValue = pStencil->compareValue;
      s_internalState.stencil.compareMask = pStencil->compareMask;
      s_internalState.stencil.onStencilFail = pStencil->onStencilFail;
      s_internalState.stencil.onDepthFail = pStencil->onDepthFail;
      s_internalState.stencil.onStencilAndDepthPass = pStencil->onStencilAndDepthPass;

      id<MTLDepthStencilState> dsState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
      [_renderer bindDepthStencil:dsState settings:pStencil];
    }
  }

  return true;
}

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth /*= 0.f*/, float maxDepth /*= 1.f*/)
{
  if (x < 0 || y < 0 || width < 1 || height < 1)
    return false;

  MTLViewport vp =
  {
    .originX = double(x),
    .originY = double(y),
    .width = double(width),
    .height = double(height),
    .znear = minDepth,
    .zfar = maxDepth
  };
  
  [_renderer bindViewport:vp];
  s_internalState.viewportZone = udInt4::create(x, y, width, height);
  
  vcGLState_Scissor(x, y, x + width, y + height);

  return true;
}

bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth)
{
  return vcGLState_SetViewport(s_internalState.viewportZone.x, s_internalState.viewportZone.y, s_internalState.viewportZone.z, s_internalState.viewportZone.w, minDepth, maxDepth);
}

bool vcGLState_Present(SDL_Window *pWindow)
{
  if (pWindow == nullptr)
    return false;

  @autoreleasepool {
    [_renderer draw];
  }

  memset(&s_internalState.frameInfo, 0, sizeof(s_internalState.frameInfo));
  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  [_renderer setFrameSize:NSMakeSize(width, height)];
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
  [_renderer setScissor:rect];
}

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel)
{
  return udMin(desiredAniLevel, g_maxAnisotropy);
}

void vcGLState_ReportGPUWork(size_t drawCount, size_t triCount, size_t uploadBytesCount)
{
  s_internalState.frameInfo.drawCount += drawCount;
  s_internalState.frameInfo.triCount += triCount;
  s_internalState.frameInfo.uploadBytesCount += uploadBytesCount;
}

bool vcGLState_IsGPUDataUploadAllowed()
{
  return (s_internalState.frameInfo.uploadBytesCount < vcGLState_MaxUploadBytesPerFrame);
}
