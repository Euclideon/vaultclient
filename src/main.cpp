#include "vcState.h"

#include "vdkContext.h"
#include "vdkServerAPI.h"
#include "vdkConfig.h"
#include "vdkPointCloud.h"
#include "vdkVersion.h"

#include <chrono>

#include "imgui.h"
#include "imgui_internal.h"

#if GRAPHICS_API_METAL
#include "imgui_ex/imgui_impl_metal.h"
#endif

#include "imgui_ex/imgui_impl_sdl.h"
#include "imgui_ex/imgui_impl_gl.h"
#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/ImGuizmo.h"
#include "imgui_ex/vcMenuButtons.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "stb_image.h"
#include "exif.h"

#include "vcConvert.h"
#include "vcVersion.h"
#include "vcGIS.h"
#include "vcClassificationColours.h"
#include "vcRender.h"
#include "vcWebFile.h"
#include "vcStrings.h"
#include "vcModals.h"
#include "vcPolygonModel.h"
#include "vcImageRenderer.h"
#include "vcProxyHelper.h"
#include "vcProject.h"
#include "vcSettingsUI.h"
#include "vcSession.h"
#include "vcPOI.h"
#include "vcStringFormat.h"
#include "vcInternalTexturesData.h"
#include "vcHotkey.h"

#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"

#include "parsers/vcUDP.h"

#include "udFile.h"
#include "udStringUtil.h"
#include "udUUID.h"

#if UDPLATFORM_EMSCRIPTEN
#include "vHTTPRequest.h"
#include <emscripten/threading.h>
#include <emscripten/emscripten.h>
#endif

UDCOMPILEASSERT(VDK_MAJOR_VERSION == 0 && VDK_MINOR_VERSION == 5 && VDK_PATCH_VERSION == 0, "This version of VDK is not compatible");

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  include <crtdbg.h>
#  include <stdio.h>

# if BUILDING_VDK
#  include "vdkDLLExport.h"

#  ifdef __cplusplus
extern "C" {
#  endif
  VDKDLL_API void vdkConfig_TrackMemoryBegin();
  VDKDLL_API bool vdkConfig_TrackMemoryEnd();
#  ifdef __cplusplus
}
#  endif
# endif

# undef main
# define main ClientMain
int main(int argc, char **args);

int SDL_main(int argc, char **args)
{
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
#if BUILDING_VDK
  vdkConfig_TrackMemoryBegin();
#endif

  int ret = main(argc, args);

#if BUILDING_VDK
  if (!vdkConfig_TrackMemoryEnd())
  {
    printf("%s\n", "Memory leaks in VDK found");

    // You've hit this because you've introduced a memory leak!
    // If you need help, uncomment `defines {"__MEMORY_DEBUG__"}` in the premake5.lua
    // This will emit filenames of what is leaking to assist in tracking down what's leaking.
    // Additionally, you can set _CrtSetBreakAlloc(<allocationNumber>);
    // inside vdkConfig_TrackMemoryEnd().
    __debugbreak();

    ret = 1;
  }
#endif

  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 0)
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks found");

    // You've hit this because you've introduced a memory leak!
    // If you need help, uncomment `defines {"__MEMORY_DEBUG__"}` in the premake5.lua
    // This will emit filenames of what is leaking to assist in tracking down what's leaking.
    // Additionally, you can set _CrtSetBreakAlloc(<allocationNumber>);
    // back up where the initial checkpoint is made.
    __debugbreak();

    ret = 1;
  }

  return ret;
}
#endif

enum vcLoginBackgroundSettings
{
  vcLBS_LoginBoxY = 40,
  vcLBS_LoginBoxH = 280,
  vcLBS_LogoW = 375,
  vcLBS_LogoH = 577,
};

const uint32_t WhitePixel = 0xFFFFFFFF;

void vcMain_ShowStartupScreen(vcState *pProgramState);
void vcRenderWindow(vcState *pProgramState);
int vcMainMenuGui(vcState *pProgramState);

bool vcMain_LangCombo(vcState *pProgramState)
{
  if (!ImGui::BeginCombo("##langCode", pProgramState->languageInfo.pLocalName))
    return false;

  for (size_t i = 0; i < pProgramState->settings.languageOptions.length; ++i)
  {
    const char *pName = pProgramState->settings.languageOptions[i].languageName;
    const char *pFilename = pProgramState->settings.languageOptions[i].filename;

    if (ImGui::Selectable(pName))
    {
      if (vcString::LoadTableFromFile(udTempStr("asset://assets/lang/%s.json", pFilename), &pProgramState->languageInfo) == udR_Success)
        udStrcpy(pProgramState->settings.window.languageCode, pFilename);
      else
        vcString::LoadTableFromFile(udTempStr("asset://assets/lang/%s.json", pProgramState->settings.window.languageCode), &pProgramState->languageInfo);
    }
  }

  ImGui::EndCombo();

  return true;
}

void vcMain_PresentationMode(vcState *pProgramState)
{
  if (pProgramState->settings.window.presentationMode)
  {
    pProgramState->settings.docksLoaded = vcSettings::vcDockLoaded::vcDL_False;
    SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }
  else
  {
    vcSettings_Save(&pProgramState->settings);
    SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }

  pProgramState->settings.window.presentationMode = !pProgramState->settings.window.presentationMode;

  if (pProgramState->settings.responsiveUI == vcPM_Responsive)
    pProgramState->lastEventTime = udGetEpochSecsUTCd();
}

void vcMain_LoadSettings(vcState *pProgramState, bool forceDefaults)
{
  vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);

  if (vcSettings_Load(&pProgramState->settings, forceDefaults))
  {
    vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

    switch (pProgramState->settings.presentation.styleIndex)
    {
    case 0: ImGui::StyleColorsDark(); ++pProgramState->settings.presentation.styleIndex; break;
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    }

    SDL_SetWindowSize(pProgramState->pWindow, pProgramState->settings.window.width, pProgramState->settings.window.height);
    SDL_SetWindowPosition(pProgramState->pWindow, pProgramState->settings.window.xpos, pProgramState->settings.window.ypos);
    if (pProgramState->settings.window.maximized)
      SDL_MaximizeWindow(pProgramState->pWindow);

    if (forceDefaults)
      vcGLState_ResizeBackBuffer(pProgramState->settings.window.width, pProgramState->settings.window.height);
  }
}

#if UDPLATFORM_EMSCRIPTEN
void vcMain_MainLoop(void *pArgs)
{
  vcState *pProgramState = (vcState*)pArgs;
#else
void vcMain_MainLoop(vcState *pProgramState)
{
#endif
  static Uint64 NOW = SDL_GetPerformanceCounter();
  static Uint64 LAST = 0;

  double frametimeMS = 0.0;
  uint32_t sleepMS = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_KEYDOWN)
      pProgramState->currentKey = vcHotkey::GetMod(event.key.keysym.scancode);

    if (!ImGui_ImplSDL2_ProcessEvent(&event))
    {
      if (event.type == SDL_WINDOWEVENT)
      {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
          pProgramState->settings.window.width = event.window.data1;
          pProgramState->settings.window.height = event.window.data2;
          vcGLState_ResizeBackBuffer(event.window.data1, event.window.data2);
        }
        else if (event.window.event == SDL_WINDOWEVENT_MOVED)
        {
          if (!pProgramState->settings.window.presentationMode)
          {
            pProgramState->settings.window.xpos = event.window.data1;
            pProgramState->settings.window.ypos = event.window.data2;
          }
        }
        else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED)
        {
          pProgramState->settings.window.maximized = true;
        }
        else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
        {
          pProgramState->settings.window.maximized = false;
        }
      }
      else if (event.type == SDL_MULTIGESTURE)
      {
        // TODO: pinch to zoom
      }
      else if (event.type == SDL_DROPFILE && pProgramState->hasContext)
      {
        pProgramState->loadList.PushBack(udStrdup(event.drop.file));
      }
      else if (event.type == SDL_QUIT)
      {
        pProgramState->programComplete = true;
      }
    }
  }

  LAST = NOW;
  NOW = SDL_GetPerformanceCounter();
  pProgramState->deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

  frametimeMS = 0.0166666667; // 60 FPS cap
  if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0 && pProgramState->settings.presentation.limitFPSInBackground)
    frametimeMS = 0.250; // 4 FPS cap when not focused

  sleepMS = (uint32_t)udMax((frametimeMS - pProgramState->deltaTime) * 1000.0, 0.0);
  udSleep(sleepMS);
  pProgramState->deltaTime += sleepMS * 0.001; // adjust delta

#if GRAPHICS_API_METAL
  ImGui_ImplMetal_NewFrame(pProgramState->pWindow);
#else
  ImGuiGL_NewFrame(pProgramState->pWindow);
#endif

  vcGizmo_BeginFrame();
  vcGLState_ResetState(true);

  vcGLState_SetViewport(0, 0, pProgramState->settings.window.width, pProgramState->settings.window.height);
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFF000000);

  if (pProgramState->finishedStartup)
    vcRenderWindow(pProgramState);
  else
    vcMain_ShowStartupScreen(pProgramState);

  ImGui::Render();

#if GRAPHICS_API_METAL
  ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData());
#else
  ImGuiGL_RenderDrawData(ImGui::GetDrawData());
