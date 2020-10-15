#include "vcState.h"

#include "udContext.h"
#include "udServerAPI.h"
#include "udConfig.h"
#include "udPointCloud.h"
#include "udVersion.h"
#include "udWeb.h"
#include "udUserUtil.h"
#include "udStreamer.h"

#include <chrono>
#include <ctime>

#include "imgui.h"
#include "imgui_internal.h"

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
#include "vcProject.h"
#include "vcSettingsUI.h"
#include "vcSession.h"
#include "vcPOI.h"
#include "vcStringFormat.h"
#include "vcHotkey.h"
#include "vcConstants.h"
#include "vcVerticalMeasureTool.h"
#include "vcQueryNode.h"
#include "vcViewpoint.h"
#include "gl/vcTextureCache.h"

#include "vcGLState.h"
#include "vcFramebuffer.h"
#include "vcSHP.h"

#include "tools/vcSceneTool.h"

#include "parsers/vcUDP.h"
#include "parsers/vc12DXML.h"
#include "parsers/vcCSV.h"

#include "udFile.h"
#include "udStringUtil.h"
#include "udUUID.h"

#include "udPlatformUtil.h"
#include "vcError.h"

#if UDPLATFORM_EMSCRIPTEN
# include "vHTTPRequest.h"
# include <emscripten/threading.h>
# include <emscripten/emscripten.h>
# include <emscripten/html5.h>
#elif UDPLATFORM_WINDOWS
# include <windows.h>
#endif

UDCOMPILEASSERT(UDSDK_MAJOR_VERSION == 2 && UDSDK_MINOR_VERSION == 0, "This version of udSDK is not compatible");

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  include <crtdbg.h>
#  include <stdio.h>

# if BUILDING_UDSDK
#  include "udDLLExport.h"

#  ifdef __cplusplus
extern "C" {
#  endif
  UDSDKDLL_API void udConfig_TrackMemoryBegin();
  UDSDKDLL_API bool udConfig_TrackMemoryEnd();
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
#if BUILDING_UDSDK
  udConfig_TrackMemoryBegin();
#endif

  int ret = main(argc, args);

#if BUILDING_UDSDK
  if (!udConfig_TrackMemoryEnd())
  {
    printf("%s\n", "Memory leaks in udSDK found");

    // You've hit this because you've introduced a memory leak!
    // If you need help, uncomment `defines {"__MEMORY_DEBUG__"}` in the premake5.lua
    // This will emit filenames of what is leaking to assist in tracking down what's leaking.
    // Additionally, you can set _CrtSetBreakAlloc(<allocationNumber>);
    // inside udConfig_TrackMemoryEnd().
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
  vcLBS_LogoAreaSize = 500,
};

const uint32_t WhitePixel = 0xFFFFFFFF;

void vcMain_ShowStartupScreen(vcState *pProgramState);
void vcMain_RenderWindow(vcState *pProgramState);

void vcMain_PresentationMode(vcState *pProgramState)
{
  if (pProgramState->settings.window.isFullscreen)
  {
    SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }
  else
  {
    vcSettings_Save(&pProgramState->settings);
    SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }

  pProgramState->settings.window.isFullscreen = !pProgramState->settings.window.isFullscreen;
}

bool vcMain_TakeScreenshot(vcState *pProgramState)
{
  pProgramState->settings.screenshot.taking = false;

  if (pProgramState == nullptr)
    return false;

  udInt2 currSize = udInt2::zero();
  vcTexture_GetSize(pProgramState->screenshot.pImage, &currSize.x, &currSize.y);

  if (currSize.x - pProgramState->settings.screenshot.resolution.x > 32 || currSize.y - pProgramState->settings.screenshot.resolution.y > 32)
    return true;

  udFindDir* poutputPath = nullptr;
  if (udOpenDir(&poutputPath, pProgramState->settings.screenshot.outputPath) != udR_Success)
  {
    //Folder may not exist, try to create.
    if (udCreateDir(pProgramState->settings.screenshot.outputPath) != udR_Success)
      return false;
  }
  udCloseDir(&poutputPath);

  char buffer[vcMaxPathLength];
  time_t rawtime;
  tm timeval;
  tm *pTime = &timeval;

#if UDPLATFORM_WINDOWS
  time(&rawtime);
  localtime_s(&timeval, &rawtime);
#else
  time(&rawtime);
  pTime = localtime(&rawtime);
#endif

  udSprintf(buffer, "%s/%.4d-%.2d-%.2d-%.2d-%.2d-%.2d.png", pProgramState->settings.screenshot.outputPath, 1900+pTime->tm_year, 1+pTime->tm_mon, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);

  udResult result = vcTexture_SaveImage(pProgramState->screenshot.pImage, vcRender_GetSceneFramebuffer(pProgramState->pActiveViewport->pRenderContext), buffer);

  // This must be run here as vcTexture_SaveImage will change which framebuffer is bound but not reset it
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);

  if (result != udR_Success)
  {
    ErrorItem status;
    status.source = vcES_File;
    status.pData = udStrdup(buffer);
    status.resultCode = result;

    vcError_AddError(pProgramState, status);

    udFileDelete(buffer);
    return false;
  }

  if (pProgramState->settings.screenshot.viewShot)
    pProgramState->pLoadImage = udStrdup(buffer);

  return true;
}

void vcMain_LoadSettings(vcState *pProgramState)
{
  if (vcSettings_Load(&pProgramState->settings))
  {
    udConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);

#if !(UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR)
    SDL_Rect bounds = {};
    SDL_Rect screenPos = { pProgramState->settings.window.xpos, pProgramState->settings.window.ypos, pProgramState->settings.window.width, pProgramState->settings.window.height };
    SDL_Rect overlap = {};

    for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
    {
      if (SDL_GetDisplayBounds(i, &bounds) == 0)
      {
        if (SDL_IntersectRect(&bounds, &screenPos, &overlap) == SDL_TRUE)
        {
          SDL_SetWindowSize(pProgramState->pWindow, pProgramState->settings.window.width, pProgramState->settings.window.height);
          SDL_SetWindowPosition(pProgramState->pWindow, pProgramState->settings.window.xpos, pProgramState->settings.window.ypos);

          if (pProgramState->settings.window.maximized)
            SDL_MaximizeWindow(pProgramState->pWindow);

          break;
        }
      }
    }
#endif
  }
}

#if UDPLATFORM_EMSCRIPTEN
void vcMain_MainLoop(void *pArgs)
{
  static int hideLoadingStatus = 0;
  if (hideLoadingStatus >= 0)
  {
    if (hideLoadingStatus == 2)
    {
      MAIN_THREAD_EM_ASM(
        if (typeof(Module["onMainLoopStart"]) !== 'undefined') {
          Module["onMainLoopStart"]();
        }
      );
      hideLoadingStatus = -1;
    }
    ++hideLoadingStatus;
  }

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
    if (event.type == SDL_KEYDOWN && !(event.key.keysym.scancode >= SDL_SCANCODE_LCTRL && event.key.keysym.scancode <= SDL_SCANCODE_RGUI))
      pProgramState->currentKey = event.key.keysym.scancode;

    if (!pProgramState->hasUsedMouse)
    {
      if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.which != SDL_TOUCH_MOUSEID)
        pProgramState->hasUsedMouse = true;

      // When SDL_WarpMouseInWindow or SDL_WarpMouseGlobal is called on startup, it triggers this event with no motion
      if (event.type == SDL_MOUSEMOTION && event.motion.which != SDL_TOUCH_MOUSEID && (event.motion.x != 0 || event.motion.xrel != 0 || event.motion.y != 0 || event.motion.yrel != 0))
        pProgramState->hasUsedMouse = true;

      if (event.type == SDL_MOUSEWHEEL && event.wheel.which != SDL_TOUCH_MOUSEID)
        pProgramState->hasUsedMouse = true;
    }

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
          if (!pProgramState->settings.window.isFullscreen)
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
        if (event.mgesture.numFingers == 2)
        {
          ImGuiIO &io = ImGui::GetIO();
          io.MousePos.x = event.mgesture.x;
          io.MousePos.y = event.mgesture.y;
          io.MouseWheel += event.mgesture.dDist * 10;
        }
      }
      else if (event.type == SDL_DROPFILE && pProgramState->hasContext)
      {
        pProgramState->loadList.PushBack(udStrdup(event.drop.file));
      }
      else if (event.type == SDL_QUIT)
      {
        if (vcModals_ConfirmEndSession(pProgramState, true))
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

#if !defined(GIT_BUILD) && UDPLATFORM_WINDOWS
  if (pProgramState->hasContext && ImGui::IsKeyPressed(SDL_SCANCODE_P))
  {
    for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
      vcRender_ReloadShaders(pProgramState->pViewports[viewportIndex].pRenderContext, pProgramState->pWorkerPool);
  }
#endif

  ImGuiGL_NewFrame(pProgramState->pWindow);

  vcGizmo_BeginFrame(pProgramState->pActiveViewport->gizmo.pContext);
  vcGLState_ResetState(true);

  vcGLState_SetViewport(0, 0, pProgramState->settings.window.width, pProgramState->settings.window.height);
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFF000000);

  if (pProgramState->finishedStartup)
    vcMain_RenderWindow(pProgramState);
  else
    vcMain_ShowStartupScreen(pProgramState);

  ImGui::Render();

  ImGuiGL_RenderDrawData(ImGui::GetDrawData());

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  vcGLState_Present(pProgramState->pWindow);

  if (ImGui::GetIO().WantSaveIniSettings)
    vcSettings_Save(&pProgramState->settings);

  if (pProgramState->pFontTTFData != nullptr && ImGui::GetIO().Fonts->IsBuilt())
  {
    ImFont *fonts[] = { ImGui::GetDefaultFont(), pProgramState->pMidFont, pProgramState->pBigFont, pProgramState->pItalicFont, pProgramState->pBoldFont, pProgramState->pBoldItalicFont };

    for (ImFont *pFont : fonts)
    {
      if (pFont == nullptr)
        continue;

      if (pFont->AreGlyphsMissing())
      {
        ImFontConfig fontCfg = ImFontConfig();
        fontCfg.FontDataOwnedByAtlas = false;
        fontCfg.MergeMode = true;
        fontCfg.DstFont = pFont;

        int len = pFont->MissingGlyphs().Size;

        ImWchar *pMissingGlyphsRanges = udAllocType(ImWchar, len * 2 + 1, udAF_Zero);
        pProgramState->requiredGlyphs.PushBack(pMissingGlyphsRanges);

        for (int i = 0; i < len; ++i)
        {
          pMissingGlyphsRanges[i * 2 + 0] = pFont->MissingGlyphs()[i];
          pMissingGlyphsRanges[i * 2 + 1] = pMissingGlyphsRanges[i * 2 + 0] + 1;
          if (pMissingGlyphsRanges[i * 2] == 65535)
            __debugbreak();
        }

        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pProgramState->pFontTTFData, pProgramState->fontTTFDataLen, pFont->FontSize, &fontCfg, pMissingGlyphsRanges);

        pFont->ResetMissingGlyphs();
      }
    }
  }

  if (pProgramState->finishedStartup)
  {
    udWorkerPool_DoPostWork(pProgramState->pWorkerPool, 8);
    vcTextureCache_ConditionallyPruneGlobal();
  }
  else
    pProgramState->finishedStartup = ((udWorkerPool_DoPostWork(pProgramState->pWorkerPool, 1) == udR_NothingToDo) && !udWorkerPool_HasActiveWorkers(pProgramState->pWorkerPool));

  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[SDL_SCANCODE_BACKSPACE] = false;
  io.KeysDown[SDL_SCANCODE_PRINTSCREEN] = false;

  if (vcHotkey::IsPressed(vcB_OpenSettingsMenu))
    pProgramState->openSettings = true;

  if (vcHotkey::IsPressed(vcB_BindingsInterface))
  {
    pProgramState->activeSetting = vcSR_KeyBindings;
    pProgramState->openSettings = true;
  }

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
#if VC_HASCONVERT
        bool convertDrop = false;
        //TODO: Use ImGui drag and drop on the docks rather than globally here
        ImGuiWindow *pConvert = ImGui::FindWindowByName("###convertDock");
        if (pConvert != nullptr && pConvert->Active && ((pConvert->DockNode != nullptr && pConvert->DockTabIsVisible) || (pConvert->DockNode == nullptr && !pConvert->Collapsed)))
        {
          if (io.MousePos.x < pConvert->Pos.x + pConvert->Size.x && io.MousePos.x > pConvert->Pos.x && io.MousePos.y > pConvert->Pos.y && io.MousePos.y < pConvert->Pos.y + pConvert->Size.y)
            convertDrop = true;
        }
#endif //VC_HASCONVERT


        udFilename loadFile(pNextLoad);
        const char *pExt = loadFile.GetExt();

        // Project Files
        if (udStrBeginsWith(pNextLoad, "euclideon:project/"))
        {
          vcProject_LoadFromServer(pProgramState, &pNextLoad[18]);
          vcModals_CloseModal(pProgramState, vcMT_Welcome);
        }
        else if (udStrEquali(pExt, ".json"))
        {
          vcProject_LoadFromURI(pProgramState, pNextLoad);
          vcModals_CloseModal(pProgramState, vcMT_Welcome);
        }
        else if (udStrEquali(pExt, ".udp"))
        {
          vcProject_CreateBlankScene(pProgramState, "UDP Import", vcPSZ_StandardGeoJSON);
          vcModals_CloseModal(pProgramState, vcMT_Welcome);

          vcUDP_Load(pProgramState, pNextLoad);
        }
        else if (udStrEquali(pExt, ".12dxml"))
        {
          vcProject_CreateBlankScene(pProgramState, "12DXML Import", vcPSZ_StandardGeoJSON);
          vcModals_CloseModal(pProgramState, vcMT_Welcome);

          if (vc12DXML_Load(pProgramState, pNextLoad) == udR_Unsupported)
            vcModals_OpenModal(pProgramState, vcMT_UnsupportedEncoding);
        }
        else if (udStrEquali(pExt, ".shp"))
        {
          vcModals_CloseModal(pProgramState, vcMT_Welcome);
          vcSHP_Load(pProgramState, pNextLoad);
        }
#if VC_HASCONVERT
        else if (convertDrop) // Everything else depends on where it was dropped
        {
          vcConvert_QueueFile(pProgramState, pNextLoad);
        }
