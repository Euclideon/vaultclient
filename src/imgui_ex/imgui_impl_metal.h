
// dear imgui: Renderer for Metal

// This needs to be used along with a Platform Binding (e.g. OSX)

// Implemented features:

// [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.

// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.

#ifndef imgui_impl_metal_h__
#define imgui_impl_metal_h__

#include <SDL2/SDL.h>

IMGUI_IMPL_API bool ImGui_ImplMetal_Init();

IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();

IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(SDL_Window *pWindow);

IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data);


// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateFontsTexture();

IMGUI_IMPL_API void ImGui_ImplMetal_DestroyFontsTexture();

IMGUI_IMPL_API bool ImGui_ImplMetal_CreateDeviceObjects();

IMGUI_IMPL_API void ImGui_ImplMetal_DestroyDeviceObjects();

#endif