#endif

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  vcGLState_Present(pProgramState->pWindow);

  if (ImGui::GetIO().WantSaveIniSettings)
    vcSettings_Save(&pProgramState->settings);

  if (pProgramState->finishedStartup)
    udWorkerPool_DoPostWork(pProgramState->pWorkerPool, 8);
  else
    pProgramState->finishedStartup = ((udWorkerPool_DoPostWork(pProgramState->pWorkerPool, 1) == udR_NothingToDo) && !udWorkerPool_HasActiveWorkers(pProgramState->pWorkerPool));

  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[SDL_SCANCODE_BACKSPACE] = false;

  if (pProgramState->finishedStartup && pProgramState->hasContext)
  {
    bool firstLoad = true;

    // Load next file in the load list (if there is one and the user has a context)
    while (pProgramState->loadList.length > 0)
    {
      const char *pNextLoad = nullptr;
      pProgramState->loadList.PopFront(&pNextLoad);

      if (pNextLoad != nullptr)
      {
        // test to see if specified filepath is valid
        bool convertDrop = false;

        //TODO: Use ImGui drag and drop on the docks rather than globally here
        if (pProgramState->settings.window.windowsOpen[vcDocks_Convert])
        {
          ImGuiWindow *pConvert = ImGui::FindWindowByName("###convertDock");
          if (pConvert != nullptr && ((pConvert->DockNode != nullptr && pConvert->DockTabIsVisible) || (pConvert->DockNode == nullptr && !pConvert->Collapsed)))
          {
            if (io.MousePos.x < pConvert->Pos.x + pConvert->Size.x && io.MousePos.x > pConvert->Pos.x && io.MousePos.y > pConvert->Pos.y && io.MousePos.y < pConvert->Pos.y + pConvert->Size.y)
              convertDrop = true;
          }
        }

        udFile *pTestFile = nullptr;
        udResult result = udFile_Open(&pTestFile, pNextLoad, udFOF_Read);
        if (result == udR_Success)
        {
          udFile_Close(&pTestFile);
        }
        else
        {
          vcState::ErrorItem status;
          status.source = vcES_File;
          status.pData = pNextLoad; // this takes ownership so we don't need to dup or free
          status.resultCode = result;

          pNextLoad = nullptr;

          pProgramState->errorItems.PushBack(status);

          continue;
        }

        udFilename loadFile(pNextLoad);
        const char *pExt = loadFile.GetExt();

        // Project Files
        if (udStrEquali(pExt, ".json"))
        {
          if (vcProject_InitFromURI(pProgramState, pNextLoad))
            pProgramState->changeActiveDock = vcDocks_Scene;
        }
        else if (udStrEquali(pExt, ".udp"))
        {
          vcProject_InitBlankScene(pProgramState);

          vcUDP_Load(pProgramState, pNextLoad);
          pProgramState->changeActiveDock = vcDocks_Scene;
        }
        else if (convertDrop) // Everything else depends on where it was dropped
        {
          vcConvert_QueueFile(pProgramState, pNextLoad);
        }
        else // We need to add it to the scene (hopefully)
        {
          vdkProjectNode *pNode = nullptr;

          if (udStrEquali(pExt, ".uds"))
          {
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "UDS", nullptr, pNextLoad, nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else if (firstLoad) // Was successful
            {
              udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
            }
          }
          else if (udStrEquali(pExt, ".vsm") || udStrEquali(pExt, ".obj"))
          {
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Polygon", nullptr, pNextLoad, nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else if (firstLoad) // Was successful
            {
              udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
            }
          }
          else if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg") || udStrEquali(pExt, ".png") || udStrEquali(pExt, ".tga") || udStrEquali(pExt, ".bmp") || udStrEquali(pExt, ".gif"))
          {
            const vcSceneItemRef &clicked = pProgramState->sceneExplorer.clickedItem;
            if (clicked.pParent != nullptr && clicked.pItem->itemtype == vdkPNT_Media)
            {
              vdkProjectNode_SetURI(pProgramState->activeProject.pProject, clicked.pItem, pNextLoad);
            }
            else
            {
              udDouble3 geolocationLongLat = udDouble3::zero();
              bool hasLocation = false;
              vcImageType imageType = vcIT_StandardPhoto;

              const unsigned char *pFileData = nullptr;
              int64_t numBytes = 0;

              if (udFile_Load(pNextLoad, (void**)&pFileData, &numBytes) == udR_Success)
              {
                // Many jpg's have exif, let's process that first
                if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg"))
                {
                  easyexif::EXIFInfo exifResult;

                  if (exifResult.parseFrom(pFileData, (int)numBytes) == PARSE_EXIF_SUCCESS)
                  {
                    if (exifResult.GeoLocation.Latitude != 0.0 || exifResult.GeoLocation.Longitude != 0.0)
                    {
                      hasLocation = true;
                      geolocationLongLat.x = exifResult.GeoLocation.Longitude;
                      geolocationLongLat.y = exifResult.GeoLocation.Latitude;
                      geolocationLongLat.z = exifResult.GeoLocation.Altitude;
                    }

                    if (exifResult.XMPMetadata != "")
                    {
                      udJSON xmp;
                      if (xmp.Parse(exifResult.XMPMetadata.c_str()) == udR_Success)
                      {
                        bool isPanorama = xmp.Get("x:xmpmeta.rdf:RDF.rdf:Description.xmlns:GPano").IsString();
                        bool isPhotosphere = xmp.Get("x:xmpmeta.rdf:RDF.rdf:Description.GPano:IsPhotosphere").AsBool();

                        if (isPanorama && isPhotosphere)
                          imageType = vcIT_PhotoSphere;
                        else if (isPanorama)
                          imageType = vcIT_Panorama;
                      }
                    }
                  }
                }

                udFree(pFileData);
              }

              if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Media", loadFile.GetFilenameWithExt(), pNextLoad, nullptr) == vE_Success)
              {
                if (hasLocation && pProgramState->gis.isProjected)
                  vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &geolocationLongLat.x);
                else
                  vcProject_UpdateNodeGeometryFromCartesian(pProgramState->activeProject.pProject, pNode, pProgramState->gis.zone, vdkPGT_Point, pProgramState->pickingSuccess ? &pProgramState->worldMousePosCartesian : &pProgramState->camera.position, 1);

                if (imageType == vcIT_PhotoSphere)
                  vdkProjectNode_SetMetadataString(pNode, "imagetype", "photosphere");
                else if (imageType == vcIT_Panorama)
                  vdkProjectNode_SetMetadataString(pNode, "imagetype", "panorama");
                else
                  vdkProjectNode_SetMetadataString(pNode, "imagetype", "standard");
              }
            }
          }
          else if (udStrEquali(pExt, ".slpk"))
          {
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "I3S", loadFile.GetFilenameWithExt(), pNextLoad, nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else if (firstLoad) // Was successful
            {
              udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
            }
          }
          else // This file isn't supported in the scene
          {
            vcState::ErrorItem status;
            status.source = vcES_File;
            status.pData = pNextLoad; // this takes ownership so we don't need to dup or free
            status.resultCode = udR_Unsupported;

            pNextLoad = nullptr;

            pProgramState->errorItems.PushBack(status);

            continue;
          }
        }

        udFree(pNextLoad);
      }

      firstLoad = false;
    }

    if (pProgramState->pLoadImage != nullptr)
    {
      vcTexture_Destroy(&pProgramState->image.pImage);

      void *pFileData = nullptr;
      int64_t fileLen = -1;

      if (udFile_Load(pProgramState->pLoadImage, &pFileData, &fileLen) == udR_Success && fileLen != 0)
      {
        int comp;
        stbi_uc *pImg = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, &pProgramState->image.width, &pProgramState->image.height, &comp, 4);

        vcTexture_Create(&pProgramState->image.pImage, pProgramState->image.width, pProgramState->image.height, pImg);

        stbi_image_free(pImg);
      }

      udFree(pFileData);

      vcModals_OpenModal(pProgramState, vcMT_ImageViewer);

      udFree(pProgramState->pLoadImage);
    }

    // Ping the server every 30 seconds
    if (pProgramState->hasContext && (udGetEpochSecsUTCf() > pProgramState->sessionInfo.expiresTimestamp - 300))
      udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_UpdateInfo, pProgramState, false);

    if (pProgramState->forceLogout)
    {
      vcSession_Logout(pProgramState);
      vcModals_OpenModal(pProgramState, vcMT_LoggedOut);
    }
  }
}

#if UDPLATFORM_EMSCRIPTEN
void vcMain_GetScreenResolution(vcState *pProgramState)
{
  pProgramState->sceneResolution.x = EM_ASM_INT_V({
    return window.innerWidth;
  });
  pProgramState->sceneResolution.y = EM_ASM_INT_V({
    return window.innerHeight;
  });
}

void vcMain_SyncFS()
{
  EM_ASM({
    // Sync from persisted state into memory
    FS.syncfs(true, function(err) { assert(!err); });
  });
}
#endif

struct vcMainLoadDataInfo
{
  vcState *pProgramState;
  const char *pFilename;

  udResult loadResult;
  void *pData;
  int64_t dataLen;
};

void vcMain_AsyncLoadWT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;
  pLoadInfo->loadResult = udFile_Load(pLoadInfo->pFilename, &pLoadInfo->pData, &pLoadInfo->dataLen);
}

void vcMain_LoadIconMT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;

  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
  unsigned char *pIconData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;

  if (pLoadInfo->loadResult == udR_Success)
  {
    pIconData = stbi_load_from_memory((stbi_uc*)pLoadInfo->pData, (int)pLoadInfo->dataLen, &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

    if (pIconData != nullptr)
    {
      pitch = iconWidth * iconBytesPerPixel;
      pitch = (pitch + 3) & ~3;

      rMask = 0xFF << 0;
      gMask = 0xFF << 8;
      bMask = 0xFF << 16;
      aMask = (iconBytesPerPixel == 4) ? (0xFF << 24) : 0;

      pIcon = SDL_CreateRGBSurfaceFrom(pIconData, iconWidth, iconHeight, iconBytesPerPixel * 8, pitch, rMask, gMask, bMask, aMask);
      if (pIcon != nullptr)
        SDL_SetWindowIcon(pLoadInfo->pProgramState->pWindow, pIcon);

      free(pIconData);
    }

    SDL_free(pIcon);
  }

  udFree(pLoadInfo->pData);
  udFree(pLoadInfo->pFilename);
}

void vcMain_LoadFontMT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;

  if (pLoadInfo->loadResult == udR_Success)
  {
    const float FontSize = 16.f;
    ImFontConfig fontCfg = ImFontConfig();
    fontCfg.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->Clear();
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg);
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

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg, characterRanges);
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()); // Still need to load Japanese seperately
#else // Debug; Only load required Glyphs
    static ImWchar characterRanges[] =
    {
      0x0020, 0x00FF, // Basic Latin + Latin Supplement
      0x2010, 0x205E, // Punctuations
      0x25A0, 0x25FF, // Geometric Shapes
      0x26A0, 0x26A1, // Exclamation in Triangle
      0
    };

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg, characterRanges);
#endif
  }

  udFree(pLoadInfo->pData);
  udFree(pLoadInfo->pFilename);
}

void vcMain_LoadStringTableMT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;

  vcString::LoadTableFromMemory((const char*)pLoadInfo->pData, &pLoadInfo->pProgramState->languageInfo);

  udFree(pLoadInfo->pData);
  udFree(pLoadInfo->pFilename);
}

void vcMain_AsyncLoad(vcState *pProgramState, const char *pFilename, udWorkerPoolCallback &&mainThreadFn)
{
  vcMainLoadDataInfo *pInfo = udAllocType(vcMainLoadDataInfo, 1, udAF_Zero);
  pInfo->pFilename = udStrdup(pFilename);
  pInfo->pProgramState = pProgramState;
  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcMain_AsyncLoadWT, pInfo, true, mainThreadFn);
}