#endif // VC_HASCONVERT
        else // We need to add it to the scene (hopefully)
        {
          udProjectNode *pNode = nullptr;

          if (udStrEquali(pExt, ".uds") || udStrBeginsWithi(pExt, ".uds?")) // Handle URLs with query params
          {
            if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "UDS", nullptr, pNextLoad, nullptr) != udE_Success)
            {
              ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              vcError_AddError(pProgramState, projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else
            {
              if (firstLoad) // Was successful
                udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);

              vcModals_CloseModal(pProgramState, vcMT_Welcome);

              // Let is know about the mouse position- using bounding box currently
              //TODO: Don't use the boundingBox
              if (pProgramState->pActiveViewport->pickingSuccess)
              {
                pNode->boundingBox[0] = pProgramState->pActiveViewport->worldMousePosCartesian.x;
                pNode->boundingBox[1] = pProgramState->pActiveViewport->worldMousePosCartesian.y;
                pNode->boundingBox[2] = pProgramState->pActiveViewport->worldMousePosCartesian.z;
              }
              else
              {
                pNode->boundingBox[0] = pProgramState->pActiveViewport->camera.position.x;
                pNode->boundingBox[1] = pProgramState->pActiveViewport->camera.position.y;
                pNode->boundingBox[2] = pProgramState->pActiveViewport->camera.position.z;
              }
            }
          }
          else if (udStrEquali(pExt, ".vsm") || udStrEquali(pExt, ".obj"))
          {
            if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Polygon", nullptr, pNextLoad, nullptr) != udE_Success)
            {
              ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              vcError_AddError(pProgramState, projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else // Was successful
            {
              vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, pProgramState->pActiveViewport->pickingSuccess ? &pProgramState->pActiveViewport->worldMousePosCartesian : &pProgramState->pActiveViewport->camera.position, 1);

              if (firstLoad)
                udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);

              vcModals_CloseModal(pProgramState, vcMT_Welcome);
            }
          }
          else if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg") || udStrEquali(pExt, ".png") || udStrEquali(pExt, ".tga") || udStrEquali(pExt, ".bmp") || udStrEquali(pExt, ".gif") || udStrEquali(pExt, ".tif") || udStrEquali(pExt, ".tiff"))
          {
            const vcSceneItemRef &clicked = pProgramState->sceneExplorer.clickedItem;
            if (clicked.pParent != nullptr && clicked.pItem->itemtype == udPNT_Media)
            {
              udProjectNode_SetURI(pProgramState->activeProject.pProject, clicked.pItem, pNextLoad);
            }
            else
            {
              udDouble3 geolocationLatLong = udDouble3::zero();
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
                      geolocationLatLong.x = exifResult.GeoLocation.Latitude;
                      geolocationLatLong.y = exifResult.GeoLocation.Longitude;
                      geolocationLatLong.z = exifResult.GeoLocation.Altitude;
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

              if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Media", loadFile.GetFilenameWithExt(), pNextLoad, nullptr) == udE_Success)
              {
                if (hasLocation && pProgramState->geozone.projection != udGZPT_Unknown)
                  vcProject_UpdateNodeGeometryFromLatLong(&pProgramState->activeProject, pNode, udPGT_Point, &geolocationLatLong, 1);
                else
                  vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, pProgramState->pActiveViewport->pickingSuccess ? &pProgramState->pActiveViewport->worldMousePosCartesian : &pProgramState->pActiveViewport->camera.position, 1);

                if (imageType == vcIT_PhotoSphere)
                  udProjectNode_SetMetadataString(pNode, "imagetype", "photosphere");
                else if (imageType == vcIT_Panorama)
                  udProjectNode_SetMetadataString(pNode, "imagetype", "panorama");
                else
                  udProjectNode_SetMetadataString(pNode, "imagetype", "standard");
              }
            }
          }
          else if (udStrEquali(pExt, ".slpk"))
          {
            if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "I3S", loadFile.GetFilenameWithExt(), pNextLoad, nullptr) != udE_Success)
            {
              ErrorItem projectError;
              projectError.source = vcES_ProjectChange;
              projectError.pData = pNextLoad; // this takes ownership so we don't need to dup or free
              projectError.resultCode = udR_ReadFailure;

              pNextLoad = nullptr;

              vcError_AddError(pProgramState, projectError);

              vcModals_OpenModal(pProgramState, vcMT_ProjectChange);

              continue;
            }
            else if (firstLoad) // Was successful
            {
              udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
              vcModals_CloseModal(pProgramState, vcMT_Welcome);
            }
          }
          else if (udStrEquali(pExt, ".csv"))
          {
            vcModals_CloseModal(pProgramState, vcMT_Welcome);

            udStrcpy(pProgramState->modelPath, loadFile);

            vcModals_OpenModal(pProgramState, vcMT_ImportAnnotations);
          }
          else // This file isn't supported in the scene
          {
            ErrorItem status;
            status.source = vcES_File;
            status.pData = pNextLoad; // this takes ownership so we don't need to dup or free
            status.resultCode = udR_Unsupported;

            pNextLoad = nullptr;

            vcError_AddError(pProgramState, status);

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
        vcImageData tempData = {};
        uint8_t *pPixels = vcTexture_DecodePixels(pFileData, (size_t)fileLen, &tempData);

        pProgramState->image.width = tempData.width;
        pProgramState->image.height = tempData.height;

        vcTexture_Create(&pProgramState->image.pImage, tempData.width, tempData.height, pPixels);

        udFree(pPixels);

        pProgramState->image.width = pProgramState->settings.viewports[0].resolution.x;
        pProgramState->image.height = pProgramState->settings.viewports[0].resolution.y;
      }

      udFree(pFileData);

      vcModals_OpenModal(pProgramState, vcMT_ImageViewer);

      udFree(pProgramState->pLoadImage);
    }

    // Check this first because vcSession_UpdateInfo doesn't update session info if the session has expired
    if (pProgramState->forceLogout)
      vcSession_Logout(pProgramState);
    else if (pProgramState->hasContext && (pProgramState->lastSync != 0.0 && udGetEpochSecsUTCf() > pProgramState->lastSync + 30)) // Ping the server every 30 seconds
      udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_UpdateInfo, pProgramState, false);
  }
}

#if UDPLATFORM_EMSCRIPTEN
void vcMain_GetScreenResolution(vcState * pProgramState)
{
  pProgramState->windowResolution.x = EM_ASM_INT_V({
    return window.innerWidth;
  });
  pProgramState->windowResolution.y = EM_ASM_INT_V({
    return window.innerHeight;
  });
}

struct vcMainSyncFSData
{
  void (*pCallback)(void *);
  udSemaphore *pSema;
};

void vcMainSyncFSFinished(void *pDataPtr)
{
  vcMainSyncFSData *pData = (vcMainSyncFSData *)pDataPtr;
  udIncrementSemaphore(pData->pSema);
}

void vcMain_SyncFS()
{
  vcMainSyncFSData data = {};
  data.pSema = udCreateSemaphore();
  data.pCallback = vcMainSyncFSFinished;

  MAIN_THREAD_EM_ASM_INT({
    // Sync from persisted state into memory
    FS.syncfs(true, function(err) {
      var FinishCallback = Atomics.load(HEAPU32, $0 >> 2);
      dynCall_vi(FinishCallback, $0);
    });
  }, &data);

  udWaitSemaphore(data.pSema);
  udDestroySemaphore(&data.pSema);
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

void vcMain_AsyncLoad(vcState *pProgramState, const char *pFilename, udWorkerPoolCallback &&mainThreadFn)
{
  vcMainLoadDataInfo *pInfo = udAllocType(vcMainLoadDataInfo, 1, udAF_Zero);
  pInfo->pFilename = udStrdup(pFilename);
  pInfo->pProgramState = pProgramState;
  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcMain_AsyncLoadWT, pInfo, true, mainThreadFn);
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

void vcMain_LoadFontItalicsBoldMT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;

  if (pLoadInfo->loadResult == udR_Success)
  {
    ImFont **ppFont = nullptr;

    if (udStrEndsWith(pLoadInfo->pFilename, "BoldItalic.ttf"))
      ppFont = &pLoadInfo->pProgramState->pBoldItalicFont;
    else if (udStrEndsWith(pLoadInfo->pFilename, "Italic.ttf"))
      ppFont = &pLoadInfo->pProgramState->pItalicFont;
    else
      ppFont = &pLoadInfo->pProgramState->pBoldFont;

    ImFontConfig fontCfg = ImFontConfig();
    fontCfg.FontDataOwnedByAtlas = false;
    fontCfg.MergeMode = false;

    static ImWchar latinCharacterRanges[] =
    {
      0x0020, 0x00FF, // Basic Latin + Latin Supplement
      0
    };

    *ppFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, ImGui::GetDefaultFont()->FontSize, &fontCfg, latinCharacterRanges);
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

    const float BigFontSize = 36.f;
    ImFontConfig bigFontCfg = ImFontConfig();
    bigFontCfg.FontDataOwnedByAtlas = false;
    bigFontCfg.MergeMode = false;

    const float MidFontSize = 26.f;
    ImFontConfig midFontCfg = ImFontConfig();
    midFontCfg.FontDataOwnedByAtlas = false;
    midFontCfg.MergeMode = false;

    static ImWchar basicCharacterRanges[] =
    {
      0x0020, 0x00FF, // Basic Latin + Latin Supplement
      0x2010, 0x205E, // Punctuations
      0x25A0, 0x25FF, // Geometric Shapes
      0x26A0, 0x26A1, // Exclamation in Triangle
      0x2713, 0x2714, // Checkmark
      0
    };

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg, basicCharacterRanges);
    pLoadInfo->pProgramState->pBigFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, BigFontSize, &bigFontCfg, basicCharacterRanges);

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, FontSize, &fontCfg, basicCharacterRanges);
    pLoadInfo->pProgramState->pMidFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pLoadInfo->pData, (int)pLoadInfo->dataLen, MidFontSize, &midFontCfg, basicCharacterRanges);
  }

  pLoadInfo->pProgramState->requiredGlyphs.Init(32);
  pLoadInfo->pProgramState->pFontTTFData = pLoadInfo->pData;
  pLoadInfo->pProgramState->fontTTFDataLen = (int)pLoadInfo->dataLen;

  vcMain_AsyncLoad(pLoadInfo->pProgramState, "asset://assets/data/NotoSans-Italic.ttf", vcMain_LoadFontItalicsBoldMT);
  vcMain_AsyncLoad(pLoadInfo->pProgramState, "asset://assets/data/NotoSans-Bold.ttf", vcMain_LoadFontItalicsBoldMT);
  vcMain_AsyncLoad(pLoadInfo->pProgramState, "asset://assets/data/NotoSans-BoldItalic.ttf", vcMain_LoadFontItalicsBoldMT);

  udFree(pLoadInfo->pFilename);
}

void vcMain_LoadStringTableMT(void *pLoadInfoPtr)
{
  vcMainLoadDataInfo *pLoadInfo = (vcMainLoadDataInfo*)pLoadInfoPtr;

  vcString::LoadTableFromMemory((const char*)pLoadInfo->pData, &pLoadInfo->pProgramState->languageInfo);

  udFree(pLoadInfo->pData);
  udFree(pLoadInfo->pFilename);
}

void vcMain_AsyncResumeSession(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState *)pProgramStatePtr;
  vcSession_Resume(pProgramState);
}

void vcMain_SetWindowName(vcState *pProgramState)
{
  const char *strings[] = { UDSTRINGIFY(VCVERSION_VERSION_ARRAY_PARTIAL), UDSTRINGIFY(VCVERSION_BUILD_NUMBER) };

  const char *pWindowName = vcStringFormat(pProgramState->branding.appName, strings, udLengthOf(strings));

  if (pWindowName != nullptr)
  {
    SDL_SetWindowTitle(pProgramState->pWindow, pWindowName);
    udFree(pWindowName);
  }
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

  HANDLE appMutex = CreateMutex(NULL, FALSE, L"udStreamApplicationMutex");
#endif //UDPLATFORM_WINDOWS

#if UDPLATFORM_EMSCRIPTEN
  vcMain_SyncFS();
  {
    char *pPath = SDL_GetPrefPath("", "");
    udConfig_SetConfigLocation(pPath);
    SDL_free(pPath);
  }
#endif

  uint32_t windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_ANDROID
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  vcState programState = {};

#if UDPLATFORM_EMSCRIPTEN
  vHTTPRequest_StartWorkerThread();
#endif

  vcSettings_RegisterAssetFileHandler();
  vcWebFile_RegisterFileHandlers();

  // default values
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_ANDROID
  // TODO: Query device and fill screen
  programState.windowResolution.x = 1920;
  programState.windowResolution.y = 1080;
  programState.settings.onScreenControls = true;
#elif UDPLATFORM_EMSCRIPTEN
  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, vcMain_GetScreenResolution, &programState);
#else
  programState.windowResolution.x = 1280;
  programState.windowResolution.y = 720;
  programState.settings.onScreenControls = false;
#endif

  for (int v = 0; v < vcMaxViewportCount; v++)
  {
    programState.settings.camera.moveSpeed[v] = 3.f;
    programState.settings.camera.fieldOfView[v] = UD_PIf * 5.f / 18.f; // 50 degrees
  }
  programState.settings.camera.lockAltitude = false;
  programState.settings.camera.nearPlane = s_CameraNearPlane;
  programState.settings.camera.farPlane = s_CameraFarPlane;

  programState.settings.languageOptions.Init(4);
  programState.settings.visualization.pointSourceID.colourMap.Init(32);
  programState.settings.projectsHistory.projects.Init(32);

  programState.passwordFieldHasFocus = true;
  programState.renaming = -1;
  programState.settings.screenshot.taking = false;

  programState.sceneExplorer.insertItem.pParent = nullptr;
  programState.sceneExplorer.insertItem.pItem = nullptr;
  programState.sceneExplorer.clickedItem.pParent = nullptr;
  programState.sceneExplorer.clickedItem.pItem = nullptr;

  programState.errorItems.Init(16);
  programState.loadList.Init(16);

  programState.showWatermark = true;

  programState.previousSRID = -1;

  programState.pSessionLock = udCreateRWLock();

  programState.pViewports = udAllocType(vcViewport, vcMaxViewportCount, udAF_Zero);
  programState.activeViewportIndex = 0;
  programState.pActiveViewport = &programState.pViewports[programState.activeViewportIndex];

  bool tryDomain = false;

  for (int i = 1; i < argc; ++i)
  {
    if (udStrBeginsWith(args[i], "--"))
    {
      if (udStrEquali(args[i], "--domain"))
      {
        tryDomain = true;
        continue;
      }
    }

#if UDPLATFORM_OSX
    // macOS assigns a unique process serial number (PSN) to all apps,
    // we should not attempt to load this as a file.
    if (!udStrBeginsWithi(args[i], "-psn"))
#endif
      programState.loadList.PushBack(udStrdup(args[i]));
  }

  udWorkerPool_Create(&programState.pWorkerPool, 4, "udStreamWorker");
  vcConvert_Init(&programState);

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    goto epilogue;

