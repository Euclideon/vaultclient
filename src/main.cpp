#include "vdkContext.h"
#include "vdkRenderContext.h"
#include "vdkModel.h"
#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl_gl3.h"
#include "imgui_ex/imgui_dock.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udPlatformUtil.h"
#include "vcCamera.h"
#include "vcRender.h"
#include "vcSettings.h"
#include "vcGIS.h"

#include <stdlib.h>
#include "udPlatform/udFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

udChunkedArray<vcModel> vcModelList;

static bool lastModelLoaded;

const char *pProjectsJSON = R"projects(
{
  "projects": [
    {
      "name": "Adelaide, South Australia",
      "models": [ "AdelaideCBD_2cm.uds" ]
    },
    {
      "name": "Juneau, Alaska",
      "models": [ "AK_Juneau.uds" ]
    },
    {
      "name": "Ben's Bike (CAD)",
      "models": [ "BenBike-0001.uds" ]
    },
    {
      "name": "Engine (CAD)",
      "models": [ "engine_Assem_lit_1mm.uds" ]
    },
    {
      "name": "Japanese House",
      "models": [ "Japanese House/furniture/Low_table.uds", "Japanese House/furniture/Rocking_Chair.uds", "Japanese House/furniture/sofa_L_cushion.uds", "Japanese House/furniture/Table_Chair.uds", "Japanese House/ssf/mirror.uds" ]
    },
    {
      "name": "Mt Morgan",
      "models": [ "Mt_Morgan_Range_Data/Grid_721_22549_0.uds", "Mt_Morgan_Range_Data/Grid_722_22546_0.uds", "Mt_Morgan_Range_Data/Grid_722_22546_1.uds", "Mt_Morgan_Range_Data/Grid_722_22547_0.uds", "Mt_Morgan_Range_Data/Grid_722_22547_1.uds", "Mt_Morgan_Range_Data/Grid_722_22548_0.uds", "Mt_Morgan_Range_Data/Grid_722_22549_0.uds", "Mt_Morgan_Range_Data/Grid_722_22550_0.uds", "Mt_Morgan_Range_Data/Grid_723_22545_0.uds", "Mt_Morgan_Range_Data/Grid_723_22545_1.uds", "Mt_Morgan_Range_Data/Grid_723_22546_0.uds", "Mt_Morgan_Range_Data/Grid_723_22546_1.uds", "Mt_Morgan_Range_Data/Grid_723_22547_0.uds", "Mt_Morgan_Range_Data/Grid_723_22548_0.uds", "Mt_Morgan_Range_Data/Grid_723_22549_0.uds", "Mt_Morgan_Range_Data/Grid_723_22550_0.uds", "Mt_Morgan_Range_Data/Grid_724_22545_0.uds", "Mt_Morgan_Range_Data/Grid_724_22545_1.uds", "Mt_Morgan_Range_Data/Grid_724_22546_0.uds" ]
    },
    {
      "name": "Saint Peterskirche",
      "models": [ "Peterskirche(SolidScan)_cleaned.ssf" ]
    },
    {
      "name": "Manhatten",
      "models": [ "plw_manhatten_full_5cm3.uds" ]
    },
    {
      "name": "Quarry",
      "models": [ "Quarry_FullResolution.uds" ]
    },
    {
      "name": "Uluru",
      "models": [ "Uluru-AyersRock_7cm.uds" ]
    },
    {
      "name": "Wien",
      "models": [ "Wien_LOD4.uds" ]
    }
  ]
}
)projects";

struct vcColumnHeader
{
  const char* pLabel;
  float size;
};

struct vaultContainer
{
  vdkContext *pContext;
  vcRenderContext *pRenderContext;
};

struct ProgramState
{
  bool programComplete;
  SDL_Window *pWindow;
  GLuint defaultFramebuffer;

  bool onScreenControls;

  vcCamera camera;

  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  uint16_t currentSRID;

  vcTexture watermarkTexture;

  bool hasContext;

  char serverURL[vcMaxPathLength];
  char username[vcMaxPathLength];
  char password[vcMaxPathLength];

  char modelPath[vcMaxPathLength];



  vcSettings settings;
  udValue projects;
};

#if UDPLATFORM_OSX
char *pBasePath = nullptr;
#endif

