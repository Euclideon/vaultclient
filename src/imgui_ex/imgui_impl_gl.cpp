#include "imgui.h"
#include "imgui_impl_gl.h"

#include "gl/vcGLState.h"
#include "gl/vcShader.h"
#include "gl/vcRenderShaders.h"
#include "gl/vcFramebuffer.h"
#include "gl/vcMesh.h"
#include "gl/vcLayout.h"
#include "gl/vcBackBuffer.h"

//GL Data
vcTexture *g_pFontTexture = nullptr;
vcShader *pImGuiShader = nullptr;
vcMesh *pImGuiMesh = nullptr;
vcShaderSampler *pImGuiSampler = nullptr;
static vcShaderConstantBuffer *g_pAttribLocationProjMtx = nullptr;

vcBackbuffer* pDefaultFramebuffer = nullptr;

static void ImGuiGL_InitPlatformInterface();
static void ImGuiGL_ShutdownPlatformInterface();

// Functions
bool ImGuiGL_Init(SDL_Window *pWindow)
{
  ImGuiIO& io = ImGui::GetIO();

  io.BackendRendererName = "imgui_vcgl";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    ImGuiGL_InitPlatformInterface();

  return vcGLState_Init(pWindow, &pDefaultFramebuffer);
}

void ImGuiGL_Shutdown()
{
  ImGuiGL_ShutdownPlatformInterface();
  ImGuiGL_DestroyDeviceObjects();
  vcGLState_Deinit();
}

void ImGuiGL_NewFrame()
{
  if (g_pFontTexture == nullptr)
    ImGuiGL_CreateDeviceObjects();

  if (!ImGui::GetIO().Fonts->IsBuilt())
    ImGuiGL_CreateFontsTexture();
}

// Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGuiGL_RenderDrawData(ImDrawData* draw_data)
{
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  ImGuiIO& io = ImGui::GetIO();
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_None, false);

  // Setup viewport, orthographic projection matrix
  vcGLState_SetViewport(0, 0, (int32_t)draw_data->DisplaySize.x, (int32_t)draw_data->DisplaySize.y);

#if GRAPHICS_API_METAL
  const udFloat4x4 ortho_projection = udFloat4x4::create(
    2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 1.0f
  );
#else
  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float T = draw_data->DisplayPos.y;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  udFloat4x4 ortho_projection = udFloat4x4::create(
      2.0f / (R - L),   0.0f,           0.0f,       0.0f,
      0.0f,         2.0f / (T - B),     0.0f,       0.0f,
      0.0f,         0.0f,           0.5f,       0.0f,
      (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f
  );
#endif
  vcShader_Bind(pImGuiShader);
  vcShader_BindConstantBuffer(pImGuiShader, g_pAttribLocationProjMtx, &ortho_projection, sizeof(ortho_projection));
  vcShader_GetSamplerIndex(&pImGuiSampler, pImGuiShader, "Texture");

  if (draw_data->CmdListsCount != 0 && pImGuiMesh == nullptr)
    vcMesh_Create(&pImGuiMesh, vcImGuiVertexLayout, 3, draw_data->CmdLists[0]->VtxBuffer.Data, draw_data->CmdLists[0]->VtxBuffer.Size, draw_data->CmdLists[0]->IdxBuffer.Data, draw_data->CmdLists[0]->IdxBuffer.Size, vcMF_Dynamic);

  // Draw
  ImVec2 pos = draw_data->DisplayPos;
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    uint32_t totalDrawn = 0;

    vcMesh_UploadData(pImGuiMesh, vcImGuiVertexLayout, 3, (void*)cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size, (void*)cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        vcGLState_Scissor((int)(pcmd->ClipRect.x - pos.x), (int)(pcmd->ClipRect.y - pos.y), (int)(pcmd->ClipRect.z - pos.x), (int)(pcmd->ClipRect.w - pos.y));
        vcShader_BindTexture(pImGuiShader, (vcTexture*)pcmd->TextureId, 0, pImGuiSampler);
        vcMesh_Render(pImGuiMesh, pcmd->ElemCount / 3, totalDrawn / 3);
      }
      totalDrawn += pcmd->ElemCount;
    }
  }

}