#if GRAPHICS_API_OPENGL
  windowFlags |= SDL_WINDOW_OPENGL;

  // Setup window
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN || UDPLATFORM_ANDROID
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

  programState.pWindow = ImGui_ImplSDL2_CreateWindow("udStream", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.windowResolution.x, programState.windowResolution.y, windowFlags);
  if (!programState.pWindow)
    goto epilogue;

  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 0.0f;

  // These get loaded on the main thread
  vcMain_LoadSettings(&programState);
  vcSettings_LoadBranding(&programState);

  vcMain_SetWindowName(&programState);

#if UDPLATFORM_EMSCRIPTEN
  // This needs to be here because the settings will load with the incorrect resolution (1280x720)
  programState.settings.window.width = (int)programState.windowResolution.x;
  programState.settings.window.height = (int)programState.windowResolution.y;
#endif

  if (!vcGLState_Init(programState.pWindow, &programState.pDefaultFramebuffer))
    goto epilogue;

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  if (!ImGuiGL_Init(programState.pWindow))
    goto epilogue;

  for (int viewportIndex = 0; viewportIndex < vcMaxViewportCount; ++viewportIndex)
  {
    vcGizmo_Create(&programState.pViewports[viewportIndex].gizmo.pContext);

    if (vcRender_Init(&programState, &programState.pViewports[viewportIndex].pRenderContext, programState.pWorkerPool, programState.settings.viewports[viewportIndex].resolution) != udR_Success)
      goto epilogue;
  }

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  vcFramebuffer_Bind(programState.pDefaultFramebuffer);
  vcTextureCache_InitGlobal(programState.pWorkerPool);

  SDL_DisableScreenSaver();

  // Async load everything else
  vcMain_AsyncLoad(&programState, udTempStr("asset://assets/lang/%s.json", programState.settings.window.languageCode), vcMain_LoadStringTableMT);
  vcMain_AsyncLoad(&programState, "asset://assets/branding/icon.png", vcMain_LoadIconMT);
  vcMain_AsyncLoad(&programState, "asset://assets/data/NotoSansCJK-Regular.ttc", vcMain_LoadFontMT);

  vcTexture_AsyncCreateFromFilename(&programState.pUITexture, programState.pWorkerPool, "asset://assets/textures/uiDark24.png", vcTFM_Linear);

  vcTexture_Create(&programState.pWhiteTexture, 1, 1, &WhitePixel);

  vcProject_CreateBlankScene(&programState, "Empty Project", vcPSZ_StandardGeoJSON);

  if (!tryDomain)
    udWorkerPool_AddTask(programState.pWorkerPool, vcMain_AsyncResumeSession, &programState, false);
  else
    udWorkerPool_AddTask(programState.pWorkerPool, vcSession_Domain, &programState, false);

#if UDPLATFORM_EMSCRIPTEN
  // Toggle fullscreen if it changed, most likely via pressing escape key
  emscripten_set_fullscreenchange_callback("#canvas", &programState, true, [](int /*eventType*/, const EmscriptenFullscreenChangeEvent *pEvent, void *pUserData) -> EM_BOOL {
    vcState *pProgramState = (vcState*)pUserData;

    if (!pEvent->isFullscreen && pProgramState->settings.window.isFullscreen)
      vcMain_PresentationMode(pProgramState);

    return true;
  });

  emscripten_set_main_loop_arg(vcMain_MainLoop, &programState, 0, 1);
#else
  while (!programState.programComplete)
    vcMain_MainLoop(&programState);
#endif

  if (!programState.openSettings || programState.activeSetting != vcSR_KeyBindings)
    vcSettings_Save(&programState.settings);

epilogue:
  udFree(programState.pReleaseNotes);
  vcSession_CleanupSession(&programState);
  programState.profileInfo.Destroy();

  vcSettingsUI_Cleanup(&programState);
  vcSettings_Cleanup(&programState.settings);

  ImGuiGL_DestroyDeviceObjects();
  ImGui::DestroyContext();

  vcConvert_Deinit(&programState);
  vcTexture_Destroy(&programState.pUITexture);
  vcTexture_Destroy(&programState.pWhiteTexture);
  vcCSV_Destroy(&programState.pImportAnnotationsContext);

  for (size_t i = 0; i < programState.loadList.length; i++)
    udFree(programState.loadList[i]);
  programState.loadList.Deinit();

  for (size_t i = 0; i < programState.errorItems.length; i++)
    udFree(programState.errorItems[i].pData);
  programState.errorItems.Deinit();

  udWorkerPool_Destroy(&programState.pWorkerPool); // This needs to occur before logout
  vcProject_Deinit(&programState, &programState.activeProject); // This needs to be destroyed before the renderer is shutdown
  for (int viewportIndex = 0; viewportIndex < vcMaxViewportCount; ++viewportIndex)
  {
    vcGizmo_Destroy(&programState.pViewports[viewportIndex].gizmo.pContext);
    vcRender_Destroy(&programState, &programState.pViewports[viewportIndex].pRenderContext);
  }
  programState.settings.activeViewportCount = 0;
  programState.pActiveViewport = nullptr;
  udFree(programState.pViewports);
  vcString::FreeTable(&programState.languageInfo);
  vcSession_Logout(&programState);

  vcProject_Deinit(&programState, &programState.activeProject); // deinit again
  vcTexture_Destroy(&programState.image.pImage);
  vcTextureCache_DeinitGlobal();

  for (ImWchar *pGlyphs : programState.requiredGlyphs)
    udFree(pGlyphs);
  programState.requiredGlyphs.Deinit();
  udFree(programState.pFontTTFData);

  udDestroyRWLock(&programState.pSessionLock);

  vcGLState_Deinit();
  udThread_DestroyCached();

#if UDPLATFORM_EMSCRIPTEN
  vHTTPRequest_ShutdownWorkerThread();
#endif

#if UDPLATFORM_WINDOWS
  CloseHandle(appMutex);
#endif

  return 0;
}

void vcMain_ProfileMenu(vcState *pProgramState)
{
  if (ImGui::BeginPopupContextItem("profileMenu", 0))
  {
    if (!pProgramState->sessionInfo.isDomain && !pProgramState->sessionInfo.isOffline)
    {
      if (ImGui::MenuItem(vcString::Get("modalProfileTitle")))
        vcModals_OpenModal(pProgramState, vcMT_Profile);

      if (ImGui::MenuItem(vcString::Get("modalChangePasswordTitle")))
        vcModals_OpenModal(pProgramState, vcMT_ChangePassword);
    }
    
    if (ImGui::MenuItem(vcString::Get("menuLogout")) && vcModals_ConfirmEndSession(pProgramState, false))
      pProgramState->forceLogout = true;

    ImGui::Separator();

    if (ImGui::MenuItem(vcString::Get("menuShowWelcome")))
      vcModals_OpenModal(pProgramState, vcMT_Welcome);

    if (ImGui::MenuItem(vcString::Get("menuProjectExport"), nullptr, nullptr))
      vcModals_OpenModal(pProgramState, vcMT_ExportProject);

    ImGui::Separator();

    udReadLockRWLock(pProgramState->pSessionLock);
    if (pProgramState->groups.length > 0)
    {
      for (auto group : pProgramState->groups)
      {
        if (ImGui::BeginMenu(udTempStr("%s##group_%s", group.pGroupName, udUUID_GetAsString(group.groupID))))
        {
          if (group.projects.length > 0)
          {
            for (auto project : group.projects)
            {
              if (ImGui::MenuItem(project.pProjectName, nullptr, nullptr) && vcProject_AbleToChange(pProgramState))
                vcProject_LoadFromServer(pProgramState, udUUID_GetAsString(project.projectID));
            }
          }
          else
          {
            ImGui::MenuItem(vcString::Get("menuProjectNone"), nullptr, nullptr, false);
          }

          ImGui::EndMenu();
        }
      }
    }
    else // No projects
    {
      ImGui::MenuItem(vcString::Get("menuProjectNone"), nullptr, nullptr, false);
    }
    udReadUnlockRWLock(pProgramState->pSessionLock);

    ImGui::EndPopup();
  }
}

void vcMain_ToggleViewport(vcState *pProgramState)
{
  if (pProgramState->settings.activeViewportCount == 1)
  {
    pProgramState->settings.activeViewportCount = 2;

    memcpy(&pProgramState->pViewports[1].camera, &pProgramState->pViewports[0].camera, sizeof(vcCamera));
    pProgramState->pViewports[1].gizmo.inUse = pProgramState->pViewports[0].gizmo.inUse;
    pProgramState->pViewports[1].gizmo.coordinateSystem = pProgramState->pViewports[0].gizmo.coordinateSystem;
    pProgramState->pViewports[1].gizmo.operation = pProgramState->pViewports[0].gizmo.operation;
    memcpy(&pProgramState->pViewports[1].gizmo.direction, &pProgramState->pViewports[0].gizmo.direction, sizeof(pProgramState->pViewports[0].gizmo.direction));
    vcRender_ClearTiles(pProgramState->pViewports[1].pRenderContext);

    //TODO: Is this necessary?
    vcRender_SetVaultContext(pProgramState, pProgramState->pViewports[1].pRenderContext);
  }
  else if (pProgramState->settings.activeViewportCount == 2)
  {
    pProgramState->settings.activeViewportCount = 1;

    // TODO: Is this necessary?
    vcRender_RemoveVaultContext(pProgramState->pViewports[1].pRenderContext);
  }
}

void vcMain_ToggleGizmoOperation(vcState *pProgramState, vcGizmoOperation op)
{
  pProgramState->activeTool = vcActiveTool_Select;
  for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
  {
    vcViewport *pViewport = &pProgramState->pViewports[viewportIndex];

    // TODO: Leaving this commented until viewport UI is implemented
    //if (pProgramState->focusedViewportIndex != viewportIndex)
    //  continue;

    pViewport->gizmo.operation = pViewport->gizmo.operation == op ? vcGO_NoGizmo : op;
  }
}

void vcMain_DisplayViewpoints(vcState *pProgramState, udProjectNode *pNode)
{
  if (pNode->pFirstChild)
    vcMain_DisplayViewpoints(pProgramState, pNode->pFirstChild);

  if (pNode->itemtype == udPNT_Viewpoint)
  {
    if (ImGui::MenuItem(pNode->pName, nullptr, (pProgramState->activeProject.pSlideshowViewpoint == pNode)))
    {
      pProgramState->activeProject.pSlideshowViewpoint = pNode;

      vcSceneItem *pSceneItem = (vcSceneItem *)pNode->pUserData;
      if (pSceneItem->m_pPreferredProjection != nullptr)
        vcProject_UseProjectionFromItem(pProgramState, pSceneItem);
      else
        pSceneItem->SetCameraPosition(pProgramState);
    }
  }

  if (pNode->pNextSibling)
    vcMain_DisplayViewpoints(pProgramState, pNode->pNextSibling);
}

void vcRenderSceneUI(vcState *pProgramState, const ImVec2 &windowPos, const ImVec2 &windowSize, udDouble3 *pCameraMoveOffset)
{
  ImGuiIO &io = ImGui::GetIO();

  float attachmentPanelSize = 0.f;
  const float panelPadding = 5.f;

  if (pProgramState->activeProject.slideshow)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y), ImGuiCond_Always, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, 300.f)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("slideshowWindow", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      if (ImGui::BeginCombo(vcString::Get("settingsViewport"), pProgramState->activeProject.pSlideshowViewpoint ? pProgramState->activeProject.pSlideshowViewpoint->pName : ""))
      {
        vcMain_DisplayViewpoints(pProgramState, pProgramState->activeProject.pFolder->m_pNode);
        ImGui::EndCombo();
      }

      if (pProgramState->activeProject.pSlideshowViewpoint)
      {
        const char *pDescription = nullptr;
        udProjectNode_GetMetadataString(pProgramState->activeProject.pSlideshowViewpoint, "description", &pDescription, nullptr);
        if (!udStrEqual(pDescription, ""))
          vcIGSW_Markdown(pProgramState, pDescription);
      }
    }

    attachmentPanelSize = ImGui::GetWindowSize().y + panelPadding;
    ImGui::End();
  }

  if (pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem != nullptr && !udStrEqual(pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem->m_pNode->itemtypeStr, "FlyPath"))
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y), ImGuiCond_Always, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("exitAttachedModeWindow", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      const char *pStr = vcStringFormat(vcString::Get("sceneCameraAttachmentWarning"), pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem->m_pNode->pName);
      ImGui::TextUnformatted(pStr);
      udFree(pStr);

      pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem->HandleAttachmentUI(pProgramState);

      if (ImGui::Button(vcString::Get("sceneCameraAttachmentDetach"), ImVec2(-1, 0)))
        pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = nullptr;
    }

    attachmentPanelSize = ImGui::GetWindowSize().y + panelPadding;
    ImGui::End();
  }

  bool showToolWindow = false;
  showToolWindow |= (pProgramState->activeTool != vcActiveTool_Select || pProgramState->sceneExplorer.selectedItems.size() > 0);
  showToolWindow |= pProgramState->pActiveViewport->gizmo.inUse;
  showToolWindow |= (pProgramState->backgroundWork.exportsRunning.Get() > 0);

#if VC_HASCONVERT
  const char *pConvertInfoBuffer = nullptr;
  int convertProgress = vcConvert_CurrentProgressPercent(pProgramState, &pConvertInfoBuffer);
  showToolWindow |= ((convertProgress >= 0) || pConvertInfoBuffer != nullptr);
