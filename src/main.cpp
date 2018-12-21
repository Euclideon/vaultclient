#include "vcState.h"

#include "vcRender.h"
#include "vcGIS.h"
#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"
#include "legacy/vcUDP.h"

#include "vdkContext.h"
#include "vdkServerAPI.h"
#include "vdkConfig.h"
#include "vdkPointCloud.h"

#include <chrono>

#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl.h"
#include "imgui_ex/imgui_impl_gl.h"
#include "imgui_ex/imgui_dock.h"
#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/ImGuizmo.h"
#include "imgui_ex/vcMenuButtons.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "vcConvert.h"
#include "vcVersion.h"
#include "vcModals.h"
#include "vcClassificationColours.h"

#include "udPlatform/udFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  include <crtdbg.h>
#  include <stdio.h>

# undef main
# define main ClientMain
int main(int argc, char **args);

int SDL_main(int argc, char **args)
{
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);

  int ret = main(argc, args);

  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 0)
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks found");

    // You've hit this because you've introduced a memory leak!
    // If you need help, define __MEMORY_DEBUG__ in the premake5.lua just before:
    // if _OPTIONS["force-vaultsdk"] then
    // This will emit filenames of what is leaking to assist in tracking down what's leaking.
    // Additionally, you can set _CrtSetBreakAlloc(<allocationNumber>);
    // back up where the initial checkpoint is made.
    __debugbreak();

    ret = 1;
  }

  return ret;
}
#endif

struct vcColumnHeader
{
  const char* pLabel;
  float size;
};

void vcRenderWindow(vcState *pProgramState);
int vcMainMenuGui(vcState *pProgramState);

int64_t vcMain_GetCurrentTime(int fractionSec = 1) // This gives 1/fractionSec factions since epoch, 5=200ms, 10=100ms etc.
{
  return std::chrono::system_clock::now().time_since_epoch().count() * fractionSec / std::chrono::system_clock::period::den;
}

void vcMain_UpdateSessionInfo(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  vdkError response = vdkContext_KeepAlive(pProgramState->pVDKContext);

  if (response != vE_Success)
    pProgramState->forceLogout = true;
  else
    pProgramState->lastServerResponse = vcMain_GetCurrentTime();
}

void vcLogin(void *pProgramStatePtr)
{
  vdkError result;
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  result = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, "EuclideonClient", pProgramState->settings.loginInfo.username, pProgramState->password);
  if (result == vE_ConnectionFailure)
    pProgramState->pLoginErrorMessage = "Could not connect to server.";
  else if (result == vE_NotAllowed)
    pProgramState->pLoginErrorMessage = "Username or Password incorrect.";
  else if (result == vE_OutOfSync)
    pProgramState->pLoginErrorMessage = "Your clock doesn't match the remote server clock.";
  else if (result == vE_SecurityFailure)
    pProgramState->pLoginErrorMessage = "Could not open a secure channel to the server.";
  else if (result == vE_ServerFailure)
    pProgramState->pLoginErrorMessage = "Unable to negotiate with server, please confirm the server address";
  else if (result != vE_Success)
    pProgramState->pLoginErrorMessage = "Unknown error occurred, please try again later.";

  if (result != vE_Success)
    return;

  vcRender_SetVaultContext(pProgramState->pRenderContext, pProgramState->pVDKContext);

  const char *pProjData = nullptr;
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "dev/projects", nullptr, &pProjData) == vE_Success)
    pProgramState->projects.Parse(pProjData);
  vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pProjData);

  const char *pPackageData = nullptr;
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/packages/latest", "{ \"packagename\": \"EuclideonClient\", \"packagevariant\": \"Windows\" }", &pPackageData) == vE_Success)
  {
    pProgramState->packageInfo.Parse(pPackageData);
    if (pProgramState->packageInfo.Get("success").AsBool())
    {
      if (pProgramState->packageInfo.Get("package.versionnumber").AsInt() <= VCVERSION_BUILD_NUMBER || VCVERSION_BUILD_NUMBER == 0)
        pProgramState->packageInfo.Destroy();
      else
        vcModals_OpenModal(pProgramState, vcMT_NewVersionAvailable);
    }
  }
  vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pPackageData);

  // Update username
  {
    const char *pSessionRawData = nullptr;
    udJSON info;

    vdkError response = vdkServerAPI_Query(pProgramState->pVDKContext, "v1/session/info", nullptr, &pSessionRawData);
    if (response == vE_Success)
    {
      if (info.Parse(pSessionRawData) == udR_Success)
      {
        if (info.Get("success").AsBool() == true)
        {
          udStrcpy(pProgramState->username, udLengthOf(pProgramState->username), info.Get("user.realname").AsString("Guest"));
          pProgramState->lastServerResponse = vcMain_GetCurrentTime();
        }
        else
        {
          response = vE_NotAllowed;
        }
      }
      else
      {
        response = vE_Failure;
      }
    }

    vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pSessionRawData);
  }

  //Context Login successful
  memset(pProgramState->password, 0, sizeof(pProgramState->password));
  if (!pProgramState->settings.loginInfo.rememberServer)
    memset(pProgramState->settings.loginInfo.serverURL, 0, sizeof(pProgramState->settings.loginInfo.serverURL));

  if (!pProgramState->settings.loginInfo.rememberUsername)
    memset(pProgramState->settings.loginInfo.username, 0, sizeof(pProgramState->settings.loginInfo.username));

  pProgramState->pLoginErrorMessage = nullptr;
  pProgramState->hasContext = true;
}

void vcLogout(vcState *pProgramState)
{
  pProgramState->hasContext = false;
  pProgramState->forceLogout = false;

  pProgramState->numSelectedModels = 0;
  pProgramState->prevSelectedModel = 0;

  if (pProgramState->pVDKContext != nullptr)
  {
    pProgramState->modelPath[0] = '\0';
    vcModel_UnloadList(pProgramState);
    pProgramState->projects.Destroy();
    vcRender_ClearPoints(pProgramState->pRenderContext);

    memset(&pProgramState->gis, 0, sizeof(pProgramState->gis));
    vdkContext_Disconnect(&pProgramState->pVDKContext);

    vcModals_OpenModal(pProgramState, vcMT_LoggedOut);
  }
}

void vcMain_LoadSettings(vcState *pProgramState, bool forceDefaults)
{
  if (vcSettings_Load(&pProgramState->settings, forceDefaults))
  {
    vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
    if (pProgramState->settings.window.maximized)
      SDL_MaximizeWindow(pProgramState->pWindow);
    else
      SDL_RestoreWindow(pProgramState->pWindow);

    SDL_SetWindowPosition(pProgramState->pWindow, pProgramState->settings.window.xpos, pProgramState->settings.window.ypos);
    //SDL_SetWindowSize(pProgramState->pWindow, pProgramState->settings.window.width, pProgramState->settings.window.height);
#endif
    switch (pProgramState->settings.presentation.styleIndex)
    {
    case 0: ImGui::StyleColorsDark(); ++pProgramState->settings.presentation.styleIndex; break;
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    }
  }
  ImGui::CaptureDefaults();
}

