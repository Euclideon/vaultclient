#include "vaultContext.h"
#include "vaultUDRenderer.h"
#include "vaultUDModel.h"
#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl_gl3.h"
#include "imgui_ex/imgui_dock.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udPlatformUtil.h"
#include "vcRender.h"
#include "vcSettings.h"
#include "vcGIS.h"

#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

udChunkedArray<vcModel> modelList;

static bool lastModelLoaded;

struct vcColumnHeader
{
  const char* pLabel;
  float size;
};

struct vaultContainer
{
  vaultContext *pContext;
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
  GLuint defaultFramebuffer;
  bool isFullscreen;

  bool onScreenControls;

  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  uint16_t currentSRID;

  bool hasContext;
  bool windowsOpen[vcdTotalDocks];

  char *pServerURL;
  char *pUsername;
  char *pPassword;

  char *pModelPath;

  struct
  {
    int vertical;
    int forward;
    int right;
  } moveDirection;

  vcSettings settings;
};

#if UDPLATFORM_OSX
char *pBasePath = nullptr;
#endif

void vcHandleSceneInput(ProgramState *pProgramState);
void vcRenderWindow(ProgramState *pProgramState, vaultContainer *pVaultContainer);
int vcMainMenuGui(ProgramState *pProgramState, vaultContainer *pVaultContainer);

void vcAddModelToList(vaultContainer *pVaultContainer, ProgramState *pProgramState, char *pFilePath);
bool vcUnloadModelList(vaultContainer *pVaultContainer);

bool vcModel_MoveToModelProjection(vaultContainer *pVaultContainer, ProgramState *pProgramState, vcModel *pModel);

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

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  char IconPath[] = ASSETDIR "Vault_Client.png";
#elif UDPLATFORM_OSX
  pBasePath = SDL_GetBasePath();
  if (pBasePath == nullptr)
    pBasePath = SDL_strdup("./");

  char fontPath[1024] = "";
  char IconPath[1024] = "";
  udSprintf(IconPath, 1024, "%s%s", pBasePath, "Vault_Client.png");
#else
  char IconPath[] = ASSETDIR "icons/Vault_Client.png";
#endif
  unsigned char *pData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;

  // default values
  programState.settings.camera.moveMode = vcCMM_Plane;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // TODO: Query device and fill screen
  programState.sceneResolution.x = 1920;
  programState.sceneResolution.y = 1080;
  programState.onScreenControls = true;
#else
  programState.sceneResolution.x = 1280;
  programState.sceneResolution.y = 720;
  programState.onScreenControls = false;
#endif
  programState.camMatrix = udDouble4x4::identity();

  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // While using the menu is tricky/impossible on iOS, default some windows to be open
  // TODO: Remove this because it's bad
  programState.windowsOpen[vcdSettings] = true;
  programState.windowsOpen[vcdScene] = true;
  programState.windowsOpen[vcdSceneExplorer] = true;
#endif

  modelList.Init(32);
  lastModelLoaded = true;

  // default string values.
  programState.pServerURL = udAllocType(char, 1024, udAF_Zero);
  programState.pUsername = udAllocType(char, 1024, udAF_Zero);
  programState.pPassword = udAllocType(char, 1024, udAF_Zero);
  programState.pModelPath = udAllocType(char, 1024, udAF_Zero);

  udStrcpy(programState.pServerURL, 1024, "http://vau-ubu-pro-001.euclideon.local");
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // Network drives aren't available on iOS
  // TODO: Something better!
  udStrcpy(programState.pModelPath, 1024, "http://pfox.euclideon.local:8080/AdelaideCBD_2cm.uds");
#else
  udStrcpy(programState.pModelPath, 1024, "V:/QA/For Tests/Geoverse MDM/AdelaideCBD_2cm.uds");