void vcMain_AsyncResumeSession(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState *)pProgramStatePtr;
  vcSession_Resume(pProgramState);
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

#if UDPLATFORM_EMSCRIPTEN
  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, vcMain_SyncFS);
#endif

  uint32_t windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  vcState programState = {};

#if UDPLATFORM_EMSCRIPTEN
  vHTTPRequest_StartWorkerThread();
#endif

  vcSettings_RegisterAssetFileHandler();
  vcWebFile_RegisterFileHandlers();

  // default values
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // TODO: Query device and fill screen
  programState.sceneResolution.x = 1920;
  programState.sceneResolution.y = 1080;
  programState.settings.onScreenControls = true;
#elif UDPLATFORM_EMSCRIPTEN
  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, vcMain_GetScreenResolution, &programState);
#else
  programState.sceneResolution.x = 1280;
  programState.sceneResolution.y = 720;
  programState.settings.onScreenControls = false;
#endif

  programState.settings.camera.lockAltitude = false;
  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

  // Dock setting
  programState.settings.docksLoaded = vcSettings::vcDockLoaded::vcDL_False;
  programState.settings.window.windowsOpen[vcDocks_Scene] = true;
  programState.settings.window.windowsOpen[vcDocks_Settings] = true;
  programState.settings.window.windowsOpen[vcDocks_SceneExplorer] = true;
  programState.settings.window.windowsOpen[vcDocks_Convert] = true;
  programState.settings.languageOptions.Init(4);

  programState.settings.hideIntervalSeconds = 3;
  programState.showUI = true;
  programState.changeActiveDock = vcDocks_Count;
  programState.passFocus = true;
  programState.renaming = -1;

  programState.sceneExplorer.insertItem.pParent = nullptr;
  programState.sceneExplorer.insertItem.pItem = nullptr;
  programState.sceneExplorer.clickedItem.pParent = nullptr;
  programState.sceneExplorer.clickedItem.pItem = nullptr;

  programState.errorItems.Init(16);
  programState.loadList.Init(16);

  vcProject_InitBlankScene(&programState);

  for (int i = 1; i < argc; ++i)
  {
#if UDPLATFORM_OSX
    // macOS assigns a unique process serial number (PSN) to all apps,
    // we should not attempt to load this as a file.
    if (!udStrBeginsWithi(args[i], "-psn"))
#endif
      programState.loadList.PushBack(udStrdup(args[i]));
  }

  udWorkerPool_Create(&programState.pWorkerPool, 4, "VaultClientWorker");
  vcConvert_Init(&programState);

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    goto epilogue;

#if GRAPHICS_API_OPENGL
  windowFlags |= SDL_WINDOW_OPENGL;

  // Setup window
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN
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
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) != 0)
    goto epilogue;
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#endif //GRAPHICS_API_OPENGL

  // Stop window from being minimized while fullscreened and focus is lost
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

  programState.pWindow = ImGui_ImplSDL2_CreateWindow(VCVERSION_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.sceneResolution.x, programState.sceneResolution.y, windowFlags);
  if (!programState.pWindow)
    goto epilogue;

  ImGui::CreateContext();
  ImGui::GetStyle().WindowRounding = 0.0f;

  vcMain_LoadSettings(&programState, false);

#if UDPLATFORM_EMSCRIPTEN
  // This needs to be here because the settings will load with the incorrect resolution (1280x720)
  programState.settings.window.width = (int)programState.sceneResolution.x;
  programState.settings.window.height = (int)programState.sceneResolution.y;
#endif

  if (!vcGLState_Init(programState.pWindow, &programState.pDefaultFramebuffer))
    goto epilogue;

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#if GRAPHICS_API_METAL
  if (!ImGui_ImplMetal_Init())
    goto epilogue;
#else
  if (!ImGuiGL_Init(programState.pWindow))
    goto epilogue;
