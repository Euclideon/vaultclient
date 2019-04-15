#import "vcMetal.h"

#import "SDL2/SDL.h"
#import "SDL2/SDL_syswm.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MTKView.h>

#import "ViewCon.h"
#include "vcStrings.h"
#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_sdl.h"

int32_t g_maxAnisotropy = 0;
ViewCon *_viewCon;

id<MTLDevice> _device;
id<MTLLibrary> _library;
id<MTLCommandQueue> _queue;

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
    _viewCon.renderer.depthStates[j] = dsState;
    
    depthStencilDesc.depthWriteEnabled = true;
    dsState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    _viewCon.renderer.depthStates[j+1] = dsState;
  }
}

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
  _device = MTLCreateSystemDefaultDevice();
  _library = [_device newLibraryWithFile:[[NSBundle mainBundle] pathForResource:@"shaders" ofType:@"metallib" ] error:nil];
  
#ifdef METAL_DEBUG
  if (!_device)
  {
    NSLog(@"Metal is not supported on this device");
  }
  else if (!_library)
  {
    NSLog(@"Shader library couldn't be created");
  }
#endif
  
  SDL_SysWMinfo wminfo;
  SDL_VERSION(&wminfo.version);
  SDL_GetWindowWMInfo(pWindow, &wminfo);
  
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  UIView *sdlview = wminfo.info.uikit.window;
#else
  NSView *sdlview = wminfo.info.cocoa.window.contentView;
#endif
  
  sdlview.autoresizesSubviews = true;
  
  _viewCon = [ViewCon alloc];
  _viewCon.Mview = [[MTKView alloc] initWithFrame:sdlview.frame device:_device];
  [sdlview addSubview:_viewCon.Mview];
  _viewCon.Mview.device = _device;
  _viewCon.Mview.framebufferOnly = false;
  _viewCon.Mview.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
  _viewCon.Mview.autoResizeDrawable = true;
  _viewCon.Mview.preferredFramesPerSecond = 30;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  _viewCon.Mview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
#else
  _viewCon.Mview.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
#endif
  
  [_viewCon viewDidLoad];
  
  vcTexture *defaultTexture, *defaultDepth;
  vcTexture_Create(&defaultTexture, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_BGRA8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  vcTexture_Create(&defaultDepth, sdlview.frame.size.width, sdlview.frame.size.height, nullptr, vcTextureFormat_D24S8, vcTFM_Nearest, false, vcTWM_Clamp, vcTCF_RenderTarget);
  
  [_viewCon.renderer.renderPasses addObject:_viewCon.Mview.currentRenderPassDescriptor];
  _viewCon.renderer.renderPasses[0].colorAttachments[0].loadAction = MTLLoadActionClear;
  _viewCon.renderer.renderPasses[0].colorAttachments[0].storeAction = MTLStoreActionStore;
  _viewCon.renderer.renderPasses[0].colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
  _viewCon.renderer.renderPasses[0].depthAttachment.loadAction = MTLLoadActionClear;
  _viewCon.renderer.renderPasses[0].depthAttachment.storeAction = MTLStoreActionStore;
  _viewCon.renderer.renderPasses[0].depthAttachment.clearDepth = 1.0;
  _viewCon.renderer.renderPasses[0].stencilAttachment.clearStencil = 0;
  
  vcFramebuffer *pFramebuffer;
  vcFramebuffer_Create(&pFramebuffer, defaultTexture, defaultDepth);
  vcFramebuffer_Bind(pFramebuffer);
  
  vcGLState_BuildDepthStates();
  
  ImGui_ImplMetal_Init();
  
#ifdef METAL_DEBUG
  if(!_viewCon.Mview)
  {
    NSLog(@"MTKView wasn't created");
  }
#endif

  *ppDefaultFramebuffer = pFramebuffer;
  
  s_internalState.viewportZone = udInt4::create(0,0,sdlview.frame.size.width, sdlview.frame.size.height);

  vcGLState_ResetState(true);
  
  return true;
}

void vcGLState_Deinit()
{
  [_viewCon.Mview removeFromSuperview];
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
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back, true, force);
  vcGLState_SetBlendMode(vcGLSBM_None, force);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true, nullptr, force);

  return true;
}

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW /*= true*/, bool force /*= false*/)
{
  if (force || fillMode != s_internalState.fillMode || cullMode != s_internalState.cullMode || isFrontCCW != s_internalState.isFrontCCW)
  {
    if (fillMode == vcGLSFM_Solid)
      [_viewCon.renderer fillMode:MTLTriangleFillModeFill];
    else
      [_viewCon.renderer fillMode:MTLTriangleFillModeLines];
    
    if (cullMode == vcGLSCM_None)
      [_viewCon.renderer cullMode:MTLCullModeNone];
    else if (cullMode == vcGLSCM_Front)
      [_viewCon.renderer cullMode:MTLCullModeFront];
    else if (cullMode == vcGLSCM_Back)
      [_viewCon.renderer cullMode:MTLCullModeBack];
    
    if (isFrontCCW)
      [_viewCon.renderer windingMode:MTLWindingCounterClockwise];
    else
      [_viewCon.renderer windingMode:MTLWindingClockwise];
    
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
    [_viewCon.renderer setBlendMode:blendMode];
  }
  return true;
}

bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil /* = nullptr */, bool force /*= false*/)
{
  udUnused(force);
  
  bool enableStencil = pStencil != nullptr;
  
  if (!enableStencil)
  {
    [_viewCon.renderer bindDepthStencil:_viewCon.renderer.depthStates[((int)depthReadMode) * 2 + doDepthWrite] settings:nullptr];
    return true;
  }
  
  MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc]init];
  
  depthStencilDesc.depthCompareFunction = mapDepthMode[depthReadMode];
  depthStencilDesc.depthWriteEnabled = doDepthWrite;
  
#if !UDPLATFORM_IOS && !UDPLATFORM_IOS_SIMULATOR
  if (enableStencil)
  {
    MTLStencilDescriptor *stencilDesc = [[MTLStencilDescriptor alloc] init];
    
    stencilDesc.readMask = (uint32)pStencil->compareMask;
    stencilDesc.writeMask = (uint32)pStencil->writeMask;
    stencilDesc.stencilCompareFunction = mapStencilFunction[pStencil->compareFunc];
    stencilDesc.stencilFailureOperation = mapStencilOperation[pStencil->onStencilFail];
    stencilDesc.depthFailureOperation = mapStencilOperation[pStencil->onDepthFail];
    stencilDesc.depthStencilPassOperation = mapStencilOperation[pStencil->onStencilAndDepthPass];
    
    depthStencilDesc.frontFaceStencil = stencilDesc;
    depthStencilDesc.backFaceStencil = stencilDesc;
  }
#endif
  
  id<MTLDepthStencilState> dsState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
  [_viewCon.renderer bindDepthStencil:dsState settings:pStencil];
  
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
  
  [_viewCon.renderer bindViewport:vp];
  s_internalState.viewportZone = udInt4::create(x, y, width, height);

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
    [_viewCon.Mview draw];
  }

  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  vcGLState_SetViewport(0, 0, width, height);
  return true;
}

void vcGLState_Scissor(int left, int top, int width, int height, bool force /*= false*/)
{
  udUnused(left);
  udUnused(top);
  udUnused(width);
  udUnused(height);
  udUnused(force);
  return;
}

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel)
{
  return udMin(desiredAniLevel, g_maxAnisotropy);
}