int main(int argc, char **args)
{
#if UDPLATFORM_WINDOWS
  if (argc > 0)
  {
    udFilename currentPath(args[0]);
    char cPathBuffer[256];
    currentPath.ExtractFolder(cPathBuffer, (int)udLengthOf(cPathBuffer));
    SetCurrentDirectoryW(udOSString(cPathBuffer));
  }
#endif //UDPLATFORM_WINDOWS

  SDL_GLContext glcontext = NULL;
  uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  vcState programState = {};

  vcSettings_RegisterAssetFileHandler();
  udFile_RegisterHTTP();

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
  unsigned char *pIconData = nullptr;
  unsigned char *pEucWatermarkData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;
  double frametimeMS = 0.0;
  uint32_t sleepMS = 0;

  const float FontSize = 16.f;
  ImFontConfig fontCfg = ImFontConfig();
  const char *pFontPath = nullptr;

  bool continueLoading = false;
  const char *pNextLoad = nullptr;

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
  vcCamera_Create(&programState.pCamera);

  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

  programState.loadList.reserve(udMax(64, argc));
  programState.vcModelList.reserve(64);

  for (int i = 1; i < argc; ++i)
    programState.loadList.push_back(udStrdup(args[i]));

  vWorkerThread_StartThreads(&programState.pWorkerPool);
  vcConvert_Init(&programState);

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
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0)
    goto epilogue;
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  // Stop window from being minimized while fullscreened and focus is lost
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

  programState.pWindow = ImGui_ImplSDL2_CreateWindow(VCVERSION_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.sceneResolution.x, programState.sceneResolution.y, windowFlags);
  if (!programState.pWindow)
    goto epilogue;

  pIconData = stbi_load(vcSettings_GetAssetPath("assets/icons/EuclideonClientIcon.png"), &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

  pitch = iconWidth * iconBytesPerPixel;
  pitch = (pitch + 3) & ~3;

  rMask = 0xFF << 0;
  gMask = 0xFF << 8;
  bMask = 0xFF << 16;
  aMask = (iconBytesPerPixel == 4) ? (0xFF << 24) : 0;

  if (pIconData != nullptr)
    pIcon = SDL_CreateRGBSurfaceFrom(pIconData, iconWidth, iconHeight, iconBytesPerPixel * 8, pitch, rMask, gMask, bMask, aMask);
  if (pIcon != nullptr)
    SDL_SetWindowIcon(programState.pWindow, pIcon);

  SDL_free(pIcon);

  glcontext = SDL_GL_CreateContext(programState.pWindow);
  if (!glcontext)
    goto epilogue;

  if (!vcGLState_Init(programState.pWindow, &programState.pDefaultFramebuffer))
    goto epilogue;

  SDL_GL_SetSwapInterval(0); // disable v-sync

  ImGui::CreateContext();
  ImGui::GetIO().ConfigResizeWindowsFromEdges = true; // Fix for ImGuiWindowFlags_ResizeFromAnySide being removed
  vcMain_LoadSettings(&programState, false);

  // setup watermark for background
  pEucWatermarkData = stbi_load(vcSettings_GetAssetPath("assets/icons/EuclideonLogo.png"), &iconWidth, &iconHeight, &iconBytesPerPixel, 0); // reusing the variables for width etc
  vcTexture_Create(&programState.pCompanyLogo, iconWidth, iconHeight, pEucWatermarkData);

  if (!ImGuiGL_Init(programState.pWindow))
    goto epilogue;

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  if (vcRender_Init(&(programState.pRenderContext), &(programState.settings), programState.pCamera, programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  vcFramebuffer_Bind(programState.pDefaultFramebuffer);

  pFontPath = vcSettings_GetAssetPath("assets/fonts/NotoSansCJKjp-Regular.otf");
  ImGui::GetIO().Fonts->AddFontFromFileTTF(pFontPath, FontSize);

  fontCfg.MergeMode = true;

#if UD_RELEASE // Load all glyphs for supported languages
  static ImWchar characterRanges[] =
  {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
    0x0E00, 0x0E7F, // Thai
    0x2010, 0x205E, // Punctuations
    0x25A0, 0x25FF, // Geometric Shapes
    0x26A0, 0x26A1, // Exclamation in Triangle
    0x2DE0, 0x2DFF, // Cyrillic Extended-A
    0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
    0x3131, 0x3163, // Korean alphabets
    0x31F0, 0x31FF, // Katakana Phonetic Extensions
    0x4e00, 0x9FAF, // CJK Ideograms
    0xA640, 0xA69F, // Cyrillic Extended-B
    0xAC00, 0xD79D, // Korean characters
    0xFF00, 0xFFEF, // Half-width characters
    0
  };

  ImGui::GetIO().Fonts->AddFontFromFileTTF(pFontPath, FontSize, &fontCfg, characterRanges);
  ImGui::GetIO().Fonts->AddFontFromFileTTF(pFontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()); // Still need to load Japanese seperately
#else // Debug; Only load required Glyphs
  static ImWchar characterRanges[] =
  {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x2010, 0x205E, // Punctuations
    0x25A0, 0x25FF, // Geometric Shapes
    0x26A0, 0x26A1, // Exclamation in Triangle
    0
  };

  ImGui::GetIO().Fonts->AddFontFromFileTTF(pFontPath, FontSize, &fontCfg, characterRanges);
#endif

  // No longer need the udTempStr after this point.
  pFontPath = nullptr;

  SDL_EnableScreenSaver();

  while (!programState.programComplete)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (!ImGui_ImplSDL2_ProcessEvent(&event))
      {
        if (event.type == SDL_WINDOWEVENT)
        {
          if (event.window.event == SDL_WINDOWEVENT_RESIZED)
          {
            programState.settings.window.width = event.window.data1;
            programState.settings.window.height = event.window.data2;
            vcGLState_ResizeBackBuffer(event.window.data1, event.window.data2);
          }
          else if (event.window.event == SDL_WINDOWEVENT_MOVED)
          {
            if (!programState.settings.window.fullscreen)
            {
              programState.settings.window.xpos = event.window.data1;
              programState.settings.window.ypos = event.window.data2;
            }
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
        else if (event.type == SDL_MULTIGESTURE)
        {
          // TODO: pinch to zoom
        }
        else if (event.type == SDL_DROPFILE && programState.hasContext)
        {
          programState.loadList.push_back(udStrdup(event.drop.file));
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

    frametimeMS = 0.03333333; // 30 FPS cap
    if ((SDL_GetWindowFlags(programState.pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0 && programState.settings.presentation.limitFPSInBackground)
      frametimeMS = 0.250; // 4 FPS cap when not focused

    sleepMS = (uint32_t)udMax((frametimeMS - programState.deltaTime) * 1000.0, 0.0);
    udSleep(sleepMS);
    programState.deltaTime += sleepMS * 0.001; // adjust delta

    ImGuiGL_NewFrame(programState.pWindow);
    vcGizmo_BeginFrame();

    vcGLState_ResetState(true);
    vcRenderWindow(&programState);

    ImGui::Render();
    ImGuiGL_RenderDrawData(ImGui::GetDrawData());

    vcGLState_Present(programState.pWindow);

    if (ImGui::GetIO().WantSaveIniSettings)
      vcSettings_Save(&programState.settings);

    ImGui::GetIO().KeysDown[SDL_SCANCODE_BACKSPACE] = false;

    if (programState.hasContext)
    {
      // Load next file in the load list (if there is one and the user has a context)
      bool firstLoad = true;
      do
      {
        continueLoading = false;

        if (programState.loadList.size() > 0)
        {
          pNextLoad = programState.loadList[0];
          programState.loadList.erase(programState.loadList.begin()); // TODO: Proper Exception Handling

          if (pNextLoad != nullptr)
          {
            udFilename loadFile(pNextLoad);

            if (udStrEquali(loadFile.GetExt(), ".uds") || udStrEquali(loadFile.GetExt(), ".ssf") || udStrEquali(loadFile.GetExt(), ".udm") || udStrEquali(loadFile.GetExt(), ".udg"))
            {
              vcModel_AddToList(&programState, pNextLoad, firstLoad);
              continueLoading = true;
            }
            else if (udStrEquali(loadFile.GetExt(), ".udp"))
            {
              if (firstLoad)
                vcModel_UnloadList(&programState);

              vcUDP_Load(&programState, pNextLoad);
            }
            else
            {
              vcConvert_AddFile(&programState, pNextLoad);
            }

            udFree(pNextLoad);
          }
        }

        firstLoad = false;
      } while (continueLoading);

      // Ping the server every 30 seconds
      if (vcMain_GetCurrentTime() > programState.lastServerAttempt + 30)
      {
        programState.lastServerAttempt = vcMain_GetCurrentTime();
        vWorkerThread_AddTask(programState.pWorkerPool, vcMain_UpdateSessionInfo, &programState, false);
      }

      vWorkerThread_DoPostWork(programState.pWorkerPool);

      if (programState.forceLogout)
      {
        vcLogout(&programState);
        vcModals_OpenModal(&programState, vcMT_LoggedOut);
      }

      if (programState.firstRun)
      {
        ImGui::CaptureDefaults();
        programState.firstRun = false;
      }
    }
  }

  vcSettings_Save(&programState.settings);
  ImGui::ShutdownDock();
  ImGui::DestroyContext();

epilogue:
  for (size_t i = 0; i < 256; ++i)
    if (programState.settings.visualization.customClassificationColorLabels[i] != nullptr)
      udFree(programState.settings.visualization.customClassificationColorLabels[i]);
  vcGIS_ClearCache();
  udFree(programState.pReleaseNotes);
  programState.projects.Destroy();
  ImGuiGL_DestroyDeviceObjects();
  vcConvert_Deinit(&programState);
  vcCamera_Destroy(&programState.pCamera);
  vcTexture_Destroy(&programState.pCompanyLogo);
  vcTexture_Destroy(&programState.pUITexture);
  free(pIconData);
  free(pEucWatermarkData);
  for (size_t i = 0; i < programState.loadList.size(); i++)
    udFree(programState.loadList[i]);
  programState.loadList.~vector();
  vcModel_UnloadList(&programState);
  programState.vcModelList.~vector();
  vcRender_Destroy(&programState.pRenderContext);
  vcTexture_Destroy(&programState.tileModal.pServerIcon);

  vWorkerThread_Shutdown(&programState.pWorkerPool); // This needs to occur before logout
  vcLogout(&programState);

  vcGLState_Deinit();

  return 0;
}

void vcRenderSceneUI(vcState *pProgramState, const ImVec2 &windowPos, const ImVec2 &windowSize, udDouble3 *pCameraMoveOffset)
{
  ImGuiIO &io = ImGui::GetIO();
  float bottomLeftOffset = 0.f;

  if (pProgramState->settings.presentation.showProjectionInfo || pProgramState->settings.presentation.showAdvancedGIS)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("Geographic Information", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar))
    {
      if (pProgramState->settings.presentation.showProjectionInfo)
      {
        if (pProgramState->gis.SRID != 0 && pProgramState->gis.isProjected)
          ImGui::Text("%s (SRID: %d)", pProgramState->gis.zone.zoneName, pProgramState->gis.SRID);
        else if (pProgramState->gis.SRID == 0)
          ImGui::Text("Not Geolocated");
        else
          ImGui::Text("Unsupported SRID: %d", pProgramState->gis.SRID);

        ImGui::Separator();
        if (ImGui::IsMousePosValid())
        {
          if (pProgramState->pickingSuccess)
          {
            ImGui::Text("Mouse Point (Projected): %.2f, %.2f, %.2f", pProgramState->worldMousePos.x, pProgramState->worldMousePos.y, pProgramState->worldMousePos.z);

            if (pProgramState->gis.isProjected)
            {
              udDouble3 mousePointInLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->worldMousePos);
              ImGui::Text("Mouse Point (WGS84): %.6f, %.6f", mousePointInLatLong.x, mousePointInLatLong.y);
            }
          }
        }
      }

      if (pProgramState->settings.presentation.showAdvancedGIS)
      {
        int newSRID = pProgramState->gis.SRID;
        if (ImGui::InputInt("Override SRID", &newSRID) && vcGIS_AcceptableSRID((vcSRID)newSRID))
        {
          if (vcGIS_ChangeSpace(&pProgramState->gis, (vcSRID)newSRID, &pProgramState->pCamera->position))
          {
            vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone
            vcGIS_ClearCache();
          }
        }
      }
    }

    ImGui::End();
  }

  // On Screen Camera Settings
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_Always, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("Camera Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
      if (pProgramState->pUITexture == nullptr)
      {
        vcTexture_CreateFromFilename(&pProgramState->pUITexture, "asset://assets/textures/uiDark24.png");
      }
      else
      {
        // Basic Settings
        if (vcMenuBarButton(pProgramState->pUITexture, "Lock Altitude", "Space", vcMBBI_LockAltitude, vcMBBG_FirstItem, (pProgramState->settings.camera.moveMode == vcCMM_Helicopter)))
          pProgramState->settings.camera.moveMode = (pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter;

        if (vcMenuBarButton(pProgramState->pUITexture, "Show Camera Information", nullptr, vcMBBI_ShowCameraSettings, vcMBBG_SameGroup, pProgramState->settings.presentation.showCameraInfo))
          pProgramState->settings.presentation.showCameraInfo = !pProgramState->settings.presentation.showCameraInfo;

        if (vcMenuBarButton(pProgramState->pUITexture, "Show Projection Information", nullptr, vcMBBI_ShowGeospatialInfo, vcMBBG_SameGroup, pProgramState->settings.presentation.showProjectionInfo))
          pProgramState->settings.presentation.showProjectionInfo = !pProgramState->settings.presentation.showProjectionInfo;

        // Gizmo Settings
        if (vcMenuBarButton(pProgramState->pUITexture, "Gizmo Translate", "B", vcMBBI_Translate, vcMBBG_NewGroup, (pProgramState->gizmo.operation == vcGO_Translate)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_B])
          pProgramState->gizmo.operation = vcGO_Translate;

        if (vcMenuBarButton(pProgramState->pUITexture, "Gizmo Rotate", "N", vcMBBI_Rotate, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Rotate)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_N])
          pProgramState->gizmo.operation = vcGO_Rotate;

        if (vcMenuBarButton(pProgramState->pUITexture, "Gizmo Scale", "M", vcMBBI_Scale, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Scale)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_M])
          pProgramState->gizmo.operation = vcGO_Scale;

        if (vcMenuBarButton(pProgramState->pUITexture, "Gizmo Local Space", "C", vcMBBI_UseLocalSpace, vcMBBG_SameGroup, (pProgramState->gizmo.coordinateSystem == vcGCS_Local)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_C])
          pProgramState->gizmo.coordinateSystem = (pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;

      }

      if (pProgramState->settings.presentation.showCameraInfo)
      {
        ImGui::Separator();

        ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &pProgramState->pCamera->position.x, 3);

        pProgramState->pCamera->eulerRotation = UD_RAD2DEG(pProgramState->pCamera->eulerRotation);

        ImGui::InputScalarN("Camera Rotation", ImGuiDataType_Double, &pProgramState->pCamera->eulerRotation.x, 3);
        pProgramState->pCamera->eulerRotation = UD_DEG2RAD(pProgramState->pCamera->eulerRotation);

        if (ImGui::SliderFloat("Move Speed", &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
          pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, 0.f);

        if (pProgramState->gis.isProjected)
        {
          ImGui::Separator();

          udDouble3 cameraLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->pCamera->matrices.camera.axis.t.toVector3());
          ImGui::Text("Lat: %.7f, Long: %.7f, Alt: %.2fm", cameraLatLong.x, cameraLatLong.y, cameraLatLong.z);

          if (pProgramState->gis.zone.latLongBoundMin != pProgramState->gis.zone.latLongBoundMax)
          {
            udDouble2 &minBound = pProgramState->gis.zone.latLongBoundMin;
            udDouble2 &maxBound = pProgramState->gis.zone.latLongBoundMax;

            if (cameraLatLong.x < minBound.x || cameraLatLong.y < minBound.y || cameraLatLong.x > maxBound.x || cameraLatLong.y > maxBound.y)
              ImGui::TextColored(ImVec4(1, 0, 0, 1), "Camera is outside recommended limits of this GeoZone");
          }
        }
      }
    }

    ImGui::End();
  }

  // On Screen Controls Overlay
  if (pProgramState->onScreenControls)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

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

      if (ImGui::VSliderFloat("##oscUDSlider", ImVec2(40, 100), &vertical, -1, 1, "U/D"))
        vertical = udClamp(vertical, -1.f, 1.f);

      ImGui::NextColumn();

      ImGui::Button("Move Camera", ImVec2(100, 100));
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

      *pCameraMoveOffset += udDouble3::create(right, forward, (double)vertical);

      ImGui::Columns(1);

      bottomLeftOffset += ImGui::GetWindowWidth();
    }

    ImGui::End();
  }

  if (pProgramState->pSceneWatermark != nullptr) // Watermark
  {
    udInt2 sizei = udInt2::zero();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    vcTexture_GetSize(pProgramState->pSceneWatermark, &sizei.x, &sizei.y);
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2((float)sizei.x, (float)sizei.y));
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("ModelWatermark", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Image(pProgramState->pSceneWatermark, ImVec2((float)sizei.x, (float)sizei.y));
    ImGui::End();
    ImGui::PopStyleVar();
  }

  if (pProgramState->settings.maptiles.mapEnabled && pProgramState->gis.isProjected)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("MapCopyright", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Text("Map Data \xC2\xA9 OpenStreetMap contributors");
    ImGui::End();
  }
}

