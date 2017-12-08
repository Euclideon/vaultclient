#ifndef IMGUIIMPLESDL_H_
#define IMGUIIMPLESDL_H_

#include "imgui.h"
#include "udPlatform.h"
#if defined(UD_OS_WINDOWS) || defined(UD_OS_LINUX) || defined(UD_OS_MACOSX)
#include "imgui_impl_sdl_gl3.h"
#elif defined(UD_OS_ANDROID) || defined (UD_OS_IOS)
#include "imgui_impl_sdl_gles2.h"
#endif

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool ImGui_ImplSdl_Init(SDL_Window* window)
{
#if UD_DESKTOP
  return ImGui_ImplSdlGL3_Init(window);
#elif defined(UD_OS_ANDROID) || defined (UD_OS_IOS)
  return ImGui_ImplSdlGLES2_Init(window);
#endif

}
IMGUI_API void ImGui_ImplSdl_Shutdown()
{
#if UD_DESKTOP
  ImGui_ImplSdlGL3_Shutdown();
#else
  ImGui_ImplSdlGLES2_Shutdown();
#endif
}
IMGUI_API void ImGui_ImplSdl_NewFrame(SDL_Window* window)
{
#if UD_DESKTOP
  ImGui_ImplSdlGL3_NewFrame(window);
#else
  ImGui_ImplSdlGLES2_NewFrame();
#endif
}
IMGUI_API bool ImGui_ImplSdl_ProcessEvent(SDL_Event* event)
{
#if UD_DESKTOP
  return ImGui_ImplSdlGL3_ProcessEvent(event);
#else
  return ImGui_ImplSdlGLES2_ProcessEvent(event);
#endif
}

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplSdl_InvalidateDeviceObjects()
{
#if UD_DESKTOP
  ImGui_ImplSdlGL3_InvalidateDeviceObjects();
#else
  ImGui_ImplSdlGLES2_InvalidateDeviceObjects();
#endif
}
IMGUI_API bool ImGui_ImplSdl_CreateDeviceObjects()
{
#if UD_DESKTOP
  return ImGui_ImplSdlGL3_CreateDeviceObjects();
#else
  return ImGui_ImplSdlGLES2_CreateDeviceObjects();
#endif
}

#endif // !IMGUIIMPLESDL_H_
