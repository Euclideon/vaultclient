#include "vaultContext.h"
#include "vaultUDRenderer.h"
#include "vaultUDModel.h"
#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl_gl3.h"
#include "imgui_ex/imgui_dock.h"
#include "SDL2/SDL.h"
#include "udPlatform/udMath.h"
#include "vcRender.h"

#include <stdlib.h>

vaultUDModel *pModel;

struct vaultContainer
{
  vaultContext *pContext;
  vaultUDModel *pModel;

  vcRenderContext *pRenderContext;
};

enum vcDocks
{
  vcdScene,
  vcdSettings,
  vcdSceneExplorer,

  vcdStyling,
  vcdUIDemo,

  vcdTotalDocks
};

struct ProgramState
{
  bool programComplete;
  SDL_Window *pWindow;

  bool planeMode;
  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  udDouble4x4 modelMatrix;

  bool hasContext;
  bool windowsOpen[vcdTotalDocks];

  char *pServerURL;
  char *pUsername;
  char *pPassword;

  char *pModelPath;
};

void vcRenderWindow(ProgramState *pProgramState, vaultContainer *pVaultContainer);

#undef main
#ifdef SDL_MAIN_NEEDED
int SDL_main(int /*argc*/, char ** /*args*/)
#else
int main(int /*argc*/, char ** /*args*/)
#endif
{
  SDL_GLContext glcontext = NULL;

  ProgramState programState = {};
  vaultContainer vContainer = {};

  // default values
  programState.planeMode = true;
  programState.sceneResolution.x = 1280;
  programState.sceneResolution.y = 720;
  programState.camMatrix = udDouble4x4::identity();

  // default string values.
  const char *plocalHost = "http://vau-ubu-pro-001.euclideon.local";
  const char *pUsername = "";
  const char *pPassword = "";
  const char *pModelPath = "V:/QA/For Tests/Geoverse MDM/AdelaideCBD_2cm.uds";

  programState.pServerURL = new char[1024];
  programState.pUsername = new char[1024];
  programState.pPassword = new char[1024];
  programState.pModelPath = new char[1024];

  memset(programState.pServerURL, 0, 1024);
  memset(programState.pUsername, 0, 1024);
  memset(programState.pPassword, 0, 1024);
  memset(programState.pModelPath, 0, 1024);

  memcpy(programState.pServerURL, plocalHost, strlen(plocalHost));
  memcpy(programState.pUsername, pUsername, strlen(pUsername));
  memcpy(programState.pPassword, pPassword, strlen(pPassword));
  memcpy(programState.pModelPath, pModelPath, strlen(pModelPath));

  Uint64 NOW;
  Uint64 LAST;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    goto epilogue;

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0)
    goto epilogue;

  //TODO: Get a STRINGIFY macro in udPlatform somewhere
#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)
#ifdef GIT_BUILD
#define WINDOW_SUFFIX " (" STRINGIFY(GIT_BUILD) " - " __DATE__ ") "
#else
#define WINDOW_SUFFIX " (DEV/DO NOT DISTRIBUTE - " __DATE__ ")"
#endif

  programState.pWindow = SDL_CreateWindow("Euclideon Client" WINDOW_SUFFIX, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!programState.pWindow)
    goto epilogue;

  glcontext = SDL_GL_CreateContext(programState.pWindow);
  if (!glcontext)
    goto epilogue;

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    goto epilogue;

  ImGui::CreateContext();

  glGetError(); // throw out first error

  if (!ImGui_ImplSdlGL3_Init(programState.pWindow, "#version 150 core"))
    goto epilogue;

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  if (vcRender_Init(&vContainer.pRenderContext, programState.sceneResolution) != udR_Success)
    goto epilogue;

  ImGui::LoadDock();
  ImGui::GetIO().Fonts->AddFontFromFileTTF("NotoSansCJKjp-Regular.otf", 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChinese());

  while (!programState.programComplete)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      programState.programComplete = (event.type == SDL_QUIT);
    }

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    programState.deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

    ImGui_ImplSdlGL3_NewFrame(programState.pWindow);

    vcRenderWindow(&programState, &vContainer);

    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(programState.pWindow);
  }

  ImGui::SaveDock();
  ImGui::DestroyContext();

epilogue:
  vaultUDModel_Unload(vContainer.pContext, &vContainer.pModel);
  vcRender_Destroy(&vContainer.pRenderContext);
  vaultContext_Disconnect(&vContainer.pContext);

  delete[] programState.pServerURL;
  delete[] programState.pUsername;
  delete[] programState.pPassword;
  delete[] programState.pModelPath;

  return 0;
}