void vcRenderSceneWindow(vcState *pProgramState)
{
  //Rendering
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 windowSize = ImGui::GetContentRegionAvail();
  ImVec2 windowPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y);

  if (windowSize.x < 1 || windowSize.y < 1)
    return;

  vcRenderData renderData = {};
  renderData.models.Init(32);
  renderData.mouse.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.y = (uint32_t)(io.MousePos.y - windowPos.y);

  udDouble3 cameraMoveOffset = udDouble3::zero();

  if (pProgramState->sceneResolution.x != windowSize.x || pProgramState->sceneResolution.y != windowSize.y) //Resize buffers
  {
    pProgramState->sceneResolution = udUInt2::create((uint32_t)windowSize.x, (uint32_t)windowSize.y);
    vcRender_ResizeScene(pProgramState->pRenderContext, pProgramState->sceneResolution.x, pProgramState->sceneResolution.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  }

  // use some data from previous frame
  pProgramState->worldMousePos = pProgramState->previousWorldMousePos;
  pProgramState->pickingSuccess = pProgramState->previousPickingSuccess;
  if (pProgramState->cameraInput.isUsingAnchorPoint)
    renderData.pWorldAnchorPos = &pProgramState->cameraInput.worldAnchorPoint;

  vcRenderSceneUI(pProgramState, windowPos, windowSize, &cameraMoveOffset);

  ImVec2 uv0 = ImVec2(0, 0);
  ImVec2 uv1 = ImVec2(1, 1);
#if GRAPHICS_API_OPENGL
  uv1.y = -1;
#endif

  {
    // Actual rendering to this texture is deferred
    vcTexture *pSceneTexture = vcRender_GetSceneTexture(pProgramState->pRenderContext);
    ImGui::ImageButton(pSceneTexture, windowSize, uv0, uv1, 0);

    // Camera update has to be here because it depends on previous ImGui state
    vcCamera_HandleSceneInput(pProgramState, cameraMoveOffset, udFloat2::create(windowSize.x, windowSize.y), udFloat2::create((float)renderData.mouse.x, (float)renderData.mouse.y));

    if (pProgramState->numSelectedModels == 1)
    {
      vcGizmo_SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
      vcGizmo_SetDrawList();

      vcGizmo_Manipulate(pProgramState->pCamera, pProgramState->gizmo.operation, pProgramState->gizmo.coordinateSystem, pProgramState->vcModelList[pProgramState->prevSelectedModel]->pWorldMatrix, nullptr, vcGAC_AllUniform);
    }
  }

  renderData.deltaTime = pProgramState->deltaTime;
  renderData.pGISSpace = &pProgramState->gis;
  renderData.pCameraSettings = &pProgramState->settings.camera;

  for (size_t i = 0; i < pProgramState->vcModelList.size(); ++i)
    renderData.models.PushBack(pProgramState->vcModelList[i]);

  // Render scene to texture
  vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);
  renderData.models.Deinit();

  pProgramState->previousWorldMousePos = renderData.worldMousePos;
  pProgramState->previousPickingSuccess = renderData.pickingSuccess;
  pProgramState->pSceneWatermark = renderData.pWatermarkTexture;
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Logout"))
        vcLogout(pProgramState);

      if (ImGui::MenuItem("Restore Defaults", nullptr))
        vcMain_LoadSettings(pProgramState, true);

      if (ImGui::MenuItem("About"))
        vcModals_OpenModal(pProgramState, vcMT_About);

      if (ImGui::MenuItem("Release Notes"))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pProgramState->programComplete = true;