#endif

  if (showToolWindow)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x, windowPos.y + attachmentPanelSize), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, windowSize.y / 2.0f)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("###toolInfoPanel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      // Gizmo Settings
      if (pProgramState->pActiveViewport->gizmo.inUse)
      {
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoTranslate"), vcB_GizmoTranslate, vcMBBI_Translate, vcMBBG_FirstItem, pProgramState->pActiveViewport->gizmo.operation == vcGO_Translate))
          vcMain_ToggleGizmoOperation(pProgramState, vcGO_Translate);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoRotate"), vcB_GizmoRotate, vcMBBI_Rotate, vcMBBG_SameGroup, pProgramState->pActiveViewport->gizmo.operation == vcGO_Rotate))
          vcMain_ToggleGizmoOperation(pProgramState, vcGO_Rotate);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoScale"), vcB_GizmoScale, vcMBBI_Scale, vcMBBG_SameGroup, pProgramState->pActiveViewport->gizmo.operation == vcGO_Scale))
          vcMain_ToggleGizmoOperation(pProgramState, vcGO_Scale);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoLocalSpace"), vcB_GizmoLocalSpace, vcMBBI_UseLocalSpace, vcMBBG_SameGroup, pProgramState->pActiveViewport->gizmo.coordinateSystem == vcGCS_Local))
        {
          for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
            pProgramState->pViewports[viewportIndex].gizmo.coordinateSystem = (pProgramState->pViewports[viewportIndex].gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;
        }

        ImGui::Separator();
      }

      // Background work
      {
#if VC_HASCONVERT
        if (convertProgress >= 0)
        {
          bool clicked = false;

          ImGui::ProgressBar(convertProgress / 100.f, ImVec2(200, 0), pConvertInfoBuffer);
          clicked |= ImGui::IsItemClicked();

          ImGui::SameLine();
          ImGui::Text("%s", vcString::Get("convertTitle"));
          clicked |= ImGui::IsItemClicked();

          if (clicked)
            vcModals_OpenModal(pProgramState, vcMT_Convert);
        }
        else if (pConvertInfoBuffer != nullptr)
        {
          ImGui::Text("%s: %s", vcString::Get("convertTitle"), pConvertInfoBuffer);

          if (ImGui::IsItemClicked())
            vcModals_OpenModal(pProgramState, vcMT_Convert);
        }
        udFree(pConvertInfoBuffer);
#endif //VC_HASCONVERT

        if (pProgramState->backgroundWork.exportsRunning.Get() > 0)
        {
          vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading, "sceneExplorerExporting");
          ImGui::TextUnformatted(vcString::Get("sceneExplorerExportRunning"));
        }

        if (pProgramState->activeTool != vcActiveTool_Select)
          ImGui::Separator();
      }

      {
        if (pProgramState->activeTool != vcActiveTool_Select && !pProgramState->modalOpen && vcHotkey::IsPressed(vcB_Cancel))
        {
          vcSceneTool::tools[pProgramState->activeTool]->OnCancel(pProgramState);
          pProgramState->activeTool = vcActiveTool_Select;
        }

        vcSceneTool::tools[pProgramState->activeTool]->SceneUI(pProgramState);
      }
    }

    ImGui::End();
  }

  // Scene Info
  bool showInfoPanel = false;

  showInfoPanel |= pProgramState->settings.presentation.showCameraInfo;
  showInfoPanel |= pProgramState->settings.presentation.showProjectionInfo;
  showInfoPanel |= pProgramState->settings.presentation.showAdvancedGIS;
  showInfoPanel |= pProgramState->settings.presentation.showDiagnosticInfo;
  showInfoPanel |= pProgramState->settings.activeViewportCount > 1;

  if (showInfoPanel)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + 32.f, windowPos.y + 32.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin(vcString::Get("sceneCameraSettings"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking))
    {
      udDouble3 cameraLatLong = udDouble3::zero();

      if (pProgramState->settings.presentation.showCameraInfo)
      {
        if (ImGui::InputScalarN(vcString::Get("sceneCameraPosition"), ImGuiDataType_Double, &pProgramState->pActiveViewport->camera.position.x, 3))
        {
          // limit the manual entry of camera position to +/- 40000000
          pProgramState->pActiveViewport->camera.position.x = udClamp(pProgramState->pActiveViewport->camera.position.x, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->pActiveViewport->camera.position.y = udClamp(pProgramState->pActiveViewport->camera.position.y, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->pActiveViewport->camera.position.z = udClamp(pProgramState->pActiveViewport->camera.position.z, -vcSL_GlobalLimit, vcSL_GlobalLimit);
        }

        if (pProgramState->geozone.projection != udGZPT_Unknown)
        {
          cameraLatLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->pActiveViewport->camera.position);
          if (ImGui::InputScalarN(vcString::Get("sceneCameraPositionGIS"), ImGuiDataType_Double, &cameraLatLong.x, 3))
            pProgramState->pActiveViewport->camera.position = udGeoZone_LatLongToCartesian(pProgramState->geozone, cameraLatLong);
        }

        udDouble2 headingPitch = UD_RAD2DEG(pProgramState->pActiveViewport->camera.headingPitch);
        if (ImGui::InputScalarN(vcString::Get("sceneCameraRotation"), ImGuiDataType_Double, &headingPitch.x, 2, nullptr, nullptr, "%.2f"))
          pProgramState->pActiveViewport->camera.headingPitch = UD_DEG2RAD(headingPitch);

        for (int v = 0; v < vcMaxViewportCount; v++)
        {
          if (v < pProgramState->settings.activeViewportCount)
          {
            if (ImGui::SliderFloat(udTempStr("%s %d", vcString::Get("sceneCameraMoveSpeed"), v), &(pProgramState->settings.camera.moveSpeed[v]), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", ImGuiSliderFlags_Logarithmic))
              pProgramState->settings.camera.moveSpeed[v] = udClamp(pProgramState->settings.camera.moveSpeed[v], vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
          }
        }

        if (pProgramState->geozone.latLongBoundMin != pProgramState->geozone.latLongBoundMax)
        {
          ImGui::Separator();

          udDouble2 &minBound = pProgramState->geozone.latLongBoundMin;
          udDouble2 &maxBound = pProgramState->geozone.latLongBoundMax;

          if (cameraLatLong.x < minBound.x || cameraLatLong.y < minBound.y || cameraLatLong.x > maxBound.x || cameraLatLong.y > maxBound.y)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", vcString::Get("sceneCameraOutOfBounds"));
        }
      }

      if (pProgramState->settings.presentation.showProjectionInfo)
      {
        ImGui::Text("%s (%s: %d)", pProgramState->geozone.displayName, vcString::Get("sceneSRID"), pProgramState->geozone.srid);

        ImGui::Separator();
        if (ImGui::IsMousePosValid())
        {
          if (pProgramState->pActiveViewport->pickingSuccess)
          {
            ImGui::Text("%s: %.2f, %.2f, %.2f", vcString::Get("sceneMousePointInfo"), pProgramState->pActiveViewport->worldMousePosCartesian.x, pProgramState->pActiveViewport->worldMousePosCartesian.y, pProgramState->pActiveViewport->worldMousePosCartesian.z);

            if (pProgramState->geozone.projection != udGZPT_Unknown)
              ImGui::Text("%s: %.6f, %.6f", vcString::Get("sceneMousePointWGS"), pProgramState->pActiveViewport->worldMousePosLongLat.y, pProgramState->pActiveViewport->worldMousePosLongLat.x);
          }
        }
      }

      if (pProgramState->settings.presentation.showAdvancedGIS)
      {
        int newSRID = pProgramState->geozone.srid;
        udGeoZone zone;

        if (ImGui::InputInt(vcString::Get("sceneOverrideSRID"), &newSRID) && udGeoZone_SetFromSRID(&zone, newSRID) == udR_Success)
        {
          bool success = false;
          for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
            success = vcGIS_ChangeSpace(&pProgramState->geozone, zone, &pProgramState->pViewports[viewportIndex].camera.position, (viewportIndex + 1 == pProgramState->settings.activeViewportCount)) || success;

          if (success)
            pProgramState->activeProject.pFolder->ChangeProjection(zone);

        }
      }

      if (pProgramState->settings.presentation.showDiagnosticInfo)
      {
        char tempData[128] = {};

        // Load List
        if (pProgramState->loadList.length > 0)
        {
          const char *strings[] = { udTempStr("%zu", pProgramState->loadList.length) };

          vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
          vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFilesQueued"), strings, udLengthOf(strings));
          ImGui::TextUnformatted(tempData);
        }

        // Error List
        size_t fileCount = 0;
        for (auto errorItem : pProgramState->errorItems)
        {
          if (errorItem.source == vcES_File)
            ++fileCount;
        }
        if (fileCount > 0)
        {
          bool isHovered = false;
          bool isClicked = false;

          const char *strings[] = { udTempStr("%zu", fileCount) };
          vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFilesFailed"), strings, udLengthOf(strings));
          ImGui::TextUnformatted(tempData);

          isHovered = ImGui::IsItemHovered();
          isClicked = ImGui::IsItemClicked();

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
            vcModals_OpenModal(pProgramState, vcMT_ErrorInformation);
        }

        // Diagnostic Information
        if (pProgramState->settings.presentation.showDiagnosticInfo)
        {
          const char *strings[] = { udTempStr("%.2f", 1.f / pProgramState->deltaTime), udTempStr("%.3f", pProgramState->deltaTime * 1000.f) };
          vcStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFPS"), strings, udLengthOf(strings));
          ImGui::TextUnformatted(tempData);
          ImGui::TextUnformatted(udTempStr("%sMiB", udCommaInt(pProgramState->streamingMemory >> 20)));
        }
      }
    }

    ImGui::End();
  }

  // Attribution
  {
    udProjectNode *pNode = pProgramState->activeProject.pRoot;
    const char *pBuffer = udStrdup("Euclideon");

    if (pProgramState->settings.maptiles.mapEnabled && pProgramState->geozone.projection != udGZPT_Unknown)
    {
      for (int mapLayer = 0; mapLayer < pProgramState->settings.maptiles.activeLayerCount; ++mapLayer)
      {
        if (pProgramState->settings.maptiles.layers[mapLayer].activeServer.attribution[0] != '\0')
          udSprintf(&pBuffer, "%s, %s", pBuffer, pProgramState->settings.maptiles.layers[mapLayer].activeServer.attribution);
      }

      if (pProgramState->settings.maptiles.demEnabled)
        udSprintf(&pBuffer, "%s, NOAA, GEBCO", pBuffer);
    }

    vcProject_ExtractAttributionText(pNode, &pBuffer);

    if (pBuffer != nullptr)
    {
      ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(0.f, 1.f));
      ImGui::SetNextWindowBgAlpha(0.5f);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

      bool open = ImGui::Begin("attributionText", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking);

      ImGui::PopStyleVar(3);

      if (open)
        ImGui::TextUnformatted(pBuffer);
      ImGui::End();
      udFree(pBuffer);
    }
  }

  // On Screen Controls Overlay
  if (pProgramState->settings.onScreenControls)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x - 40, windowPos.y + windowSize.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("OnScrnControls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      ImGui::SetWindowSize(ImVec2(165, 120));

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
    }

    ImGui::End();
  }

  static float fadeVal = 0.f;
  if (pProgramState->isStreaming || fadeVal > 0)
  {
    static float rotAmount = 0;
    rotAmount += (float)pProgramState->deltaTime;

    if (pProgramState->isStreaming)
      fadeVal = udMin(1.f, fadeVal + (float)pProgramState->deltaTime);
    else
      fadeVal = udMax(0.f, fadeVal - (float)pProgramState->deltaTime);

    float a = udSin(rotAmount * UD_PIf) * 15.f;
    float b = udCos(rotAmount * UD_PIf) * 15.f;

    float x = windowPos.x + windowSize.x - 40.f - 15.f - (pProgramState->settings.onScreenControls ? 165 : 0);
    float y = windowPos.y + windowSize.y - 15.f;

    ImVec2 p[] = {
      { x - b + a, y - a - b },
      { x + b + a, y + a - b },
      { x + b - a, y + a + b },
      { x - b - a, y - a + b }
    };

    ImVec2 uv[] = {
      { 0.75f, 0.75f },
      { 1.00f, 0.75f },
      { 1.00f, 1.00f },
      { 0.75f, 1.00f }
    };

    ImGui::SetNextWindowPos(ImVec2(x + 30.f, y + 30.f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::SetNextWindowSize(ImVec2(60, 60));

    if (ImGui::Begin("StreamingIcon", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      ImGui::GetWindowDrawList()->AddImageQuad(pProgramState->pUITexture, p[0], p[1], p[2], p[3], uv[0], uv[1], uv[2], uv[3], ((int)(0xFF * fadeVal) << 24) | 0xFFFFFF);

      if (ImGui::IsWindowHovered())
        ImGui::SetTooltip("%s", vcString::Get("sceneStreaming"));
    }
    ImGui::End();

    ImGui::PopStyleVar();

    pProgramState->isStreaming = false;
  }


  // Tool Panel
  {
    ImVec2 buttonPanelPos = ImVec2(windowPos.x + 2.f, windowPos.y + 2.f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowPos(buttonPanelPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.f);

    bool panelOpen = ImGui::Begin("toolPanel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PopStyleVar(2);

    if (panelOpen)
    {
      // Avatar Profile
      vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneProfileMenu"), vcB_Invalid, vcMBBI_Burger, vcMBBG_FirstItem);
      vcMain_ProfileMenu(pProgramState);

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuNewScene"), vcB_Invalid, vcMBBI_NewProject, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_CreateProject);

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuProjectImport"), vcB_Invalid, vcMBBI_Open, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_LoadProject);

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuProjectSave"), vcB_Invalid, vcMBBI_Save, vcMBBG_SameGroup))
      {
        udProjectLoadSource loadSource = udProjectLoadSource_Memory;
        if (udProject_GetLoadSource(pProgramState->activeProject.pProject, &loadSource) == udE_Success)
        {
          if (loadSource == udProjectLoadSource_Memory)
            vcModals_OpenModal(pProgramState, vcMT_ExportProject);
          else
            vcProject_Save(pProgramState);
        }
      }

      vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuProjectShare"), vcB_Invalid, vcMBBI_Share, vcMBBG_SameGroup);
      if (ImGui::BeginPopupContextItem("##shareSettingsPopup", 0))
      {
        static int messagePlace = -1;
        static char shareLinkBrowser[vcMaxPathLength] = {};
        static char shareLinkApp[vcMaxPathLength] = {};
        static bool foundProject = false;
        static bool isPublic = false;
        static bool isShared = false;
        static bool isSharable = false;
        static const char *pUUID = nullptr;

        if (ImGui::IsWindowAppearing())
        {
          messagePlace = -1;

          if (udProject_GetProjectUUID(pProgramState->activeProject.pProject, &pUUID) == udE_Success)
          {
            udSprintf(shareLinkBrowser, "%s/client/?f=euclideon:project/%s", pProgramState->settings.loginInfo.serverURL, pUUID);
            udSprintf(shareLinkApp, "euclideon:project/%s", pUUID);

            foundProject = false;
            isSharable = false;
            isShared = false;
            isPublic = false;

            udReadLockRWLock(pProgramState->pSessionLock);

            if (pProgramState->groups.length > 0)
            {
              for (vcGroupInfo &group : pProgramState->groups)
              {
                if (!foundProject && group.projects.length > 0)
                {
                  for (vcProjectInfo &project : group.projects)
                  {
                    if (udStrEqual(udUUID_GetAsString(project.projectID), pUUID))
                    {
                      foundProject = true;
                      isSharable = (group.permissionLevel >= vcGroupPermissions_Editor);
                      isShared = project.isShared;
                      isPublic = (group.visibility < vcGroupVisibility_Private);
                      break;
                    }
                  }
                }
              }
            }

            udReadUnlockRWLock(pProgramState->pSessionLock);
          }
          else
          {
            messagePlace = -2;
          }
        }

        if (messagePlace > -2)
        {
          ImGui::TextUnformatted(vcString::Get("shareInstructions"));

          ImGui::Separator();

          struct
          {
            const char *pLabel;
            char *pBuffer;
            const char *pCopyText;
          } shareOptions[] = {
            { vcString::Get("shareLinkBrowser"), shareLinkBrowser, vcString::Get("shareCopyBrowserLink") },
            { vcString::Get("shareLinkApp"), shareLinkApp, vcString::Get("shareCopyAppCode") }
          };

          for (size_t i = 0; i < udLengthOf(shareOptions); ++i)
          {
            ImGui::PushID(udTempStr("shareItem%zu", i));

            ImGui::InputText(shareOptions[i].pLabel, shareOptions[i].pBuffer, vcMaxPathLength, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
            ImGui::SameLine();
            if (vcMenuBarButton(pProgramState->pUITexture, shareOptions[i].pCopyText, vcB_Invalid, vcMBBI_Layers, vcMBBG_FirstItem, false, (20.f/24.f)))
            {
              if (SDL_SetClipboardText(shareOptions[i].pBuffer) == 0)
                messagePlace = (int)i;
            }

            if (messagePlace == (int)i)
            {
              ImGui::SameLine();
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Success);
              ImGui::TextUnformatted(vcString::Get("shareLinkCopied"));
            }

            ImGui::PopID();
          }

          ImGui::Separator();

          if (!foundProject)
            ImGui::TextUnformatted(vcString::Get("shareStatusUnknown"));
          else if (isPublic)
            ImGui::TextUnformatted(vcString::Get("shareStatusPublic"));
          else if (isShared)
            ImGui::TextUnformatted(vcString::Get("shareStatusShared"));
          else
            ImGui::TextUnformatted(vcString::Get("shareStatusNotShared"));

          if (foundProject && isSharable && !isPublic)
          {
            ImGui::SameLine();

            if (isShared)
            {
              if (ImGui::Button(vcString::Get("shareMakeUnshare")))
              {
                if (udProject_SetLinkShareStatus(pProgramState->pUDSDKContext, pUUID, false) == udE_Success)
                  isShared = false;
                else
                  isSharable = false;
              }
            }
            else
            {
              if (ImGui::Button(vcString::Get("shareMakeShare")))
              {
                if (udProject_SetLinkShareStatus(pProgramState->pUDSDKContext, pUUID, true) == udE_Success)
                  isShared = true;
                else
                  isSharable = false;
              }
            }
          }
        }
        else
        {
          ImGui::TextUnformatted(vcString::Get("shareNotPossible"));
        }

        ImGui::EndPopup();
      }

#if VC_HASCONVERT
      if (pProgramState->branding.convertEnabled && vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuConvert"), vcB_Invalid, vcMBBI_Convert, vcMBBG_SameGroup))
        vcModals_OpenModal(pProgramState, vcMT_Convert);
#endif //VC_HASCONVERT

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuSettings"), vcB_OpenSettingsMenu, vcMBBI_Settings, vcMBBG_SameGroup))
        pProgramState->openSettings = true;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuToggleViewport"), vcB_Invalid, vcMBBI_SplitViewport, vcMBBG_SameGroup))
        vcMain_ToggleViewport(pProgramState);

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuHelp"), vcB_Invalid, vcMBBI_Help, vcMBBG_SameGroup))
        vcWebFile_OpenBrowser("https://desk.euclideon.com/");

      if (pProgramState->lastSuccessfulSave + 5.0 > udGetEpochSecsUTCf())
      {
        ImGui::SameLine();
        vcIGSW_ShowLoadStatusIndicator(vcSLS_Success);
        ImGui::SameLine();
        ImGui::TextUnformatted(vcString::Get("menuProjectSaved"));
      }
    }

    ImGui::End();

    buttonPanelPos.y += 32.f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowPos(buttonPanelPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.f);

    panelOpen = ImGui::Begin("toolPanel2", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PopStyleVar(2);

    if (panelOpen)
    {
      // Hide/show screen explorer
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toggleSceneExplorer"), vcB_ToggleSceneExplorer, vcMBBI_Layers, vcMBBG_FirstItem, !pProgramState->settings.presentation.sceneExplorerCollapsed) || (vcHotkey::IsPressed(vcB_ToggleSceneExplorer) && !ImGui::IsAnyItemActive()))
      {
        pProgramState->settings.presentation.sceneExplorerCollapsed = !pProgramState->settings.presentation.sceneExplorerCollapsed;
        pProgramState->settings.presentation.columnSizeCorrect = false;
      }

      // Activate Select Tool
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolSelect"), vcB_ToggleSelectTool, vcMBBI_Select, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_Select)) || (vcHotkey::IsPressed(vcB_ToggleSelectTool) && !ImGui::IsAnyItemActive()))
        pProgramState->activeTool = vcActiveTool_Select;

      // Activate Voxel Inspector
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolInspect"), vcB_ToggleInspectionTool, vcMBBI_Inspect, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_Inspect)) || (vcHotkey::IsPressed(vcB_ToggleInspectionTool) && !ImGui::IsAnyItemActive()))
        pProgramState->activeTool = vcActiveTool_Inspect;

      // Activate Annotate
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolAnnotate"), vcB_ToggleAnnotateTool, vcMBBI_AddPointOfInterest, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_Annotate)) || (vcHotkey::IsPressed(vcB_ToggleAnnotateTool) && !ImGui::IsAnyItemActive()))
      {
        vcProject_ClearSelection(pProgramState);
        pProgramState->activeTool = vcActiveTool_Annotate;
      }

      // Activate Measure Line
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolMeasureLine"), vcB_ToggleMeasureLineTool, vcMBBI_MeasureLine, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_MeasureLine)) || (vcHotkey::IsPressed(vcB_ToggleMeasureLineTool) && !ImGui::IsAnyItemActive()))
      {
        vcProject_ClearSelection(pProgramState);
        pProgramState->activeTool = vcActiveTool_MeasureLine;
      }

      // Activate Measure Area
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolMeasureArea"), vcB_ToggleMeasureAreaTool, vcMBBI_MeasureArea, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_MeasureArea)) || (vcHotkey::IsPressed(vcB_ToggleMeasureAreaTool) && !ImGui::IsAnyItemActive()))
      {
        vcProject_ClearSelection(pProgramState);
        pProgramState->activeTool = vcActiveTool_MeasureArea;
      }

      // Activate Measure Height
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolMeasureHeight"), vcB_ToggleMeasureHeightTool, vcMBBI_MeasureHeight, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_MeasureHeight)) || (vcHotkey::IsPressed(vcB_ToggleMeasureHeightTool) && !ImGui::IsAnyItemActive()))
      {
        vcProject_ClearSelection(pProgramState);
        pProgramState->activeTool = vcActiveTool_MeasureHeight;
      }

      // Activate Angle Measurement Tool
      /*
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("toolMeasureAngle"), vcB_ToggleMeasureHeightTool, vcMBBI_AngleTool, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_MeasureHeight)) || (vcHotkey::IsPressed(vcB_ToggleMeasureHeightTool) && !ImGui::IsAnyItemActive()))
      {
        vcProject_ClearSelection(pProgramState);
        pProgramState->activeTool = vcActiveTool_MeasureHeight;
      }
      */

      if (pProgramState->branding.filtersEnabled)
      {
        // Add Filter box
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneAddFilterBox"), vcB_AddBoxFilter, vcMBBI_AddBoxFilter, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_AddBoxFilter)) || (vcHotkey::IsPressed(vcB_AddBoxFilter) && !ImGui::IsAnyItemActive()))
        {
          vcProject_ClearSelection(pProgramState);
          pProgramState->activeTool = vcActiveTool_AddBoxFilter;
        }

        // Add Filter Sphere
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneAddFilterSphere"), vcB_AddSphereFilter, vcMBBI_AddSphereFilter, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_AddSphereFilter)) || (vcHotkey::IsPressed(vcB_AddSphereFilter) && !ImGui::IsAnyItemActive()))
        {
          vcProject_ClearSelection(pProgramState);
          pProgramState->activeTool = vcActiveTool_AddSphereFilter;
        }

        // Add Filter Cylinder
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneAddFilterCylinder"), vcB_AddCylinderFilter, vcMBBI_AddCylinderFilter, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_AddCylinderFilter)) || (vcHotkey::IsPressed(vcB_AddCylinderFilter) && !ImGui::IsAnyItemActive()))
        {
          vcProject_ClearSelection(pProgramState);
          pProgramState->activeTool = vcActiveTool_AddCylinderFilter;
        }

        // Add Filter Cross Section
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneAddCrossSection"), vcB_AddSimpleCrossSection, vcMBBI_AddCrossSectionFilter, vcMBBG_FirstItem, (pProgramState->activeTool == vcActiveTool_AddSimpleCrossSection)) || (vcHotkey::IsPressed(vcB_AddSimpleCrossSection) && !ImGui::IsAnyItemActive()))
        {
          vcProject_ClearSelection(pProgramState);
          pProgramState->activeTool = vcActiveTool_AddSimpleCrossSection;
        }
      }
    }

    ImGui::End();
  }

  // Scene Control Panel
  {
    ImVec2 buttonPanelPos = ImVec2(windowPos.x + windowSize.x - 2.f, windowPos.y + windowSize.y - 2.f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowPos(buttonPanelPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.f);

    bool panelOpen = ImGui::Begin("controlPanel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PopStyleVar(2);

    if (panelOpen)
    {
      // Compass
      {
        udDouble3 cameraDirection = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, pProgramState->pActiveViewport->camera.headingPitch).apply({ 0, 1, 0 });

        udDouble3 up = vcGIS_GetWorldLocalUp(pProgramState->geozone, pProgramState->pActiveViewport->camera.position);
        udDouble3 northDir = vcGIS_GetWorldLocalNorth(pProgramState->geozone, pProgramState->pActiveViewport->camera.position);

        // if cameraDirection is directly opposite of worldUp (mapmode) we just set camRight Vector to 0 so compass doesn't spin around randomly
        udDouble3 camRight = udDot(cameraDirection, up) < (-1.0 + UD_EPSILON) ? udDouble3::zero() : udCross(cameraDirection, up);
        udDouble3 camFlat = udCross(up, camRight);

        double angle = udATan2(camFlat.x * northDir.y - camFlat.y * northDir.x, camFlat.x * northDir.x + camFlat.y * northDir.y);
        float northX = -(float)udSin(angle);
        float northY = -(float)udCos(angle);

        ImGui::PushID("compassButton");
        if (ImGui::ButtonEx("", ImVec2(28, 28)))
        {
          pProgramState->pActiveViewport->cameraInput.startAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, pProgramState->pActiveViewport->camera.headingPitch);
          pProgramState->pActiveViewport->cameraInput.targetAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, udDouble2::create(0, pProgramState->pActiveViewport->camera.headingPitch.y));
          pProgramState->pActiveViewport->cameraInput.inputState = vcCIS_Rotate;
          pProgramState->pActiveViewport->cameraInput.progress = 0.0;
        }
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(vcString::Get("sceneTrueNorthTooltip"));
          ImGui::EndTooltip();
        }
        ImGui::PopID();

        ImVec2 sizeMin = ImGui::GetItemRectMin();
        ImVec2 sizeMax = ImGui::GetItemRectMax();
        float distance = (sizeMax.x - sizeMin.x) / 2.f;
        ImVec2 middle = ImVec2((sizeMin.x + sizeMax.x) / 2, (sizeMin.y + sizeMax.y) / 2);
        ImVec2 north = ImVec2(middle.x + northX * distance, middle.y + northY * distance);
        ImVec2 south = ImVec2(middle.x - northX * distance, middle.y - northY * distance);
        ImGui::GetWindowDrawList()->AddLine(middle, north, 0xFF0000FF, 2);
        ImGui::GetWindowDrawList()->AddLine(middle, south, 0xFFFFFFFF, 2);
      }

      // Lock Altitude
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneLockAltitude"), vcB_LockAltitude, vcMBBI_LockAltitude, vcMBBG_FirstItem, pProgramState->settings.camera.lockAltitude))
        pProgramState->settings.camera.lockAltitude = !pProgramState->settings.camera.lockAltitude;

      // Map Settings
      vcMenuBarButton(pProgramState->pUITexture, vcString::Get("mapSettings"), vcB_Invalid, vcMBBI_MapMode, vcMBBG_FirstItem);
      if (ImGui::BeginPopupContextItem("##mapSettingsPopup", 0))
      {
        vcSettingsUI_BasicMapSettings(pProgramState, true);

        ImGui::EndPopup();
      }

      // Visualizations
      vcMenuBarButton(pProgramState->pUITexture, vcString::Get("settingsVis"), vcB_Invalid, vcMBBI_Visualization, vcMBBG_FirstItem);
      if (ImGui::BeginPopupContextItem("##visualizationsPopup", 0))
      {
        if (ImGui::Button(vcString::Get("settingsRestoreDefaults")))
          vcSettings_Load(&pProgramState->settings, true, vcSC_Visualization);
        ImGui::SameLine();
        ImGui::TextUnformatted(vcString::Get("settingsVis"));

        ImGui::Separator();

        ImGui::BeginChild("visualistationSelection", ImVec2(500, 400));
        vcSettingsUI_SceneVisualizationSettings(pProgramState);
        ImGui::EndChild();

        ImGui::EndPopup();
      }

#if !UDPLATFORM_EMSCRIPTEN
      // Fullscreens - needs to trigger on mouse down, not mouse up in Emscripten to avoid problems
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneFullscreen"), vcB_Fullscreen, vcMBBI_FullScreen, vcMBBG_FirstItem, pProgramState->settings.window.isFullscreen) || ImGui::IsItemClicked(0))
        vcMain_PresentationMode(pProgramState);
#endif
    }

    ImGui::End();
  }
}

void vcRenderScene_HandlePicking(vcState *pProgramState, vcRenderData &renderData, bool useTool)
{
  const double farPlaneDist = pProgramState->settings.camera.farPlane * pProgramState->settings.camera.farPlane;

  // We have to resolve UD vs. Polygon
  bool selectUD = pProgramState->pActiveViewport->pickingSuccess && (pProgramState->pActiveViewport->udModelPickedIndex != -1); // UD was successfully picked (last frame)
  double udDist = (selectUD ? udMagSq3(pProgramState->pActiveViewport->worldMousePosCartesian - pProgramState->pActiveViewport->camera.position) : farPlaneDist);

  bool getResultsImmediately = useTool || ImGui::IsMouseClicked(0, false) || ImGui::IsMouseClicked(1, false) || ImGui::IsMouseClicked(2, false);
  vcRenderPickResult pickResult = vcRender_PolygonPick(pProgramState, pProgramState->pActiveViewport->pRenderContext, renderData, getResultsImmediately);

  bool selectPolygons = pickResult.success;
  double polyDist = (selectPolygons ? udMagSq3(pickResult.position - pProgramState->pActiveViewport->camera.position) : farPlaneDist);

  // resolve pick
  selectUD = selectUD && (udDist < polyDist);
  selectPolygons = selectPolygons && (polyDist < udDist);

  if (selectPolygons)
  {
    pProgramState->pActiveViewport->pickingSuccess = true;
    pProgramState->pActiveViewport->udModelPickedIndex = -1;
    pProgramState->pActiveViewport->worldMousePosCartesian = pickResult.position;
  }

  if (!pickResult.success && !selectPolygons && !selectUD)
    pProgramState->pActiveViewport->pickingSuccess = false;

  if (useTool)
  {
    if (!pProgramState->pActiveViewport->isUsingAnchorPoint)
    {
      pProgramState->pActiveViewport->isUsingAnchorPoint = true;
      pProgramState->pActiveViewport->worldAnchorPoint = pProgramState->pActiveViewport->worldMousePosCartesian;
    }

    vcSceneTool::tools[pProgramState->activeTool]->HandlePicking(pProgramState, renderData, pickResult);
  }
  else //Not 'using' tool but might need to 'preview' the tool
  {
    vcSceneTool::tools[pProgramState->activeTool]->PreviewPicking(pProgramState, renderData, pickResult);
  }
}

void vcMain_ShowSceneExplorerWindow(vcState *pProgramState)
{
  if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddModel"), vcB_AddUDS, vcMBBI_AddPointCloud, vcMBBG_FirstItem) || vcHotkey::IsPressed(vcB_AddUDS))
    vcModals_OpenModal(pProgramState, vcMT_AddSceneItem);

  if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddFolder"), vcB_Invalid, vcMBBI_AddFolder, vcMBBG_SameGroup))
  {
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Folder", vcString::Get("sceneExplorerFolderDefaultName"), nullptr, nullptr) != udE_Success)
    {
      ErrorItem projectError = {};
      projectError.source = vcES_ProjectChange;
      projectError.pData = udStrdup(vcString::Get("sceneExplorerAddFolder"));
      projectError.resultCode = udR_Failure_;

      vcError_AddError(pProgramState, projectError);

      vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
    }
  }

  if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddViewpoint"), vcB_Invalid, vcMBBI_SaveViewport, vcMBBG_SameGroup))
  {
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Camera", vcString::Get("viewpointDefaultName"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->camera.position, 1);

      udProjectNode_SetMetadataDouble(pNode, "transform.heading", pProgramState->pActiveViewport->camera.headingPitch.x);
      udProjectNode_SetMetadataDouble(pNode, "transform.pitch", pProgramState->pActiveViewport->camera.headingPitch.y);
    }
    else
    {
      ErrorItem projectError = {};
      projectError.source = vcES_ProjectChange;
      projectError.pData = udStrdup(vcString::Get("sceneExplorerAddViewpoint"));
      projectError.resultCode = udR_Failure_;

      vcError_AddError(pProgramState, projectError);

      vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
    }
  }

  if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddViewpointWithVisSetting"), vcB_Invalid, vcMBBI_SaveViewportWithVis, vcMBBG_SameGroup))
  {
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "Camera", vcString::Get("viewpointDefaultName"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->camera.position, 1);

      udProjectNode_SetMetadataDouble(pNode, "transform.heading", pProgramState->pActiveViewport->camera.headingPitch.x);
      udProjectNode_SetMetadataDouble(pNode, "transform.pitch", pProgramState->pActiveViewport->camera.headingPitch.y);

      vcViewpoint::SaveSettings(pProgramState, pNode);
    }
    else
    {
      ErrorItem projectError = {};
      projectError.source = vcES_ProjectChange;
      projectError.pData = udStrdup(vcString::Get("sceneExplorerAddViewpointWithVisSetting"));
      projectError.resultCode = udR_Failure_;

      vcError_AddError(pProgramState, projectError);

      vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
    }
  }

  vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddOther"), vcB_Invalid, vcMBBI_AddOther, vcMBBG_SameGroup);
  if (ImGui::BeginPopupContextItem(vcString::Get("sceneExplorerAddOther"), 0))
  {
    if (ImGui::MenuItem(vcString::Get("sceneExplorerAddFeed"), nullptr, nullptr))
    {
      udProjectNode *pNode = nullptr;
      if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "IOT", vcString::Get("liveFeedDefaultName"), nullptr, nullptr) != udE_Success)
      {
        ErrorItem projectError;
        projectError.source = vcES_ProjectChange;
        projectError.pData = udStrdup(vcString::Get("sceneExplorerAddFeed"));
        projectError.resultCode = udR_Failure_;

        vcError_AddError(pProgramState, projectError);

        vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
      }
    }

    if (ImGui::MenuItem(vcString::Get("sceneExplorerAddFlythrough"), nullptr, nullptr))
    {
      udProjectNode *pNode = nullptr;
      if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "FlyPath", vcString::Get("flythroughDefaultName"), nullptr, nullptr) != udE_Success)
      {
        ErrorItem projectError;
        projectError.source = vcES_ProjectChange;
        projectError.pData = udStrdup(vcString::Get("sceneExplorerAddFlythrough"));
        projectError.resultCode = udR_Failure_;

        vcError_AddError(pProgramState, projectError);

        vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
      }
      else
      {
        udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
      }
    }
    if (ImGui::MenuItem(vcString::Get("sceneExplorerAddShapeFile"), nullptr, nullptr))
      vcModals_OpenModal(pProgramState, vcMT_ImportShapeFile);

    ImGui::EndPopup();
  }

  if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerRemove"), vcB_Remove, vcMBBI_Remove, vcMBBG_NewGroup) || (vcHotkey::IsPressed(vcB_Remove) && !ImGui::IsAnyItemActive()))
    vcProject_RemoveSelected(pProgramState);

  // Tree view for the scene
  ImGui::Separator();

  if (ImGui::BeginChild("SceneExplorerList", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
  {
    if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("menuProjectSettingsTitle"), vcB_Invalid, vcMBBI_ProjectSettings, vcMBBG_FirstItem))
      vcModals_OpenModal(pProgramState, vcMT_ProjectSettings);

    ImGui::SameLine();
    ImGui::TextWrapped("%s", pProgramState->activeProject.pRoot->pName);

    ImGui::Separator();

    if (!ImGui::IsMouseDragging(0) && pProgramState->sceneExplorer.insertItem.pParent != nullptr)
    {
      // Ensure a circular reference is not created
      bool itemFound = false;

      for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size() && !itemFound; ++i)
      {
        const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
        if (item.pItem->itemtype == udPNT_Folder)
          itemFound = vcProject_ContainsItem(item.pItem, pProgramState->sceneExplorer.insertItem.pParent);

        itemFound = itemFound || item.pItem == pProgramState->sceneExplorer.insertItem.pItem;
      }

      if (!itemFound)
      {
        for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
        {
          const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
          udProjectNode* pNode = item.pItem;

          udProjectNode_MoveChild(pProgramState->activeProject.pProject, item.pParent, pProgramState->sceneExplorer.insertItem.pParent, pNode, pProgramState->sceneExplorer.insertItem.pItem);

          // Update the selected item information to repeat drag and drop
          pProgramState->sceneExplorer.selectedItems[i].pParent = pProgramState->sceneExplorer.insertItem.pParent;

          pProgramState->sceneExplorer.clickedItem = pProgramState->sceneExplorer.selectedItems[i];
        }
      }
      pProgramState->sceneExplorer.insertItem = { nullptr, nullptr };
    }

    size_t i = 0;
    if (pProgramState->activeProject.pFolder)
      pProgramState->activeProject.pFolder->HandleSceneExplorerUI(pProgramState, &i);
  }
  ImGui::EndChild();
}