bool ImGuiGL_CreateFontsTexture()
{
  if (g_pFontTexture != nullptr)
    vcTexture_Destroy(&g_pFontTexture);

  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels = nullptr;
  int width = 0;
  int height = 0;

  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

  if (width > 0 && height > 0 && pixels != nullptr)
    vcTexture_Create(&g_pFontTexture, width, height, pixels);

  // Store our identifier
  io.Fonts->TexID = g_pFontTexture;

  return true;
}

void ImGuiGL_DestroyFontsTexture()
{
  vcTexture_Destroy(&g_pFontTexture);
}

bool ImGuiGL_CreateDeviceObjects()
{
  // This function is pretty messy now to support loading fonts on the fly but also to remain _close_ to how the ImGui sample works to make it easy to merge future changes

  if (pImGuiShader == nullptr)
    vcShader_CreateFromText(&pImGuiShader, g_ImGuiVertexShader, g_ImGuiFragmentShader, vcImGuiVertexLayout);

  if (g_pAttribLocationProjMtx == nullptr)
    vcShader_GetConstantBuffer(&g_pAttribLocationProjMtx, pImGuiShader, "u_EveryFrame", sizeof(udFloat4x4));

  ImGuiGL_CreateFontsTexture();

  return true;
}

void ImGuiGL_DestroyDeviceObjects()
{
    vcShader_DestroyShader(&pImGuiShader);
    vcMesh_Destroy(&pImGuiMesh);
    ImGuiGL_DestroyFontsTexture();
}

///////////////////// MULTI WINDOW SUPPORT

static void ImGuiGL_CreateWindow(ImGuiViewport *viewport)
{
  vcBackbuffer *pWindow = nullptr;
  vcBackbuffer_Create(&pWindow, (SDL_Window*)viewport->PlatformHandle, (int)viewport->Size.x, (int)viewport->Size.y);
  viewport->RendererUserData = pWindow;
}

static void ImGuiGL_DestroyWindow(ImGuiViewport *viewport)
{
  // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
  vcBackbuffer *pWindow = (vcBackbuffer *)viewport->RendererUserData;

  if(pWindow == nullptr)
    vcBackbuffer_Destroy(&pWindow);

  viewport->RendererUserData = NULL;
}

static void ImGuiGL_SetWindowSize(ImGuiViewport *viewport, ImVec2 size)
{
  vcBackbuffer *pWindow = (vcBackbuffer *)viewport->RendererUserData;

  if (pWindow != nullptr)
    vcBackbuffer_WindowResized(pWindow, (int)size.x, (int)size.y);
}

static void ImGuiGL_RenderWindow(ImGuiViewport *viewport, void *)
{
  vcBackbuffer *pWindow = (vcBackbuffer *)viewport->RendererUserData;

  vcFramebufferClearOperation clear = vcFramebufferClearOperation_None;
  if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
    clear = vcFramebufferClearOperation_All;

  vcBackbuffer_Bind(pWindow, clear, 0xFF0000FF);
  ImGuiGL_RenderDrawData(viewport->DrawData);
}

static void ImGuiGL_SwapBuffers(ImGuiViewport *viewport, void *)
{
  vcBackbuffer *pWindow = (vcBackbuffer *)viewport->RendererUserData;
  vcBackbuffer_Present(pWindow);
}

static void ImGuiGL_InitPlatformInterface()
{
  ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
  platform_io.Renderer_CreateWindow = ImGuiGL_CreateWindow;
  platform_io.Renderer_DestroyWindow = ImGuiGL_DestroyWindow;
  platform_io.Renderer_SetWindowSize = ImGuiGL_SetWindowSize;
  platform_io.Renderer_RenderWindow = ImGuiGL_RenderWindow;
  platform_io.Renderer_SwapBuffers = ImGuiGL_SwapBuffers;
}

static void ImGuiGL_ShutdownPlatformInterface()
{
  ImGui::DestroyPlatformWindows();
}