#endif

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows"))
    {
      ImGui::MenuItem("Scene", nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Scene]);
      ImGui::MenuItem("Scene Explorer", nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]);
      ImGui::MenuItem("Settings", nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Settings]);
      ImGui::MenuItem("Convert", nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Convert]);
      ImGui::Separator();
      ImGui::EndMenu();
    }

    udJSONArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (ImGui::BeginMenu("Projects", pProjectList != nullptr && pProjectList->length > 0))
    {
      if (ImGui::MenuItem("New Scene", nullptr, nullptr))
        vcModel_UnloadList(pProgramState);

      ImGui::Separator();

      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcModel_UnloadList(pProgramState);

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
            vcModel_AddToList(pProgramState, pProjectList->GetElement(i)->Get("models[%zu]", j).AsString());
        }
      }

      ImGui::EndMenu();
    }

    char endBarInfo[512] = {};

    if (pProgramState->loadList.size() > 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("(%zu Files Queued) / ", pProgramState->loadList.size()));

    if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), "Inactive / ");

    if (pProgramState->packageInfo.Get("success").AsBool())
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("Update Available [%s] / ", pProgramState->packageInfo.Get("package.versionstring").AsString()));

    if (pProgramState->settings.presentation.showDiagnosticInfo)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("FPS: %.3f (%.2fms) / ", 1.f / pProgramState->deltaTime, pProgramState->deltaTime * 1000.f));

    int64_t currentTime = vcMain_GetCurrentTime();

    for (int i = 0; i < vdkLT_Count; ++i)
    {
      vdkLicenseInfo info = {};
      if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, (vdkLicenseType)i, &info) == vE_Success)
      {
        if (info.queuePosition < 0 && (uint64_t)currentTime < info.expiresTimestamp)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (%" PRIu64 "secs) / ", i == vdkLT_Render ? "Render" : "Convert", (info.expiresTimestamp - currentTime)));
        else if (info.queuePosition < 0)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (expired) / ", i == vdkLT_Render ? "Render" : "Convert"));
        else
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (%" PRId64 " in Queue) / ", i == vdkLT_Render ? "Render" : "Convert", info.queuePosition));
      }
    }

    udStrcat(endBarInfo, udLengthOf(endBarInfo), pProgramState->username);

    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(endBarInfo).x - 25);
    ImGui::Text("%s", endBarInfo);

    // Connection status indicator
    {
      ImGui::SameLine(ImGui::GetContentRegionMax().x - 20);
      if (pProgramState->lastServerResponse + 30 > currentTime)
        ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "\xE2\x97\x8F");
      else if (pProgramState->lastServerResponse + 60 > currentTime)
        ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x97\x8F");
      else
        ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x97\x8F");
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Connection Status");
        ImGui::EndTooltip();
      }
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