#endif

  if (vcRender_Init(&programState, &(programState.pRenderContext), programState.pWorkerPool, programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  vcFramebuffer_Bind(programState.pDefaultFramebuffer);

  SDL_DisableScreenSaver();

  // Async load everything else
  vcMain_AsyncLoad(&programState, udTempStr("asset://assets/lang/%s.json", programState.settings.window.languageCode), vcMain_LoadStringTableMT);
  vcMain_AsyncLoad(&programState, "asset://assets/icons/EuclideonClientIcon.png", vcMain_LoadIconMT);
  vcMain_AsyncLoad(&programState, "asset://assets/fonts/NotoSansCJKjp-Regular.otf", vcMain_LoadFontMT);

  vcTexture_AsyncCreateFromFilename(&programState.pCompanyLogo, programState.pWorkerPool, "asset://assets/textures/logo.png");
  vcTexture_AsyncCreateFromFilename(&programState.pUITexture, programState.pWorkerPool, "asset://assets/textures/uiDark24.png");

  vcTexture_Create(&programState.pWhiteTexture, 1, 1, &WhitePixel);

  vcTexture_CreateFromMemory(&programState.pCompanyWatermark, (void *)logoData, logoDataSize);

  udWorkerPool_AddTask(programState.pWorkerPool, vcMain_AsyncResumeSession, &programState, false);

#if UDPLATFORM_EMSCRIPTEN
  emscripten_set_main_loop_arg(vcMain_MainLoop, &programState, 0, 1);
#else
  while (!programState.programComplete)
    vcMain_MainLoop(&programState);
#endif

  vcSettings_Save(&programState.settings);

epilogue:
  udFree(programState.pReleaseNotes);
  programState.projects.Destroy();

  vcSettings_Cleanup(&programState.settings);

#if GRAPHICS_API_METAL
  ImGui_ImplMetal_Shutdown();
#else
  ImGuiGL_DestroyDeviceObjects();
#endif
  ImGui::DestroyContext();

  vcConvert_Deinit(&programState);
  vcTexture_Destroy(&programState.pCompanyLogo);
  vcTexture_Destroy(&programState.pCompanyWatermark);
  vcTexture_Destroy(&programState.pUITexture);
  vcTexture_Destroy(&programState.pWhiteTexture);

  for (size_t i = 0; i < programState.loadList.length; i++)
    udFree(programState.loadList[i]);
  programState.loadList.Deinit();

  for (size_t i = 0; i < programState.errorItems.length; i++)
    udFree(programState.errorItems[i].pData);
  programState.errorItems.Deinit();

  udWorkerPool_Destroy(&programState.pWorkerPool); // This needs to occur before logout
  vcProject_Deinit(&programState, &programState.activeProject); // This needs to be destroyed before the renderer is shutdown
  vcRender_Destroy(&programState, &programState.pRenderContext);
  vcTexture_Destroy(&programState.tileModal.pServerIcon);
  vcString::FreeTable(&programState.languageInfo);
  vcSession_Logout(&programState);

  vcProject_Deinit(&programState, &programState.activeProject);
  vcTexture_Destroy(&programState.image.pImage);

  vcGLState_Deinit();
  udThread_DestroyCached();

#if UDPLATFORM_EMSCRIPTEN
  vHTTPRequest_ShutdownWorkerThread();
#endif

  return 0;
}

void vcRenderSceneUI(vcState *pProgramState, const ImVec2 &windowPos, const ImVec2 &windowSize, udDouble3 *pCameraMoveOffset)
{
  ImGuiIO &io = ImGui::GetIO();
  float bottomLeftOffset = 0.f;

  if (pProgramState->cameraInput.pAttachedToSceneItem != nullptr)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x / 2, windowPos.y), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("exitAttachedModeWindow", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      const char *pStr = vcStringFormat(vcString::Get("sceneCameraAttachmentWarning"), pProgramState->cameraInput.pAttachedToSceneItem->m_pNode->pName);
      ImGui::TextUnformatted(pStr);
      udFree(pStr);

      pProgramState->cameraInput.pAttachedToSceneItem->HandleAttachmentUI(pProgramState);

      if (ImGui::Button(vcString::Get("sceneCameraAttachmentDetach"), ImVec2(-1, 0)))
      {
        pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
      }
    }

    ImGui::End();
  }

  if (pProgramState->settings.presentation.showProjectionInfo || pProgramState->settings.presentation.showAdvancedGIS)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin(vcString::Get("sceneGeographicInfo"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      if (pProgramState->settings.presentation.showProjectionInfo)
      {
        if (pProgramState->gis.SRID != 0 && pProgramState->gis.isProjected)
          ImGui::Text("%s (%s: %d)", pProgramState->gis.zone.zoneName, vcString::Get("sceneSRID"), pProgramState->gis.SRID);
        else if (pProgramState->gis.SRID == 0)
          ImGui::TextUnformatted(vcString::Get("sceneNotGeolocated"));
        else
          ImGui::Text("%s: %d", vcString::Get("sceneUnsupportedSRID"), pProgramState->gis.SRID);

        ImGui::Separator();
        if (ImGui::IsMousePosValid())
        {
          if (pProgramState->pickingSuccess)
          {
            ImGui::Text("%s: %.2f, %.2f, %.2f", vcString::Get("sceneMousePointInfo"), pProgramState->worldMousePosCartesian.x, pProgramState->worldMousePosCartesian.y, pProgramState->worldMousePosCartesian.z);

            if (pProgramState->gis.isProjected)
              ImGui::Text("%s: %.6f, %.6f", vcString::Get("sceneMousePointWGS"), pProgramState->worldMousePosLongLat.y, pProgramState->worldMousePosLongLat.x);
          }
        }
      }

      if (pProgramState->settings.presentation.showAdvancedGIS)
      {
        int newSRID = pProgramState->gis.SRID;
        udGeoZone zone;

        if (ImGui::InputInt(vcString::Get("sceneOverrideSRID"), &newSRID) && udGeoZone_SetFromSRID(&zone, newSRID) == udR_Success)
        {
          if (vcGIS_ChangeSpace(&pProgramState->gis, zone, &pProgramState->camera.position))
          {
            pProgramState->activeProject.pFolder->ChangeProjection(zone);
            vcRender_ClearTiles(pProgramState->pRenderContext);
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
    if (ImGui::Begin(vcString::Get("sceneCameraSettings"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      // Basic Settings
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneLockAltitude"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_LockAltitude)), vcMBBI_LockAltitude, vcMBBG_FirstItem, pProgramState->settings.camera.lockAltitude))
        pProgramState->settings.camera.lockAltitude = !pProgramState->settings.camera.lockAltitude;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneCameraInfo"), nullptr, vcMBBI_ShowCameraSettings, vcMBBG_SameGroup, pProgramState->settings.presentation.showCameraInfo))
        pProgramState->settings.presentation.showCameraInfo = !pProgramState->settings.presentation.showCameraInfo;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneProjectionInfo"), nullptr, vcMBBI_ShowGeospatialInfo, vcMBBG_SameGroup, pProgramState->settings.presentation.showProjectionInfo))
        pProgramState->settings.presentation.showProjectionInfo = !pProgramState->settings.presentation.showProjectionInfo;

      // Gizmo Settings
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoTranslate"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_GizmoTranslate)), vcMBBI_Translate, vcMBBG_NewGroup, pProgramState->gizmo.operation == vcGO_Translate))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Translate ? vcGO_NoGizmo : vcGO_Translate;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoRotate"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_GizmoRotate)), vcMBBI_Rotate, vcMBBG_SameGroup, pProgramState->gizmo.operation == vcGO_Rotate))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Rotate ? vcGO_NoGizmo : vcGO_Rotate;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoScale"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_GizmoScale)), vcMBBI_Scale, vcMBBG_SameGroup, pProgramState->gizmo.operation == vcGO_Scale))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Scale ? vcGO_NoGizmo : vcGO_Scale;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoLocalSpace"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_GizmoLocalSpace)), vcMBBI_UseLocalSpace, vcMBBG_SameGroup, pProgramState->gizmo.coordinateSystem == vcGCS_Local))
        pProgramState->gizmo.coordinateSystem = (pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;

      // Fullscreens
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneFullscreen"), SDL_GetScancodeName((SDL_Scancode)vcHotkey::Get(vcB_Fullscreen)), vcMBBI_FullScreen, vcMBBG_NewGroup, pProgramState->settings.window.presentationMode))
        vcMain_PresentationMode(pProgramState);

      if (pProgramState->settings.presentation.showCameraInfo)
      {
        ImGui::Separator();

        if (ImGui::InputScalarN(vcString::Get("sceneCameraPosition"), ImGuiDataType_Double, &pProgramState->camera.position.x, 3))
        {
          // limit the manual entry of camera position to +/- 40000000
          pProgramState->camera.position.x = udClamp(pProgramState->camera.position.x, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->camera.position.y = udClamp(pProgramState->camera.position.y, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->camera.position.z = udClamp(pProgramState->camera.position.z, -vcSL_GlobalLimit, vcSL_GlobalLimit);
        }

        pProgramState->camera.eulerRotation = UD_RAD2DEG(pProgramState->camera.eulerRotation);
        ImGui::InputScalarN(vcString::Get("sceneCameraRotation"), ImGuiDataType_Double, &pProgramState->camera.eulerRotation.x, 3);
        pProgramState->camera.eulerRotation = UD_DEG2RAD(pProgramState->camera.eulerRotation);

        if (ImGui::SliderFloat(vcString::Get("sceneCameraMoveSpeed"), &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
          pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_GlobalLimitSmallf);

        if (pProgramState->gis.isProjected)
        {
          ImGui::Separator();

          udDouble3 cameraLatLong = udGeoZone_CartesianToLatLong(pProgramState->gis.zone, pProgramState->camera.matrices.camera.axis.t.toVector3());

          char tmpBuf[128];
          const char *latLongAltStrings[] = { udTempStr("%.7f", cameraLatLong.x), udTempStr("%.7f", cameraLatLong.y), udTempStr("%.2fm", cameraLatLong.z) };
          ImGui::TextUnformatted(vcStringFormat(tmpBuf, udLengthOf(tmpBuf), vcString::Get("sceneCameraLatLongAlt"), latLongAltStrings, udLengthOf(latLongAltStrings)));

          if (pProgramState->gis.zone.latLongBoundMin != pProgramState->gis.zone.latLongBoundMax)
          {
            udDouble2 &minBound = pProgramState->gis.zone.latLongBoundMin;
            udDouble2 &maxBound = pProgramState->gis.zone.latLongBoundMax;

            if (cameraLatLong.x < minBound.x || cameraLatLong.y < minBound.y || cameraLatLong.x > maxBound.x || cameraLatLong.y > maxBound.y)
              ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", vcString::Get("sceneCameraOutOfBounds"));
          }
        }
      }
    }

    ImGui::End();
  }

  // Alert for no render license
  vdkLicenseInfo info = {};
  if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, vdkLT_Render, &info) == vE_Success && info.queuePosition >= 0)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::Begin("waitingForRenderLicensePanel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
      ImGui::TextUnformatted(vcString::Get("menuBarWaitingForRenderLicense"));
    ImGui::End();
  }

  // On Screen Controls Overlay
  if (pProgramState->settings.onScreenControls)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("OnScrnControls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      ImGui::SetWindowSize(ImVec2(175, 120));

      ImGui::Columns(2, NULL, false);

      ImGui::SetColumnWidth(0, 50);

      double forward = 0;
      double right = 0;
      float vertical = 0;

      if (ImGui::VSliderFloat("##oscUDSlider", ImVec2(40, 100), &vertical, -1, 1, vcString::Get("sceneCameraOSCUpDown")))
        vertical = udClamp(vertical, -1.f, 1.f);

      ImGui::NextColumn();

      ImGui::Button(vcString::Get("sceneCameraOSCMove"), ImVec2(100, 100));
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

  udInt2 logoSize = udInt2::zero();
  if (pProgramState->settings.presentation.showEuclideonLogo)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    vcTexture_GetSize(pProgramState->pCompanyWatermark, &logoSize.x, &logoSize.y);
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2((float)logoSize.x, (float)logoSize.y));
    ImGui::SetNextWindowBgAlpha(1.0f);

    if (ImGui::Begin("LogoBox", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Image(pProgramState->pCompanyWatermark, ImVec2((float)logoSize.x, (float)logoSize.y));
    ImGui::End();
    ImGui::PopStyleVar();
  }

  udInt2 sizei = udInt2::zero();
  if (pProgramState->pSceneWatermark != nullptr) // Watermark
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    vcTexture_GetSize(pProgramState->pSceneWatermark, &sizei.x, &sizei.y);

    if (pProgramState->settings.presentation.showEuclideonLogo)
      sizei *= .5;

    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + windowSize.y - logoSize.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
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

    if (ImGui::Begin(vcString::Get("sceneMapCopyright"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::TextUnformatted(vcString::Get("sceneMapData"));
    ImGui::End();
  }
}

void vcRenderScene_HandlePicking(vcState *pProgramState, vcRenderData &renderData, bool doSelect)
{
  const double farPlaneDist = 3 * (pProgramState->settings.camera.farPlane * pProgramState->settings.camera.farPlane);

  // We have to resolve UD vs. Polygon
  bool selectUD = pProgramState->pickingSuccess && (pProgramState->udModelPickedIndex != -1); // UD was successfully picked (last frame)
  double udDist = (selectUD ? udMagSq3(pProgramState->worldMousePosCartesian - pProgramState->camera.position) : farPlaneDist);

  bool getResultsImmediately = doSelect || ImGui::IsMouseClicked(0, false) || ImGui::IsMouseClicked(1, false) || ImGui::IsMouseClicked(2, false);
  vcRenderPickResult pickResult = vcRender_PolygonPick(pProgramState, pProgramState->pRenderContext, renderData, getResultsImmediately);

  bool selectPolygons = pickResult.success;
  double polyDist = (selectPolygons ? udMagSq3(pickResult.position - pProgramState->camera.position) : farPlaneDist);

  // resolve pick
  selectUD = selectUD && (udDist < polyDist);
  selectPolygons = selectPolygons && !selectUD;

  if (selectPolygons)
  {
    pProgramState->pickingSuccess = true;
    pProgramState->worldMousePosCartesian = pickResult.position;
    pProgramState->udModelPickedIndex = -1;

    if (!pProgramState->isUsingAnchorPoint)
    {
      pProgramState->isUsingAnchorPoint = true;
      pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
    }
  }

  if (doSelect)
  {
    if (selectUD)
    {
      if (pProgramState->udModelPickedIndex < (int)renderData.models.length)
        udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, renderData.models[pProgramState->udModelPickedIndex]->m_pNode->UUID);
    }
    else if (selectPolygons)
    {
      if (pickResult.pPolygon != nullptr)
      {
        udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pickResult.pPolygon->pSceneItem->m_pNode->UUID);

        if (pickResult.pPolygon->sceneItemInternalId != 0)
          pickResult.pPolygon->pSceneItem->SelectSubitem(pickResult.pPolygon->sceneItemInternalId);
      }
      else if (pickResult.pModel != nullptr)
      {
        udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pickResult.pModel->m_pNode->UUID);
      }
      else
      {
        vcProject_ClearSelection(pProgramState);
      }
    }
    else
    {
      vcProject_ClearSelection(pProgramState);
    }
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
  renderData.labels.Init(32);
  renderData.waterVolumes.Init(32);
  renderData.polyModels.Init(64);
  renderData.images.Init(32);
  renderData.viewSheds.Init(32);
  renderData.mouse.position.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.position.y = (uint32_t)(io.MousePos.y - windowPos.y);
  renderData.mouse.clicked = io.MouseClicked[1];

  udDouble3 cameraMoveOffset = udDouble3::zero();

  if (pProgramState->sceneResolution.x != windowSize.x || pProgramState->sceneResolution.y != windowSize.y) //Resize buffers
  {
    pProgramState->sceneResolution = udUInt2::create((uint32_t)windowSize.x, (uint32_t)windowSize.y);
    vcRender_ResizeScene(pProgramState, pProgramState->pRenderContext, pProgramState->sceneResolution.x, pProgramState->sceneResolution.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  }

  if (!pProgramState->modalOpen && (vcHotkey::IsPressed(vcB_Fullscreen) || ImGui::IsNavInputPressed(ImGuiNavInput_TweakFast, ImGuiInputReadMode_Released)))
    vcMain_PresentationMode(pProgramState);
  if (pProgramState->settings.responsiveUI == vcPM_Show)
    pProgramState->showUI = true;

  if (!pProgramState->settings.window.presentationMode || pProgramState->settings.responsiveUI == vcPM_Show || pProgramState->showUI)
    vcRenderSceneUI(pProgramState, windowPos, windowSize, &cameraMoveOffset);

  {
    vcRender_BeginFrame(pProgramState, pProgramState->pRenderContext, renderData);

    ImVec2 uv0 = ImVec2(0, 0);
    ImVec2 uv1 = ImVec2(renderData.sceneScaling.x, renderData.sceneScaling.y);
#if GRAPHICS_API_OPENGL
    // flip vertically
    uv1.y = 0;
    uv0.y = renderData.sceneScaling.y;
#endif

    // Actual rendering to this texture is deferred
    ImGui::ImageButton(renderData.pSceneTexture, windowSize, uv0, uv1, 0);

    static bool wasContextMenuOpenLastFrame = false;
    bool selectItem = (io.MouseDragMaxDistanceSqr[0] < (io.MouseDragThreshold*io.MouseDragThreshold)) && ImGui::IsMouseReleased(0) && ImGui::IsItemHovered();
 
    if (io.MouseDragMaxDistanceSqr[1] < (io.MouseDragThreshold*io.MouseDragThreshold) && ImGui::BeginPopupContextItem("SceneContext"))
    {
      static bool hadMouse = false;
      static udDouble3 mousePosCartesian;
      static udDouble3 mousePosLongLat;

      if (!wasContextMenuOpenLastFrame || ImGui::IsMouseClicked(1))
      {
        hadMouse = pProgramState->pickingSuccess;
        mousePosCartesian = pProgramState->worldMousePosCartesian;
        mousePosLongLat = pProgramState->worldMousePosLongLat;
      }

      if (hadMouse)
      {
        if (pProgramState->sceneExplorer.selectedItems.size() == 1)
        {
          const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[0];
          if (item.pItem->itemtype == vdkPNT_PointOfInterest && item.pItem->pUserData != nullptr)
          {
            vcPOI* pPOI = (vcPOI*)item.pItem->pUserData;

            if (ImGui::MenuItem(vcString::Get("scenePOIAddPoint")))
              pPOI->AddPoint(pProgramState, mousePosCartesian);
          }
        }

        if (ImGui::BeginMenu(vcString::Get("sceneAddMenu")))
        {
          vdkProjectNode *pNode = nullptr;

          if (ImGui::MenuItem(vcString::Get("sceneAddPOI")))
          {
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOIDefaultName"), nullptr, nullptr) == vE_Success)
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &mousePosLongLat.x);

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::MenuItem(vcString::Get("sceneAddAOI")))
          {
            vcProject_ClearSelection(pProgramState);

            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOIAreaDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Polygon, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
            }

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::MenuItem(vcString::Get("sceneBeginAreaMeasure")))
          {
            vcProject_ClearSelection(pProgramState);

            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOIAreaDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Polygon, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
              vdkProjectNode_SetMetadataBool(pNode, "showArea", true);
            }

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::MenuItem(vcString::Get("sceneAddLine")))
          {
            vcProject_ClearSelection(pProgramState);

            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOILineDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_LineString, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
            }

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::MenuItem(vcString::Get("sceneBeginLineMeasure")))
          {
            vcProject_ClearSelection(pProgramState);

            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOILineDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_LineString, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
              vdkProjectNode_SetMetadataBool(pNode, "showLength", true);
            }

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::MenuItem(vcString::Get("sceneAddViewShed")))
          {
            vcProject_ClearSelection(pProgramState);

            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "ViewMap", vcString::Get("sceneExplorerViewShedDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Polygon, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
            }

            ImGui::CloseCurrentPopup();
          }

          if (ImGui::BeginMenu(vcString::Get("sceneAddFilter")))
          {
            double scaleFactor = udMag3(pProgramState->camera.position - mousePosCartesian) / 10.0; // 1/10th of the screen

            if (ImGui::MenuItem(vcString::Get("sceneAddFilterBox")))
            {
              vcProject_ClearSelection(pProgramState);

              if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "QFilter", vcString::Get("sceneExplorerFilterBoxDefaultName"), nullptr, nullptr) == vE_Success)
              {
                vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &mousePosLongLat.x);
                vdkProjectNode_SetMetadataString(pNode, "shape", "box");
                vdkProjectNode_SetMetadataDouble(pNode, "size.x", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.y", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.z", scaleFactor);
                udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
              }

              ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem(vcString::Get("sceneAddFilterSphere")))
            {
              vcProject_ClearSelection(pProgramState);

              if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "QFilter", vcString::Get("sceneExplorerFilterSphereDefaultName"), nullptr, nullptr) == vE_Success)
              {
                vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &mousePosLongLat.x);
                vdkProjectNode_SetMetadataString(pNode, "shape", "sphere");
                vdkProjectNode_SetMetadataDouble(pNode, "size.x", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.y", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.z", scaleFactor);
                udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
              }

              ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem(vcString::Get("sceneAddFilterCylinder")))
            {
              vcProject_ClearSelection(pProgramState);

              if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "QFilter", vcString::Get("sceneExplorerFilterCylinderDefaultName"), nullptr, nullptr) == vE_Success)
              {
                vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &mousePosLongLat.x);
                vdkProjectNode_SetMetadataString(pNode, "shape", "cylinder");
                vdkProjectNode_SetMetadataDouble(pNode, "size.x", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.y", scaleFactor);
                vdkProjectNode_SetMetadataDouble(pNode, "size.z", scaleFactor);
                udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
              }

              ImGui::CloseCurrentPopup();
            }

            ImGui::EndMenu();
          }

          ImGui::EndMenu();
        }

        if (pProgramState->settings.maptiles.mapEnabled && pProgramState->gis.isProjected && pProgramState->settings.maptiles.mapHeight != mousePosLongLat.z)
        {
          if (ImGui::MenuItem(vcString::Get("sceneSetMapHeight")))
          {
            pProgramState->settings.maptiles.mapHeight = (float)mousePosLongLat.z;
            ImGui::CloseCurrentPopup();
          }
        }

        if (ImGui::MenuItem(vcString::Get("sceneMoveTo")))
        {
          pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
          pProgramState->cameraInput.startPosition = pProgramState->camera.position;
          pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->camera.eulerRotation);
          pProgramState->cameraInput.progress = 0.0;

          pProgramState->isUsingAnchorPoint = true;
          pProgramState->worldAnchorPoint = mousePosCartesian;
        }

        if (ImGui::MenuItem(vcString::Get("sceneResetRotation")))
        {
          //TODO: Smooth this over time after fixing inputs
          pProgramState->camera.eulerRotation = udDouble3::zero();
        }
      }
      else
      {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
      wasContextMenuOpenLastFrame = true;
    }
    else
    {
      wasContextMenuOpenLastFrame = false;
    }

    // Orbit around centre when fully pressed, show crosshair when partially pressed (also see vcCamera_HandleSceneInput())
    if (io.NavInputs[ImGuiNavInput_FocusNext] > 0.15f) // Right Trigger
    {
      udInt2 centrePoint = { (int)windowSize.x / 2, (int)windowSize.y / 2 };
      renderData.mouse.position = centrePoint;

      // Need to adjust crosshair position slightly
      centrePoint += pProgramState->settings.window.presentationMode ? udInt2::create(-8, -8) : udInt2::create(-2, -2);

      ImVec2 sceneWindowPos = ImGui::GetWindowPos();
      sceneWindowPos = ImVec2(sceneWindowPos.x + centrePoint.x, sceneWindowPos.y + centrePoint.y);

      ImGui::GetWindowDrawList()->AddImage(pProgramState->pUITexture, ImVec2((float)sceneWindowPos.x, (float)sceneWindowPos.y), ImVec2((float)sceneWindowPos.x + 24, (float)sceneWindowPos.y + 24), ImVec2(0, 0.375), ImVec2(0.09375, 0.46875));
    }


    pProgramState->activeProject.pFolder->AddToScene(pProgramState, &renderData);

    vcRenderScene_HandlePicking(pProgramState, renderData, selectItem);

    // Camera update has to be here because it depends on previous ImGui state
    vcCamera_HandleSceneInput(pProgramState, cameraMoveOffset, udFloat2::create(windowSize.x, windowSize.y), udFloat2::create((float)renderData.mouse.position.x, (float)renderData.mouse.position.y));

    if (pProgramState->sceneExplorer.clickedItem.pParent && pProgramState->sceneExplorer.clickedItem.pItem && !pProgramState->modalOpen)
    {
      vcGizmo_SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
      vcGizmo_SetDrawList();

      vcSceneItemRef clickedItemRef = pProgramState->sceneExplorer.clickedItem;
      vcSceneItem *pItem = (vcSceneItem*)clickedItemRef.pItem->pUserData;

      if (pItem != nullptr)
      {
        udDouble4x4 temp = pItem->GetWorldSpaceMatrix();
        temp.axis.t.toVector3() = pItem->GetWorldSpacePivot();

        udDouble4x4 delta = udDouble4x4::identity();

        double snapAmt = 0.1;

        if (pProgramState->gizmo.operation == vcGO_Rotate)
          snapAmt = 15.0;
        else if (pProgramState->gizmo.operation == vcGO_Translate)
          snapAmt = 0.25;

        vcGizmoAllowedControls allowedControls = vcGAC_All;
        for (vcSceneItemRef &ref : pProgramState->sceneExplorer.selectedItems)
          allowedControls = (vcGizmoAllowedControls)(allowedControls & ((vcSceneItem*)ref.pItem->pUserData)->GetAllowedControls());

        vcGizmo_Manipulate(&pProgramState->camera, pProgramState->gizmo.operation, pProgramState->gizmo.coordinateSystem, temp, &delta, allowedControls, io.KeyShift ? snapAmt : 0.0);

        if (!(delta == udDouble4x4::identity()))
        {
          for (vcSceneItemRef &ref : pProgramState->sceneExplorer.selectedItems)
            ((vcSceneItem*)ref.pItem->pUserData)->ApplyDelta(pProgramState, delta);
        }
      }
    }
    else
    {
      vcGizmo_ResetState();
    }
  }

  vcRender_SceneImGui(pProgramState, pProgramState->pRenderContext, renderData);

  // Render scene to texture
  vcRender_RenderScene(pProgramState, pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);
  
  // Clean up
  renderData.models.Deinit();
  renderData.fences.Deinit();
  renderData.labels.Deinit();
  renderData.waterVolumes.Deinit();
  renderData.polyModels.Deinit();
  renderData.images.Deinit();
  renderData.viewSheds.Deinit();

  // Can only assign longlat positions in projected space
  if (pProgramState->gis.isProjected)
    pProgramState->worldMousePosLongLat = udGeoZone_CartesianToLatLong(pProgramState->gis.zone, pProgramState->worldMousePosCartesian, true);
  else
    pProgramState->worldMousePosLongLat = pProgramState->worldMousePosCartesian;

  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
}

