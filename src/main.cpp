#include "vcState.h"

#include "vdkContext.h"
#include "vdkRenderContext.h"
#include "vdkModel.h"
#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl_gl3.h"
#include "imgui_ex/imgui_dock.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "udPlatform/udChunkedArray.h"
#include "vcCamera.h"
#include "vcRender.h"
#include "vcGIS.h"

#include <stdlib.h>
#include "udPlatform/udFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  include <crtdbg.h>
#  include <stdio.h>
#endif

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

#if UDPLATFORM_OSX
char *pBasePath = nullptr;
#endif

void vcHandleSceneInput(vcState *pProgramState);
void vcRenderWindow(vcState *pProgramState);
int vcMainMenuGui(vcState *pProgramState);

void vcSettings_LoadSettings(vcState *pProgramState, bool forceDefaults);
bool vcLogout(vcState *pProgramState);

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath);
bool vcModel_UnloadList(vcState *pProgramState);
bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
int SDL_main(int /*argc*/, char ** /*args*/)
#else
int main(int /*argc*/, char ** /*args*/)
#endif
{
#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  SDL_GLContext glcontext = NULL;
  uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  vcState programState = {};

  udFile_RegisterHTTP();

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  char IconPath[] = ASSETDIR "EuclideonClientIcon.png";
  char EucWatermarkPath[] = ASSETDIR "EuclideonLogo.png";
#elif UDPLATFORM_OSX
  pBasePath = SDL_GetBasePath();
  if (pBasePath == nullptr)
    pBasePath = SDL_strdup("./");

  char fontPath[vcMaxPathLength] = "";
  char IconPath[vcMaxPathLength] = "";
  char EucWatermarkPath[vcMaxPathLength] = "";
  udSprintf(IconPath, vcMaxPathLength, "%s%s", pBasePath, "EuclideonClientIcon.png");
  udSprintf(EucWatermarkPath, vcMaxPathLength, "%s%s", pBasePath, "EuclideonLogo.png");
#else
  char IconPath[] = ASSETDIR "icons/EuclideonClientIcon.png";
  char EucWatermarkPath[] = ASSETDIR "icons/EuclideonLogo.png";
#endif
  unsigned char *pIconData = nullptr;
  unsigned char *pEucWatermarkData = nullptr;
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
  vcCamera_Create(&programState.pCamera);

  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

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

  vcTexture_Create(&programState.pWatermarkTexture, iconWidth, iconHeight, vcTextureFormat_RGBA8);
  vcTexture_UploadPixels(programState.pWatermarkTexture, pEucWatermarkData, iconWidth, iconHeight);

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

  if (vcRender_Init(&(programState.pRenderContext), &(programState.settings), programState.pCamera, programState.sceneResolution) != udR_Success)
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
          vcModel_AddToList(&programState, event.drop.file);
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

    vcRenderWindow(&programState);

    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(programState.pWindow);

    if (ImGui::GetIO().WantSaveIniSettings)
      vcSettings_Save(&programState.settings);
  }

  vcSettings_Save(&programState.settings);
  ImGui::DestroyContext();

epilogue:
  vcCamera_Destroy(&programState.pCamera);
  vcTexture_Destroy(&programState.pWatermarkTexture);
  free(pIconData);
  free(pEucWatermarkData);
  vcModel_UnloadList(&programState);
  vcModelList.Deinit();
  vcRender_Destroy(&programState.pRenderContext);
  vdkContext_Disconnect(&programState.pContext);

#if UDPLATFORM_OSX
  SDL_free(pBasePath);
#endif

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 0)
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks found");
  }
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  return 0;
}