#endif

  Uint64 NOW;
  Uint64 LAST;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    goto epilogue;

  // Setup window
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0)
    goto epilogue;
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0)
    goto epilogue;
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  //TODO: Get a STRINGIFY macro in udPlatform somewhere
#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)
#ifdef GIT_BUILD
#define WINDOW_SUFFIX " (" STRINGIFY(GIT_BUILD) " - " __DATE__ ") "
#else
#define WINDOW_SUFFIX " (DEV/DO NOT DISTRIBUTE - " __DATE__ ")"
#endif

  programState.pWindow = SDL_CreateWindow("Euclideon Client" WINDOW_SUFFIX, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.sceneResolution.x, programState.sceneResolution.y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!programState.pWindow)
    goto epilogue;

  pData = stbi_load(IconPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

  pitch = iconWidth * iconBytesPerPixel;
  pitch = (pitch + 3) & ~3;

  rMask = 0xFF << 0;
  gMask = 0xFF << 8;
  bMask = 0xFF << 16;
  aMask = (iconBytesPerPixel == 4) ? (0xFF << 24) : 0;

  if (pData != nullptr)
    pIcon = SDL_CreateRGBSurfaceFrom(pData, iconWidth, iconHeight, iconBytesPerPixel * 8, pitch, rMask, gMask, bMask, aMask);
  if(pIcon != nullptr)
    SDL_SetWindowIcon(programState.pWindow, pIcon);

  SDL_free(pIcon);
  free(pData);

  glcontext = SDL_GL_CreateContext(programState.pWindow);
  if (!glcontext)
    goto epilogue;

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // Rendering to framebuffer 0 isn't valid when using UIKit in SDL2
  // so we need to use the framebuffer that SDL2 provides us.
  {
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(programState.pWindow, &wminfo);
    programState.defaultFramebuffer = wminfo.info.uikit.framebuffer;
  }
#else
  programState.defaultFramebuffer = 0;
#endif

#if UDPLATFORM_WINDOWS
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    goto epilogue;
#endif

  ImGui::CreateContext();

  glGetError(); // throw out first error

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  if (!ImGui_ImplSdlGL3_Init(programState.pWindow, "#version 300 es"))
    goto epilogue;
#else
  if (!ImGui_ImplSdlGL3_Init(programState.pWindow, "#version 150 core"))
    goto epilogue;
#endif

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  if (vcRender_Init(&vContainer.pRenderContext, &(programState.settings), programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, programState.defaultFramebuffer);

  ImGui::LoadDock();
#if UDPLATFORM_OSX
  udSprintf(fontPath, 1024, "%s%s", pBasePath, "NotoSansCJKjp-Regular.otf");
  ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChinese());
#else
  ImGui::GetIO().Fonts->AddFontFromFileTTF(ASSETDIR "fonts/NotoSansCJKjp-Regular.otf", 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChinese());
#endif

  SDL_EnableScreenSaver();

  while (!programState.programComplete)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSdlGL3_ProcessEvent(&event);

      if (event.type == SDL_DROPFILE && programState.hasContext)
      {
        udFree(programState.pModelPath);
        programState.pModelPath = udStrdup(event.drop.file);
        vcAddModelToList(&vContainer, &programState, programState.pModelPath);
      }

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
  vcUnloadModelList(&vContainer);
  modelList.Deinit();
  vcRender_Destroy(&vContainer.pRenderContext);
  vaultContext_Disconnect(&vContainer.pContext);

#if UDPLATFORM_OSX
  SDL_free(pBasePath);
#endif

  udFree(programState.pServerURL);
  udFree(programState.pUsername);
  udFree(programState.pPassword);
  udFree(programState.pModelPath);

  return 0;
}