void vcMain_UpdateStatusBar(vcState *pProgramState)
{
  //Note that items are drawn right to left (reverse order) to right-align

  int64_t currentTime = udGetEpochSecsUTCd();
  float xPosition = ImGui::GetContentRegionMax().x - 20.f;

  vdkContext_GetSessionInfo(pProgramState->pVDKContext, &pProgramState->sessionInfo);

  char tempData[128] = {};

  // Username
  {
    const char *pTemp = udTempStr("%s (%.3f)", pProgramState->sessionInfo.displayName, pProgramState->sessionInfo.expiresTimestamp - udGetEpochSecsUTCf());

    xPosition -= ImGui::CalcTextSize(pTemp).x;
    
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(pTemp);
  }

  // Load List
  if (pProgramState->loadList.length > 0)
  {
    const char *strings[] = { udTempStr("%zu", pProgramState->loadList.length) };
    vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFilesQueued"), strings, udLengthOf(strings));
    udStrcat(tempData, " / ");

    xPosition -= ImGui::CalcTextSize(tempData).x;
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(tempData);

    xPosition -= ImGui::CalcTextSize("\xE2\x96\xB2").x + 5.f;
    ImGui::SameLine(xPosition);
    vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading, false);
  }

  // Error List
  if (pProgramState->errorItems.length > 0)
  {
    bool isHovered = false;
    bool isClicked = false;

    const char *strings[] = { udTempStr("%zu", pProgramState->errorItems.length) };
    vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFilesFailed"), strings, udLengthOf(strings));
    udStrcat(tempData, " / ");

    xPosition -= ImGui::CalcTextSize(tempData).x;
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(tempData);
    isHovered = ImGui::IsItemHovered();
    isClicked = ImGui::IsItemClicked();

    xPosition -= ImGui::CalcTextSize("\xE2\x9A\xA0").x + 5.f;
    ImGui::SameLine(xPosition);

    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    isHovered = isHovered || ImGui::IsItemHovered();
    isClicked = isClicked || ImGui::IsItemClicked();

    if (isHovered)
    {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted(vcString::Get("menuBarFilesFailedTooltip"));
      ImGui::EndTooltip();
    }

    if (isClicked)
      vcModals_OpenModal(pProgramState, vcMT_UnsupportedFile);
  }

  if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0)
  {
    udStrcpy(tempData, vcString::Get("menuBarInactive"));
    udStrcat(tempData, " / ");

    xPosition -= ImGui::CalcTextSize(tempData).x;
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(tempData);
  }

  // New Version Available
  if (pProgramState->packageInfo.Get("success").AsBool())
  {
    udStrcpy(tempData, udTempStr("%s [%s] / ", vcString::Get("menuBarUpdateAvailable"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    xPosition -= ImGui::CalcTextSize(tempData).x;
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(tempData);
  }

  // Diagnostic Information
  if (pProgramState->settings.presentation.showDiagnosticInfo)
  {
    const char *strings[] = { udTempStr("%.2f", 1.f / pProgramState->deltaTime), udTempStr("%.3f", pProgramState->deltaTime * 1000.f) };
    vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFPS"), strings, udLengthOf(strings));
    udStrcat(tempData, " / ");

    xPosition -= ImGui::CalcTextSize(tempData).x;
    ImGui::SameLine(xPosition);
    ImGui::TextUnformatted(tempData);
  }

  for (int i = 0; i < vdkLT_Count; ++i)
  {
    vdkLicenseInfo info = {};
    if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, (vdkLicenseType)i, &info) == vE_Success)
    {
      if (info.queuePosition < 0 && (uint64_t)currentTime < info.expiresTimestamp)
        udStrcpy(tempData, udTempStr("%s %s (%" PRIu64 "%s) / ", i == vdkLT_Render ? vcString::Get("menuBarRender") : vcString::Get("menuBarConvert"), vcString::Get("menuBarLicense"), (info.expiresTimestamp - currentTime), vcString::Get("menuBarSecondsAbbreviation")));
      else if (info.queuePosition < 0)
        udStrcpy(tempData, udTempStr("%s %s / ", i == vdkLT_Render ? vcString::Get("menuBarRender") : vcString::Get("menuBarConvert"), vcString::Get("menuBarLicenseExpired")));
      else if (info.queuePosition == 0)
        udStrcpy(tempData, udTempStr("%s / ", i == vdkLT_Render ? vcString::Get("menuBarWaitingForRenderLicense") : vcString::Get("convertAwaitingLicense")));
      else
        udStrcpy(tempData, udTempStr("%s (%" PRId64 " %s) / ", i == vdkLT_Render ? vcString::Get("menuBarWaitingForRenderLicense") : vcString::Get("convertAwaitingLicense"), info.queuePosition, vcString::Get("menuBarLicenseQueued")));

      xPosition -= ImGui::CalcTextSize(tempData).x;
      ImGui::SameLine(xPosition);
      ImGui::TextUnformatted(tempData);
    }
  }
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu(vcString::Get("menuSystem")))
    {
      if (ImGui::MenuItem(vcString::Get("menuLogout")))
        vcSession_Logout(pProgramState);

      if (ImGui::MenuItem(vcString::Get("menuAbout")))
        vcModals_OpenModal(pProgramState, vcMT_About);

      if (ImGui::MenuItem(vcString::Get("menuReleaseNotes")))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

      if (ImGui::MenuItem(vcString::Get("menuBindings")))
        vcModals_OpenModal(pProgramState, vcMT_Bindings);

      if (pProgramState->settings.languageOptions.length > 0 && ImGui::BeginMenu(vcString::Get("menuLanguage")))
      {
        for (size_t i = 0; i < pProgramState->settings.languageOptions.length; ++i)
        {
          const char *pName = pProgramState->settings.languageOptions[i].languageName;
          const char *pFilename = pProgramState->settings.languageOptions[i].filename;

          if (ImGui::MenuItem(pName, nullptr, udStrEqual(pProgramState->settings.window.languageCode, pFilename)))
          {
            if (vcString::LoadTableFromFile(udTempStr("asset://assets/lang/%s.json", pFilename), &pProgramState->languageInfo) == udR_Success)
              udStrcpy(pProgramState->settings.window.languageCode, pFilename);
            else
              vcString::LoadTableFromFile(udTempStr("asset://assets/lang/%s.json", pProgramState->settings.window.languageCode), &pProgramState->languageInfo);
          }
        }

        ImGui::EndMenu();
      }


      if (ImGui::BeginMenu(vcString::Get("menuExperimentalFeatures")))
      {
        ImGui::Separator();

        ImGui::EndMenu();
      }

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
      if (ImGui::MenuItem(vcString::Get("menuQuit"), "Alt+F4"))
        pProgramState->programComplete = true;
#endif

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(vcString::Get("menuWindows")))
    {
      ImGui::MenuItem(vcString::Get("menuScene"), nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Scene]);
      ImGui::MenuItem(vcString::Get("menuSceneExplorer"), nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]);
      ImGui::MenuItem(vcString::Get("menuSettings"), nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Settings]);
      ImGui::MenuItem(vcString::Get("menuConvert"), nullptr, &pProgramState->settings.window.windowsOpen[vcDocks_Convert]);
      ImGui::Separator();
      ImGui::EndMenu();
    }

    udJSONArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (ImGui::BeginMenu(vcString::Get("menuProjects"), pProjectList != nullptr && pProjectList->length > 0))
    {
      if (ImGui::MenuItem(vcString::Get("menuNewScene"), nullptr, nullptr))
        vcProject_InitBlankScene(pProgramState);

      if (ImGui::MenuItem(vcString::Get("menuProjectExport"), nullptr, nullptr))
        vcModals_OpenModal(pProgramState, vcMT_ExportProject);

      if (ImGui::MenuItem(vcString::Get("menuProjectImport"), nullptr, nullptr))
        vcModals_OpenModal(pProgramState, vcMT_ImportProject);

      ImGui::Separator();

      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcProject_InitBlankScene(pProgramState);
          bool moveTo = true;

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
          {
            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "UDS", nullptr, pProjectList->GetElement(i)->Get("models[%zu]", j).AsString(), nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = udStrdup(pProjectList->GetElement(i)->Get("models[%zu]", j).AsString());
              projectError.resultCode = udR_Failure_;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
            }
            else
            {
              if (moveTo)
                udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
              moveTo = false;
            }
          }

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("feeds").ArrayLength(); ++j)
          {
            const char *pFeedName = pProjectList->GetElement(i)->Get("feeds[%zu].name", j).AsString();

            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "IOT", pFeedName, nullptr, nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = udStrdup(pFeedName);
              projectError.resultCode = udR_Failure_;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
            }

            if (udUUID_IsValid(pProjectList->GetElement(i)->Get("feeds[%zu].groupid", j).AsString()))
              vdkProjectNode_SetMetadataString(pNode, "groupid", pProjectList->GetElement(i)->Get("feeds[%zu].groupid", j).AsString());
          }
        }
      }

      ImGui::EndMenu();
    }

    vcMain_UpdateStatusBar(pProgramState);

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcChangeTab(vcState *pProgramState, vcDocks dock)
{
  if (pProgramState->changeActiveDock == dock)
  {
    ImGui::SetWindowFocus();
    pProgramState->changeActiveDock = vcDocks_Count;
  }
}

