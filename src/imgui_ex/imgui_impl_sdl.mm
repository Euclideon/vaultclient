#include "udPlatform.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# import <UIKit/UIKit.h>
#elif UDPLATFORM_OSX
# import <Cocoa/Cocoa.h>
#else
# error "Unsupported platform!"
#endif

float ImGui_ImplSDL2_GetBackingScaleFactor(SDL_Window *pWindow)
{
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
  return sdlview.window.backingScaleFactor;
}