void vcRenderGizmo(vcState *pProgramState, const ImVec2 &viewportPosition, const udUInt2 &viewportResolution)
{
  ImGuiIO &io = ImGui::GetIO();

  bool couldOpen = true;
  if (pProgramState->modalOpen)
    couldOpen = false;
  ImGuiWindow *pSetting = ImGui::FindWindowByName("###settingsDock");
  if (pSetting != nullptr && (pSetting->Active || pSetting->WasActive))
    couldOpen = false;

  pProgramState->pActiveViewport->gizmo.inUse = false;
  if (couldOpen && pProgramState->sceneExplorer.clickedItem.pParent && pProgramState->sceneExplorer.clickedItem.pItem && !pProgramState->modalOpen)
  {
    vcGizmo_SetRect(pProgramState->pActiveViewport->gizmo.pContext, viewportPosition.x, viewportPosition.y, (float)viewportResolution.x, (float)viewportResolution.y);
    vcGizmo_SetDrawList(pProgramState->pActiveViewport->gizmo.pContext);

    vcSceneItemRef clickedItemRef = pProgramState->sceneExplorer.clickedItem;
    vcSceneItem *pItem = (vcSceneItem*)clickedItemRef.pItem->pUserData;

    if (pItem != nullptr)
    {
      pProgramState->pActiveViewport->gizmo.inUse = true;

      udDouble4x4 temp = pItem->GetWorldSpaceMatrix();
      temp.axis.t.toVector3() = pItem->GetWorldSpacePivot();

      udDouble4x4 delta = udDouble4x4::identity();

      double snapAmt = 0.1;

      if (pProgramState->pActiveViewport->gizmo.operation == vcGO_Rotate)
        snapAmt = 15.0;
      else if (pProgramState->pActiveViewport->gizmo.operation == vcGO_Translate)
        snapAmt = 0.25;

      vcGizmoAllowedControls allowedControls = vcGAC_All;
      for (vcSceneItemRef &ref : pProgramState->sceneExplorer.selectedItems)
        allowedControls = (vcGizmoAllowedControls)(allowedControls & ((vcSceneItem*)ref.pItem->pUserData)->GetAllowedControls());

      //read direction axes again.
      if (pProgramState->pActiveViewport->gizmo.operation == vcGO_Scale || pProgramState->pActiveViewport->gizmo.coordinateSystem == vcGCS_Local)
      {
        pProgramState->pActiveViewport->gizmo.direction[0] = udDouble3::create(1, 0, 0);
        pProgramState->pActiveViewport->gizmo.direction[1] = udDouble3::create(0, 1, 0);
        pProgramState->pActiveViewport->gizmo.direction[2] = udDouble3::create(0, 0, 1);
      }
      else
      {
        vcGIS_GetOrthonormalBasis(pProgramState->geozone, pItem->GetWorldSpacePivot(), &pProgramState->pActiveViewport->gizmo.direction[2], &pProgramState->pActiveViewport->gizmo.direction[1], &pProgramState->pActiveViewport->gizmo.direction[0]);
      }

      vcGizmo_Manipulate(pProgramState->pActiveViewport->gizmo.pContext, &pProgramState->pActiveViewport->camera, pProgramState->pActiveViewport->gizmo.direction, pProgramState->pActiveViewport->gizmo.operation, pProgramState->pActiveViewport->gizmo.coordinateSystem, temp, &delta, allowedControls, io.KeyShift ? snapAmt : 0.0);

      if (!(delta == udDouble4x4::identity()))
      {
        for (vcSceneItemRef &ref : pProgramState->sceneExplorer.selectedItems)
          ((vcSceneItem*)ref.pItem->pUserData)->ApplyDelta(pProgramState, delta);
      }
    }
  }
  else
  {
    vcGizmo_ResetState(pProgramState->pActiveViewport->gizmo.pContext);
  }
}

