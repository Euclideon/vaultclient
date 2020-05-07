#include "imgui.h"
#include "imgui_impl_gl.h"
#include "imgui_impl_sdl.h"

#include "vcGLState.h"
#include "vcShader.h"
#include "vcFramebuffer.h"
#include "vcMesh.h"
#include "vcLayout.h"

//GL Data
vcTexture *g_pFontTexture = nullptr;
vcShader *pImGuiShader = nullptr;
vcMesh *pImGuiMesh = nullptr;
vcShaderSampler *pImGuiSampler = nullptr;
static vcShaderConstantBuffer *g_pAttribLocationProjMtx = nullptr;

// Functions
bool ImGuiGL_Init(SDL_Window *pWindow)
{
  ImGui_ImplSDL2_InitForOpenGL(pWindow);
  return true;
}

void ImGuiGL_Shutdown()
{
  ImGui_ImplSDL2_Shutdown();
  ImGuiGL_DestroyDeviceObjects();
}

void ImGuiGL_NewFrame(SDL_Window *pWindow)
{
  if (g_pFontTexture == nullptr)
    ImGuiGL_CreateDeviceObjects();

  if (!ImGui::GetIO().Fonts->IsBuilt())
    ImGuiGL_CreateFontsTexture();

  ImGui_ImplSDL2_NewFrame(pWindow);
  ImGui::NewFrame();
}

// Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGuiGL_RenderDrawData(ImDrawData* draw_data)
{
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width <= 0 || fb_height <= 0)
    return;
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
  vcGLState_SetBlendMode(vcGLSBM_Interpolative);
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

  // Setup viewport, orthographic projection matrix
  vcGLState_SetViewport(0, 0, fb_width, fb_height);

  const udFloat4x4 ortho_projection = udFloat4x4::create(
    2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 1.0f
  );
  vcShader_Bind(pImGuiShader);
  vcShader_BindConstantBuffer(pImGuiShader, g_pAttribLocationProjMtx, &ortho_projection, sizeof(ortho_projection));
  vcShader_GetSamplerIndex(&pImGuiSampler, pImGuiShader, "Texture");

  if (draw_data->CmdListsCount != 0 && pImGuiMesh == nullptr)
    vcMesh_Create(&pImGuiMesh, vcImGuiVertexLayout, 3, draw_data->CmdLists[0]->VtxBuffer.Data, (uint32_t)draw_data->CmdLists[0]->VtxBuffer.Size, draw_data->CmdLists[0]->IdxBuffer.Data, (uint32_t)draw_data->CmdLists[0]->IdxBuffer.Size, vcMF_Dynamic);

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
    vcShader_CreateFromFile(&pImGuiShader, "asset://assets/shaders/imguiVertexShader", "asset://assets/shaders/imguiFragmentShader", vcImGuiVertexLayout);

  if (g_pAttribLocationProjMtx == nullptr)
  {
    vcShader_Bind(pImGuiShader);
    vcShader_GetConstantBuffer(&g_pAttribLocationProjMtx, pImGuiShader, "u_EveryFrame", sizeof(udFloat4x4));
  }

  ImGuiGL_CreateFontsTexture();

  return true;
}

void ImGuiGL_DestroyDeviceObjects()
{
    vcShader_DestroyShader(&pImGuiShader);
    vcMesh_Destroy(&pImGuiMesh);
    ImGuiGL_DestroyFontsTexture();
}