void vcHandleSceneInput(ProgramState *pProgramState)
{
  //Setup and default values
  ImGuiIO& io = ImGui::GetIO();

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);
  SDL_Keymod modState = SDL_GetModState();

  float speed = pProgramState->settings.camera.moveSpeed; // 3 units per second default

  bool isHovered = ImGui::IsItemHovered();
  bool isLeftClicked = ImGui::IsMouseClicked(0, false);
  bool isRightClicked = ImGui::IsMouseClicked(1, false);
  bool isFocused = ImGui::IsWindowFocused();

  static bool clickedLeftWhileHovered = false;
  static bool clickedRightWhileHovered = false;
  if (isHovered && isLeftClicked)
    clickedLeftWhileHovered = true;

  if (isHovered && isRightClicked)
    clickedRightWhileHovered = true;

  if ((modState & KMOD_CTRL) > 0)
    speed *= 0.1; // slow

  if ((modState & KMOD_SHIFT) > 0)
    speed *= 10.0;  // fast

  // when focused
  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    if (pProgramState->onScreenControls)
    {
      pProgramState->moveDirection.forward += ((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
      pProgramState->moveDirection.right += ((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
      pProgramState->moveDirection.vertical += ((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);
    }
    else
    {
      pProgramState->moveDirection.forward = ((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
      pProgramState->moveDirection.right = ((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
      pProgramState->moveDirection.vertical = ((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);
    }



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

    if (isHovered)
    {
      if (io.MouseWheel > 0)
        pProgramState->settings.camera.moveSpeed *= 1.1f;
      if (io.MouseWheel < 0)
        pProgramState->settings.camera.moveSpeed /= 1.1f;

      pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
    }
  }

  // Move the camera
  udDouble4 direction = pProgramState->camMatrix.axis.y * speed * pProgramState->moveDirection.forward + pProgramState->camMatrix.axis.x * speed * pProgramState->moveDirection.right;
  if (pProgramState->settings.camera.moveMode == vcCMM_Helicopter)
  {
    direction.z = 0;
    if (direction.x != 0 || direction.y != 0)
      direction = udNormalize3(direction) * speed;

  }
  direction += udDouble4{ 0, 0, (double) (pProgramState->moveDirection.vertical * speed), 0 };
  pProgramState->camMatrix.axis.t += direction * pProgramState->deltaTime;

  pProgramState->moveDirection.vertical = 0;
  pProgramState->moveDirection.forward = 0;
  pProgramState->moveDirection.right = 0;
}

void vcRenderSceneWindow(vaultContainer *pVaultContainer, ProgramState *pProgramState)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImVec2 windowPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y);

  if (size.x < 1 || size.y < 1)
    return;

  if (pProgramState->sceneResolution.x != size.x || pProgramState->sceneResolution.y != size.y) //Resize buffers
  {
    vcRender_ResizeScene(pVaultContainer->pRenderContext, (uint32_t)size.x, (uint32_t)size.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pProgramState->defaultFramebuffer);
  }

  vcRenderData renderData = {};
  renderData.cameraMatrix = pProgramState->camMatrix;
  renderData.srid = pProgramState->currentSRID;
  renderData.models.Init(32);

  for (size_t i = 0; i < modelList.length; ++i)
  {
    renderData.models.PushBack(&modelList[i]);
  }

  vcTexture texture = vcRender_RenderScene(pVaultContainer->pRenderContext, renderData, pProgramState->defaultFramebuffer);

  renderData.models.Deinit();

  ImGui::Image((ImTextureID)((size_t)texture.id), size, ImVec2(0, 0), ImVec2(1, -1));

  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x - 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("SceneOverlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      ImGui::Text("Scene Info Overlay");

      ImGui::Separator();
      ImGui::Text("SRID: %d", pProgramState->currentSRID);

      if (pProgramState->currentSRID != 0)
      {
        udDouble3 cameraLatLong;
        vcGIS_LocalZoneToLatLong(pProgramState->currentSRID, pProgramState->camMatrix.axis.t.toVector3(), &cameraLatLong);
        ImGui::Text("LAT/LONG: %f %f", cameraLatLong.x, cameraLatLong.y);
      }

      ImGui::Separator();
      if (ImGui::IsMousePosValid())
        ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
      else
        ImGui::Text("Mouse Position: <invalid>");
    }
    ImGui::End();
  }

  // On Screen Controls Overlay
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + 5.f, windowPos.y + size.y - 5.f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (pProgramState->onScreenControls)
    {
      if (ImGui::Begin("OnScreenControls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      {
        ImGui::Text("Controls");

        ImGui::Separator();

        ImGui::Columns(3, NULL, false);

        ImGui::SetColumnWidth(0, 50);
        ImGui::SetColumnWidth(1, 25);
        ImGui::SetColumnWidth(2, 50);

        ImGui::PushID("oscUDSlider");
        ImGui::VSliderInt("",ImVec2(25,100), &(pProgramState->moveDirection.vertical), -1, 1, "U/D");
        ImGui::PopID();

        ImGui::NextColumn();
        ImGui::NextColumn();

        ImGui::PushID("oscFBSlider");
        ImGui::VSliderInt("", ImVec2(25,100), &(pProgramState->moveDirection.forward), -1, 1, "F/B");
        ImGui::PopID();

        ImGui::Columns(1);

        ImGui::PushID("oscLRSlider");
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("", &(pProgramState->moveDirection.right), -1, 1, "L/R");
        ImGui::PopItemWidth();
        ImGui::PopID();
        //ImGui::Text("V:%i/F:%i/R:%i", pProgramState->moveDirection.vertical, pProgramState->moveDirection.forward, pProgramState->moveDirection.right); // display code for actual values
      }
      ImGui::End();
    }

  }
}

int vcMainMenuGui(ProgramState *pProgramState, vaultContainer *pVaultContainer)
{
  int menuHeight = 0;
  vaultError err;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Logout"))
      {
        static const char *pErrorMessage = nullptr;

        if (!vcUnloadModelList(pVaultContainer))
          pErrorMessage = "Error unloading models!";

        if (pErrorMessage == nullptr)
        {
          err = vaultContext_Logout(pVaultContainer->pContext);
          if (err == vE_Success)
            pProgramState->hasContext = false;
        }
        else
        {
          ImGui::OpenPopup("Logout Error");
        }
      }
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pProgramState->programComplete = true;
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows"))
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
  glBindFramebuffer(GL_FRAMEBUFFER, pProgramState->defaultFramebuffer);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  vaultError err;
  SDL_Keymod modState = SDL_GetModState();

  int menuHeight = (!pProgramState->hasContext) ? 0 : vcMainMenuGui(pProgramState, pVaultContainer);

  //keyboard/mouse handling
  if (ImGui::IsKeyReleased(SDL_SCANCODE_F11))
  {
    pProgramState->isFullscreen = !pProgramState->isFullscreen;
    if (pProgramState->isFullscreen)
      SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
      SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }
#if UDPLATFORM_WINDOWS
  ImGuiIO& io = ImGui::GetIO(); // for future key commands as well

  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;
#endif
  //end keyboard/mouse handling

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
    if (ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
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

      ImGui::SameLine();
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
    if (ImGui::BeginPopupModal("Logout Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Error logging out! (0x00)");
      if (ImGui::Button("OK", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();
      ImGui::SetItemDefaultFocus();
      ImGui::EndPopup();
    }

    if (ImGui::BeginDock("Scene", &pProgramState->windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ResizeFromAnySide))
    {
      vcRenderSceneWindow(pVaultContainer, pProgramState);
      vcHandleSceneInput(pProgramState);
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->windowsOpen[vcdSceneExplorer], ImGuiWindowFlags_ResizeFromAnySide))
    {
      ImGui::InputText("Model Path", pProgramState->pModelPath, 1024);
      if (ImGui::Button("Load Model!"))
        vcAddModelToList(pVaultContainer, pProgramState, pProgramState->pModelPath);

      if (!lastModelLoaded)
        ImGui::Text("Invalid File/Not Found...");

      ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &pProgramState->camMatrix.axis.t.toVector3().x, 3);

      ImGui::RadioButton("PlaneMode", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Plane);
      ImGui::SameLine();
      ImGui::RadioButton("HeliMode", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Helicopter);

      // Models
      vcColumnHeader headers[] =
      {
        { "Model List", 400 },
        { "Goto", 50 },
        { "Visible", 50 },
        { "", 50 }, // unload column
        { "", 1 } // Null Column at end
      };
      static bool modelSelection[vcMaxModels] = {};

      ImGui::Columns(UDARRAYSIZE(headers), "ModelTableColumns", true);
      ImGui::Separator();

      float offset = 0.f;
      for (size_t i = 0; i < UDARRAYSIZE(headers); ++i)
      {

        ImGui::Text("%s", headers[i].pLabel);
        ImGui::SetColumnOffset(-1, offset);
        offset += headers[i].size;
        ImGui::NextColumn();
      }
      ImGui::Separator();
      // Table Contents

      for (size_t i = 0; i < modelList.length; i++)
      {
        // Column 1 - Model
        char modelLabelID[32] = "";
        udSprintf(modelLabelID, UDARRAYSIZE(modelLabelID), "ModelLabel%i", i);
        ImGui::PushID(modelLabelID);
        if (ImGui::Selectable(modelList[i].modelPath, modelSelection[i]))
        {
          if ((modState & KMOD_CTRL) == 0)
            memset(modelSelection, false, sizeof(modelSelection));
          modelSelection[i] = !modelSelection[i];
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        {
          double midPoint[3];
          vaultUDModel_GetModelCenter(pVaultContainer->pContext, modelList[i].pVaultModel, midPoint);
          pProgramState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);
        }

        ImVec2 textSize = ImGui::CalcTextSize(modelList[i].modelPath);
        if (ImGui::IsItemHovered() && (textSize.x >= headers[i].size))
          ImGui::SetTooltip("%s", modelList[i].modelPath);

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 2 - Goto button
        char gotoButtonID[32] = "";
        udSprintf(gotoButtonID, UDARRAYSIZE(gotoButtonID), "GotoButton%i", i);
        ImGui::PushID(gotoButtonID);
        if (ImGui::Button("Goto"))
        {
          double midPoint[3];
          vaultUDModel_GetModelCenter(pVaultContainer->pContext, modelList[i].pVaultModel, midPoint);
          pProgramState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);
        }
        ImGui::PopID();
        ImGui::NextColumn();
        // Column 3 - Visible
        char checkboxID[32] = "";
        udSprintf(checkboxID, UDARRAYSIZE(checkboxID), "ModelVisibleCheckbox%i", i);
        ImGui::PushID(checkboxID);
        ImGui::Checkbox("", &(modelList[i].modelVisible));
        ImGui::PopID();
        ImGui::NextColumn();
        // Column 4
        char unloadModelID[32] = "";
        udSprintf(unloadModelID, UDARRAYSIZE(unloadModelID), "UnloadModelButton%i", i);
        ImGui::PushID(unloadModelID);
        if (ImGui::Button("X"))
        {
          // unload model
          err = vaultUDModel_Unload(pVaultContainer->pContext, &(modelList[i].pVaultModel));
          if (err != vE_Success)
            goto epilogue;
          modelList.RemoveAt(i);

          for (size_t j = i; j < vcMaxModels; j++)
            modelSelection[j] = modelSelection[j + 1];
          modelSelection[vcMaxModels - 1] = false;

          lastModelLoaded = true;
          i--;
        }
        ImGui::PopID();
        ImGui::NextColumn();
        // Null Column
        ImGui::NextColumn();
      }
      ImGui::Columns(1);
      // End Models
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("StyleEditor", &pProgramState->windowsOpen[vcdStyling], ImGuiWindowFlags_ResizeFromAnySide))
      ImGui::ShowStyleEditor();
    ImGui::EndDock();

    if (pProgramState->windowsOpen[vcdUIDemo])
      ImGui::ShowDemoWindow();

    if (ImGui::BeginDock("Settings", &pProgramState->windowsOpen[vcdSettings], ImGuiWindowFlags_ResizeFromAnySide))
    {
      static int styleIndex = 1; // dark
      if (ImGui::Combo("Style", &styleIndex, "Classic\0Dark\0Light\0"))
      {
        switch (styleIndex)
        {
        case 0: ImGui::StyleColorsClassic(); break;
        case 1: ImGui::StyleColorsDark(); break;
        case 2: ImGui::StyleColorsLight(); break;
        }
      }

      ImGui::SliderFloat("Camera Speed", &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "Camera Speed = %.3f", 2.f);

      ImGui::SliderFloat("Camera Near Plane", &(pProgramState->settings.camera.nearPlane), vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax, "Camera Near Plane = %.3f", 2.f);
      ImGui::SliderFloat("Camera Far Plane", &(pProgramState->settings.camera.farPlane), vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "Camera Far Plane = %.3f", 2.f);

      float fovDeg = UD_RAD2DEGf(pProgramState->settings.camera.fieldOfView);
      ImGui::SliderFloat("Camera Field Of View", &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "Camera Field of View = %.0f");
      pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(fovDeg);
    }
    ImGui::EndDock();
  }

epilogue:
  //TODO: Cleanup
  return;
}

void vcAddModelToList(vaultContainer *pVaultContainer, ProgramState *pProgramState, char *pFilePath)
{
  if (pFilePath == nullptr)
    return;

  vcModel model = {};
  model.modelLoaded = true;

  udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pFilePath);

  if(vaultUDModel_Load(pVaultContainer->pContext, &model.pVaultModel, pFilePath) == vE_Success)
  {
    const char *pMetadata;
    if (vaultUDModel_GetMetadata(pVaultContainer->pContext, model.pVaultModel, &pMetadata) == vE_Success)
      model.metadata.Parse(pMetadata);

    vcModel_MoveToModelProjection(pVaultContainer, pProgramState, &model);

    modelList.PushBack(model);
  }
}

bool vcUnloadModelList(vaultContainer *pVaultContainer)
{
  vaultError err;
  for (int i = 0; i < (int) modelList.length; i++)
  {
    vaultUDModel *pVaultModel;
    pVaultModel = modelList[i].pVaultModel;
    err = vaultUDModel_Unload(pVaultContainer->pContext, &pVaultModel);
    if (err != vE_Success)
      return false;
    modelList[i].modelLoaded = false;
  }
  while (modelList.length > 0)
    modelList.PopFront();

  return true;
}

bool vcModel_MoveToModelProjection(vaultContainer *pVaultContainer, ProgramState *pProgramState, vcModel *pModel)
{
  if (pVaultContainer == nullptr || pProgramState == nullptr || pModel == nullptr)
    return false;

  double midPoint[3];
  vaultUDModel_GetModelCenter(pVaultContainer->pContext, pModel->pVaultModel, midPoint);
  pProgramState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);

  const char *pSRID = pModel->metadata.Get("ProjectionID").AsString();
  const char *pWKT = pModel->metadata.Get("ProjectionWKT").AsString();

  if (pSRID != nullptr)
  {
    pSRID = udStrchr(pSRID, ":");
    if (pSRID != nullptr)
      pProgramState->currentSRID = (uint16_t)udStrAtou(&pSRID[1]);
    else
      pProgramState->currentSRID = 0;
  }
  else if (pWKT != nullptr)
  {
    // Not sure?
  }
  else //No SRID available so set back to no projection
  {
    pProgramState->currentSRID = 0;
  }

  return true;
}