void vcMain_RenderSceneWindow(vcState *pProgramState)
{
  static int wasViewportContextMenuOpenLastFrame = -1;
  static bool wasViewportContextMenuOnPickable = false;

  //Rendering
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 windowSize = ImGui::GetContentRegionAvail();
  ImVec2 windowPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y);

  if (windowSize.x < 1 || windowSize.y < 1)
    return;

#if !UDPLATFORM_EMSCRIPTEN
  if (vcHotkey::IsPressed(vcB_Fullscreen) || ImGui::IsNavInputTest(ImGuiNavInput_TweakFast, ImGuiInputReadMode_Released))
    vcMain_PresentationMode(pProgramState);
#endif

  // TODO: Screenshot only viewport 0
  if (pProgramState->settings.screenshot.taking && pProgramState->settings.viewports[0].resolution != pProgramState->settings.screenshot.resolution)
  {
    pProgramState->settings.viewports[0].resolution = pProgramState->settings.screenshot.resolution;

    vcRender_ResizeScene(pProgramState, pProgramState->pActiveViewport->pRenderContext, pProgramState->settings.screenshot.resolution.x, pProgramState->settings.screenshot.resolution.y);
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);

    // Immediately update camera
    vcCamera_UpdateMatrices(pProgramState->geozone, &pProgramState->pActiveViewport->camera, pProgramState->settings.camera, 0, udFloat2::create(pProgramState->settings.viewports[0].resolution));
  }

  if (pProgramState->exportVideo && pProgramState->settings.viewports[0].resolution != pProgramState->exportVideoResolution)
  {
    pProgramState->settings.viewports[0].resolution = pProgramState->exportVideoResolution;
    vcRender_ResizeScene(pProgramState, pProgramState->pActiveViewport->pRenderContext, pProgramState->exportVideoResolution.x, pProgramState->exportVideoResolution.y);
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);

    // Immediately update camera
    vcCamera_UpdateMatrices(pProgramState->geozone, &pProgramState->pActiveViewport->camera, pProgramState->settings.camera, 0, udFloat2::create(pProgramState->settings.viewports[0].resolution));
  }

  udDouble3 cameraMoveOffset = udDouble3::zero();
  vcRenderSceneUI(pProgramState, windowPos, windowSize, &cameraMoveOffset);

  {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Columns(pProgramState->settings.activeViewportCount, nullptr, false);
    ImGui::PopStyleVar(); // Item Spacing

    // At the moment some functionality is restricted to certain viewports
    for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
    {
      ImVec2 viewportPosition = ImVec2(windowPos.x + ImGui::GetCursorPosX(), windowPos.y + ImGui::GetCursorPosY());
      udUInt2 viewportResolution = udUInt2::create((uint32_t)ImGui::GetColumnWidth(), (uint32_t)windowSize.y);

      vcRenderData renderData = {};

      renderData.models.Init(32);
      renderData.fences.Init(32);
      renderData.labels.Init(32);
      renderData.waterVolumes.Init(32);
      renderData.polyModels.Init(64);
      renderData.images.Init(32);
      renderData.lines.Init(32);
      renderData.depthLines.Init(32);
      renderData.viewSheds.Init(32);
      renderData.pins.Init(512);
      renderData.mouse.position.x = (uint32_t)(io.MousePos.x - viewportPosition.x);
      renderData.mouse.position.y = (uint32_t)(io.MousePos.y - viewportPosition.y);
      renderData.mouse.clicked = io.MouseClicked[1];

      pProgramState->activeViewportIndex = viewportIndex;
      pProgramState->pActiveViewport = &pProgramState->pViewports[viewportIndex];

      // TODO: Screenshot only viewport 0
      if ((viewportIndex != 0 || (!pProgramState->settings.screenshot.taking && !pProgramState->exportVideo)) && (pProgramState->settings.viewports[viewportIndex].resolution.x != viewportResolution.x || pProgramState->settings.viewports[viewportIndex].resolution.y != viewportResolution.y)) //Resize buffers
      {
        pProgramState->settings.viewports[viewportIndex].resolution = viewportResolution;
        vcRender_ResizeScene(pProgramState, pProgramState->pActiveViewport->pRenderContext, viewportResolution.x, viewportResolution.y);

        // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
        // this isn't valid on iOS when using UIKit.
        vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);

        // Update camera immediately
        vcCamera_UpdateMatrices(pProgramState->geozone, &pProgramState->pActiveViewport->camera, pProgramState->settings.camera, viewportIndex, udFloat2::create((float)viewportResolution.x, (float)viewportResolution.y));
      }

      bool renderEuclideonWatermark = pProgramState->settings.presentation.showEuclideonLogo;
      bool isFixedOrthographicViewport = pProgramState->settings.camera.mapMode[viewportIndex];
      if (isFixedOrthographicViewport)
      {
        pProgramState->pActiveViewport->camera.headingPitch = udDouble2::create(0.0f, -UD_HALF_PI);

        vcCamera_UpdateMatrices(pProgramState->geozone, &pProgramState->pActiveViewport->camera, pProgramState->settings.camera, viewportIndex, udFloat2::create((float)pProgramState->settings.viewports[viewportIndex].resolution.x, (float)pProgramState->settings.viewports[viewportIndex].resolution.y));

        // disable watermark rendering
        pProgramState->settings.presentation.showEuclideonLogo = false;
      }

      pProgramState->pActiveViewport->pickingSuccess = false;
      vcRender_BeginFrame(pProgramState->pActiveViewport->pRenderContext, renderData);

      if (viewportIndex > 0)
      {
        udFloat2 splitterSize = udFloat2::create(pProgramState->settings.window.touchscreenFriendly ? 6.0f : 3.0f, (float)viewportResolution.y);
        vcIGSW_verticalSplitter(udTempStr("###viewportSpliter%d", viewportIndex), splitterSize, viewportIndex, ImGui::GetColumnWidth(viewportIndex), ImGui::GetColumnWidth(viewportIndex - 1));
      }

      if (ImGui::BeginChild(udTempStr("###sceneViewport%d", viewportIndex), ImVec2((float)viewportResolution.x, (float)viewportResolution.y), false))
      {
        // Actual rendering to this texture is deferred
        ImGui::Image(renderData.pSceneTexture, ImVec2((float)viewportResolution.x, (float)viewportResolution.y), ImVec2(0, 0), ImVec2(renderData.sceneScaling.x, renderData.sceneScaling.y));
      }
      ImGui::EndChild();

      // TODO: Screenshot which viewport?

      if (viewportIndex == 0 && (pProgramState->settings.screenshot.taking || pProgramState->exportVideo))
        pProgramState->screenshot.pImage = renderData.pSceneTexture;

      bool useTool = (io.MouseDragMaxDistanceSqr[0] < (io.MouseDragThreshold * io.MouseDragThreshold)) && ImGui::IsMouseReleased(0) && ImGui::IsItemHovered();

      // Orbit around centre when fully pressed, show crosshair when partially pressed (also see vcCamera_HandleSceneInput())
      if (io.NavInputs[ImGuiNavInput_FocusNext] > 0.15f) // Right Trigger
      {
        udInt2 centrePoint = { (int)windowSize.x / 2, (int)windowSize.y / 2 };
        renderData.mouse.position = centrePoint;

        // Need to adjust crosshair position slightly
        centrePoint += pProgramState->settings.window.isFullscreen ? udInt2::create(-8, -8) : udInt2::create(-2, -2);

        ImVec2 sceneWindowPos = ImGui::GetWindowPos();
        sceneWindowPos = ImVec2(sceneWindowPos.x + centrePoint.x, sceneWindowPos.y + centrePoint.y);

        ImGui::GetWindowDrawList()->AddImage(pProgramState->pUITexture, ImVec2((float)sceneWindowPos.x, (float)sceneWindowPos.y), ImVec2((float)sceneWindowPos.x + 24, (float)sceneWindowPos.y + 24), ImVec2(0, 0.375), ImVec2(0.09375, 0.46875));
      }

      if (vcHotkey::IsPressed(vcB_Remove) && !ImGui::IsAnyItemActive() && !pProgramState->modalOpen)
        vcProject_RemoveSelected(pProgramState);

      pProgramState->activeProject.pFolder->AddToScene(pProgramState, &renderData);

      // Render scene to texture
      vcRender_RenderScene(pProgramState, pProgramState->pActiveViewport->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);

      vcRenderGizmo(pProgramState, viewportPosition, viewportResolution);

      pProgramState->pActiveViewport->isMouseOver = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped);
      if (pProgramState->pActiveViewport->isMouseOver)
        vcRenderScene_HandlePicking(pProgramState, renderData, useTool);

      // Camera update has to be here because it depends on previous ImGui state
      vcCamera_HandleSceneInput(pProgramState, pProgramState->pActiveViewport, viewportIndex, cameraMoveOffset, udFloat2::create((float)pProgramState->settings.viewports[viewportIndex].resolution.x, (float)pProgramState->settings.viewports[viewportIndex].resolution.y), udFloat2::create((float)renderData.mouse.position.x, (float)renderData.mouse.position.y));

      // Clean up
      renderData.models.Deinit();
      renderData.fences.Deinit();
      renderData.labels.Deinit();
      renderData.waterVolumes.Deinit();
      renderData.polyModels.Deinit();
      renderData.images.Deinit();
      renderData.lines.Deinit();
      renderData.depthLines.Deinit();
      renderData.viewSheds.Deinit();
      renderData.pins.Deinit();

      // Handle context menu
      if ((wasViewportContextMenuOpenLastFrame == -1 || wasViewportContextMenuOpenLastFrame == viewportIndex))
      {
        if ((io.MouseDragMaxDistanceSqr[1] < (io.MouseDragThreshold * io.MouseDragThreshold) && ImGui::BeginPopupContextItem("SceneContext")))
        {
          if (pProgramState->focusedViewportIndex == viewportIndex && wasViewportContextMenuOpenLastFrame != viewportIndex)
          {
            wasViewportContextMenuOpenLastFrame = viewportIndex;
            wasViewportContextMenuOnPickable = pProgramState->pActiveViewport->pickingSuccess;
            pProgramState->pActiveViewport->worldMousePickPosCartesian = pProgramState->pActiveViewport->worldMousePosCartesian;
          }

          if (wasViewportContextMenuOpenLastFrame == viewportIndex)
          {
            if (wasViewportContextMenuOnPickable)
            {
              if (pProgramState->sceneExplorer.selectedItems.size() == 1)
              {
                const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[0];
                if (item.pItem->itemtype == udPNT_PointOfInterest && item.pItem->pUserData != nullptr && item.pItem->geomtype != udPGT_Point)
                {
                  vcPOI *pPOI = (vcPOI *)item.pItem->pUserData;

                  if (ImGui::MenuItem(vcString::Get("scenePOIAddPoint")))
                    pPOI->AddPoint(pProgramState, pProgramState->pActiveViewport->worldMousePickPosCartesian);
                }
              }

              if (ImGui::BeginMenu(vcString::Get("sceneAddMenu")))
              {
                udProjectNode *pNode = nullptr;

                if (ImGui::MenuItem(vcString::Get("sceneAddViewShed")))
                {
                  vcProject_ClearSelection(pProgramState);

                  if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "ViewMap", vcString::Get("sceneExplorerViewShedDefaultName"), nullptr, nullptr) == udE_Success)
                  {
                    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, &pProgramState->pActiveViewport->worldMousePickPosCartesian, 1);
                    udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
                  }

                  ImGui::CloseCurrentPopup();
                }

                ImGui::EndMenu();
              }

              if (ImGui::MenuItem(vcString::Get("sceneMoveTo")))
              {
                pProgramState->pActiveViewport->cameraInput.inputState = vcCIS_MovingToPoint;
                pProgramState->pActiveViewport->cameraInput.startPosition = pProgramState->pActiveViewport->camera.position;
                pProgramState->pActiveViewport->cameraInput.startAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, pProgramState->pActiveViewport->camera.headingPitch);
                pProgramState->pActiveViewport->cameraInput.progress = 0.0;

                pProgramState->pActiveViewport->isUsingAnchorPoint = true;
                pProgramState->pActiveViewport->worldAnchorPoint = pProgramState->pActiveViewport->worldMousePickPosCartesian;
              }
            }

            if (ImGui::MenuItem(vcString::Get("sceneResetRotation")))
            {
              //TODO: Smooth this over time after fixing inputs
              pProgramState->pActiveViewport->camera.headingPitch = udDouble2::zero();
            }
          }

          ImGui::EndPopup();
        }
        else
        {
          wasViewportContextMenuOpenLastFrame = -1;
        }
      }

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGui::NextColumn();
      ImGui::PopStyleVar(); // Item Spacing

      // Reset some settings
      // TODO: This is a hack
      pProgramState->settings.presentation.showEuclideonLogo = renderEuclideonWatermark;
    }

    udStreamerInfo streamingStatus = {};
    udStreamer_Update(&streamingStatus);
    pProgramState->isStreaming |= (streamingStatus.active != 0);
    pProgramState->streamingMemory = streamingStatus.memoryInUse;

    ImGui::Columns(1);
  }

  // Future-proofing - viewport 0 is the primary viewport.
  pProgramState->activeViewportIndex = 0;
  pProgramState->pActiveViewport = &pProgramState->pViewports[pProgramState->activeViewportIndex];

  // Can only assign longlat positions in projected space
  if (pProgramState->geozone.projection != udGZPT_Unknown)
    pProgramState->pActiveViewport->worldMousePosLongLat = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->pActiveViewport->worldMousePosCartesian, true);
  else
    pProgramState->pActiveViewport->worldMousePosLongLat = pProgramState->pActiveViewport->worldMousePosCartesian;

  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
}