bool vcMain_U32ColorPicker(const char *pLabel, uint32_t *pColor, ImGuiColorEditFlags flags)
{
  float colors[4];

  colors[0] = ((((*pColor) >> 16) & 0xFF) / 255.f); // Blue
  colors[1] = ((((*pColor) >> 8) & 0xFF) / 255.f); // Green
  colors[2] = ((((*pColor) >> 0) & 0xFF) / 255.f); // Red
  colors[3] = ((((*pColor) >> 24) & 0xFF) / 255.f); // Alpha

  if (ImGui::ColorEdit4(pLabel, colors, flags))
  {
    uint32_t val = 0;

    val |= ((int)(colors[0] * 255) << 16); // Blue
    val |= ((int)(colors[1] * 255) << 8); // Green
    val |= ((int)(colors[2] * 255) << 0); // Red
    val |= ((int)(colors[3] * 255) << 24); // Alpha

    *pColor = val;

    return true;
  }

  return false;
}

void vcMain_ShowLoadStatusIndicator(vcModelLoadStatus loadStatus, bool sameLine = true)
{
  const char *loadingChars[] = { "\xE2\x96\xB2", "\xE2\x96\xB6", "\xE2\x96\xBC", "\xE2\x97\x80" };
  int64_t currentLoadingChar = vcMain_GetCurrentTime(10);

  // Load Status (if any)
  if (loadStatus == vcMLS_Pending)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Yellow Exclamation in Triangle
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", "Pending");

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcMLS_Loading)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", loadingChars[currentLoadingChar % udLengthOf(loadingChars)]); // Yellow Spinning clock
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", "Loading");

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcMLS_Failed || loadStatus == vcMLS_OpenFailure)
  {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    if (ImGui::IsItemHovered())
    {
      if (loadStatus == vcMLS_OpenFailure)
        ImGui::SetTooltip("%s", "Could not open the model, perhaps it is missing or you don't have permission to access it.");
      else
        ImGui::SetTooltip("%s", "Failed to load model");
    }

    if (sameLine)
      ImGui::SameLine();
  }
}