void vcHandleSceneInput(ProgramState *pProgramState)
{
  ImGuiIO& io = ImGui::GetIO();

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);
  SDL_Keymod modState = SDL_GetModState();

  bool isHovered = true;// ImGui::IsItemHovered();
  bool isLeftClicked = ImGui::IsMouseClicked(0, false);
  bool isRightClicked = ImGui::IsMouseClicked(1, false);
  bool isFocused = ImGui::IsWindowFocused();

  static bool clickedLeftWhileHovered = false;
  static bool clickedRightWhileHovered = false;
  if (isHovered && isLeftClicked)
    clickedLeftWhileHovered = true;

  if (isHovered && isRightClicked)
    clickedRightWhileHovered = true;

  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    if (clickedLeftWhileHovered)
    {
      clickedLeftWhileHovered = io.MouseDown[0];
      if (io.MouseDown[0])
      {
        //cam_x -= mouseDelta.x / size.x*cam_width;
        //cam_y += mouseDelta.y / size.y*cam_height;
        udDouble4 translation = pProgramState->camMatrix.axis.t;
        pProgramState->camMatrix.axis.t = { 0,0,0,1 };
        pProgramState->camMatrix = udDouble4x4::rotationAxis({ 0,0,-1 }, mouseDelta.x / 100.0) * pProgramState->camMatrix; // rotate on global axis and add back in the position
        pProgramState->camMatrix.axis.t = translation;
        pProgramState->camMatrix *= udDouble4x4::rotationAxis({ -1,0,0 }, mouseDelta.y / 100.0); // rotate on local axis, since this is the only one there will be no problem
      }
    }

    if (clickedRightWhileHovered)
    {
      clickedRightWhileHovered = io.MouseDown[1];
      if (io.MouseDown[1])
      {
        // do something here on right click
      }
    }

    float speed = 3; // 3 units per second
    if ((modState & KMOD_CTRL) > 0)
      speed *= 0.1; // slow

    if ((modState & KMOD_SHIFT) > 0)
      speed *= 10.0;  // fast

    float deltaMoveForward = speed * ((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
    float deltaMoveRight = speed * ((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
    float deltaMoveUp = speed * ((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);

    // Move the camera
    udDouble4 direction;
    if (pProgramState->planeMode)
    {
      direction = pProgramState->camMatrix.axis.y * deltaMoveForward + pProgramState->camMatrix.axis.x * deltaMoveRight + udDouble4{ 0, 0, (double)deltaMoveUp, 0 }; // don't use the camera orientation
    }
    else
    {
      direction = pProgramState->camMatrix.axis.y * deltaMoveForward + pProgramState->camMatrix.axis.x * deltaMoveRight;
      direction.z = 0;
      direction += udDouble4{ 0, 0, (double)deltaMoveUp, 0 }; // don't use the camera orientation
    }

    pProgramState->camMatrix.axis.t += direction * pProgramState->deltaTime;

    if (ImGui::IsMouseDoubleClicked(0)) {
      //fullscreen = !fullscreen;
    }
  }
}

void vcRenderSceneWindow(vaultContainer *pVaultContainer, ProgramState *pProgramState)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();

  if (size.x < 1 || size.y < 1)
    return;

  if (pProgramState->sceneResolution.x != size.x || pProgramState->sceneResolution.y != size.y) //Resize buffers
    vcRender_ResizeScene(pVaultContainer->pRenderContext, (uint32_t)size.x, (uint32_t)size.y);

  vcRenderData renderData = {};
  renderData.cameraMatrix = pProgramState->camMatrix;
  vcTexture texture = vcRender_RenderScene(pVaultContainer->pRenderContext, renderData);

  ImGui::Image((ImTextureID)((size_t)texture.id), size, ImVec2(0, 0), ImVec2(1, -1));
}

int vcMainMenuGui(ProgramState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pProgramState->programComplete = true;
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
      ImGui::MenuItem("Scene", nullptr, &pProgramState->windowsOpen[vcdScene]);
      ImGui::MenuItem("Scene Explorer", nullptr, &pProgramState->windowsOpen[vcdSceneExplorer]);
      ImGui::MenuItem("Settings", nullptr, &pProgramState->windowsOpen[vcdSettings]);
      ImGui::Separator();
      if (ImGui::BeginMenu("Debug Windows"))
      {
        ImGui::MenuItem("Styling", nullptr, &pProgramState->windowsOpen[vcdStyling]);
        ImGui::MenuItem("UI Debug Menu", nullptr, &pProgramState->windowsOpen[vcdUIDemo]);
        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcRenderWindow(ProgramState *pProgramState, vaultContainer *pVaultContainer)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  vaultError err;

  int menuHeight = (!pProgramState->hasContext) ? 0 : vcMainMenuGui(pProgramState);

  if (ImGui::GetIO().DisplaySize.y > 0)
  {
    ImVec2 pos = ImVec2(0.f, (float)menuHeight);
    ImVec2 size = ImGui::GetIO().DisplaySize;
    size.y -= pos.y;
    ImGui::RootDock(pos, ImVec2(size.x, size.y - 25.0f));

    // Draw status bar (no docking)
    ImGui::SetNextWindowSize(ImVec2(size.x, 25.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, size.y - 6.0f), ImGuiCond_Always);
    ImGui::Begin("statusbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    ImGui::End();
  }

  if (!pProgramState->hasContext)
  {
    if (ImGui::Begin("Login"))
    {
      static const char *pErrorMessage = nullptr;
      if (pErrorMessage != nullptr)
        ImGui::Text("%s", pErrorMessage);

      ImGui::InputText("ServerURL", pProgramState->pServerURL, 1024);
      ImGui::InputText("Username", pProgramState->pUsername, 1024);
      ImGui::InputText("Password", pProgramState->pPassword, 1024, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsNoBlank);

      if (ImGui::Button("Login!"))
      {
        err = vaultContext_Connect(&pVaultContainer->pContext, pProgramState->pServerURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vaultContext_Login(pVaultContainer->pContext, pProgramState->pUsername, pProgramState->pPassword);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vaultContext_GetLicense(pVaultContainer->pContext, vaultLT_Basic);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              vcRender_SetVaultContext(pVaultContainer->pRenderContext, pVaultContainer->pContext);
              pProgramState->hasContext = true;
              }
            }
          }
      }

      if (ImGui::Button("Guest Login!"))
      {
        err = vaultContext_Connect(&pVaultContainer->pContext, pProgramState->pServerURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vaultContext_Login(pVaultContainer->pContext, pProgramState->pUsername, pProgramState->pPassword);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vaultContext_GetLicense(pVaultContainer->pContext, vaultLT_Basic);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              vcRender_SetVaultContext(pVaultContainer->pRenderContext, pVaultContainer->pContext);
              pProgramState->hasContext = true;
              }
            }
          }

        if (pErrorMessage != nullptr)
        {
          ImGui::Text("%s", pErrorMessage);
        }
      }
    }

    ImGui::End();
  }
  else
  {
    if (ImGui::BeginDock("Scene", &pProgramState->windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar))
    {
      vcHandleSceneInput(pProgramState);
      vcRenderSceneWindow(pVaultContainer, pProgramState);
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->windowsOpen[vcdSceneExplorer]))
      ImGui::Text("Placeholder!");
    ImGui::EndDock();

    if (ImGui::BeginDock("StyleEditor", &pProgramState->windowsOpen[vcdStyling]))
      ImGui::ShowStyleEditor();
    ImGui::EndDock();

    if (ImGui::BeginDock("UIDebugMenu", &pProgramState->windowsOpen[vcdUIDemo]))
      ImGui::ShowDemoWindow();
    ImGui::EndDock();

    if (ImGui::BeginDock("Settings", &pProgramState->windowsOpen[vcdSettings]))
    {
      ImGui::InputText("Model Path", pProgramState->pModelPath, 1024);

      if (ImGui::Button("Load Model!"))
      {
        if (pVaultContainer->pModel != nullptr)
          vaultUDModel_Unload(pVaultContainer->pContext, &pVaultContainer->pModel);

        double midPoint[3];

        vaultUDModel_Load(pVaultContainer->pContext, &pVaultContainer->pModel, pProgramState->pModelPath);
        vaultUDModel_GetLocalMatrix(pVaultContainer->pContext, pVaultContainer->pModel, pProgramState->modelMatrix.a);
        vaultUDModel_GetModelCenter(pVaultContainer->pContext, pVaultContainer->pModel, midPoint);

        pModel = pVaultContainer->pModel; // TEMP
        pProgramState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);
      }

      udFloat3 modelT = udFloat3::create(pProgramState->camMatrix.axis.t.toVector3());
      if (ImGui::InputFloat3("Camera Position", &modelT.x))
        pProgramState->camMatrix.axis.t = udDouble4::create(udDouble3::create(modelT), 1.f);

      if (pVaultContainer->pModel != nullptr)
      {
        if (ImGui::Button("Unload Model"))
        {
          err = vaultUDModel_Unload(pVaultContainer->pContext, &pVaultContainer->pModel);
          if (err != vE_Success)
            goto epilogue;
        }
      }

      if (ImGui::Button("Logout"))
      {
        static const char *pErrorMessage = nullptr;

        if (pVaultContainer->pModel != nullptr)
        {
          err = vaultUDModel_Unload(pVaultContainer->pContext, &pVaultContainer->pModel);
          if (err != vE_Success)
            pErrorMessage = "Could not unload model";
        }

        if (pErrorMessage == nullptr)
        {
          err = vaultContext_Logout(pVaultContainer->pContext);
          if (err == vE_Success)
            pProgramState->hasContext = false;
        }
        else
        {
          ImGui::Text("%s", pErrorMessage);
        }
      }

      if (ImGui::RadioButton("PlaneMode", pProgramState->planeMode))
        pProgramState->planeMode = true;
      if (ImGui::RadioButton("HeliMode", !pProgramState->planeMode))
        pProgramState->planeMode = false;
    }
    ImGui::EndDock();
  }

epilogue:
  //TODO: Cleanup
  return;
}