void vcMain_ShowStartupScreen(vcState *pProgramState)
{
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 size = io.DisplaySize;

  static double logoFade = 0.0;

  ImDrawList *pDrawList = ImGui::GetBackgroundDrawList();

  pDrawList->AddRectFilledMultiColor(ImVec2(0, 0), size, pProgramState->branding.colours[0], pProgramState->branding.colours[1], pProgramState->branding.colours[2], pProgramState->branding.colours[3]);

  float amt = (float)udSin(ImGui::GetTime()) * 50.f;
  float baseY = size.y - vcLBS_LoginBoxY - 160;
  pDrawList->AddBezierCurve(ImVec2(0, baseY), ImVec2(size.x * 0.33f, baseY + amt), ImVec2(size.x * 0.67f, baseY - amt), ImVec2(size.x, baseY), 0xFFFFFFFF, 4.f);

  int sizeX = 0;
  int sizeY = 0;

  vcTexture *pTexture = vcTextureCache_Get("asset://assets/branding/logo.png", vcTFM_Linear, true, vcTWM_Clamp);
  vcTexture_GetSize(pTexture, &sizeX, &sizeY);
  if (sizeX > 1 && sizeY > 1)
  {
    logoFade += pProgramState->deltaTime * 10; // Fade in really fast
    uint32_t fadeIn = (0xFFFFFF | (udMin(uint32_t(logoFade * 255), 255U) << 24));

    if (sizeX > 0 && sizeY > 0)
    {
      float ratio = (float)sizeX / sizeY;

      float scaling = 1.0;
      if (ratio >= 1.0)
        scaling = udMin(0.9f * (size.y - vcLBS_LoginBoxH) / udMax(vcLBS_LogoAreaSize, sizeX), 1.f);
      else
        scaling = udMin(0.9f * (size.y - vcLBS_LoginBoxH) / udMax(vcLBS_LogoAreaSize, sizeY), 1.f);

      float yOff = (size.y - vcLBS_LoginBoxH) / 2.f;
      pDrawList->AddImage(pTexture, ImVec2((size.x - sizeX * scaling) / 2.f, yOff - (scaling * sizeY * 0.5f)), ImVec2((size.x + sizeX * scaling) / 2, yOff + (sizeY * scaling * 0.5f)), ImVec2(0, 0), ImVec2(1, 1), fadeIn);

      //Leaving this here for when someone inevitably needs to fix the scaling issues
      //pDrawList->AddRect(ImVec2((size.x - sizeX * scaling) / 2.f, yOff - (scaling * sizeY * 0.5f)), ImVec2((size.x + sizeX * scaling) / 2, yOff + (sizeY * scaling * 0.5f)), 0xFF00FFFF);
    }
  }
}

void vcMain_RegisterUser(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  if (udUserUtil_Register(pProgramState->settings.loginInfo.serverURL, pProgramState->modelPath, pProgramState->settings.loginInfo.email, VCAPPNAME, pProgramState->modalTempBool) == udE_Success)
    pProgramState->loginStatus = vcLS_RegisterCheckEmail;
  else
    pProgramState->loginStatus = vcLS_RegisterTryPortal;
}

void vcMain_ForgotPassword(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  if (udUserUtil_ForgotPassword(pProgramState->settings.loginInfo.serverURL, pProgramState->settings.loginInfo.email) == udE_Success)
    pProgramState->loginStatus = vcLS_ForgotPasswordCheckEmail;
  else
    pProgramState->loginStatus = vcLS_ForgotPasswordTryPortal;
}

