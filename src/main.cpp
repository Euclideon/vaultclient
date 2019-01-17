#include "vcState.h"

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
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "vcConvert.h"
#include "vcVersion.h"
#include "vcGIS.h"
#include "vcClassificationColours.h"
#include "vcPOI.h"
#include "vcRender.h"
#include "vcWebFile.h"
#include "vcStrings.h"
#include "vcModals.h"

#include "vCore/vStringFormat.h"

#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"

#include "legacy/vcUDP.h"

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
    pProgramState->pLoginErrorMessage = pStrLoginConnectionError;
  else if (result == vE_NotAllowed)
    pProgramState->pLoginErrorMessage = pStrLoginAuthError;
  else if (result == vE_OutOfSync)
    pProgramState->pLoginErrorMessage = pStrLoginSyncError;
  else if (result == vE_SecurityFailure)
    pProgramState->pLoginErrorMessage = pStrLoginSecurityError;
  else if (result == vE_ServerFailure)
    pProgramState->pLoginErrorMessage = pStrLoginServerError;
  else if (result == vE_ProxyError)
    pProgramState->pLoginErrorMessage = pStrLoginProxyError;
  else if (result != vE_Success)
    pProgramState->pLoginErrorMessage = pStrLoginOtherError;

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
    vcScene_RemoveAll(pProgramState);
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
  vcWebFile_RegisterFileHandlers();

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

  programState.settings.hideIntervalSeconds = 3;
  programState.showUI = false;

  programState.loadList.reserve(udMax(64, argc));
  programState.sceneList.reserve(64);

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

  vcStrings_LoadStrings(&programState, "Strings.json");

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
            if (!programState.settings.window.presentationMode)
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
              vcModel_AddToList(&programState, nullptr, pNextLoad, firstLoad);
              continueLoading = true;
            }
            else if (udStrEquali(loadFile.GetExt(), ".udp"))
            {
              if (firstLoad)
                vcScene_RemoveAll(&programState);

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
  vcScene_RemoveAll(&programState);
  programState.sceneList.~vector();
  vcRender_Destroy(&programState.pRenderContext);
  vcTexture_Destroy(&programState.tileModal.pServerIcon);
  vcStrings_FreeStrings();
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

    if (ImGui::Begin(pStrGeographicInfo, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar))
    {
      if (pProgramState->settings.presentation.showProjectionInfo)
      {
        if (pProgramState->gis.SRID != 0 && pProgramState->gis.isProjected)
          ImGui::Text("%s (%s: %d)", pProgramState->gis.zone.zoneName, pStrSRID, pProgramState->gis.SRID);
        else if (pProgramState->gis.SRID == 0)
          ImGui::Text("%s", pStrNotGeolocated);
        else
          ImGui::Text("%s: %d", pStrUnsupportedSRID, pProgramState->gis.SRID);

        ImGui::Separator();
        if (ImGui::IsMousePosValid())
        {
          if (pProgramState->pickingSuccess)
          {
            ImGui::Text("%s: %.2f, %.2f, %.2f", pStrMousePointInfo, pProgramState->worldMousePos.x, pProgramState->worldMousePos.y, pProgramState->worldMousePos.z);

            if (pProgramState->gis.isProjected)
            {
              udDouble3 mousePointInLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->worldMousePos);
              ImGui::Text("%s: %.6f, %.6f", pStrMousePointWGS, mousePointInLatLong.x, mousePointInLatLong.y);
            }
          }
        }
      }

      if (pProgramState->settings.presentation.showAdvancedGIS)
      {
        int newSRID = pProgramState->gis.SRID;
        if (ImGui::InputInt(pStrOverrideSRID, &newSRID) && vcGIS_AcceptableSRID((vcSRID)newSRID))
        {
          if (vcGIS_ChangeSpace(&pProgramState->gis, (vcSRID)newSRID, &pProgramState->pCamera->position))
          {
            vcScene_UpdateItemToCurrentProjection(pProgramState, nullptr); // Update all models to new zone
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
    if (ImGui::Begin(pStrCameraSettings, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
      if (pProgramState->pUITexture == nullptr)
      {
        vcTexture_CreateFromFilename(&pProgramState->pUITexture, "asset://assets/textures/uiDark24.png");
      }
      else
      {
        // Basic Settings
        if (vcMenuBarButton(pProgramState->pUITexture, pStrLockAltitude, pStrLockAltKey, vcMBBI_LockAltitude, vcMBBG_FirstItem, (pProgramState->settings.camera.moveMode == vcCMM_Helicopter)))
          pProgramState->settings.camera.moveMode = (pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter;

        if (vcMenuBarButton(pProgramState->pUITexture, pStrCameraInfo, nullptr, vcMBBI_ShowCameraSettings, vcMBBG_SameGroup, pProgramState->settings.presentation.showCameraInfo))
          pProgramState->settings.presentation.showCameraInfo = !pProgramState->settings.presentation.showCameraInfo;

        if (vcMenuBarButton(pProgramState->pUITexture, pStrProjectionInfo, nullptr, vcMBBI_ShowGeospatialInfo, vcMBBG_SameGroup, pProgramState->settings.presentation.showProjectionInfo))
          pProgramState->settings.presentation.showProjectionInfo = !pProgramState->settings.presentation.showProjectionInfo;

        // Gizmo Settings
        if (vcMenuBarButton(pProgramState->pUITexture, pStrGizmoTranslate, pStrTranslateKey, vcMBBI_Translate, vcMBBG_NewGroup, (pProgramState->gizmo.operation == vcGO_Translate)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_B])
          pProgramState->gizmo.operation = vcGO_Translate;

        if (vcMenuBarButton(pProgramState->pUITexture, pStrGizmoRotate, pStrRotateKey, vcMBBI_Rotate, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Rotate)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_N])
          pProgramState->gizmo.operation = vcGO_Rotate;

        if (vcMenuBarButton(pProgramState->pUITexture, pStrGizmoScale, pStrScaleKey, vcMBBI_Scale, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Scale)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_M])
          pProgramState->gizmo.operation = vcGO_Scale;

        if (vcMenuBarButton(pProgramState->pUITexture, pStrGizmoLocalSpace, pStrLocalKey, vcMBBI_UseLocalSpace, vcMBBG_SameGroup, (pProgramState->gizmo.coordinateSystem == vcGCS_Local)) || ImGui::IsKeyPressed(SDL_SCANCODE_C, false))
          pProgramState->gizmo.coordinateSystem = (pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;

        // Fullscreen
        if (vcMenuBarButton(pProgramState->pUITexture, pStrFullscreen, pStrFullscreenKey, vcMBBI_FullScreen, vcMBBG_NewGroup, pProgramState->settings.window.presentationMode) || ImGui::IsKeyPressed(SDL_SCANCODE_F5, false))
        {
          pProgramState->settings.window.presentationMode = !pProgramState->settings.window.presentationMode;
          if (pProgramState->settings.window.presentationMode)
            SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
          else
            SDL_SetWindowFullscreen(pProgramState->pWindow, 0);

          if (pProgramState->settings.responsiveUI == vcPM_Responsive)
            pProgramState->lastEventTime = vcMain_GetCurrentTime();
        }
      }

      if (pProgramState->settings.presentation.showCameraInfo)
      {
        ImGui::Separator();

        ImGui::InputScalarN(pStrCameraPosition, ImGuiDataType_Double, &pProgramState->pCamera->position.x, 3);

        pProgramState->pCamera->eulerRotation = UD_RAD2DEG(pProgramState->pCamera->eulerRotation);

        ImGui::InputScalarN(pStrCameraRotation, ImGuiDataType_Double, &pProgramState->pCamera->eulerRotation.x, 3);
        pProgramState->pCamera->eulerRotation = UD_DEG2RAD(pProgramState->pCamera->eulerRotation);

        if (ImGui::SliderFloat(pStrMoveSpeed, &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
          pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, 0.f);

        if (pProgramState->gis.isProjected)
        {
          ImGui::Separator();

          udDouble3 cameraLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->pCamera->matrices.camera.axis.t.toVector3());

          ImGui::Text(pStrLatLongAlt, cameraLatLong.x, cameraLatLong.y, cameraLatLong.z);

          if (pProgramState->gis.zone.latLongBoundMin != pProgramState->gis.zone.latLongBoundMax)
          {
            udDouble2 &minBound = pProgramState->gis.zone.latLongBoundMin;
            udDouble2 &maxBound = pProgramState->gis.zone.latLongBoundMax;

            if (cameraLatLong.x < minBound.x || cameraLatLong.y < minBound.y || cameraLatLong.x > maxBound.x || cameraLatLong.y > maxBound.y)
              ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", pStrCameraOutOfBounds);
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

    if (ImGui::Begin(pStrOnScrnControls, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      ImGui::SetWindowSize(ImVec2(175, 150));
      ImGui::Text("%s", pStrControls);

      ImGui::Separator();


      ImGui::Columns(2, NULL, false);

      ImGui::SetColumnWidth(0, 50);

      double forward = 0;
      double right = 0;
      float vertical = 0;

      if (ImGui::VSliderFloat("##oscUDSlider", ImVec2(40, 100), &vertical, -1, 1, "U/D"))
        vertical = udClamp(vertical, -1.f, 1.f);

      ImGui::NextColumn();

      ImGui::Button(pStrMoveCamera, ImVec2(100, 100));
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

    if (ImGui::Begin(pStrModelWatermark, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Image(pProgramState->pSceneWatermark, ImVec2((float)sizei.x, (float)sizei.y));
    ImGui::End();
    ImGui::PopStyleVar();
  }

  if (pProgramState->settings.maptiles.mapEnabled && pProgramState->gis.isProjected)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin(pStrMapCopyright, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Text("%s", pStrMapData);
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
  renderData.fences.Init(32);
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

      vcGizmo_Manipulate(pProgramState->pCamera, pProgramState->gizmo.operation, pProgramState->gizmo.coordinateSystem, &pProgramState->sceneList[pProgramState->prevSelectedModel]->sceneMatrix, nullptr, vcGAC_AllUniform, pProgramState->sceneList[pProgramState->prevSelectedModel]->pivot);
    }
  }

  renderData.deltaTime = pProgramState->deltaTime;
  renderData.pGISSpace = &pProgramState->gis;
  renderData.pCameraSettings = &pProgramState->settings.camera;

  for (size_t i = 0; i < pProgramState->sceneList.size(); ++i)
  {
    if (pProgramState->sceneList[i]->type == vcSOT_PointCloud)
    {
      renderData.models.PushBack((vcModel*)pProgramState->sceneList[i]);
    }
    else if (pProgramState->sceneList[i]->type == vcSOT_PointOfInterest)
    {
      vcPOI* pPOI = (vcPOI*)pProgramState->sceneList[i];

      if (pPOI->pFence != nullptr)
        renderData.fences.PushBack(pPOI->pFence);
    }
  }

  // Render scene to texture
  vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);
  renderData.models.Deinit();
  renderData.fences.Deinit();

  pProgramState->previousWorldMousePos = renderData.worldMousePos;
  pProgramState->previousPickingSuccess = renderData.pickingSuccess;
  pProgramState->pSceneWatermark = renderData.pWatermarkTexture;
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu(pStrMenuSystem))
    {
      if (ImGui::MenuItem(pStrMenuLogout))
        vcLogout(pProgramState);

      if (ImGui::MenuItem(pStrMenuRestoreDefaults, nullptr))
        vcMain_LoadSettings(pProgramState, true);

      if (ImGui::MenuItem(pStrMenuAbout))
        vcModals_OpenModal(pProgramState, vcMT_About);

      if (ImGui::MenuItem(pStrMenuReleaseNotes))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
      if (ImGui::MenuItem(pStrMenuQuit, "Alt+F4"))
        pProgramState->programComplete = true;
#endif

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(pStrWindows))
    {
      ImGui::MenuItem(pStrScene, nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Scene]);
      ImGui::MenuItem(pStrSceneExplorer, nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]);
      ImGui::MenuItem(pStrSettings, nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Settings]);
      ImGui::MenuItem(pStrConvert, nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Convert]);
      ImGui::Separator();
      ImGui::EndMenu();
    }

    udJSONArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (ImGui::BeginMenu(pStrProjects, pProjectList != nullptr && pProjectList->length > 0))
    {
      if (ImGui::MenuItem(pStrNewScene, nullptr, nullptr))
        vcScene_RemoveAll(pProgramState);

      ImGui::Separator();

      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcScene_RemoveAll(pProgramState);

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
            vcModel_AddToList(pProgramState, nullptr, pProjectList->GetElement(i)->Get("models[%zu]", j).AsString());
        }
      }

      ImGui::EndMenu();
    }

    char endBarInfo[512] = {};

    if (pProgramState->loadList.size() > 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("(%zu %s) / ", pProgramState->loadList.size(), pStrEndBarFiles));

    if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), pStrInactiveSlash);

    if (pProgramState->packageInfo.Get("success").AsBool())
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s [%s] / ", pStrUpdateAvailable, pProgramState->packageInfo.Get("package.versionstring").AsString()));

    if (pProgramState->settings.presentation.showDiagnosticInfo)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s: %.3f (%.2fms) / ", pStrFPS, 1.f / pProgramState->deltaTime, pProgramState->deltaTime * 1000.f));

    int64_t currentTime = vcMain_GetCurrentTime();

    for (int i = 0; i < vdkLT_Count; ++i)
    {
      vdkLicenseInfo info = {};
      if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, (vdkLicenseType)i, &info) == vE_Success)
      {
        if (info.queuePosition < 0 && (uint64_t)currentTime < info.expiresTimestamp)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s (%" PRIu64 "%s) / ", i == vdkLT_Render ? pStrRender : pStrConvert, pStrLicense, (info.expiresTimestamp - currentTime), pStrSecs));
        else if (info.queuePosition < 0)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s / ", i == vdkLT_Render ? pStrRender : pStrConvert, pStrLicenseExpired));
        else
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s (%" PRId64 " %s) / ", i == vdkLT_Render ? pStrRender : pStrConvert, pStrLicense, info.queuePosition, pStrLicenseQueued));
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
        ImGui::Text("%s", pStrConnectionStatus);
        ImGui::EndTooltip();
      }
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcMain_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, bool sameLine = true)
{
  const char *loadingChars[] = { "\xE2\x96\xB2", "\xE2\x96\xB6", "\xE2\x96\xBC", "\xE2\x97\x80" };
  int64_t currentLoadingChar = vcMain_GetCurrentTime(10);

  // Load Status (if any)
  if (loadStatus == vcSLS_Pending)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Yellow Exclamation in Triangle
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", pStrPending);

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Loading)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", loadingChars[currentLoadingChar % udLengthOf(loadingChars)]); // Yellow Spinning clock
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", pStrLoading);

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Failed || loadStatus == vcSLS_OpenFailure)
  {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    if (ImGui::IsItemHovered())
    {
      if (loadStatus == vcSLS_OpenFailure)
        ImGui::SetTooltip("%s", pStrModelOpenFailure);
      else
        ImGui::SetTooltip("%s", pStrModelLoadFailure);
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
  ImGuiIO& io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

  if (pProgramState->settings.responsiveUI == vcPM_Responsive)
  {
    if (io.MouseDelta.x != 0.0 || io.MouseDelta.y != 0.0)
    {
      pProgramState->lastEventTime = vcMain_GetCurrentTime();
      pProgramState->showUI = true;
    }
    else if ((vcMain_GetCurrentTime() - pProgramState->lastEventTime) > pProgramState->settings.hideIntervalSeconds)
    {
      pProgramState->showUI = false;
    }
  }

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
    ImGui::Begin(pStrWatermark, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::Image(pProgramState->pCompanyLogo, ImVec2(301, 161), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();
    ImGui::PopStyleColor();

    if (udStrEqual(pProgramState->pLoginErrorMessage, pStrPending))
    {
      ImGui::SetNextWindowSize(ImVec2(500, 160));
      if (ImGui::Begin(pStrLoginWaiting, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
      {
        vcMain_ShowLoadStatusIndicator(vcSLS_Loading);
        ImGui::Text("%s", pStrChecking);
      }
      ImGui::End();
    }
    else
    {
      ImGui::SetNextWindowSize(ImVec2(500, 160), ImGuiCond_Appearing);
      if (ImGui::Begin(pStrLogin, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
      {
        if (pProgramState->pLoginErrorMessage != nullptr)
          ImGui::Text("%s", pProgramState->pLoginErrorMessage);

        bool tryLogin = false;

        // Server URL
        tryLogin |= ImGui::InputText(pStrServerURL, pProgramState->settings.loginInfo.serverURL, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        if (pProgramState->pLoginErrorMessage == nullptr && !pProgramState->settings.loginInfo.rememberServer)
          ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
        ImGui::SameLine();
        ImGui::Checkbox(pStrRememberServer, &pProgramState->settings.loginInfo.rememberServer);

        // Username
        tryLogin |= ImGui::InputText(pStrUsername, pProgramState->settings.loginInfo.username, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        if (pProgramState->pLoginErrorMessage == nullptr && pProgramState->settings.loginInfo.rememberServer && !pProgramState->settings.loginInfo.rememberUsername)
          ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
        ImGui::SameLine();
        ImGui::Checkbox(pStrRememberUser, &pProgramState->settings.loginInfo.rememberUsername);

        // Password
        ImVec2 buttonSize;
        if (pProgramState->passFocus)
        {
          ImGui::Button(pStrShow);
          ImGui::SameLine(0, 0);
          buttonSize = ImGui::GetItemRectSize();
        }
        if (ImGui::IsItemActive() && pProgramState->passFocus)
          tryLogin |= ImGui::InputText(pStrPassword, pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
        else
          tryLogin |= ImGui::InputText(pStrPassword, pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

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
          pProgramState->pLoginErrorMessage = pStrCredentials;

        if (ImGui::Button(pStrLoginButton) || tryLogin)
        {
          pProgramState->pLoginErrorMessage = pStrPending;
          vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLogin, pProgramState, false);
        }

        if (SDL_GetModState() & KMOD_CAPS)
        {
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "%s", pStrCapsWarning);
        }

        ImGui::Separator();

        if (ImGui::TreeNode(pStrAdvancedSettings))
        {
          if (ImGui::InputText(pStrProxyAddress, pProgramState->settings.loginInfo.proxy, vcMaxPathLength))
            vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

          if (ImGui::Checkbox(pStrIgnoreCert, &pProgramState->settings.loginInfo.ignoreCertificateVerification))
            vdkConfig_IgnoreCertificateVerification(pProgramState->settings.loginInfo.ignoreCertificateVerification);

          if (pProgramState->settings.loginInfo.ignoreCertificateVerification)
            ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "%s", pStrIgnoreCertWarning);

          ImGui::TreePop();
        }
      }
      ImGui::End();
    }

    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::SetNextWindowPos(ImVec2(0, size.y), ImGuiCond_Always, ImVec2(0, 1));
    if (ImGui::Begin(pStrLoginScreenPopups, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
    {
      if (ImGui::Button(pStrReleaseNotes))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

      ImGui::SameLine();
      if (ImGui::Button(pStrAbout))
        vcModals_OpenModal(pProgramState, vcMT_About);
    }
    ImGui::End();
    ImGui::PopStyleColor();
  }
  else
  {
    if (ImGui::BeginDock(pStrSceneExplorer, &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]))
    {
      if (vcMenuBarButton(pProgramState->pUITexture, pStrAddUDS, "Ctrl+U", vcMBBI_AddPointCloud, vcMBBG_FirstItem) || (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeysDown[SDL_SCANCODE_U]))
        vcModals_OpenModal(pProgramState, vcMT_AddUDS);

      if (vcMenuBarButton(pProgramState->pUITexture, pStrAddPOI, nullptr, vcMBBI_AddPointOfInterest, vcMBBG_SameGroup))
        vcPOI_AddToList(pProgramState, "Point of Interest", 0xFFFFFFFF, 14, udDouble3::zero(), 0);

      if (vcMenuBarButton(pProgramState->pUITexture, pStrAddAOI, nullptr, vcMBBI_AddAreaOfInterest, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, pStrAddLines, nullptr, vcMBBI_AddLines, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, pStrAddFolder, nullptr, vcMBBI_AddFolder, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_NotYetImplemented);

      if (vcMenuBarButton(pProgramState->pUITexture, pStrRemove, pStrDeleteKey, vcMBBI_Remove, vcMBBG_NewGroup) || ImGui::GetIO().KeysDown[SDL_SCANCODE_DELETE])
      {
        if (pProgramState->numSelectedModels != 0) // Indented check for clarity
        {
          // if multiple selected and removed
          if (pProgramState->numSelectedModels > 1)
          {
            size_t removed = 0;

            for (size_t iter = 0; iter < pProgramState->sceneList.size(); ++iter)
            {
              size_t index = iter - removed;

              if (pProgramState->sceneList[index]->selected)
              {
                vcScene_RemoveItem(pProgramState, index);
                ++removed;
              }
            }
          }
          else
          {
            vcScene_RemoveItem(pProgramState, pProgramState->prevSelectedModel);
          }

          pProgramState->numSelectedModels = 0;
          pProgramState->prevSelectedModel = 0;
        }
      }

      // Tree view for the scene
      ImGui::Separator();

      static size_t draggedIndex = 0;
      static size_t drawSeperatorBefore = SIZE_MAX;
      size_t hovered = SIZE_MAX;

      size_t i = 0;

      if (!ImGui::IsMouseDragging())
        drawSeperatorBefore = SIZE_MAX;

      for (i = 0; i < pProgramState->sceneList.size(); ++i)
      {
        // This block is also after the loop
        if (drawSeperatorBefore == i)
        {
          ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 0.f, 1.f)); // RGBA
          ImGui::Separator();
          ImGui::PopStyleColor();
        }

        // Visibility
        ImGui::Checkbox(udTempStr("##SXIVisible%zu", i), &pProgramState->sceneList[i]->visible);
        ImGui::SameLine();

        if (pProgramState->sceneList[i]->pImGuiFunc != nullptr)
        {
          if (ImGui::ArrowButton(udTempStr("##SXIExpanded%zu", i), pProgramState->sceneList[i]->expanded ? ImGuiDir_Down : ImGuiDir_Right))
            pProgramState->sceneList[i]->expanded = !pProgramState->sceneList[i]->expanded;
          ImGui::SameLine();
        }

        vcMain_ShowLoadStatusIndicator((vcSceneLoadStatus)pProgramState->sceneList[i]->loadStatus);

        // The actual model
        if (ImGui::Selectable(pProgramState->sceneList[i]->pName, pProgramState->sceneList[i]->selected))
        {
          if ((modState & KMOD_CTRL) == 0)
          {
            for (size_t j = 0; j < pProgramState->sceneList.size(); ++j)
              pProgramState->sceneList[j]->selected = false;

            pProgramState->numSelectedModels = 0;
          }

          if (modState & KMOD_SHIFT)
          {
            size_t startInd = udMin(i, pProgramState->prevSelectedModel);
            size_t endInd = udMax(i, pProgramState->prevSelectedModel);
            for (size_t j = startInd; j <= endInd; ++j)
            {
              pProgramState->sceneList[j]->selected = true;
              pProgramState->numSelectedModels++;
            }
          }
          else
          {
            pProgramState->sceneList[i]->selected = !pProgramState->sceneList[i]->selected;
            pProgramState->numSelectedModels += (pProgramState->sceneList[i]->selected ? 1 : 0);
          }

          pProgramState->prevSelectedModel = i;
        }

        if (ImGui::BeginDragDropSource())
        {
          draggedIndex = i;
          ImGui::SetDragDropPayload("SceneItem", &pProgramState->sceneList[i], sizeof(vcSceneItem*), ImGuiCond_Once);
          ImGui::Text("%s", pProgramState->sceneList[i]->pName);
          ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
          const ImGuiPayload *pPayload;

          pPayload = ImGui::AcceptDragDropPayload("SceneItem", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

          if (pPayload != nullptr)
          {
            ImVec2 minPos = ImGui::GetItemRectMin();
            ImVec2 maxPos = ImGui::GetItemRectMax();
            ImVec2 mousePos = ImGui::GetMousePos();

            if (udAbs(mousePos.y - minPos.y) < udAbs(mousePos.y - maxPos.y))
              drawSeperatorBefore = i;
            else
              drawSeperatorBefore = i + 1;

            hovered = i;
          }

          pPayload = ImGui::AcceptDragDropPayload("SceneItem", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
          if (pPayload != nullptr)
          {
            if (draggedIndex < drawSeperatorBefore)
              --drawSeperatorBefore;

            pProgramState->sceneList.erase(pProgramState->sceneList.begin() + draggedIndex);
            pProgramState->sceneList.insert(pProgramState->sceneList.begin() + drawSeperatorBefore, *(vcSceneItem**)pPayload->Data);

            drawSeperatorBefore = SIZE_MAX;
          }

          ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem(udTempStr("ModelContextMenu_%zu", i)))
        {
          if (pProgramState->sceneList[i]->pZone != nullptr && ImGui::Selectable(pStrUseProjection))
          {
            if (vcGIS_ChangeSpace(&pProgramState->gis, pProgramState->sceneList[i]->pZone->srid, &pProgramState->pCamera->position))
              vcScene_UpdateItemToCurrentProjection(pProgramState, nullptr); // Update all models to new zone
          }

          if (ImGui::Selectable(pStrMoveTo))
          {
            udDouble3 localSpaceCenter = vcScene_GetItemWorldSpacePivotPoint(pProgramState->sceneList[i]);

            // Transform the camera position. Don't do the entire matrix as it may lead to inaccuracy/de-normalised camera
            if (pProgramState->gis.isProjected && pProgramState->sceneList[i]->pZone != nullptr && pProgramState->sceneList[i]->pZone->srid != pProgramState->gis.SRID)
              localSpaceCenter = udGeoZone_TransformPoint(localSpaceCenter, *pProgramState->sceneList[i]->pZone, pProgramState->gis.zone);

            pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
            pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
            pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
            pProgramState->cameraInput.worldAnchorPoint = localSpaceCenter;
            pProgramState->cameraInput.progress = 0.0;
          }

          ImGui::EndPopup();
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          vcScene_UseProjectFromItem(pProgramState, pProgramState->sceneList[i]);

        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("%s", pProgramState->sceneList[i]->pName);

        // Show additional settings from ImGui
        if (pProgramState->sceneList[i]->expanded)
        {
          ImGui::Indent();
          ImGui::PushID(udTempStr("SXIExpanded%zu", i));

          pProgramState->sceneList[i]->pImGuiFunc(pProgramState, pProgramState->sceneList[i]);

          ImGui::PopID();
          ImGui::Unindent();
        }
      }

      if (hovered == SIZE_MAX)
        drawSeperatorBefore = SIZE_MAX;

      // This block is also in the loop above
      if (drawSeperatorBefore == i)
      {
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 0.f, 1.f)); // RGBA
        ImGui::Separator();
        ImGui::PopStyleColor();
      }
    }
    ImGui::EndDock();

    if (!pProgramState->settings.window.presentationMode || pProgramState->showUI || pProgramState->settings.responsiveUI == vcPM_Show)
    {
      if (ImGui::BeginDock(pStrScene, &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
        vcRenderSceneWindow(pProgramState);
      ImGui::EndDock();
    }
    else
    {
      // Dummy scene dock, otherwise the docks get shuffled around
      if (ImGui::BeginDock(pStrScene, &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
        ImGui::Dummy(ImVec2((float)pProgramState->sceneResolution.x, (float)pProgramState->sceneResolution.y));
      ImGui::EndDock();

      ImGui::SetNextWindowSize(size);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2,2));
      ImGui::SetNextWindowPos(ImVec2(0,0));
      if (ImGui::Begin(pStrScene, &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoDecoration))
        vcRenderSceneWindow(pProgramState);
      ImGui::End();
      ImGui::PopStyleVar();
    }

    if (ImGui::BeginDock(pStrConvert, &pProgramState->settings.window.windowsOpen[vcDocks_Convert]))
      vcConvert_ShowUI(pProgramState);
    ImGui::EndDock();

    if (ImGui::BeginDock(pStrSettings, &pProgramState->settings.window.windowsOpen[vcDocks_Settings]))
    {
      if (ImGui::CollapsingHeader(pStrSettingsAppearance))
      {
        int styleIndex = pProgramState->settings.presentation.styleIndex - 1;

        if (ImGui::Combo(pStrTheme, &styleIndex, pStrThemeOptions))
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

        ImGui::Checkbox(pStrShowDiagnostics, &pProgramState->settings.presentation.showDiagnosticInfo);
        ImGui::Checkbox(pStrAdvancedGIS, &pProgramState->settings.presentation.showAdvancedGIS);
        ImGui::Checkbox(pStrLimitFPS, &pProgramState->settings.presentation.limitFPSInBackground);

        ImGui::Checkbox(pStrShowCompass, &pProgramState->settings.presentation.showCompass);

        if (ImGui::Combo(pStrPresentationUI, (int*)&pProgramState->settings.responsiveUI, pStrResponsiveOptions))
          pProgramState->showUI = false;

        ImGui::Combo(pStrMouseAnchor, (int*)&pProgramState->settings.presentation.mouseAnchor, pStrAnchorOptions);
        ImGui::Combo(pStrVoxelShape, &pProgramState->settings.presentation.pointMode, pStrVoxelOptions);
      }

      if (ImGui::CollapsingHeader(pStrInputControlsID))
      {
        ImGui::Checkbox(pStrOnScreenControls, &pProgramState->onScreenControls);

        if (ImGui::Checkbox(pStrTouchUI, &pProgramState->settings.window.touchscreenFriendly))
        {
          ImGuiStyle& style = ImGui::GetStyle();
          style.TouchExtraPadding = pProgramState->settings.window.touchscreenFriendly ? ImVec2(4, 4) : ImVec2();
        }

        ImGui::Checkbox(pStrInvertX, &pProgramState->settings.camera.invertX);
        ImGui::Checkbox(pStrInvertY, &pProgramState->settings.camera.invertY);

        ImGui::Text("%s", pStrMousePivot);
        const char *mouseModes[] = { pStrTumble, pStrOrbit, pStrPan };
        const char *scrollwheelModes[] = { pStrDolly, pStrChangeMoveSpeed };

        // Checks so the casts below are safe
        UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.cameraMouseBindings[0]) == sizeof(int), "Bindings is no longer sizeof(int)");
        UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.scrollWheelMode) == sizeof(int), "ScrollWheel is no longer sizeof(int)");

        ImGui::Combo(pStrLeft, (int*)&pProgramState->settings.camera.cameraMouseBindings[0], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo(pStrMiddle, (int*)&pProgramState->settings.camera.cameraMouseBindings[2], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo(pStrRight, (int*)&pProgramState->settings.camera.cameraMouseBindings[1], mouseModes, (int)udLengthOf(mouseModes));
        ImGui::Combo(pStrScrollWheel, (int*)&pProgramState->settings.camera.scrollWheelMode, scrollwheelModes, (int)udLengthOf(scrollwheelModes));
      }

      if (ImGui::CollapsingHeader(pStrViewportID))
      {
        if (ImGui::SliderFloat(pStrNearPlane, &pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax, "%.3fm", 2.f))
        {
          pProgramState->settings.camera.nearPlane = udClamp(pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax);
          pProgramState->settings.camera.farPlane = udMin(pProgramState->settings.camera.farPlane, pProgramState->settings.camera.nearPlane * vcSL_CameraNearFarPlaneRatioMax);
        }

        if (ImGui::SliderFloat(pStrFarPlane, &pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
        {
          pProgramState->settings.camera.farPlane = udClamp(pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax);
          pProgramState->settings.camera.nearPlane = udMax(pProgramState->settings.camera.nearPlane, pProgramState->settings.camera.farPlane / vcSL_CameraNearFarPlaneRatioMax);
        }

        //const char *pLensOptions = " Custom FoV\0 7mm\0 11mm\0 15mm\0 24mm\0 30mm\0 50mm\0 70mm\0 100mm\0";
        if (ImGui::Combo(pStrCameraLense, &pProgramState->settings.camera.lensIndex, vcCamera_GetLensNames(), vcLS_TotalLenses))
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
          if (ImGui::SliderFloat(pStrFOV, &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, pStrDegreesFormat))
            pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
        }
      }

      if (ImGui::CollapsingHeader(pStrElevationFormat))
      {
        ImGui::Checkbox(pStrMapTiles, &pProgramState->settings.maptiles.mapEnabled);

        if (pProgramState->settings.maptiles.mapEnabled)
        {
          ImGui::Checkbox(pStrMouseLock, &pProgramState->settings.maptiles.mouseInteracts);

          if (ImGui::Button(pStrTileServer,ImVec2(-1,0)))
            vcModals_OpenModal(pProgramState, vcMT_TileServer);

          ImGui::SliderFloat(pStrMapHeight, &pProgramState->settings.maptiles.mapHeight, -1000.f, 1000.f, "%.3fm", 2.f);

          const char* blendModes[] = { pStrHybrid, pStrOverlay, pStrUnderlay };
          if (ImGui::BeginCombo(pStrBlending, blendModes[pProgramState->settings.maptiles.blendMode]))
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

          if (ImGui::SliderFloat(pStrTransparency, &pProgramState->settings.maptiles.transparency, 0.f, 1.f, "%.3f"))
            pProgramState->settings.maptiles.transparency = udClamp(pProgramState->settings.maptiles.transparency, 0.f, 1.f);

          if (ImGui::Button(pStrSetHeight))
            pProgramState->settings.maptiles.mapHeight = (float)pProgramState->pCamera->position.z;
        }
      }

      if (ImGui::CollapsingHeader(pStrVisualizationFormat))
      {
        const char *visualizationModes[] = { pStrColour, pStrIntensity, pStrClassification };
        ImGui::Combo(pStrDisplayMode, (int*)&pProgramState->settings.visualization.mode, visualizationModes, (int)udLengthOf(visualizationModes));

        if (pProgramState->settings.visualization.mode == vcVM_Intensity)
        {
          // Temporary until https://github.com/ocornut/imgui/issues/467 is resolved, then use commented out code below
          float temp[] = { (float)pProgramState->settings.visualization.minIntensity, (float)pProgramState->settings.visualization.maxIntensity };
          ImGui::SliderFloat(pStrMinIntensity, &temp[0], 0.f, temp[1], "%.0f", 4.f);
          ImGui::SliderFloat(pStrMaxIntensity, &temp[1], temp[0], 65535.f, "%.0f", 4.f);
          pProgramState->settings.visualization.minIntensity = (int)temp[0];
          pProgramState->settings.visualization.maxIntensity = (int)temp[1];
        }

        if (pProgramState->settings.visualization.mode == vcVM_Classification)
        {
          ImGui::Checkbox(pStrShowColorTable, &pProgramState->settings.visualization.useCustomClassificationColours);

          if (pProgramState->settings.visualization.useCustomClassificationColours)
          {
            ImGui::SameLine();
            if (ImGui::Button(pStrRestoreColorsID))
              memcpy(pProgramState->settings.visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pProgramState->settings.visualization.customClassificationColors));

            vcIGSW_ColorPickerU32(pStrColorNeverClassified, &pProgramState->settings.visualization.customClassificationColors[0], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorUnclassified, &pProgramState->settings.visualization.customClassificationColors[1], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorGround, &pProgramState->settings.visualization.customClassificationColors[2], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorLowVegetation, &pProgramState->settings.visualization.customClassificationColors[3], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorMediumVegetation, &pProgramState->settings.visualization.customClassificationColors[4], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorHighVegetation, &pProgramState->settings.visualization.customClassificationColors[5], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorBuilding, &pProgramState->settings.visualization.customClassificationColors[6], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorLowPoint, &pProgramState->settings.visualization.customClassificationColors[7], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorKeyPoint, &pProgramState->settings.visualization.customClassificationColors[8], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorWater, &pProgramState->settings.visualization.customClassificationColors[9], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorRail, &pProgramState->settings.visualization.customClassificationColors[10], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorRoadSurface, &pProgramState->settings.visualization.customClassificationColors[11], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorReserved, &pProgramState->settings.visualization.customClassificationColors[12], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorWireGuard, &pProgramState->settings.visualization.customClassificationColors[13], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorWireConductor, &pProgramState->settings.visualization.customClassificationColors[14], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorTransmissionTower, &pProgramState->settings.visualization.customClassificationColors[15], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorWireStructureConnector, &pProgramState->settings.visualization.customClassificationColors[16], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorBridgeDeck, &pProgramState->settings.visualization.customClassificationColors[17], ImGuiColorEditFlags_NoAlpha);
            vcIGSW_ColorPickerU32(pStrColorHighNoise, &pProgramState->settings.visualization.customClassificationColors[18], ImGuiColorEditFlags_NoAlpha);

            if (ImGui::TreeNode(pStrColorReservedColors))
            {
              for (int i = 19; i < 64; ++i)
                vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, pStrReserved), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
              ImGui::TreePop();
            }

            if (ImGui::TreeNode(pStrColorUserDefinable))
            {
              for (int i = 64; i <= 255; ++i)
              {
                char buttonID[12], inputID[3];
                if (pProgramState->settings.visualization.customClassificationColorLabels[i] == nullptr)
                  vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, pStrUserDefined), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                else
                  vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, pProgramState->settings.visualization.customClassificationColorLabels[i]), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                udSprintf(buttonID, 12, "%s##%d", pStrRename, i);
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
                  if (ImGui::Button(pStrSet))
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
        ImGui::Checkbox(pStrEdgeHighlighting, &pProgramState->settings.postVisualization.edgeOutlines.enable);
        if (pProgramState->settings.postVisualization.edgeOutlines.enable)
        {
          ImGui::SliderInt(pStrEdgeWidth, &pProgramState->settings.postVisualization.edgeOutlines.width, 1, 10);

          // TODO: Make this less awful. 0-100 would make more sense than 0.0001 to 0.001.
          ImGui::SliderFloat(pStrEdgeThreshold, &pProgramState->settings.postVisualization.edgeOutlines.threshold, 0.001f, 10.0f, "%.3f");
          ImGui::ColorEdit4(pStrEdgeColour, &pProgramState->settings.postVisualization.edgeOutlines.colour.x);
        }

        // Post visualization - Colour by Height
        ImGui::Checkbox(pStrColourHeight, &pProgramState->settings.postVisualization.colourByHeight.enable);
        if (pProgramState->settings.postVisualization.colourByHeight.enable)
        {
          ImGui::ColorEdit4(pStrColourStart, &pProgramState->settings.postVisualization.colourByHeight.minColour.x);
          ImGui::ColorEdit4(pStrColourEnd, &pProgramState->settings.postVisualization.colourByHeight.maxColour.x);

          // TODO: Set min/max to the bounds of the model? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat(pStrColourHeightStart, &pProgramState->settings.postVisualization.colourByHeight.startHeight, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat(pStrColourHeightEnd, &pProgramState->settings.postVisualization.colourByHeight.endHeight, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Colour by Depth
        ImGui::Checkbox(pStrColourDepth, &pProgramState->settings.postVisualization.colourByDepth.enable);
        if (pProgramState->settings.postVisualization.colourByDepth.enable)
        {
          ImGui::ColorEdit4(pStrDepthColour, &pProgramState->settings.postVisualization.colourByDepth.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat(pStrColourDepthStart, &pProgramState->settings.postVisualization.colourByDepth.startDepth, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat(pStrColourDepthEnd, &pProgramState->settings.postVisualization.colourByDepth.endDepth, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Contours
        ImGui::Checkbox(pStrEnableContours, &pProgramState->settings.postVisualization.contours.enable);
        if (pProgramState->settings.postVisualization.contours.enable)
        {
          ImGui::ColorEdit4(pStrContoursColour, &pProgramState->settings.postVisualization.contours.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat(pStrContoursDistances, &pProgramState->settings.postVisualization.contours.distances, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat(pStrContoursBandHeight, &pProgramState->settings.postVisualization.contours.bandHeight, 0.f, 1000.f, "%.3f");
        }
      }
    }

    ImGui::EndDock();
  }

  vcModals_DrawModals(pProgramState);
}