void vcHandleSceneInput(vcState *pProgramState)
{
  //Setup and default values
  ImGuiIO& io = ImGui::GetIO();

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);
  SDL_Keymod modState = SDL_GetModState();

  float speedModifier = 1.f;

  udDouble3 moveOffset = udDouble3::zero();
  udDouble3 rotationOffset = udDouble3::zero();
  bool isOrbitActive = false;

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
    speedModifier *= 0.1; // slow

  if ((modState & KMOD_SHIFT) > 0)
    speedModifier *= 10.0;  // fast

  // when focused
  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    moveOffset.y += (float)((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
    moveOffset.x += (float)((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
    moveOffset.z += (float)((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);

    if (moveOffset != udDouble3::zero() || isLeftClicked || isRightClicked) // if input detected
      pProgramState->zoomPath.isZooming = false;

    if (pProgramState->settings.camera.moveMode == vcCMM_Orbit && isLeftClicked)
    {
      pProgramState->orbitPos = pProgramState->worldMousePos;
      pProgramState->storedDeltaAngle = vcCamera_CreateStoredRotation(pProgramState->pCamera, pProgramState->orbitPos);
    }

    if (clickedLeftWhileHovered && !isLeftClicked)
    {
      clickedLeftWhileHovered = io.MouseDown[0];
      if (io.MouseDown[0])
      {
        rotationOffset.x = -mouseDelta.x / 100.f;
        rotationOffset.y = -mouseDelta.y / 100.f;
        rotationOffset.z = 0.f;

        isOrbitActive = true;
      }
    }

    if (ImGui::IsMouseDoubleClicked(0))
    {
      if (pProgramState->worldMousePos != udDouble3::zero())
      {
        pProgramState->zoomPath.isZooming = true;
        pProgramState->zoomPath.startPos = vcCamera_GetMatrix(pProgramState->pCamera).axis.t.toVector3();
        pProgramState->zoomPath.endPos = pProgramState->worldMousePos;
        pProgramState->zoomPath.progress = 0.0;
      }
    }

    if (isRightClicked)
      pProgramState->currentMeasurePoint = pProgramState->worldMousePos;

    if (isHovered)
    {
      if (io.MouseWheel > 0)
        pProgramState->settings.camera.moveSpeed *= 1.1f;
      if (io.MouseWheel < 0)
        pProgramState->settings.camera.moveSpeed /= 1.1f;

      pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
    }
  }

  if (pProgramState->zoomPath.isZooming)
    vcCamera_TravelZoomPath(pProgramState->pCamera, &pProgramState->settings.camera, pProgramState, pProgramState->deltaTime);

  if(pProgramState->settings.camera.moveMode == vcCMM_Orbit)
    vcCamera_Apply(pProgramState->pCamera, &pProgramState->settings.camera, rotationOffset, moveOffset, pProgramState->deltaTime, isOrbitActive, pProgramState->orbitPos, pProgramState->storedDeltaAngle, speedModifier);
  else
    vcCamera_Apply(pProgramState->pCamera, &pProgramState->settings.camera, rotationOffset, moveOffset, pProgramState->deltaTime, speedModifier);

  pProgramState->camMatrix = vcCamera_GetMatrix(pProgramState->pCamera);
}

void vcRenderSceneWindow(vcState *pProgramState)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImVec2 windowPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y);

  if (size.x < 1 || size.y < 1)
    return;

  if (pProgramState->sceneResolution.x != size.x || pProgramState->sceneResolution.y != size.y) //Resize buffers
  {
    vcRender_ResizeScene(pProgramState->pRenderContext, (uint32_t)size.x, (uint32_t)size.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pProgramState->defaultFramebuffer);
  }

  vcRenderData renderData = {};
  renderData.cameraMatrix = pProgramState->camMatrix;
  renderData.srid = pProgramState->currentSRID;
  renderData.models.Init(32);

  ImGuiIO &io = ImGui::GetIO();
  renderData.mouse.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.y = (uint32_t)(io.MousePos.y - windowPos.y);

  for (size_t i = 0; i < vcModelList.length; ++i)
  {
    renderData.models.PushBack(&vcModelList[i]);
  }

  vcTexture *pTexture = vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->defaultFramebuffer);

  renderData.models.Deinit();

  ImGui::Image((ImTextureID)((size_t)pTexture->id), size, ImVec2(0, 0), ImVec2(1, -1));

  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x - 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("Geographic Information", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      if (pProgramState->currentSRID != 0)
        ImGui::Text("SRID: %d", pProgramState->currentSRID);
      else
        ImGui::Text("Not Geolocated");

      if (pProgramState->settings.showFPS)
      {
        ImGui::Separator();
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
      }

      ImGui::Separator();
      if (ImGui::IsMousePosValid())
      {
        ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        ImGui::Text("Mouse World Pos (x/y/z): (%f,%f,%f)", renderData.worldMousePos.x, renderData.worldMousePos.y, renderData.worldMousePos.z);

        udDouble3 mousePointInLatLong;
        pProgramState->worldMousePos = renderData.worldMousePos;

        if (pProgramState->worldMousePos != udDouble3::zero())
          vcGIS_LocalToLatLong(pProgramState->currentSRID, renderData.worldMousePos, &mousePointInLatLong);
        else
          mousePointInLatLong = udDouble3::zero();

        if(pProgramState->currentSRID != 0)
          ImGui::Text("Mouse World Pos (L/L): (%f,%f)", mousePointInLatLong.x, mousePointInLatLong.y);

        ImGui::Text("Selected Pos (x/y/z): (%f,%f,%f)", pProgramState->currentMeasurePoint.x, pProgramState->currentMeasurePoint.y, pProgramState->currentMeasurePoint.z);
      }
      else
      {
        ImGui::Text("Mouse Position: <invalid>");
      }
    }

    ImGui::End();
  }

  // On Screen Camera Settings
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.3f);
    if (ImGui::Begin("Camera Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize))
    {
      udDouble3 cameraPosition = vcCamera_GetMatrix(pProgramState->pCamera).axis.t.toVector3();
      if(ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &cameraPosition.x, 3))
        vcCamera_SetPosition(pProgramState->pCamera, cameraPosition);

      udDouble3 cameraRotation = vcCamera_GetMatrix(pProgramState->pCamera).extractYPR();
      if (ImGui::InputScalarN("Camera Rotation", ImGuiDataType_Double, &cameraRotation.x, 3))
        vcCamera_SetRotation(pProgramState->pCamera, cameraRotation);

      if (pProgramState->currentSRID != 0)
      {
        udDouble3 cameraLatLong;
        vcGIS_LocalToLatLong(pProgramState->currentSRID, pProgramState->camMatrix.axis.t.toVector3(), &cameraLatLong);
        ImGui::Text("Lat: %.7f, Long: %.7f, Alt: %.2fm", cameraLatLong.x, cameraLatLong.y, cameraLatLong.z);
      }
      ImGui::RadioButton("PlaneMode", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Plane);
      ImGui::SameLine();
      ImGui::RadioButton("HeliMode", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Helicopter);
      ImGui::SameLine();
      ImGui::RadioButton("OrbitMode", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Orbit);

      if (ImGui::SliderFloat("Move Speed", &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 2.f))
        pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, 0.f);
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

        double forward = 0;
        double right = 0;
        float vertical = 0;

        ImGui::PushID("oscUDSlider");

        if(ImGui::VSliderFloat("",ImVec2(40,100), &vertical, -1, 1, "U/D"))
          vertical = udClamp(vertical, -1.f, 1.f);

        ImGui::PopID();

        ImGui::NextColumn();

        ImGui::Button("Move Camera", ImVec2(100,100));
        if (ImGui::IsItemActive())
        {
          // Draw a line between the button and the mouse cursor
          ImDrawList* draw_list = ImGui::GetWindowDrawList();
          draw_list->PushClipRectFullScreen();
          draw_list->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
          draw_list->PopClipRect();

          ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);

          forward = -1.f * value_raw.y / vcSL_OSCPixelRatio;
          right = value_raw.x / vcSL_OSCPixelRatio;
        }

        vcCamera_Apply(pProgramState->pCamera, &pProgramState->settings.camera, udDouble3::zero(), udDouble3::create(right, forward, (double) vertical),pProgramState->deltaTime);
        ImGui::Columns(1);
      }

      ImGui::End();
    }

  }
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Logout"))
      {
        if(!vcLogout(pProgramState))
          ImGui::OpenPopup("Logout Error");
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
            vcModel_UnloadList(pProgramState);

            for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
            {
              char buffer[vcMaxPathLength];
              udSprintf(buffer, vcMaxPathLength, "%s/%s", pProgramState->settings.resourceBase, pProjectList->GetElement(i)->Get("models[%d]", j).AsString());
              vcModel_AddToList(pProgramState, buffer);
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

void vcRenderWindow(vcState *pProgramState)
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
    float menuHeight = (float)vcMainMenuGui(pProgramState);
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

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::Image((ImTextureID)((size_t)pProgramState->pWatermarkTexture->id), ImVec2(301, 161), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();
    ImGui::PopStyleColor();

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
        err = vdkContext_Connect(&pProgramState->pContext, pProgramState->serverURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vdkContext_Login(pProgramState->pContext, pProgramState->username, pProgramState->password);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vdkContext_GetLicense(pProgramState->pContext, vdkLT_Basic);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              //Context Login successful
              vcRender_CreateTerrain(pProgramState->pRenderContext, &pProgramState->settings);

              vcRender_SetVaultContext(pProgramState->pRenderContext, pProgramState->pContext);
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

      if (ImGui::Button("Local"))
      {
        udStrcpy(pProgramState->serverURL, vcMaxPathLength, "http://vau-ubu-pro-001.euclideon.local");
        udStrcpy(pProgramState->settings.resourceBase, vcMaxPathLength, "http://127.0.0.1:8080");
        udStrcpy(pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, "http://127.0.0.1:8123");
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
      vcRenderSceneWindow(pProgramState);
      vcHandleSceneInput(pProgramState);
    }

    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->settings.window.windowsOpen[vcdSceneExplorer], ImGuiWindowFlags_ResizeFromAnySide))
    {
      ImGui::InputText("Model Path", pProgramState->modelPath, vcMaxPathLength);
      if (ImGui::Button("Load Model!"))
        vcModel_AddToList(pProgramState, pProgramState->modelPath);

      if (!lastModelLoaded)
        ImGui::Text("Invalid File/Not Found...");

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
        { "Del", 35 }, // unload column
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

            pProgramState->numSelectedModels = 0;
          }

          if (modState & KMOD_SHIFT)
          {
            size_t startInd = udMin(i, pProgramState->prevSelectedModel);
            size_t endInd = udMax(i, pProgramState->prevSelectedModel);
            for (size_t j = startInd; j <= endInd; ++j)
            {
              vcModelList[j].modelSelected = true;
              pProgramState->numSelectedModels++;
            }
          }
          else
          {
            vcModelList[i].modelSelected = !vcModelList[i].modelSelected;
            pProgramState->numSelectedModels += vcModelList[i].modelSelected ? 1 : 0;
          }

          pProgramState->prevSelectedModel = i;
        }

        if (ImGui::BeginPopupContextItem(modelLabelID))
        {
          if (ImGui::Selectable("Properties", false))
          {
            pProgramState->popupTrigger[vcPopup_ModelProperties] = true;
            pProgramState->selectedModelProperties.index = i;
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          vcModel_MoveToModelProjection(pProgramState, &vcModelList[i]);

        ImVec2 textSize = ImGui::CalcTextSize(vcModelList[i].modelPath);
        if (ImGui::IsItemHovered() && (textSize.x >= headers[0].size))
          ImGui::SetTooltip("%s", vcModelList[i].modelPath);

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 2 - Visible
        char checkboxID[32] = "";
        udSprintf(checkboxID, UDARRAYSIZE(checkboxID), "ModelVisibleCheckbox%i", i);
        ImGui::PushID(checkboxID);
        if (ImGui::Checkbox("", &(vcModelList[i].modelVisible)) && vcModelList[i].modelSelected && pProgramState->numSelectedModels > 1)
        {
          for (size_t j = 0; j < vcModelList.length; ++j)
          {
            if (vcModelList[j].modelSelected)
              vcModelList[j].modelVisible = vcModelList[i].modelVisible;
          }
        }

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 3 - Unload Model
        char unloadModelID[32] = "";
        udSprintf(unloadModelID, UDARRAYSIZE(unloadModelID), "UnloadModelButton%i", i);
        ImGui::PushID(unloadModelID);
        if (ImGui::Button("X",ImVec2(20,20)))
        {
          if (pProgramState->numSelectedModels > 1 && vcModelList[i].modelSelected) // if multiple selected and removed
          {
            //unload selected models
            for (size_t j = 0; j < vcModelList.length; ++j)
            {
              if (vcModelList[j].modelSelected)
              {
                // unload model
                err = vdkModel_Unload(pProgramState->pContext, &(vcModelList[j].pVaultModel));
                if (err != vE_Success)
                  goto epilogue;

                vcModelList.RemoveAt(j);

                lastModelLoaded = true;
                j--;
              }
            }

            i = (pProgramState->numSelectedModels > i) ? 0 : (i - pProgramState->numSelectedModels);
          }
          else
          {
            // unload model
            err = vdkModel_Unload(pProgramState->pContext, &(vcModelList[i].pVaultModel));
            if (err != vE_Success)
              goto epilogue;

            vcModelList.RemoveAt(i);

            lastModelLoaded = true;
            i--;
          }
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

      ImGui::Checkbox("On Screen Controls", &pProgramState->onScreenControls);

      ImGui::Checkbox("Invert X-axis", &pProgramState->settings.camera.invertX);
      ImGui::Checkbox("Invert Y-axis", &pProgramState->settings.camera.invertY);

      if (ImGui::SliderFloat("Near Plane", &pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax, "%.3fm", 2.f))
      {
        pProgramState->settings.camera.nearPlane = udClamp(pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax);
        pProgramState->settings.camera.farPlane = udMin(pProgramState->settings.camera.farPlane, pProgramState->settings.camera.nearPlane * vcSL_CameraNearFarPlaneRatioMax);
      }

      if (ImGui::SliderFloat("Far Plane", &pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
      {
        pProgramState->settings.camera.farPlane = udClamp(pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax);
        pProgramState->settings.camera.nearPlane = udMax(pProgramState->settings.camera.nearPlane, pProgramState->settings.camera.farPlane / vcSL_CameraNearFarPlaneRatioMax);
      }

      //const char *pLensOptions = " Custom FoV\0 7mm\0 11mm\0 15mm\0 24mm\0 30mm\0 50mm\0 70mm\0 100mm\0";
      if (ImGui::Combo("Camera Lens (fov)", &pProgramState->settings.camera.lensIndex, vcCamera_GetLensNames(), vcLS_TotalLenses))
      {
        switch (pProgramState->settings.camera.lensIndex)
        {
        case vcLS_Custom:
          /*Custom FoV*/
          break;
        case vcLS_7mm:
          pProgramState->settings.camera.fieldOfView = vcLens7mm;
          break;
        case vcLS_11mm:
          pProgramState->settings.camera.fieldOfView = vcLens11mm;
          break;
        case vcLS_15mm:
          pProgramState->settings.camera.fieldOfView = vcLens15mm;
          break;
        case vcLS_24mm:
          pProgramState->settings.camera.fieldOfView = vcLens24mm;
          break;
        case vcLS_30mm:
          pProgramState->settings.camera.fieldOfView = vcLens30mm;
          break;
        case vcLS_50mm:
          pProgramState->settings.camera.fieldOfView = vcLens50mm;
          break;
        case vcLS_70mm:
          pProgramState->settings.camera.fieldOfView = vcLens70mm;
          break;
        case vcLS_100mm:
          pProgramState->settings.camera.fieldOfView = vcLens100mm;
          break;
        }
      }

      if (pProgramState->settings.camera.lensIndex == vcLS_Custom)
      {
        float fovDeg = UD_RAD2DEGf(pProgramState->settings.camera.fieldOfView);
        if (ImGui::SliderFloat("Field Of View", &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "%.0f Degrees"))
          pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
      }

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

        if(ImGui::SliderFloat("Transparency", &pProgramState->settings.maptiles.transparency, 0.f, 1.f, "%.3f"))
          pProgramState->settings.maptiles.transparency = udClamp(pProgramState->settings.maptiles.transparency, 0.f, 1.f);
      }
    }

    ImGui::EndDock();

    //Handle popups produced when vdkContext exists

    if (pProgramState->popupTrigger[vcPopup_ModelProperties])
    {
      ImGui::OpenPopup("Model Properties");

      pProgramState->selectedModelProperties.pMetadata = vcModelList[pProgramState->selectedModelProperties.index].pMetadata;

      const char *pWatermark = pProgramState->selectedModelProperties.pMetadata->Get("Watermark").AsString();
      if (pWatermark)
      {
        uint8_t *pImage = nullptr;
        size_t imageLen = 0;
        if (udBase64Decode(&pImage, &imageLen, pWatermark) == udR_Success)
        {
          int imageWidth, imageHeight, imageChannels;
          unsigned char *pImageData = stbi_load_from_memory(pImage, (int)imageLen, &imageWidth, &imageHeight, &imageChannels, 0);
          vcTexture_Create(&pProgramState->selectedModelProperties.pWatermarkTexture, imageWidth, imageHeight, vcTextureFormat_RGBA8, GL_NEAREST, false, pImageData);
          STBI_FREE(pImageData);
        }

        udFree(pImage);
      }

      ImGui::SetNextWindowSize(ImVec2(400, 600));

      pProgramState->popupTrigger[vcPopup_ModelProperties] = false;
    }

    if (ImGui::BeginPopupModal("Model Properties", NULL, ImGuiWindowFlags_ResizeFromAnySide))
    {
      ImGui::Text("File:");

      ImGui::TextWrapped("  %s", vcModelList[pProgramState->selectedModelProperties.index].modelPath);

      ImGui::Separator();

      if (pProgramState->selectedModelProperties.pMetadata == nullptr)
      {
        ImGui::Text("No model information found.");
      }
      else
      {
        for (size_t i = 0; i < pProgramState->selectedModelProperties.pMetadata->MemberCount(); ++i)
        {
          const char *pMemberName = pProgramState->selectedModelProperties.pMetadata->GetMemberName(i);

          if (udStrEqual(pMemberName, "ProjectionWKT") || udStrEqual(pMemberName, "Watermark"))
            continue;

          ImGui::TextWrapped("%s -> %s", pMemberName, pProgramState->selectedModelProperties.pMetadata->GetMember(i)->AsString(""));
        }

        ImGui::Separator();

        if (pProgramState->selectedModelProperties.pWatermarkTexture != nullptr)
        {
          ImGui::Text("Watermark");

          ImVec2 imageSize = ImVec2((float)pProgramState->selectedModelProperties.pWatermarkTexture->width, (float)pProgramState->selectedModelProperties.pWatermarkTexture->height);
          ImVec2 imageLimits = ImVec2(ImGui::GetContentRegionAvailWidth(), 100.f);

          if (imageSize.y > imageLimits.y)
          {
            imageSize.x *= imageLimits.y / imageSize.y;
            imageSize.y = imageLimits.y;
          }

          if (imageSize.x > imageLimits.x)
          {
            imageSize.y *= imageLimits.x / imageSize.x;
            imageSize.x = imageLimits.x;
          }

          ImGui::Image((ImTextureID)(size_t)pProgramState->selectedModelProperties.pWatermarkTexture->id, imageSize);
          ImGui::Separator();
        }
      }

      if (ImGui::Button("Close"))
      {
        if(pProgramState->selectedModelProperties.pWatermarkTexture != nullptr)
          vcTexture_Destroy(&pProgramState->selectedModelProperties.pWatermarkTexture);

        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

epilogue:
  //TODO: Cleanup
  return;
}

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath)
{
  if (pFilePath == nullptr)
    return;

  vcModel model = {};
  model.modelLoaded = true;
  model.modelVisible = true;
  model.modelSelected = false;
  model.pMetadata = udAllocType(udValue, 1, udAF_Zero);

  udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pFilePath);

  if(vdkModel_Load(pProgramState->pContext, &model.pVaultModel, pFilePath) == vE_Success)
  {
    const char *pMetadata;
    if (vdkModel_GetMetadata(pProgramState->pContext, model.pVaultModel, &pMetadata) == vE_Success)
      model.pMetadata->Parse(pMetadata);

    vcModel_MoveToModelProjection(pProgramState, &model);

    vcModelList.PushBack(model);
  }
}

bool vcModel_UnloadList(vcState *pProgramState)
{
  vdkError err;
  for (int i = 0; i < (int) vcModelList.length; i++)
  {
    vdkModel *pVaultModel;
    pVaultModel = vcModelList[i].pVaultModel;
    err = vdkModel_Unload(pProgramState->pContext, &pVaultModel);
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

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  double midPoint[3];
  vdkModel_GetModelCenter(pProgramState->pContext, pModel->pVaultModel, midPoint);
  vcCamera_SetPosition(pProgramState->pCamera, udDouble3::create(midPoint[0], midPoint[1], midPoint[2]));

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

void vcSettings_LoadSettings(vcState *pProgramState, bool forceDefaults)
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


bool vcLogout(vcState *pProgramState)
{
  bool success = true;

  success &= vcModel_UnloadList(pProgramState);
  success &= (vcRender_DestroyTerrain(pProgramState->pRenderContext) == udR_Success);

  pProgramState->currentSRID = 0;

  success = success && vdkContext_Logout(pProgramState->pContext) == vE_Success;
  pProgramState->hasContext = !success;

  return success;
}