void vcMain_ShowLoginWindow(vcState *pProgramState)
{
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));

  ImGuiIO io = ImGui::GetIO();
  ImVec2 size = io.DisplaySize;

  vcMain_ShowStartupScreen(pProgramState);

  ImGui::SetNextWindowBgAlpha(0.f);
  ImGui::SetNextWindowPos(ImVec2(0, size.y), ImGuiCond_Always, ImVec2(0, 1));
  if (ImGui::Begin("LoginScreenPopups", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
  {
    ImGui::PushItemWidth(130.f);

    if (ImGui::Button(vcString::Get("settingsTitle")))
      pProgramState->openSettings = true;

    ImGui::SameLine();

    // Show the language combo, if its not visible and the selected translation isn't for the current version, display an error tooltip
    if (!vcSettingsUI_LangCombo(pProgramState) && !udStrEqual(pProgramState->languageInfo.pTargetVersion, VCSTRINGIFY(VCVERSION_VERSION_ARRAY_PARTIAL)))
    {
      ImGui::SetNextWindowBgAlpha(0.5f);
      ImGui::SetNextWindowPos(ImGui::GetItemRectMin(), ImGuiCond_Always, ImVec2(0, 1));

      if (ImGui::Begin("LoginScreenBadTranslation", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_Tooltip))
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", vcString::Get("settingsAppearanceTranslationDifferentVersion"));
      ImGui::End();
    }

    ImGui::PopItemWidth();
  }
  ImGui::End();

  ImGui::SetNextWindowBgAlpha(0.2f);
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

    const char *pSupportStr = vcStringFormat(vcString::Get("loginSupportDirectEmail"), pProgramState->branding.supportEmail);
    ImGui::TextUnformatted(pSupportStr);
    udFree(pSupportStr);
    if (ImGui::IsItemClicked())
      vcWebFile_OpenBrowser(udTempStr("mailto:%s?subject=%s", pProgramState->branding.supportEmail, "udStream%20" VCVERSION_VERSION_STRING "%20Support")); //TODO: Escape Appname and put that here as well
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }
  ImGui::End();

  ImGui::SetNextWindowBgAlpha(0.2f);
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

  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(400, 130));
  ImGui::SetNextWindowBgAlpha(0.6f);
  ImGui::SetNextWindowSize(ImVec2(-1, -1));
  ImGui::SetNextWindowPos(ImVec2(size.x / 2, size.y - vcLBS_LoginBoxY), ImGuiCond_Always, ImVec2(0.5, 1.0));

  const char *loginStatusKeys[] = {
    "loginMessageCredentials",
    "loginMessageCredentials",
    "loginEnterURL",
    "loginMessageChecking",
    "loginErrorConnection",
    "loginErrorAuth",
    "loginErrorTimeSync",
    "loginErrorSecurity",
    "loginErrorNegotiate",
    "loginErrorProxy",
    "loginErrorProxyAuthPending",
    "loginErrorProxyAuthFailed",
    "loginErrorTimeout",
    "loginErrorInvalidLicense",
    "loginErrorNotSupported",
    "loginErrorOther",

    "loginForgot",
    "loginForgotPending",
    "loginForgotCheckEmail",
    "loginForgotTryPortal",

    "loginRegister",
    "loginRegisterPending",
    "loginRegisterCheckEmail",
    "loginRegisterTryPortal"
  };
  UDCOMPILEASSERT(vcLS_Count == udLengthOf(loginStatusKeys), "Status Keys Updated, Update string table");

  if (pProgramState->loginStatus == vcLS_Pending || pProgramState->loginStatus == vcLS_RegisterPending || pProgramState->loginStatus == vcLS_ForgotPasswordPending)
  {
    if (ImGui::Begin("loginTitle", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
      vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
      ImGui::TextUnformatted(vcString::Get(loginStatusKeys[pProgramState->loginStatus]));
    }
    ImGui::End();
  }
  else if (pProgramState->loginStatus == vcLS_ForgotPassword || pProgramState->loginStatus == vcLS_ForgotPasswordTryPortal || pProgramState->loginStatus == vcLS_Register || pProgramState->loginStatus == vcLS_RegisterTryPortal)
  {
    if (ImGui::Begin("loginTitle", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
      bool submit = false;

      if (ImGui::SmallButton(vcString::Get("loginBackToLogin")))
        pProgramState->loginStatus = vcLS_EnterCredentials;

      ImGui::SameLine();

      ImGui::TextUnformatted(vcString::Get(loginStatusKeys[pProgramState->loginStatus]));

      submit = vcIGSW_InputText(vcString::Get("loginServerURL"), pProgramState->settings.loginInfo.serverURL, ImGuiInputTextFlags_EnterReturnsTrue) || submit;
      submit = vcIGSW_InputText(vcString::Get("loginUsername"), pProgramState->settings.loginInfo.email, ImGuiInputTextFlags_EnterReturnsTrue) || submit;

      if (pProgramState->loginStatus == vcLS_Register)
      {
        submit = vcIGSW_InputText(vcString::Get("loginRealname"), pProgramState->modelPath, ImGuiInputTextFlags_EnterReturnsTrue) || submit;
        ImGui::Checkbox(vcString::Get("loginRegisterOptInMarketing"), &pProgramState->modalTempBool);
      }

      if (ImGui::Button(vcString::Get(loginStatusKeys[pProgramState->loginStatus])) || submit)
      {
        if (pProgramState->loginStatus == vcLS_Register)
        {
          pProgramState->loginStatus = vcLS_RegisterPending;
          udWorkerPool_AddTask(pProgramState->pWorkerPool, vcMain_RegisterUser, pProgramState, false);
        }
        else
        {
          pProgramState->loginStatus = vcLS_ForgotPasswordPending;
          udWorkerPool_AddTask(pProgramState->pWorkerPool, vcMain_ForgotPassword, pProgramState, false);
        }
      }

    }
    ImGui::End();
  }
  else
  {
    if (ImGui::Begin("loginTitle", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
      const bool isErrorStatus = udStrBeginsWith(loginStatusKeys[pProgramState->loginStatus], "loginError");

      if (isErrorStatus)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));

      ImGui::TextUnformatted(vcString::Get(loginStatusKeys[pProgramState->loginStatus]));

      if (isErrorStatus)
        ImGui::PopStyleColor();

      // Tool for support to get reasons for failures, requires Alt & Ctrl
      if (pProgramState->logoutReason != udE_Success && io.KeyAlt && io.KeyCtrl)
      {
        ImGui::SameLine();
        ImGui::Text("E:%d", (int)pProgramState->logoutReason);
      }

      bool tryLogin = false;

      // Server URL
      tryLogin |= vcIGSW_InputText(vcString::Get("loginServerURL"), pProgramState->settings.loginInfo.serverURL, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->loginStatus == vcLS_NoStatus && !pProgramState->settings.loginInfo.rememberServer)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox(udTempStr("%s##rememberServerURL", vcString::Get("loginRememberServer")), &pProgramState->settings.loginInfo.rememberServer);

      // Username
      tryLogin |= vcIGSW_InputText(vcString::Get("loginUsername"), pProgramState->settings.loginInfo.email, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->loginStatus == vcLS_NoStatus && pProgramState->settings.loginInfo.rememberServer && !pProgramState->settings.loginInfo.rememberEmail)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox(udTempStr("%s##rememberUser", vcString::Get("loginRememberUser")), &pProgramState->settings.loginInfo.rememberEmail);

      // Password
      ImVec2 buttonSize;
      if (pProgramState->passwordFieldHasFocus)
      {
        ImGui::Button(vcString::Get("loginShowPassword"));
        ImGui::SameLine(0, 0);
        buttonSize = ImGui::GetItemRectSize();
      }
      if (ImGui::IsItemActive() && pProgramState->passwordFieldHasFocus)
        tryLogin |= vcIGSW_InputText(vcString::Get("loginPassword"), pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      else
        tryLogin |= vcIGSW_InputText(vcString::Get("loginPassword"), pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

      if (pProgramState->passwordFieldHasFocus && ImGui::IsMouseClicked(0))
      {
        ImVec2 minPos = ImGui::GetItemRectMin();
        ImVec2 maxPos = ImGui::GetItemRectMax();
        if (io.MouseClickedPos->x < minPos.x - buttonSize.x || io.MouseClickedPos->x > maxPos.x || io.MouseClickedPos->y < minPos.y || io.MouseClickedPos->y > maxPos.y)
          pProgramState->passwordFieldHasFocus = false;
      }

      if (!pProgramState->passwordFieldHasFocus && udStrlen(pProgramState->password) == 0)
        pProgramState->passwordFieldHasFocus = true;

      if (pProgramState->loginStatus == vcLS_NoStatus && pProgramState->settings.loginInfo.rememberServer && pProgramState->settings.loginInfo.rememberEmail)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);

      if (pProgramState->loginStatus == vcLS_NoStatus)
      {
        pProgramState->loginStatus = vcLS_EnterCredentials;
      }
      else if (pProgramState->loginStatus == vcLS_ProxyAuthRequired)
      {
        pProgramState->openSettings = true;
        pProgramState->activeSetting = vcSR_Connection;
        pProgramState->settings.loginInfo.tested = true;
        pProgramState->settings.loginInfo.testStatus = vcLS_ProxyAuthRequired;
      }

      if (ImGui::Button(vcString::Get("loginRegister")))
      {
        pProgramState->loginStatus = vcLS_Register;
        pProgramState->modalTempBool = false;
      }

      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("loginForgot")))
        pProgramState->loginStatus = vcLS_ForgotPassword;

      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("loginButton")) || tryLogin)
      {
        if (*pProgramState->settings.loginInfo.serverURL == '\0')
        {
          pProgramState->loginStatus = vcLS_NoServerURL;
        }
        else
        {
          pProgramState->passwordFieldHasFocus = false;
          pProgramState->loginStatus = vcLS_Pending;
          udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_Login, pProgramState, false);
        }
      }

      if (SDL_GetModState() & KMOD_CAPS)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "%s", vcString::Get("loginCapsWarning"));
      }

      static bool loginSupportURLHovered = false;
      vcIGSW_URLText(vcString::Get("supportLoginMessage"), pProgramState->branding.supportURLLogin, &loginSupportURLHovered);
    }
    ImGui::End();
  }


  ImGui::PopStyleVar(); // ImGuiStyleVar_WindowMinSize
  ImGui::PopStyleVar(); // ImGuiStyleVar_WindowRounding
}

void vcMain_RenderWindow(vcState *pProgramState)
{
  ImGuiIO &io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

#if UDPLATFORM_WINDOWS
  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;
#endif


  // screenshot waits an entire frame to allow correct setup
  if (pProgramState->settings.screenshot.taking)
    vcMain_TakeScreenshot(pProgramState);

  if (vcHotkey::IsPressed(vcB_TakeScreenshot))
    pProgramState->settings.screenshot.taking = true;


  //end keyboard/mouse handling

  if (!pProgramState->hasContext)
  {
    vcMain_ShowLoginWindow(pProgramState);
  }
  else
  {
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    bool rootDockOpen = ImGui::Begin("rootdockTesting", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::PopStyleVar(2); //Window Padding & Border

    if (rootDockOpen)
    {
      int sceneExplorerSize = pProgramState->settings.presentation.sceneExplorerSize;
      vcWindowLayout layout = pProgramState->settings.presentation.layout;

      if (pProgramState->settings.presentation.sceneExplorerCollapsed)
      {
        sceneExplorerSize = 0;
        layout = vcWL_SceneRight; // This fixes a min-columnsize issue in ImGui
        pProgramState->settings.presentation.columnSizeCorrect = false;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGui::Columns(2, nullptr, false);
      ImGui::PopStyleVar(); // Item Spacing

      if (!pProgramState->settings.presentation.columnSizeCorrect)
      {
        if (layout == vcWL_SceneLeft)
          ImGui::SetColumnWidth(0, size.x - sceneExplorerSize);
        else if (layout == vcWL_SceneRight)
          ImGui::SetColumnWidth(0, (float)sceneExplorerSize);

        pProgramState->settings.presentation.columnSizeCorrect = true;
      }

      switch (layout)
      {
      case vcWL_SceneLeft:
        if (ImGui::GetColumnWidth(0) != size.x - sceneExplorerSize && !pProgramState->settings.presentation.sceneExplorerCollapsed)
          pProgramState->settings.presentation.sceneExplorerSize = (int)(size.x - ImGui::GetColumnWidth());

        if (ImGui::BeginChild(udTempStr("%s###sceneDock", vcString::Get("sceneTitle"))))
          vcMain_RenderSceneWindow(pProgramState);
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::NextColumn();
        ImGui::PopStyleVar(); // Item Spacing

        if (!pProgramState->settings.presentation.sceneExplorerCollapsed)
        {
          udFloat2 splitterSize = udFloat2::create(pProgramState->settings.window.touchscreenFriendly ? 6.0f : 3.0f, ImGui::GetWindowHeight());
          vcIGSW_verticalSplitter(udTempStr("###sceneExplorerSpliterRight"), splitterSize, 1, size.x - ImGui::GetColumnWidth(0), size.x - ImGui::GetColumnWidth(1));
        }
        if (ImGui::BeginChild(udTempStr("%s###sceneExplorerDock", vcString::Get("sceneExplorerTitle"))))
          vcMain_ShowSceneExplorerWindow(pProgramState);
        ImGui::EndChild();

        break;

      case vcWL_SceneRight:

        if (ImGui::BeginChild(udTempStr("%s###sceneExplorerDock", vcString::Get("sceneExplorerTitle"))))
          vcMain_ShowSceneExplorerWindow(pProgramState);
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::NextColumn();
        ImGui::PopStyleVar(); // Item Spacing

        if (!pProgramState->settings.presentation.sceneExplorerCollapsed)
        {
          udFloat2 splitterSize = udFloat2::create(pProgramState->settings.window.touchscreenFriendly ? 6.0f : 3.0f, ImGui::GetWindowHeight());
          vcIGSW_verticalSplitter(udTempStr("###sceneExplorerSpliterLeft"), splitterSize, 1, size.x - ImGui::GetColumnWidth(0), size.x - ImGui::GetColumnWidth(1));
        }
        if (ImGui::GetColumnWidth(0) != sceneExplorerSize && !pProgramState->settings.presentation.sceneExplorerCollapsed)
          pProgramState->settings.presentation.sceneExplorerSize = (int)ImGui::GetColumnWidth(0);

        if (ImGui::BeginChild(udTempStr("%s###sceneDock", vcString::Get("sceneTitle"))))
          vcMain_RenderSceneWindow(pProgramState);
        ImGui::EndChild();

        break;
      }

      ImGui::Columns(1);
    }

    ImGui::End();
  }

  vcSettingsUI_Show(pProgramState);
  vcModals_DrawModals(pProgramState);
}