void vcHandleSceneInput(ProgramState *pProgramState);
void vcRenderWindow(ProgramState *pProgramState, vaultContainer *pVaultContainer);
int vcMainMenuGui(ProgramState *pProgramState, vaultContainer *pVaultContainer);

void vcSettings_LoadSettings(ProgramState *pProgramState, bool forceDefaults);

void vcModel_AddToList(vaultContainer *pVaultContainer, ProgramState *pProgramState, const char *pFilePath);
bool vcModel_UnloadList(vaultContainer *pVaultContainer);
bool vcModel_MoveToModelProjection(vaultContainer *pVaultContainer, ProgramState *pProgramState, vcModel *pModel);

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
int SDL_main(int /*argc*/, char ** /*args*/)
#else
int main(int /*argc*/, char ** /*args*/)
#endif
{
  SDL_GLContext glcontext = NULL;
  uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  ProgramState programState = {};
  vaultContainer vContainer = {};

  udFile_RegisterHTTP();

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  char IconPath[] = ASSETDIR "Vault_Client.png";
  char EucWatermarkPath[] = ASSETDIR "EuclideonClientWM.png";
#elif UDPLATFORM_OSX
  pBasePath = SDL_GetBasePath();
  if (pBasePath == nullptr)
    pBasePath = SDL_strdup("./");

  char fontPath[vcMaxPathLength] = "";
  char IconPath[vcMaxPathLength] = "";
  char EucWatermarkPath[vcMaxPathLength] = "";
  udSprintf(IconPath, vcMaxPathLength, "%s%s", pBasePath, "Vault_Client.png");
  udSprintf(EucWatermarkPath, vcMaxPathLength, "%s%s", pBasePath, "EuclideonClientWM.png");
#else
  char IconPath[] = ASSETDIR "icons/Vault_Client.png";
  char EucWatermarkPath[] = ASSETDIR "icons/EuclideonClientWM.png";
#endif
  unsigned char *pIconData = nullptr;
  unsigned char *pEucWatermarkData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;

  // default values
  programState.settings.cameraSettings.moveMode = vcCMM_Plane;
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
  vcCamera_Create(&programState.camera);

  programState.settings.cameraSettings.moveSpeed = 3.f;
  programState.settings.cameraSettings.nearPlane = 0.5f;
  programState.settings.cameraSettings.farPlane = 10000.f;
  programState.settings.cameraSettings.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

  vcModelList.Init(32);
  lastModelLoaded = true;

  // default string values.
  udStrcpy(programState.serverURL, vcMaxPathLength, "http://vau-ubu-pro-001.euclideon.local");
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // Network drives aren't available on iOS
  // TODO: Something better!
  udStrcpy(programState.modelPath, vcMaxPathLength, "http://pfox.euclideon.local:8080/AdelaideCBD_2cm.uds");
#else
  udStrcpy(programState.modelPath, vcMaxPathLength, "V:/QA/For Tests/Geoverse MDM/AdelaideCBD_2cm.uds");
#endif

  programState.settings.maptiles.mapEnabled = true;
  programState.settings.maptiles.mapHeight = 0.f;
  programState.settings.maptiles.transparency = 1.f;
  udStrcpy(programState.settings.maptiles.tileServerAddress, vcMaxPathLength, "http://pfox.euclideon.local:8123");

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

  programState.pWindow = SDL_CreateWindow("Euclideon Client" WINDOW_SUFFIX, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.sceneResolution.x, programState.sceneResolution.y, windowFlags);
  if (!programState.pWindow)
    goto epilogue;

  pIconData = stbi_load(IconPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

  pitch = iconWidth * iconBytesPerPixel;
  pitch = (pitch + 3) & ~3;

  rMask = 0xFF << 0;
  gMask = 0xFF << 8;
  bMask = 0xFF << 16;
  aMask = (iconBytesPerPixel == 4) ? (0xFF << 24) : 0;

  if (pIconData != nullptr)
    pIcon = SDL_CreateRGBSurfaceFrom(pIconData, iconWidth, iconHeight, iconBytesPerPixel * 8, pitch, rMask, gMask, bMask, aMask);
  if(pIcon != nullptr)
    SDL_SetWindowIcon(programState.pWindow, pIcon);

  SDL_free(pIcon);

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
  vcSettings_LoadSettings(&programState, false);

  glGetError(); // throw out first error

  // setup watermark for background

  pEucWatermarkData = stbi_load(EucWatermarkPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0); // reusing the variables for width etc

  programState.watermarkTexture = vcTextureCreate(iconWidth, iconHeight, vcTextureFormat_RGBA8);
  vcTextureUploadPixels(&programState.watermarkTexture, pEucWatermarkData, iconWidth, iconHeight);

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  if (!ImGui_ImplSdlGL3_Init(programState.pWindow, "#version 300 es"))
    goto epilogue;
#else
  if (!ImGui_ImplSdlGL3_Init(programState.pWindow, "#version 150 core"))
    goto epilogue;
#endif

  programState.projects.Parse(pProjectsJSON);

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  if (vcRender_Init(&vContainer.pRenderContext, &(programState.settings), &programState.camera, programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, programState.defaultFramebuffer);

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  ImGui::GetIO().Fonts->AddFontFromFileTTF(ASSETDIR "/NotoSansCJKjp-Regular.otf", 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChinese());
#elif UDPLATFORM_OSX
  udSprintf(fontPath, vcMaxPathLength, "%s%s", pBasePath, "NotoSansCJKjp-Regular.otf");
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
      if (!ImGui_ImplSdlGL3_ProcessEvent(&event))
      {
        if (event.type == SDL_WINDOWEVENT)
        {
          if (event.window.event == SDL_WINDOWEVENT_RESIZED)
          {
            programState.settings.window.width = event.window.data1;
            programState.settings.window.height = event.window.data2;
          }
          else if (event.window.event == SDL_WINDOWEVENT_MOVED)
          {
            programState.settings.window.xpos = event.window.data1;
            programState.settings.window.ypos = event.window.data2;
          }
          else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED)
          {
            programState.settings.window.maximized = true;
          }
          else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
          {
            programState.settings.window.maximized = false;
          }
        }
        else if (event.type == SDL_DROPFILE && programState.hasContext)
        {
          vcModel_AddToList(&vContainer, &programState, event.drop.file);
        }
        else if (event.type == SDL_QUIT)
        {
          programState.programComplete = true;
        }
      }
    }

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    programState.deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

    ImGui_ImplSdlGL3_NewFrame(programState.pWindow);

    vcRenderWindow(&programState, &vContainer);

    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(programState.pWindow);

    if (ImGui::GetIO().WantSaveIniSettings)
      vcSettings_Save(&programState.settings);
  }

  vcSettings_Save(&programState.settings);
  ImGui::DestroyContext();