void vcMain_ShowStartupScreen(vcState *pProgramState)
{
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 size = io.DisplaySize;

  static double logoFade = 0.0;

  ImDrawList *pDrawList = ImGui::GetBackgroundDrawList();

  pDrawList->AddRectFilledMultiColor(ImVec2(0, 0), size, 0xFFB5A245, 0xFFE3D9A8, 0xFFCDBC71, 0xFF998523);

  float amt = (float)udSin(ImGui::GetTime()) * 50.f;
  float baseY = size.y * 0.75f;
  pDrawList->AddBezierCurve(ImVec2(0, baseY), ImVec2(size.x * 0.33f, baseY + amt), ImVec2(size.x * 0.67f, baseY - amt), ImVec2(size.x, baseY), 0xFFFFFFFF, 4.f);

  if (pProgramState->pCompanyLogo != nullptr)
  {
    logoFade += pProgramState->deltaTime * 10; // Fade in really fast
    uint32_t fadeIn = (0xFFFFFF | (udMin(uint32_t(logoFade * 255), 255U) << 24));

    float scaling = udMin(0.9f * (size.y - vcLBS_LoginBoxH) / vcLBS_LogoH, 1.f);
    float yOff = (size.y - vcLBS_LoginBoxH) / 2.f;
    pDrawList->AddImage(pProgramState->pCompanyLogo, ImVec2((size.x - vcLBS_LogoW * scaling) / 2.f, yOff - (vcLBS_LogoH * scaling * 0.5f)), ImVec2((size.x + vcLBS_LogoW * scaling) / 2, yOff + (vcLBS_LogoH * scaling * 0.5f)), ImVec2(0, 0), ImVec2(1, 1), fadeIn);
  }
}