void vcRenderWindow(vcState *pProgramState)
{
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  vcGLState_SetViewport(0, 0, pProgramState->settings.window.width, pProgramState->settings.window.height);
  vcFramebuffer_Clear(pProgramState->pDefaultFramebuffer, 0xFF000000);

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
    ImGui::SetNextWindowPos(ImVec2(size.x - 5, size.y - 5), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::Image(pProgramState->pCompanyLogo, ImVec2(301, 161), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();
    ImGui::PopStyleColor();

    if (udStrEqual(pProgramState->pLoginErrorMessage, "Pending"))
    {
      ImGui::SetNextWindowSize(ImVec2(500, 160));
      if (ImGui::Begin("Login##LoginWaiting", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        ImGui::Text("Checking with server...");
      ImGui::End();
    }
    else
    {
      ImGui::SetNextWindowSize(ImVec2(500, 160), ImGuiCond_Appearing);
      if (ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
      {
        if (pProgramState->pLoginErrorMessage != nullptr)
          ImGui::Text("%s", pProgramState->pLoginErrorMessage);

        bool tryLogin = false;

        // Server URL
        tryLogin |= ImGui::InputText("ServerURL", pProgramState->settings.loginInfo.serverURL, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        if (pProgramState->pLoginErrorMessage == nullptr && !pProgramState->settings.loginInfo.rememberServer)
          ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
        ImGui::SameLine();
        ImGui::Checkbox("Remember##rememberServerURL", &pProgramState->settings.loginInfo.rememberServer);

        // Username
        tryLogin |= ImGui::InputText("Username", pProgramState->settings.loginInfo.username, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        if (pProgramState->pLoginErrorMessage == nullptr && pProgramState->settings.loginInfo.rememberServer && !pProgramState->settings.loginInfo.rememberUsername)
          ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
        ImGui::SameLine();
        ImGui::Checkbox("Remember##rememberUsername", &pProgramState->settings.loginInfo.rememberUsername);

        // Password
        ImVec2 buttonSize;
        if (pProgramState->passFocus)
        {
          ImGui::Button("Show");
          ImGui::SameLine(0, 0);
          buttonSize = ImGui::GetItemRectSize();
        }
        if (ImGui::IsItemActive() && pProgramState->passFocus)
          tryLogin |= ImGui::InputText("Password", pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        else
          tryLogin |= ImGui::InputText("Password", pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

        if (pProgramState->passFocus && ImGui::IsMouseClicked(0))
        {
          ImVec2 minPos = ImGui::GetItemRectMin();
          ImVec2 maxPos = ImGui::GetItemRectMax();
          if (io.MouseClickedPos->x < minPos.x - buttonSize.x || io.MouseClickedPos->x > maxPos.x || io.MouseClickedPos->y < minPos.y || io.MouseClickedPos->y > maxPos.y)
            pProgramState->passFocus = false;
        }
        if (!pProgramState->passFocus && udStrlen(pProgramState->password) == 0)
          pProgramState->passFocus = true;

        if (pProgramState->pLoginErrorMessage == nullptr && pProgramState->settings.loginInfo.rememberServer && pProgramState->settings.loginInfo.rememberUsername)
          ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);

        if (pProgramState->pLoginErrorMessage == nullptr)
          pProgramState->pLoginErrorMessage = "Please enter your credentials...";

        if (ImGui::Button("Login!") || tryLogin)
        {
          pProgramState->pLoginErrorMessage = "Pending";
          vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLogin, pProgramState, false);
        }

        if (SDL_GetModState() & KMOD_CAPS)
        {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "Caps Lock is Enabled!");
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Advanced Connection Settings"))
        {
          if (ImGui::InputText("Proxy Address", pProgramState->settings.loginInfo.proxy, vcMaxPathLength))
            vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

          if (ImGui::Checkbox("Ignore Certificate Verification", &pProgramState->settings.loginInfo.ignoreCertificateVerification))
            vdkConfig_IgnoreCertificateVerification(pProgramState->settings.loginInfo.ignoreCertificateVerification);

          if (pProgramState->settings.loginInfo.ignoreCertificateVerification)
            ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "THIS IS A DANGEROUS SETTING, ONLY SET THIS ON REQUEST FROM YOUR SYSTEM ADMINISTRATOR");

          ImGui::TreePop();
        }
      }
      ImGui::End();
    }

    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::SetNextWindowPos(ImVec2(0, size.y), ImGuiCond_Always, ImVec2(0, 1));
    if (ImGui::Begin("LoginScreenPopups", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
    {
      if (ImGui::Button("Release Notes"))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

      ImGui::SameLine();
      if (ImGui::Button("About"))
        vcModals_OpenModal(pProgramState, vcMT_About);
    }
    ImGui::End();
    ImGui::PopStyleColor();
  }
  else
  {
    if (ImGui::BeginDock("Scene Explorer", &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]))
    {
      if (vcMenuBarButton(pProgramState->pUITexture, "Add UDS", "Ctrl+U", vcMBBI_AddPointCloud, vcMBBG_FirstItem) || (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeysDown[SDL_SCANCODE_U]))
        vcModals_OpenModal(pProgramState, vcMT_AddUDS);

      if (vcMenuBarButton(pProgramState->pUITexture, "Add Point of Interest", nullptr, vcMBBI_AddPointOfInterest, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, "Add Area of Interest", nullptr, vcMBBI_AddAreaOfInterest, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, "Add Lines", nullptr, vcMBBI_AddLines, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, "Add Folder", nullptr, vcMBBI_AddFolder, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, "Remove Selected", "Delete", vcMBBI_Remove, vcMBBG_NewGroup) || ImGui::GetIO().KeysDown[SDL_SCANCODE_DELETE])
      {
        if (pProgramState->numSelectedModels != 0) // Indented check for clarity
        {
          // if multiple selected and removed
          if (pProgramState->numSelectedModels > 1)
          {
            size_t removed = 0;

            for (size_t iter = 0; iter < pProgramState->vcModelList.size(); ++iter)
            {
              size_t index = iter - removed;

              if (pProgramState->vcModelList[index]->selected)
              {
                vcModel_RemoveFromList(pProgramState, index);
                ++removed;
              }
            }
          }
          else
          {
            vcModel_RemoveFromList(pProgramState, pProgramState->prevSelectedModel);
          }

          pProgramState->numSelectedModels = 0;
          pProgramState->prevSelectedModel = 0;
        }
      }

      // Tree view for the scene
      ImGui::Separator();

      for (size_t i = 0; i < pProgramState->vcModelList.size(); ++i)
      {
        vcMain_ShowLoadStatusIndicator((vcModelLoadStatus)pProgramState->vcModelList[i]->loadStatus);

        // Visibility
        ImGui::Checkbox(udTempStr("##ModelVisible%zu", i), &pProgramState->vcModelList[i]->visible);
        ImGui::SameLine();

        // The actual model
        if (ImGui::Selectable(pProgramState->vcModelList[i]->path, pProgramState->vcModelList[i]->selected))
        {
          if ((modState & KMOD_CTRL) == 0)
          {
            for (size_t j = 0; j < pProgramState->vcModelList.size(); ++j)
              pProgramState->vcModelList[j]->selected = false;

            pProgramState->numSelectedModels = 0;
          }

          if (modState & KMOD_SHIFT)
          {
            size_t startInd = udMin(i, pProgramState->prevSelectedModel);
            size_t endInd = udMax(i, pProgramState->prevSelectedModel);
            for (size_t j = startInd; j <= endInd; ++j)
            {
              pProgramState->vcModelList[j]->selected = true;
              pProgramState->numSelectedModels++;
            }
          }
          else
          {
            pProgramState->vcModelList[i]->selected = !pProgramState->vcModelList[i]->selected;
            pProgramState->numSelectedModels += (pProgramState->vcModelList[i]->selected ? 1 : 0);
          }

          pProgramState->prevSelectedModel = i;
        }

        if (ImGui::BeginPopupContextItem(udTempStr("ModelContextMenu_%zu", i)))
        {
          if (ImGui::Checkbox("Flip Y/Z Up", &pProgramState->vcModelList[i]->flipYZ)) //Technically this is a rotation around X actually...
            vcModel_UpdateMatrix(pProgramState, pProgramState->vcModelList[i]);

          ImGui::Separator();

          if (pProgramState->vcModelList[i]->pZone != nullptr && ImGui::Selectable("Use Projection"))
          {
            if (vcGIS_ChangeSpace(&pProgramState->gis, pProgramState->vcModelList[i]->pZone->srid, &pProgramState->pCamera->position))
              vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone
          }

          if (ImGui::Selectable("Move To"))
          {
            udDouble3 localSpaceCenter = vcModel_GetPivotPointWorldSpace(pProgramState->vcModelList[i]);

            // Transform the camera position. Don't do the entire matrix as it may lead to inaccuracy/de-normalised camera
            if (pProgramState->gis.isProjected && pProgramState->vcModelList[i]->pZone != nullptr && pProgramState->vcModelList[i]->pZone->srid != pProgramState->gis.SRID)
              localSpaceCenter = udGeoZone_TransformPoint(localSpaceCenter, *pProgramState->vcModelList[i]->pZone, pProgramState->gis.zone);

            pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
            pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
            pProgramState->cameraInput.worldAnchorPoint = localSpaceCenter;
            pProgramState->cameraInput.progress = 0.0;
          }

          ImGui::Separator();

          if (ImGui::Selectable("Properties"))
          {
            pProgramState->selectedModelProperties.index = i;
            vcModals_OpenModal(pProgramState, vcMT_ModelProperties);
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          vcModel_MoveToModelProjection(pProgramState, pProgramState->vcModelList[i]);

        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("%s", pProgramState->vcModelList[i]->path);

      }
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("Scene", &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
      vcRenderSceneWindow(pProgramState);
    ImGui::EndDock();

    if (ImGui::BeginDock("Convert", &pProgramState->settings.window.windowsOpen[vcDocks_Convert]))
      vcConvert_ShowUI(pProgramState);
    ImGui::EndDock();

    if (ImGui::BeginDock("Settings", &pProgramState->settings.window.windowsOpen[vcDocks_Settings]))
    {
      if (ImGui::CollapsingHeader("Appearance##Settings"))
      {
        int styleIndex = pProgramState->settings.presentation.styleIndex - 1;
        if (ImGui::Combo("Theme", &styleIndex, "Dark\0Light\0"))
        {
          pProgramState->settings.presentation.styleIndex = styleIndex + 1;
          switch (styleIndex)
          {
          case 0: ImGui::StyleColorsDark(); break;
          case 1: ImGui::StyleColorsLight(); break;
          }
        }

        // Checks so the casts below are safe
        UDCOMPILEASSERT(sizeof(pProgramState->settings.presentation.mouseAnchor) == sizeof(int), "MouseAnchor is no longer sizeof(int)");

        ImGui::Checkbox("Show Diagnostic Information", &pProgramState->settings.presentation.showDiagnosticInfo);
        ImGui::Checkbox("Show Advanced GIS Settings", &pProgramState->settings.presentation.showAdvancedGIS);
        ImGui::Checkbox("Limit FPS In Background", &pProgramState->settings.presentation.limitFPSInBackground);

        ImGui::Checkbox("Show Compass On Screen", &pProgramState->settings.presentation.showCompass);
        ImGui::Combo("Mouse Anchor Style", (int*)&pProgramState->settings.presentation.mouseAnchor, "None\0Orbit\0Compass\0");
        ImGui::Combo("Voxel Shape", &pProgramState->settings.presentation.pointMode, "Rectangles\0Cubes\0Points\0");
      }

      if (ImGui::CollapsingHeader("Input & Controls##Settings"))
      {
        ImGui::Checkbox("On Screen Controls", &pProgramState->onScreenControls);

        if (ImGui::Checkbox("Touch Friendly UI", &pProgramState->settings.window.touchscreenFriendly))
        {
          ImGuiStyle& style = ImGui::GetStyle();
          style.TouchExtraPadding = pProgramState->settings.window.touchscreenFriendly ? ImVec2(4, 4) : ImVec2();
        }

        ImGui::Checkbox("Invert X-axis", &pProgramState->settings.camera.invertX);
        ImGui::Checkbox("Invert Y-axis", &pProgramState->settings.camera.invertY);

        ImGui::Text("Mouse Pivot Bindings");
        const char *mouseModes[] = { "Tumble", "Orbit", "Pan" };
        const char *scrollwheelModes[] = { "Dolly", "Change Move Speed" };

        // Checks so the casts below are safe
        UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.cameraMouseBindings[0]) == sizeof(int), "Bindings is no longer sizeof(int)");
        UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.scrollWheelMode) == sizeof(int), "ScrollWheel is no longer sizeof(int)");

        ImGui::Combo("Left", (int*)&pProgramState->settings.camera.cameraMouseBindings[0], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo("Middle", (int*)&pProgramState->settings.camera.cameraMouseBindings[2], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo("Right", (int*)&pProgramState->settings.camera.cameraMouseBindings[1], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo("Scroll Wheel", (int*)&pProgramState->settings.camera.scrollWheelMode, scrollwheelModes, (int)udLengthOf(scrollwheelModes));
      }

      if (ImGui::CollapsingHeader("Viewport##Settings"))
      {
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
      }

      if (ImGui::CollapsingHeader("Maps & Elevation##Settings"))
      {
        ImGui::Checkbox("Map Tiles", &pProgramState->settings.maptiles.mapEnabled);

        if (pProgramState->settings.maptiles.mapEnabled)
        {
          ImGui::Checkbox("Mouse can lock to maps", &pProgramState->settings.maptiles.mouseInteracts);

          if (ImGui::Button("Tile Server",ImVec2(-1,0)))
            vcModals_OpenModal(pProgramState, vcMT_TileServer);

          ImGui::SliderFloat("Map Height", &pProgramState->settings.maptiles.mapHeight, -1000.f, 1000.f, "%.3fm", 2.f);

          const char* blendModes[] = { "Hybrid", "Overlay", "Underlay" };
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

          if (ImGui::SliderFloat("Transparency", &pProgramState->settings.maptiles.transparency, 0.f, 1.f, "%.3f"))
            pProgramState->settings.maptiles.transparency = udClamp(pProgramState->settings.maptiles.transparency, 0.f, 1.f);

          if (ImGui::Button("Set to Camera Height"))
            pProgramState->settings.maptiles.mapHeight = (float)pProgramState->pCamera->position.z;
        }
      }

      if (ImGui::CollapsingHeader("Visualization##Settings"))
      {
        const char *visualizationModes[] = { "Colour", "Intensity", "Classification" };
        ImGui::Combo("Display Mode", (int*)&pProgramState->settings.visualization.mode, visualizationModes, (int)udLengthOf(visualizationModes));

        if (pProgramState->settings.visualization.mode == vcVM_Intensity)
        {
          // Temporary until https://github.com/ocornut/imgui/issues/467 is resolved, then use commented out code below
          float temp[] = { (float)pProgramState->settings.visualization.minIntensity, (float)pProgramState->settings.visualization.maxIntensity };
          ImGui::SliderFloat("Min Intensity", &temp[0], 0.f, temp[1], "%.0f", 4.f);
          ImGui::SliderFloat("Max Intensity", &temp[1], temp[0], 65535.f, "%.0f", 4.f);
          pProgramState->settings.visualization.minIntensity = (int)temp[0];
          pProgramState->settings.visualization.maxIntensity = (int)temp[1];
        }

        if (pProgramState->settings.visualization.mode == vcVM_Classification)
        {
          ImGui::Checkbox("Show Custom Classification Color Table", &pProgramState->settings.visualization.useCustomClassificationColours);

          if (pProgramState->settings.visualization.useCustomClassificationColours)
          {
            ImGui::SameLine();
            if (ImGui::Button("Restore Defaults##RestoreClassificationColors"))
              memcpy(pProgramState->settings.visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pProgramState->settings.visualization.customClassificationColors));

            vcMain_U32ColorPicker("0. Never Classified", &pProgramState->settings.visualization.customClassificationColors[0], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("1. Unclassified", &pProgramState->settings.visualization.customClassificationColors[1], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("2. Ground", &pProgramState->settings.visualization.customClassificationColors[2], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("3. Low Vegetation", &pProgramState->settings.visualization.customClassificationColors[3], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("4. Medium Vegetation", &pProgramState->settings.visualization.customClassificationColors[4], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("5. High Vegetation", &pProgramState->settings.visualization.customClassificationColors[5], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("6. Building", &pProgramState->settings.visualization.customClassificationColors[6], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("7. Low Point / Noise", &pProgramState->settings.visualization.customClassificationColors[7], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("8. Key Point / Reserved", &pProgramState->settings.visualization.customClassificationColors[8], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("9. Water", &pProgramState->settings.visualization.customClassificationColors[9], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("10. Rail", &pProgramState->settings.visualization.customClassificationColors[10], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("11. Road Surface", &pProgramState->settings.visualization.customClassificationColors[11], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("12. Reserved", &pProgramState->settings.visualization.customClassificationColors[12], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("13. Wire Guard / Shield", &pProgramState->settings.visualization.customClassificationColors[13], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("14. Wire Conductor / Phase", &pProgramState->settings.visualization.customClassificationColors[14], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("15. Transmission Tower", &pProgramState->settings.visualization.customClassificationColors[15], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("16. Wire Structure Connector", &pProgramState->settings.visualization.customClassificationColors[16], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("17. Bridge Deck", &pProgramState->settings.visualization.customClassificationColors[17], ImGuiColorEditFlags_NoAlpha);
            vcMain_U32ColorPicker("18. High Noise", &pProgramState->settings.visualization.customClassificationColors[18], ImGuiColorEditFlags_NoAlpha);

            if (ImGui::TreeNode("19 - 63 Reserved"))
            {
              for (int i = 19; i < 64; ++i)
                vcMain_U32ColorPicker(udTempStr("%d. Reserved", i), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
              ImGui::TreePop();
            }

            if (ImGui::TreeNode("64 - 255 User definable"))
            {
              for (int i = 64; i <= 255; ++i)
              {
                char buttonID[12], inputID[3];
                if (pProgramState->settings.visualization.customClassificationColorLabels[i] == nullptr)
                  vcMain_U32ColorPicker(udTempStr("%d. User Defined", i), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                else
                  vcMain_U32ColorPicker(udTempStr("%d. %s", i, pProgramState->settings.visualization.customClassificationColorLabels[i]), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                udSprintf(buttonID, 12, "Rename##%d", i);
                udSprintf(inputID, 3, "##I%d", i);
                ImGui::SameLine();
                if (ImGui::Button(buttonID))
                {
                  pProgramState->renaming = i;
                  pProgramState->renameText[0] = '\0';
                }
                if (pProgramState->renaming == i)
                {
                  ImGui::InputText(inputID, pProgramState->renameText, 30, ImGuiInputTextFlags_AutoSelectAll);
                  ImGui::SameLine();
                  if (ImGui::Button("Set"))
                  {
                    if (pProgramState->settings.visualization.customClassificationColorLabels[i] != nullptr)
                      udFree(pProgramState->settings.visualization.customClassificationColorLabels[i]);
                    pProgramState->settings.visualization.customClassificationColorLabels[i] = udStrdup(pProgramState->renameText);
                    pProgramState->renaming = -1;
                  }
                }
              }
              ImGui::TreePop();
            }
          }
        }

        // Post visualization - Edge Highlighting
        ImGui::Checkbox("Enable Edge Highlighting", &pProgramState->settings.postVisualization.edgeOutlines.enable);
        if (pProgramState->settings.postVisualization.edgeOutlines.enable)
        {
          ImGui::SliderInt("Edge Highlighting Width", &pProgramState->settings.postVisualization.edgeOutlines.width, 1, 10);

          // TODO: Make this less awful. 0-100 would make more sense than 0.0001 to 0.001.
          ImGui::SliderFloat("Edge Highlighting Threshold", &pProgramState->settings.postVisualization.edgeOutlines.threshold, 0.001f, 10.0f, "%.3f");
          ImGui::ColorEdit4("Edge Highlighting Colour", &pProgramState->settings.postVisualization.edgeOutlines.colour.x);
        }

        // Post visualization - Colour by Height
        ImGui::Checkbox("Enable Colour by Height", &pProgramState->settings.postVisualization.colourByHeight.enable);
        if (pProgramState->settings.postVisualization.colourByHeight.enable)
        {
          ImGui::ColorEdit4("Colour by Height Start Colour", &pProgramState->settings.postVisualization.colourByHeight.minColour.x);
          ImGui::ColorEdit4("Colour by Height End Colour", &pProgramState->settings.postVisualization.colourByHeight.maxColour.x);

          // TODO: Set min/max to the bounds of the model? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Colour by Height Start Height", &pProgramState->settings.postVisualization.colourByHeight.startHeight, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Colour by Height End Height", &pProgramState->settings.postVisualization.colourByHeight.endHeight, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Colour by Depth
        ImGui::Checkbox("Enable Colour by Depth", &pProgramState->settings.postVisualization.colourByDepth.enable);
        if (pProgramState->settings.postVisualization.colourByDepth.enable)
        {
          ImGui::ColorEdit4("Colour by Depth Colour", &pProgramState->settings.postVisualization.colourByDepth.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Colour by Depth Start Depth", &pProgramState->settings.postVisualization.colourByDepth.startDepth, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Colour by Depth End Depth", &pProgramState->settings.postVisualization.colourByDepth.endDepth, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Contours
        ImGui::Checkbox("Enable Contours", &pProgramState->settings.postVisualization.contours.enable);
        if (pProgramState->settings.postVisualization.contours.enable)
        {
          ImGui::ColorEdit4("Contours Colour", &pProgramState->settings.postVisualization.contours.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Contours Distances", &pProgramState->settings.postVisualization.contours.distances, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Contours Band Height", &pProgramState->settings.postVisualization.contours.bandHeight, 0.f, 1000.f, "%.3f");
        }
      }
    }

    ImGui::EndDock();

    if (vcModals_IsOpening(pProgramState, vcMT_ModelProperties))
    {
      ImGui::OpenPopup("Model Properties");
      ImGui::SetNextWindowSize(ImVec2(400, 600));
    }

    if (ImGui::BeginPopupModal("Model Properties", NULL, ImGuiWindowFlags_HorizontalScrollbar))
    {
      pProgramState->selectedModelProperties.pMetadata = pProgramState->vcModelList[pProgramState->selectedModelProperties.index]->pMetadata;
      pProgramState->selectedModelProperties.pWatermarkTexture = pProgramState->vcModelList[pProgramState->selectedModelProperties.index]->pWatermark;

      ImGui::Text("File:");

      ImGui::TextWrapped("  %s", pProgramState->vcModelList[pProgramState->selectedModelProperties.index]->path);

      ImGui::Separator();

      if (pProgramState->selectedModelProperties.pMetadata == nullptr)
      {
        ImGui::Text("No model information found.");
      }
      else
      {
        vcImGuiValueTreeObject(pProgramState->selectedModelProperties.pMetadata);
        ImGui::Separator();

        if (pProgramState->selectedModelProperties.pWatermarkTexture != nullptr)
        {
          ImGui::Text("Watermark");

          udInt2 imageSizei;
          vcTexture_GetSize(pProgramState->selectedModelProperties.pWatermarkTexture, &imageSizei.x, &imageSizei.y);

          ImVec2 imageSize = ImVec2((float)imageSizei.x, (float)imageSizei.y);
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

          ImGui::Image((ImTextureID)(size_t)pProgramState->selectedModelProperties.pWatermarkTexture, imageSize);
          ImGui::Separator();
        }
      }

      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();

      ImGui::EndPopup();
    }
  }

  vcModals_DrawModals(pProgramState);
}