epilogue:
  vcTextureDestroy(&programState.watermarkTexture);
  free(pIconData);
  free(pEucWatermarkData);
  vcModel_UnloadList(&vContainer);
  vcModelList.Deinit();
  vcRender_Destroy(&vContainer.pRenderContext);
  vdkContext_Disconnect(&vContainer.pContext);

#if UDPLATFORM_OSX
  SDL_free(pBasePath);
#endif

  return 0;
}

void vcHandleSceneInput(ProgramState *pProgramState)
{
  //Setup and default values
  ImGuiIO& io = ImGui::GetIO();

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);
  SDL_Keymod modState = SDL_GetModState();

  float speed = pProgramState->settings.cameraSettings.moveSpeed; // 3 units per second default

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
      pProgramState->camera.moveDirection.forward += (float)((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
      pProgramState->camera.moveDirection.right += (float)((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
      pProgramState->camera.moveDirection.vertical += (float)((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);
    }
    else
    {
      pProgramState->camera.moveDirection.forward = (float) ((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
      pProgramState->camera.moveDirection.right = (float)((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
      pProgramState->camera.moveDirection.vertical = (float)((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);
    }



    if (clickedLeftWhileHovered && !isLeftClicked)
    {
      clickedLeftWhileHovered = io.MouseDown[0];
      if (io.MouseDown[0])
      {
        float yawAmount = -mouseDelta.x / 100.f;
        float pitchAmount = mouseDelta.y / 100.f;
        float rollAmount = 0.f;

        if (pProgramState->settings.cameraSettings.invertX)
          yawAmount *= -1;
        if (pProgramState->settings.cameraSettings.invertY)
          pitchAmount *= -1;

        pProgramState->camera.yprRotation += udDouble3::create(yawAmount, pitchAmount, rollAmount);
        pProgramState->camera.yprRotation.y = udClamp(pProgramState->camera.yprRotation.y, (double) -UD_PI/2, (double) UD_PI/2);
      }
    }

    if (isHovered)
    {
      if (io.MouseWheel > 0)
        pProgramState->settings.cameraSettings.moveSpeed *= 1.1f;
      if (io.MouseWheel < 0)
        pProgramState->settings.cameraSettings.moveSpeed /= 1.1f;

      pProgramState->settings.cameraSettings.moveSpeed = udClamp(pProgramState->settings.cameraSettings.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
    }
  }

  // Move the camera

  udDouble3 addPos = udDouble3::create(pProgramState->camera.moveDirection.right, pProgramState->camera.moveDirection.forward, 0);

  addPos = (udDouble4x4::rotationYPR(pProgramState->camera.yprRotation) * udDouble4::create(addPos, 1)).toVector3();
  if (pProgramState->settings.cameraSettings.moveMode == vcCMM_Helicopter)
  {
    addPos.z = 0;
    if (addPos.x != 0 || addPos.y != 0)
      addPos = udNormalize3(addPos);
  }
  addPos.z += pProgramState->camera.moveDirection.vertical;
  addPos *= speed * pProgramState->deltaTime;

  pProgramState->camera.position += addPos;

  pProgramState->camMatrix = vcCamera_GetMatrix(&pProgramState->camera);

  pProgramState->camera.moveDirection.vertical = 0;
  pProgramState->camera.moveDirection.forward = 0;
  pProgramState->camera.moveDirection.right = 0;
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

  for (size_t i = 0; i < vcModelList.length; ++i)
  {
    renderData.models.PushBack(&vcModelList[i]);
  }

  vcTexture texture = vcRender_RenderScene(pVaultContainer->pRenderContext, renderData, pProgramState->defaultFramebuffer);

  renderData.models.Deinit();

  ImGui::Image((ImTextureID)((size_t)texture.id), size, ImVec2(0, 0), ImVec2(1, -1));

  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x - 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("SceneOverlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      if (pProgramState->currentSRID != 0)
      {
        udDouble3 cameraLatLong;
        vcGIS_LocalToLatLong(pProgramState->currentSRID, pProgramState->camMatrix.axis.t.toVector3(), &cameraLatLong);

        ImGui::Text("SRID: %d", pProgramState->currentSRID);
        ImGui::Text("LAT/LONG: %f %f", cameraLatLong.x, cameraLatLong.y);
      }
      else
      {
        ImGui::Text("Not Geolocated");
      }

      if (pProgramState->settings.showFPS)
      {
        ImGui::Separator();
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
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
      if (ImGui::Begin("OnScreenControls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      {
        ImGui::SetWindowSize(ImVec2(175, 150));
        ImGui::Text("Controls");

        ImGui::Separator();


        ImGui::Columns(2, NULL, false);

        ImGui::SetColumnWidth(0, 50);

        ImGui::PushID("oscUDSlider");
        ImGui::VSliderFloat("",ImVec2(40,100), &pProgramState->camera.moveDirection.vertical, -1, 1, "U/D");
        ImGui::PopID();

        ImGui::NextColumn();

        ImGuiIO &io = ImGui::GetIO();

        ImGui::Button("Move Camera",ImVec2(100,100));
        if (ImGui::IsItemActive())
        {
          // Draw a line between the button and the mouse cursor
          ImDrawList* draw_list = ImGui::GetWindowDrawList();
          draw_list->PushClipRectFullScreen();
          draw_list->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
          draw_list->PopClipRect();

          ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);

          pProgramState->camera.moveDirection.forward = -1.f * value_raw.y / vcSL_OSCPixelRatio;
          pProgramState->camera.moveDirection.right = value_raw.x / vcSL_OSCPixelRatio;

          if (pProgramState->camera.moveDirection.forward > 1.f)
            pProgramState->camera.moveDirection.forward = 1.f;
          else if (pProgramState->camera.moveDirection.forward < -1.f)
            pProgramState->camera.moveDirection.forward = -1.f;

          if (pProgramState->camera.moveDirection.right> 1.f)
            pProgramState->camera.moveDirection.right = 1.f;
          else if (pProgramState->camera.moveDirection.right< -1.f)
            pProgramState->camera.moveDirection.right = -1.f;

        }

        ImGui::Columns(1);
      }
      ImGui::End();
    }

  }
}

int vcMainMenuGui(ProgramState *pProgramState, vaultContainer *pVaultContainer)
{
  int menuHeight = 0;
  vdkError err;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Logout"))
      {
        static const char *pErrorMessage = nullptr;

        if (!vcModel_UnloadList(pVaultContainer))
          pErrorMessage = "Error unloading models!";

        if (pErrorMessage == nullptr)
        {
          err = vdkContext_Logout(pVaultContainer->pContext);
          if (err == vE_Success)
            pProgramState->hasContext = false;
        }
        else
        {
          ImGui::OpenPopup("Logout Error");
        }
      }

      if (ImGui::MenuItem("Restore Defaults", nullptr))
        vcSettings_LoadSettings(pProgramState, true);

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pProgramState->programComplete = true;
#endif

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows"))
    {
      ImGui::MenuItem("Scene", nullptr, &pProgramState->settings.window.windowsOpen[vcdScene]);
      ImGui::MenuItem("Scene Explorer", nullptr, &pProgramState->settings.window.windowsOpen[vcdSceneExplorer]);
      ImGui::MenuItem("Settings", nullptr, &pProgramState->settings.window.windowsOpen[vcdSettings]);
      ImGui::Separator();
      if (ImGui::BeginMenu("Debug"))
      {
        ImGui::MenuItem("Styling", nullptr, &pProgramState->settings.window.windowsOpen[vcdStyling]);
        ImGui::MenuItem("UI Debug Menu", nullptr, &pProgramState->settings.window.windowsOpen[vcdUIDemo]);
        ImGui::MenuItem("Show FPS", nullptr, &pProgramState->settings.showFPS);
        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }

    udValueArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (pProjectList != nullptr)
    {
      if (ImGui::BeginMenu("Projects", pProjectList->length > 0 && !udStrEqual(pProgramState->settings.resourceBase, "")))
      {
        for (size_t i = 0; i < pProjectList->length; ++i)
        {
          if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
          {
            vcModel_UnloadList(pVaultContainer);

            for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
            {
              char buffer[vcMaxPathLength];
              udSprintf(buffer, vcMaxPathLength, "%s/%s", pProgramState->settings.resourceBase, pProjectList->GetElement(i)->Get("models[%d]", j).AsString());
              vcModel_AddToList(pVaultContainer, pProgramState, buffer);
            }
          }
        }

        ImGui::EndMenu();
      }
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

  vdkError err;
  SDL_Keymod modState = SDL_GetModState();

  //keyboard/mouse handling
  if (ImGui::IsKeyReleased(SDL_SCANCODE_F11))
  {
    pProgramState->settings.window.fullscreen = !pProgramState->settings.window.fullscreen;
    if (pProgramState->settings.window.fullscreen)
      SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
      SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }

  ImGuiIO& io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

#if UDPLATFORM_WINDOWS
  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;
#endif

  //end keyboard/mouse handling

  if (pProgramState->hasContext)
  {
    float menuHeight = (float)vcMainMenuGui(pProgramState, pVaultContainer);
    ImGui::RootDock(ImVec2(0, menuHeight), ImVec2(size.x, size.y - menuHeight));
  }
  else
  {
    ImGui::RootDock(ImVec2(0, 0), ImVec2(size.x, size.y));
  }

  if (!pProgramState->hasContext)
  {
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::SetNextWindowPos(ImVec2(size.x-5,size.y-5), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::Image((ImTextureID)((size_t)pProgramState->watermarkTexture.id), ImVec2(512, 512), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(500, 150));
    if (ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
      static const char *pErrorMessage = nullptr;
      if (pErrorMessage != nullptr)
        ImGui::Text("%s", pErrorMessage);

      ImGui::InputText("ServerURL", pProgramState->serverURL, vcMaxPathLength);
      ImGui::InputText("Username", pProgramState->username, vcMaxPathLength);
      ImGui::InputText("Password", pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsNoBlank);

      if (ImGui::Button("Login!"))
      {
        err = vdkContext_Connect(&pVaultContainer->pContext, pProgramState->serverURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vdkContext_Login(pVaultContainer->pContext, pProgramState->username, pProgramState->password);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vdkContext_GetLicense(pVaultContainer->pContext, vdkLT_Basic);
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
    }

    ImGui::End();

    if (ImGui::Begin("_DEVSERVER", nullptr, ImGuiWindowFlags_NoCollapse))
    {
      ImGui::Text("Vault Server: %s", pProgramState->serverURL);
      ImGui::Text("Resource Location: %s", pProgramState->settings.resourceBase);
      ImGui::Text("Tile Server: %s", pProgramState->settings.maptiles.tileServerAddress);

      ImGui::Separator();

      if (ImGui::Button("Use 'pfox'"))
      {
        udStrcpy(pProgramState->serverURL, vcMaxPathLength, "http://vau-ubu-pro-001.euclideon.local");
        udStrcpy(pProgramState->settings.resourceBase, vcMaxPathLength, "http://pfox.euclideon.local:8080");
        udStrcpy(pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, "http://pfox.euclideon.local:8123");
      }

      ImGui::SameLine();

      if (ImGui::Button("Use 'pfox-vpn'"))
      {
        udStrcpy(pProgramState->serverURL, vcMaxPathLength, "http://vau-ubu-pro-001.euclideon.local");
        udStrcpy(pProgramState->settings.resourceBase, vcMaxPathLength, "http://pfox-vpn.euclideon.local:8080");
        udStrcpy(pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, "http://pfox-vpn.euclideon.local:8123");
      }

      ImGui::SameLine();

      if (ImGui::Button("Use Dan Conference"))
      {
        udStrcpy(pProgramState->serverURL, vcMaxPathLength, "http://192.168.1.1");
        udStrcpy(pProgramState->settings.resourceBase, vcMaxPathLength, "http://192.168.1.1:8080");
        udStrcpy(pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, "http://192.168.1.1:8123");
      }

      ImGui::SameLine();
      static bool custom = false;
      ImGui::Checkbox("Custom", &custom);
      if (custom)
      {
        static int ipBlocks[] = { 192, 168, 1, 1 };
        bool changed = false;

        changed |= ImGui::SliderInt("1", &ipBlocks[0], 0, 255);
        changed |= ImGui::SliderInt("2", &ipBlocks[1], 0, 255);
        changed |= ImGui::SliderInt("3", &ipBlocks[2], 0, 255);
        changed |= ImGui::SliderInt("4", &ipBlocks[3], 0, 255);

        if (changed)
        {
          udSprintf(pProgramState->serverURL, vcMaxPathLength, "http://%d.%d.%d.%d", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
          udSprintf(pProgramState->settings.resourceBase, vcMaxPathLength, "http://%d.%d.%d.%d:8080", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
          udSprintf(pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, "http://%d.%d.%d.%d:8123", ipBlocks[0], ipBlocks[1], ipBlocks[2], ipBlocks[3]);
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

    if (ImGui::BeginDock("Scene", &pProgramState->settings.window.windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ResizeFromAnySide))
    {
      vcRenderSceneWindow(pVaultContainer, pProgramState);
      vcHandleSceneInput(pProgramState);
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->settings.window.windowsOpen[vcdSceneExplorer], ImGuiWindowFlags_ResizeFromAnySide))
    {
      ImGui::InputText("Model Path", pProgramState->modelPath, vcMaxPathLength);
      if (ImGui::Button("Load Model!"))
        vcModel_AddToList(pVaultContainer, pProgramState, pProgramState->modelPath);

      if (!lastModelLoaded)
        ImGui::Text("Invalid File/Not Found...");

      ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &pProgramState->camera.position.x, 3);

      // Models

      int minMaxColumnSize[][2] =
      {
        {50,500},
        {40,40},
        {35,35},
        {1,1}
      };

      vcColumnHeader headers[] =
      {
        { "Model List", 400 },
        { "Show", 40 },
        { "", 35 }, // unload column
        { "", 1 } // Null Column at end
      };

      int col1Size = (int)ImGui::GetContentRegionAvailWidth();
      col1Size -= 40 + 35; // subtract size of two buttons
      if (col1Size > minMaxColumnSize[0][1])
        col1Size = minMaxColumnSize[0][1];
      if (col1Size < minMaxColumnSize[0][0])
        col1Size = minMaxColumnSize[0][0];
      headers[0].size = (float) col1Size;


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

      for (size_t i = 0; i < vcModelList.length; ++i)
      {
        // Column 1 - Model
        char modelLabelID[32] = "";
        udSprintf(modelLabelID, UDARRAYSIZE(modelLabelID), "ModelLabel%i", i);
        ImGui::PushID(modelLabelID);
        if (ImGui::Selectable(vcModelList[i].modelPath, vcModelList[i].modelSelected))
        {
          if ((modState & KMOD_CTRL) == 0)
          {
            for (size_t j = 0; j < vcModelList.length; ++j)
            {
              vcModelList[j].modelSelected = false;
            }
          }

          vcModelList[i].modelSelected = true;
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          vcModel_MoveToModelProjection(pVaultContainer, pProgramState, &vcModelList[i]);

        ImVec2 textSize = ImGui::CalcTextSize(vcModelList[i].modelPath);
        if (ImGui::IsItemHovered() && (textSize.x >= headers[i].size))
          ImGui::SetTooltip("%s", vcModelList[i].modelPath);

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 2 - Visible
        char checkboxID[32] = "";
        udSprintf(checkboxID, UDARRAYSIZE(checkboxID), "ModelVisibleCheckbox%i", i);
        ImGui::PushID(checkboxID);
        ImGui::Checkbox("", &(vcModelList[i].modelVisible));
        ImGui::PopID();
        ImGui::NextColumn();
        // Column 3 - Unload Model
        char unloadModelID[32] = "";
        udSprintf(unloadModelID, UDARRAYSIZE(unloadModelID), "UnloadModelButton%i", i);
        ImGui::PushID(unloadModelID);
        if (ImGui::Button("X",ImVec2(20,20)))
        {
          // unload model
          err = vdkModel_Unload(pVaultContainer->pContext, &(vcModelList[i].pVaultModel));
          if (err != vE_Success)
            goto epilogue;

          vcModelList.RemoveAt(i);

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

    if (ImGui::BeginDock("StyleEditor", &pProgramState->settings.window.windowsOpen[vcdStyling], ImGuiWindowFlags_ResizeFromAnySide))
      ImGui::ShowStyleEditor();
    ImGui::EndDock();

    if (pProgramState->settings.window.windowsOpen[vcdUIDemo])
      ImGui::ShowDemoWindow();

    if (ImGui::BeginDock("Settings", &pProgramState->settings.window.windowsOpen[vcdSettings], ImGuiWindowFlags_ResizeFromAnySide))
    {
      ImGui::Text("UI Settings");

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

      ImGui::Separator();
      ImGui::Text("Camera");

      ImGui::RadioButton("PlaneMode", (int*)&pProgramState->settings.cameraSettings.moveMode, vcCMM_Plane);
      ImGui::SameLine();
      ImGui::RadioButton("HeliMode", (int*)&pProgramState->settings.cameraSettings.moveMode, vcCMM_Helicopter);
      ImGui::SameLine();
      ImGui::Checkbox("On Screen Controls", &pProgramState->onScreenControls);

      ImGui::Checkbox("Invert X-axis", &pProgramState->settings.cameraSettings.invertX);
      ImGui::SameLine();
      ImGui::Checkbox("Invert Y-axis", &pProgramState->settings.cameraSettings.invertY);

      ImGui::SliderFloat("Move Speed", &(pProgramState->settings.cameraSettings.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 2.f);
      if (ImGui::SliderFloat("Near Plane", &pProgramState->settings.cameraSettings.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax, "%.3fm", 2.f))
        pProgramState->settings.cameraSettings.farPlane = udMin(pProgramState->settings.cameraSettings.farPlane, pProgramState->settings.cameraSettings.nearPlane * vcSL_CameraNearFarPlaneRatioMax);

      if (ImGui::SliderFloat("Far Plane", &pProgramState->settings.cameraSettings.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
        pProgramState->settings.cameraSettings.nearPlane = udMax(pProgramState->settings.cameraSettings.nearPlane, pProgramState->settings.cameraSettings.farPlane / vcSL_CameraNearFarPlaneRatioMax);

      float fovDeg = UD_RAD2DEGf(pProgramState->settings.cameraSettings.fieldOfView);
      ImGui::SliderFloat("Field Of View", &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "%.0f Degrees");
      pProgramState->settings.cameraSettings.fieldOfView = UD_DEG2RADf(fovDeg);


      ImGui::Separator();

      ImGui::Checkbox("Map Tiles", &pProgramState->settings.maptiles.mapEnabled);

      if (pProgramState->settings.maptiles.mapEnabled)
      {
        ImGui::InputText("Tile Server", pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength);

        ImGui::SliderFloat("Map Height", &pProgramState->settings.maptiles.mapHeight, -1000.f, 1000.f, "%.3fm", 2.f);

        const char* blendModes[] = { "Hybrid", "Overlay" };
        if (ImGui::BeginCombo("Blending", blendModes[pProgramState->settings.maptiles.blendMode]))
        {
          for (size_t n = 0; n < UDARRAYSIZE(blendModes); ++n)
          {
            bool isSelected = (pProgramState->settings.maptiles.blendMode == n);

            if (ImGui::Selectable(blendModes[n], isSelected))
              pProgramState->settings.maptiles.blendMode = (vcMapTileBlendMode)n;

            if (isSelected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }

        ImGui::SliderFloat("Transparency", &pProgramState->settings.maptiles.transparency, 0.f, 1.f, "%.3f");
      }
    }
    ImGui::EndDock();
  }

epilogue:
  //TODO: Cleanup
  return;
}

void vcModel_AddToList(vaultContainer *pVaultContainer, ProgramState *pProgramState, const char *pFilePath)
{
  if (pFilePath == nullptr)
    return;

  vcModel model = {};
  model.modelLoaded = true;
  model.modelVisible = true;
  model.modelSelected = false;
  model.pMetadata = udAllocType(udValue, 1, udAF_Zero);

  udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pFilePath);

  if(vdkModel_Load(pVaultContainer->pContext, &model.pVaultModel, pFilePath) == vE_Success)
  {
    const char *pMetadata;
    if (vdkModel_GetMetadata(pVaultContainer->pContext, model.pVaultModel, &pMetadata) == vE_Success)
      model.pMetadata->Parse(pMetadata);

    vcModel_MoveToModelProjection(pVaultContainer, pProgramState, &model);

    vcModelList.PushBack(model);
  }
}

bool vcModel_UnloadList(vaultContainer *pVaultContainer)
{
  vdkError err;
  for (int i = 0; i < (int) vcModelList.length; i++)
  {
    vdkModel *pVaultModel;
    pVaultModel = vcModelList[i].pVaultModel;
    err = vdkModel_Unload(pVaultContainer->pContext, &pVaultModel);
    vcModelList[i].pMetadata->Destroy();
    udFree(vcModelList[i].pMetadata);
    if (err != vE_Success)
      return false;
    vcModelList[i].modelLoaded = false;
  }
  while (vcModelList.length > 0)
    vcModelList.PopFront();

  return true;
}

bool vcModel_MoveToModelProjection(vaultContainer *pVaultContainer, ProgramState *pProgramState, vcModel *pModel)
{
  if (pVaultContainer == nullptr || pProgramState == nullptr || pModel == nullptr)
    return false;

  double midPoint[3];
  vdkModel_GetModelCenter(pVaultContainer->pContext, pModel->pVaultModel, midPoint);
  pProgramState->camera.position = udDouble3::create(midPoint[0], midPoint[1], midPoint[2]);

  const char *pSRID = pModel->pMetadata->Get("ProjectionID").AsString();
  const char *pWKT = pModel->pMetadata->Get("ProjectionWKT").AsString();

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

void vcSettings_LoadSettings(ProgramState *pProgramState, bool forceDefaults)
{
  if (vcSettings_Load(&pProgramState->settings, forceDefaults))
  {
#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
    if (pProgramState->settings.window.fullscreen)
      SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
      SDL_SetWindowFullscreen(pProgramState->pWindow, 0);

    if (pProgramState->settings.window.maximized)
      SDL_MaximizeWindow(pProgramState->pWindow);
    else
      SDL_RestoreWindow(pProgramState->pWindow);

    SDL_SetWindowPosition(pProgramState->pWindow, pProgramState->settings.window.xpos, pProgramState->settings.window.ypos);
    SDL_SetWindowSize(pProgramState->pWindow, pProgramState->settings.window.width, pProgramState->settings.window.height);
#endif
  }
}