void vcMain_ShowLoginWindow(vcState *pProgramState)
{
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));

  ImGuiIO io = ImGui::GetIO();
  ImVec2 size = io.DisplaySize;

  vcMain_ShowStartupScreen(pProgramState);

  ImGui::SetNextWindowBgAlpha(0.f);
  ImGui::SetNextWindowPos(ImVec2(0, size.y), ImGuiCond_Always, ImVec2(0, 1));
  if (ImGui::Begin("LoginScreenPopups", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
  {
    ImGui::PushItemWidth(130.f);

    if (ImGui::Button(vcString::Get("loginRestoreDefaults")))
      vcMain_LoadSettings(pProgramState, true);

    ImGui::SameLine();
    if (ImGui::Button(vcString::Get("loginReleaseNotes")))
      vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

    ImGui::SameLine();
    if (ImGui::Button(vcString::Get("loginAbout")))
      vcModals_OpenModal(pProgramState, vcMT_About);

    ImGui::SameLine();

    // Show the language combo, if its not visible and the selected translation isn't for the current version, display an error tooltip
    if (!vcMain_LangCombo(pProgramState) && !udStrEqual(pProgramState->languageInfo.pTargetVersion, VCSTRINGIFY(VCVERSION_VERSION_ARRAY_PARTIAL)))
    {
      ImGui::SetNextWindowBgAlpha(0.5f);
      ImGui::SetNextWindowPos(ImGui::GetItemRectMin(), ImGuiCond_Always, ImVec2(0, 1));

      if (ImGui::Begin("LoginScreenBadTranslation", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_Tooltip))
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", vcString::Get("settingsAppearanceTranslationDifferentVersion"));
      ImGui::End();
    }

    // Let the user change the look and feel on the login page
    const char *themeOptions[] = { vcString::Get("settingsAppearanceDark"), vcString::Get("settingsAppearanceLight") };
    ImGui::SameLine();
    int styleIndex = pProgramState->settings.presentation.styleIndex - 1;
    if (ImGui::Combo("##theme", &styleIndex, themeOptions, (int)udLengthOf(themeOptions)))
    {
      pProgramState->settings.presentation.styleIndex = styleIndex + 1;
      switch (styleIndex)
      {
      case 0: ImGui::StyleColorsDark(); break;
      case 1: ImGui::StyleColorsLight(); break;
      }
    }

    ImGui::PopItemWidth();
  }
  ImGui::End();

  ImGui::SetNextWindowBgAlpha(0.1f);
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always, ImVec2(0, 0));
  if (ImGui::Begin("LoginScreenSupportInfo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
  {
    const char *issueTrackerStrings[] = { "https://github.com/euclideon/vaultclient/issues", "GitHub" };
    const char *pIssueTrackerStr = vcStringFormat(vcString::Get("loginSupportIssueTracker"), issueTrackerStrings, udLengthOf(issueTrackerStrings));
    ImGui::TextUnformatted(pIssueTrackerStr);
    udFree(pIssueTrackerStr);
    if (ImGui::IsItemClicked())
      vcWebFile_OpenBrowser("https://github.com/euclideon/vaultclient/issues");
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    const char *pSupportStr = vcStringFormat(vcString::Get("loginSupportDirectEmail"), "support@euclideon.com");
    ImGui::TextUnformatted(pSupportStr);
    udFree(pSupportStr);
    if (ImGui::IsItemClicked())
      vcWebFile_OpenBrowser("mailto:support@euclideon.com?subject=Vault%20Client%20" VCVERSION_VERSION_STRING "%20Support");
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }
  ImGui::End();

  ImGui::SetNextWindowBgAlpha(0.1f);
  ImGui::SetNextWindowPos(ImVec2(size.x - 10, 10), ImGuiCond_Always, ImVec2(1, 0));
  if (ImGui::Begin("LoginScreenTranslationInfo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
  {
    const char *translationStrings[] = { pProgramState->languageInfo.pLocalName, pProgramState->languageInfo.pTranslatorName, pProgramState->languageInfo.pTranslatorContactEmail };
    const char *pTranslationInfo = vcStringFormat(vcString::Get("loginTranslationBy"), translationStrings, udLengthOf(translationStrings));
    ImGui::TextUnformatted(pTranslationInfo);
    udFree(pTranslationInfo);
    if (ImGui::IsItemClicked())
      vcWebFile_OpenBrowser(udTempStr("mailto:%s", pProgramState->languageInfo.pTranslatorContactEmail));
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }
  ImGui::End();

  ImGui::PopStyleColor(); // Border Colour

  ImGui::SetNextWindowSize(ImVec2(500, 160), ImGuiCond_Appearing);
  ImGui::SetNextWindowPos(ImVec2(size.x / 2, size.y - vcLBS_LoginBoxY), ImGuiCond_Always, ImVec2(0.5, 1.0));

  const char *loginStatusKeys[] = { "loginMessageCredentials", "loginMessageCredentials", "loginEnterURL", "loginPending", "loginErrorConnection", "loginErrorAuth", "loginErrorTimeSync", "loginErrorSecurity", "loginErrorNegotiate", "loginErrorProxy", "loginErrorProxyAuthPending", "loginErrorProxyAuthPending", "loginErrorProxyAuthFailed", "loginErrorOther" };

  if (pProgramState->loginStatus == vcLS_Pending)
  {
    if (ImGui::Begin("loginTitle", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
      vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
      ImGui::TextUnformatted(vcString::Get("loginMessageChecking"));
    }
    ImGui::End();
  }
  else
  {
    if (ImGui::Begin("loginTitle", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
      ImGui::TextUnformatted(vcString::Get(loginStatusKeys[pProgramState->loginStatus]));

      // Tool for support to get reasons for failures, requires Alt & Ctrl
      if (pProgramState->logoutReason != vE_Success && io.KeyAlt && io.KeyCtrl)
      {
        ImGui::SameLine();
        ImGui::Text("E:%d", (int)pProgramState->logoutReason);
      }

      bool tryLogin = false;

      // Server URL
      tryLogin |= ImGui::InputText(vcString::Get("loginServerURL"), pProgramState->settings.loginInfo.serverURL, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->loginStatus == vcLS_NoStatus && !pProgramState->settings.loginInfo.rememberServer)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox(udTempStr("%s##rememberServerURL", vcString::Get("loginRememberServer")), &pProgramState->settings.loginInfo.rememberServer);

      // Username
      tryLogin |= ImGui::InputText(vcString::Get("loginUsername"), pProgramState->settings.loginInfo.username, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->loginStatus == vcLS_NoStatus && pProgramState->settings.loginInfo.rememberServer && !pProgramState->settings.loginInfo.rememberUsername)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox(udTempStr("%s##rememberUser", vcString::Get("loginRememberUser")), &pProgramState->settings.loginInfo.rememberUsername);

      // Password
      ImVec2 buttonSize;
      if (pProgramState->passFocus)
      {
        ImGui::Button(vcString::Get("loginShowPassword"));
        ImGui::SameLine(0, 0);
        buttonSize = ImGui::GetItemRectSize();
      }
      if (ImGui::IsItemActive() && pProgramState->passFocus)
        tryLogin |= ImGui::InputText(vcString::Get("loginPassword"), pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      else
        tryLogin |= ImGui::InputText(vcString::Get("loginPassword"), pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

      if (pProgramState->passFocus && ImGui::IsMouseClicked(0))
      {
        ImVec2 minPos = ImGui::GetItemRectMin();
        ImVec2 maxPos = ImGui::GetItemRectMax();
        if (io.MouseClickedPos->x < minPos.x - buttonSize.x || io.MouseClickedPos->x > maxPos.x || io.MouseClickedPos->y < minPos.y || io.MouseClickedPos->y > maxPos.y)
          pProgramState->passFocus = false;
      }

      if (!pProgramState->passFocus && udStrlen(pProgramState->password) == 0)
        pProgramState->passFocus = true;

      if (pProgramState->loginStatus == vcLS_NoStatus && pProgramState->settings.loginInfo.rememberServer && pProgramState->settings.loginInfo.rememberUsername)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);

      if (pProgramState->loginStatus == vcLS_NoStatus)
      {
        pProgramState->loginStatus = vcLS_EnterCredentials;
      }
      else if (pProgramState->loginStatus == vcLS_ProxyAuthRequired)
      {
        pProgramState->loginStatus = vcLS_ProxyAuthPending;
        vcModals_OpenModal(pProgramState, vcMT_ProxyAuth);
      }
      else if (pProgramState->loginStatus == vcLS_ProxyAuthPending && !pProgramState->modalOpen)
      {
        tryLogin = true; // Retries the login after proxy info is entered
      }

      if (ImGui::Button(vcString::Get("loginButton")) || tryLogin)
      {
        if (*pProgramState->settings.loginInfo.serverURL == '\0')
        {
          pProgramState->loginStatus = vcLS_NoServerURL;
        }
        else
        {
          pProgramState->passFocus = false;
          pProgramState->loginStatus = vcLS_Pending;
          udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_Login, pProgramState, false);
        }
      }

      if (SDL_GetModState() & KMOD_CAPS)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "%s", vcString::Get("loginCapsWarning"));
      }

      ImGui::Separator();

      if (ImGui::TreeNode(vcString::Get("loginAdvancedSettings")))
      {
        // Make sure its actually off before doing the auto-proxy check
        if (ImGui::Checkbox(vcString::Get("loginProxyAutodetect"), &pProgramState->settings.loginInfo.autoDetectProxy) && pProgramState->settings.loginInfo.autoDetectProxy)
          vcProxyHelper_AutoDetectProxy(pProgramState);

        if (ImGui::InputText(vcString::Get("loginProxyAddress"), pProgramState->settings.loginInfo.proxy, vcMaxPathLength, pProgramState->settings.loginInfo.autoDetectProxy ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None) && !pProgramState->settings.loginInfo.autoDetectProxy)
          vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

        ImGui::SameLine();
        if (ImGui::Button(vcString::Get("loginProxyTest")))
        {
          if (pProgramState->settings.loginInfo.autoDetectProxy)
            vcProxyHelper_AutoDetectProxy(pProgramState);

          //TODO: Decide what to do with other errors
          if (vcProxyHelper_TestProxy(pProgramState) == vE_ProxyAuthRequired)
            vcModals_OpenModal(pProgramState, vcMT_ProxyAuth);
        }

        if (ImGui::InputText(vcString::Get("loginUserAgent"), pProgramState->settings.loginInfo.userAgent, vcMaxPathLength))
          vdkConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);

        // TODO: Consider reading user agent strings from a file
        const char *UAOptions[] = { "Mozilla" };
        const char *UAStrings[] = { "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:53.0) Gecko/20100101 Firefox/53.0" };

        int UAIndex = -1;
        if (ImGui::Combo(udTempStr("%s###loginUserAgentPresets", vcString::Get("loginSelectUserAgent")), &UAIndex, UAOptions, (int)udLengthOf(UAOptions)))
        {
          udStrcpy(pProgramState->settings.loginInfo.userAgent, UAStrings[UAIndex]);
          vdkConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);
        }

        if (ImGui::Checkbox(vcString::Get("loginIgnoreCert"), &pProgramState->settings.loginInfo.ignoreCertificateVerification))
          vdkConfig_IgnoreCertificateVerification(pProgramState->settings.loginInfo.ignoreCertificateVerification);

        if (pProgramState->settings.loginInfo.ignoreCertificateVerification)
          ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "%s", vcString::Get("loginIgnoreCertWarning"));

        ImGui::TreePop();
      }
    }
    ImGui::End();
  }
}

void vcRenderWindow(vcState *pProgramState)
{
  ImGuiIO &io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

  if (pProgramState->settings.responsiveUI == vcPM_Responsive)
  {
    if (io.MouseDelta.x != 0.0 || io.MouseDelta.y != 0.0)
    {
      pProgramState->lastEventTime = udGetEpochSecsUTCd();
      pProgramState->showUI = true;
    }
    else if ((udGetEpochSecsUTCd() - pProgramState->lastEventTime) > pProgramState->settings.hideIntervalSeconds)
    {
      pProgramState->showUI = false;
    }
  }

#if UDPLATFORM_WINDOWS
  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;
#endif

  if (vcHotkey::IsPressed(vcB_BindingsInterface))
    vcModals_OpenModal(pProgramState, vcMT_Bindings);

  //end keyboard/mouse handling

  if (pProgramState->hasContext && !pProgramState->settings.window.presentationMode)
  {
    int margin = vcMainMenuGui(pProgramState);

    if (pProgramState->settings.docksLoaded != vcSettings::vcDockLoaded::vcDL_True)
      pProgramState->settings.rootDock = ImGui::GetID("MyDockspace");

    ImGui::SetNextWindowSize(ImVec2(size.x, size.y));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowBgAlpha(0.f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("RootDockContainer", 0, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    ImGui::DockSpace(pProgramState->settings.rootDock, ImVec2(size.x, size.y - margin));
    ImGui::End();
  }

  if (!pProgramState->hasContext)
  {
    vcMain_ShowLoginWindow(pProgramState);
  }
  else
  {
    if (pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer] && !pProgramState->settings.window.presentationMode)
    {
      if (ImGui::Begin(udTempStr("%s###sceneExplorerDock", vcString::Get("sceneExplorerTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]))
      {
        char buffer[50] = {};
        vcHotkey::GetKeyName(vcB_AddUDS, buffer);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddUDS"), buffer, vcMBBI_AddPointCloud, vcMBBG_FirstItem) || vcHotkey::IsPressed(vcB_AddUDS))
          vcModals_OpenModal(pProgramState, vcMT_AddUDS);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddFolder"), nullptr, vcMBBI_AddFolder, vcMBBG_SameGroup))
        {
          vdkProjectNode *pNode = nullptr;
          if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Folder", vcString::Get("sceneExplorerFolderDefaultName"), nullptr, nullptr) != vE_Success)
          {
            vcState::ErrorItem projectError;
            projectError.source = vcES_ProjectChange;
            projectError.pData = udStrdup(vcString::Get("sceneExplorerAddFolder"));
            projectError.resultCode = udR_Failure_;

            pProgramState->errorItems.PushBack(projectError);

            vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
          }
        }

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddViewpoint"), nullptr, vcMBBI_SaveViewport, vcMBBG_SameGroup))
        {
          vdkProjectNode *pNode;
          if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Camera", vcString::Get("viewpointDefaultName"), nullptr, nullptr) == vE_Success)
          {
            udDouble3 cameraPositionInLongLat = udGeoZone_CartesianToLatLong(pProgramState->gis.zone, pProgramState->camera.position, true);

            if (pProgramState->gis.isProjected)
              vdkProjectNode_SetGeometry(pProgramState->activeProject.pProject, pNode, vdkPGT_Point, 1, &cameraPositionInLongLat.x);

            vdkProjectNode_SetMetadataDouble(pNode, "transform.rotation.x", pProgramState->camera.eulerRotation.x);
            vdkProjectNode_SetMetadataDouble(pNode, "transform.rotation.y", pProgramState->camera.eulerRotation.y);
            vdkProjectNode_SetMetadataDouble(pNode, "transform.rotation.z", pProgramState->camera.eulerRotation.z);
          }
          else
          {
            vcState::ErrorItem projectError;
            projectError.source = vcES_ProjectChange;
            projectError.pData = udStrdup(vcString::Get("sceneExplorerAddViewpoint"));
            projectError.resultCode = udR_Failure_;

            pProgramState->errorItems.PushBack(projectError);

            vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
          }
        }

        vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddOther"), nullptr, vcMBBI_AddOther, vcMBBG_SameGroup);
        if (ImGui::BeginPopupContextItem(vcString::Get("sceneExplorerAddOther"), 0))
        {
          if (pProgramState->sceneExplorer.selectedItems.size() == 1)
          {
            const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[0];
            if (item.pItem->itemtype == vdkPNT_PointOfInterest)
            {
              vcPOI* pPOI = (vcPOI*)item.pItem->pUserData;

              if (ImGui::MenuItem(vcString::Get("scenePOIAddPoint")))
                pPOI->AddPoint(pProgramState, pProgramState->worldAnchorPoint);
            }
          }

          if (ImGui::MenuItem(vcString::Get("sceneExplorerAddFeed"), nullptr, nullptr))
          {
            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "IOT", vcString::Get("liveFeedDefaultName"), nullptr, nullptr) != vE_Success)
            {
              vcState::ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = udStrdup(vcString::Get("sceneExplorerAddFeed"));
              projectError.resultCode = udR_Failure_;

              pProgramState->errorItems.PushBack(projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
            }
          }

          ImGui::EndPopup();
        }

        vcHotkey::GetKeyName(vcB_Remove, buffer);
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerRemove"), buffer, vcMBBI_Remove, vcMBBG_NewGroup) || (vcHotkey::IsPressed(vcB_Remove) && !ImGui::IsAnyItemActive()))
          vcProject_RemoveSelected(pProgramState);

        // Tree view for the scene
        ImGui::Separator();

        if (ImGui::BeginChild("SceneExplorerList", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
          if (!ImGui::IsMouseDragging() && pProgramState->sceneExplorer.insertItem.pParent != nullptr)
          {
            // Ensure a circular reference is not created
            bool itemFound = false;
            for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size() && !itemFound; ++i)
            {
              const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
              if (item.pItem->itemtype == vdkPNT_Folder)
                itemFound = vcProject_ContainsItem(item.pItem, pProgramState->sceneExplorer.insertItem.pItem);

              itemFound = itemFound || item.pItem == pProgramState->sceneExplorer.insertItem.pItem;
            }

            if (!itemFound)
            {
              for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
              {
                const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
                vdkProjectNode* pNode = item.pItem;

                vdkProjectNode_MoveChild(pProgramState->activeProject.pProject, item.pParent, pProgramState->sceneExplorer.insertItem.pParent, pNode, pProgramState->sceneExplorer.insertItem.pItem);

                // Update the selected item information to repeat drag and drop
                pProgramState->sceneExplorer.selectedItems[i].pParent = pProgramState->sceneExplorer.insertItem.pParent;

                pProgramState->sceneExplorer.clickedItem = pProgramState->sceneExplorer.selectedItems[i];
              }
            }
            pProgramState->sceneExplorer.insertItem = { nullptr, nullptr };
          }

          size_t i = 0;
          if (pProgramState->activeProject.pFolder)
            pProgramState->activeProject.pFolder->HandleImGui(pProgramState, &i);
        }
        ImGui::EndChild();
      }
      ImGui::End();
    }

    if (pProgramState->settings.window.windowsOpen[vcDocks_Convert] && !pProgramState->settings.window.presentationMode)
    {
      if (ImGui::Begin(udTempStr("%s###convertDock", vcString::Get("convertTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_Convert]))
        vcConvert_ShowUI(pProgramState);

      vcChangeTab(pProgramState, vcDocks_Convert);
      ImGui::End();
    }

    if (pProgramState->settings.window.windowsOpen[vcDocks_Scene])
    {
      if (!pProgramState->settings.window.presentationMode)
      {
        if (ImGui::Begin(udTempStr("%s###sceneDock", vcString::Get("sceneTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
          vcRenderSceneWindow(pProgramState);
        vcChangeTab(pProgramState, vcDocks_Scene);
        ImGui::End();
      }
      else
      {
        ImGui::SetNextWindowSize(size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        bool sceneWindow = ImGui::Begin(udTempStr("%s###scenePresentation", vcString::Get("sceneTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_Scene], ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        if (sceneWindow)
          vcRenderSceneWindow(pProgramState);

        ImGui::End();
      }
    }

    if (pProgramState->settings.window.windowsOpen[vcDocks_Settings] && !pProgramState->settings.window.presentationMode)
      vcSettingsUI_Show(pProgramState);

    if (pProgramState->settings.pActive[0] != nullptr)
    {
      for (int i = 0; i < vcDocks_Count; ++i)
      {
        if (pProgramState->settings.pActive[i] == nullptr)
          break;

        ImGui::SetWindowFocus(pProgramState->settings.pActive[i]->Name);
        pProgramState->settings.pActive[i] = nullptr;
      }
    }

    if (pProgramState->settings.docksLoaded != vcSettings::vcDockLoaded::vcDL_True)
    {
      vcSettings_Load(&pProgramState->settings, false, vcSC_Docks);

      // Don't show the window in a bad state
      ImDrawData *pData = ImGui::GetDrawData();
      if (pData != nullptr)
        pData->Clear();
    }
  }

  vcModals_DrawModals(pProgramState);
}
