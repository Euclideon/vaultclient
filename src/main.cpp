#include "vcState.h"

#include "vdkContext.h"
#include "vdkServerAPI.h"
#include "vdkConfig.h"
#include "vdkPointCloud.h"

#include <chrono>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_ex/imgui_impl_sdl.h"

#if defined(GRAPHICS_API_METAL)
#include "imgui_ex/imgui_impl_metal.h"
#endif

#include "imgui_ex/imgui_impl_gl.h"
#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/ImGuizmo.h"
#include "imgui_ex/vcMenuButtons.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "exif.h"

#include "vcConvert.h"
#include "vcVersion.h"
#include "vcTime.h"
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

#include "vcPOI.h"
#include "vcLiveFeed.h"
#include "vcMedia.h"

#include "vCore/vStringFormat.h"

#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"

#include "legacy/vcUDP.h"

#include "udFile.h"
#include "udStringUtil.h"

#if UDPLATFORM_EMSCRIPTEN
#include "vHTTPRequest.h"
#endif

#include "stb_image.h"

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
    // If you need help, defines {"__MEMORY_DEBUG__"} in the premake5.lua just before:
    // if _OPTIONS["force-vaultsdk"] then
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
    // If you need help, defines {"__MEMORY_DEBUG__"} in the premake5.lua just before:
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

void vcMain_LangCombo(vcState *pProgramState)
{
  if (!ImGui::BeginCombo("##langCode", pProgramState->languageInfo.pLocalName))
    return;

  for (size_t i = 0; i < pProgramState->settings.languageOptions.length; ++i)
  {
    const char *pName = pProgramState->settings.languageOptions[i].languageName;
    const char *pFilename = pProgramState->settings.languageOptions[i].filename;

    if (ImGui::Selectable(pName))
    {
      if (vcString::LoadTable(udTempStr("asset://assets/lang/%s.json", pFilename), &pProgramState->languageInfo) == udR_Success)
        udStrcpy(pProgramState->settings.window.languageCode, pFilename);
      else
        vcString::LoadTable(udTempStr("asset://assets/lang/%s.json", pProgramState->settings.window.languageCode), &pProgramState->languageInfo);
    }
  }

  ImGui::EndCombo();
}

void vcMain_UpdateSessionInfo(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  vdkError response = vdkContext_KeepAlive(pProgramState->pVDKContext);

  pProgramState->logoutReason = response;
  double now = vcTime_GetEpochSecsF();

  if (response == vE_SessionExpired || now - 180.0 > pProgramState->lastServerResponse)
    pProgramState->forceLogout = true;
  else if (response == vE_Success)
    pProgramState->lastServerResponse = now;
}

void vcMain_PresentationMode(vcState *pProgramState)
{
  if (pProgramState->settings.window.presentationMode)
  {
    pProgramState->settings.docksLoaded = false;
    SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }
  else
  {
    vcSettings_Save(&pProgramState->settings);
    SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }

  pProgramState->settings.window.presentationMode = !pProgramState->settings.window.presentationMode;

  if (pProgramState->settings.responsiveUI == vcPM_Responsive)
    pProgramState->lastEventTime = vcTime_GetEpochSecs();
}

const char* vcMain_GetOSName()
{
#if UDPLATFORM_ANDROID
  return "Android";
#elif UDPLATFORM_EMSCRIPTEN
  return "Emscripten";
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  return "iOS";
#elif UDPLATFORM_LINUX
  // TODO: Handle other distributions
  return "Ubuntu";
#elif UDPLATFORM_OSX
  return "macOS";
#elif UDPLATFORM_WINDOWS
  return "Windows";
#else
  return "Unknown";
#endif
}

void vcLogin(void *pProgramStatePtr)
{
  vdkError result;
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  result = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, "EuclideonVaultClient" VCVERSION_VERSION_STRING, pProgramState->settings.loginInfo.username, pProgramState->password);
  if (result == vE_ConnectionFailure)
    pProgramState->loginStatus = vcLS_ConnectionError;
  else if (result == vE_AuthFailure)
    pProgramState->loginStatus = vcLS_AuthError;
  else if (result == vE_OutOfSync)
    pProgramState->loginStatus = vcLS_TimeSync;
  else if (result == vE_SecurityFailure)
    pProgramState->loginStatus = vcLS_SecurityError;
  else if (result == vE_ServerFailure || result == vE_ParseError)
    pProgramState->loginStatus = vcLS_NegotiationError;
  else if (result == vE_ProxyError)
    pProgramState->loginStatus = vcLS_ProxyError;
  else if (result == vE_ProxyAuthRequired)
    pProgramState->loginStatus = vcLS_ProxyAuthRequired;
  else if (result != vE_Success)
    pProgramState->loginStatus = vcLS_OtherError;

  pProgramState->logoutReason = result;

  if (result != vE_Success)
    return;

  vcRender_SetVaultContext(pProgramState->pRenderContext, pProgramState->pVDKContext);

  const char *pProjData = nullptr;
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "dev/projects", nullptr, &pProjData) == vE_Success)
    pProgramState->projects.Parse(pProjData);
  vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pProjData);

  const char *pPackageData = nullptr;
  const char *pPostJSON = udTempStr("{ \"packagename\": \"EuclideonVaultClient\", \"packagevariant\": \"%s\" }", vcMain_GetOSName());
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/packages/latest", pPostJSON, &pPackageData) == vE_Success)
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
          pProgramState->lastServerResponse = vcTime_GetEpochSecsF();
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

  pProgramState->loginStatus = vcLS_NoStatus;
  pProgramState->hasContext = true;
}

void vcLogout(vcState *pProgramState)
{
  pProgramState->hasContext = false;
  pProgramState->forceLogout = false;

  if (pProgramState->pVDKContext != nullptr)
  {
    // Cancel all convert jobs
    if (pProgramState->pConvertContext != nullptr)
    {
      // Cancel all jobs
      for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; i++)
      {
        vdkConvert_Cancel(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[i]->pConvertContext);
      }

      // Wait for jobs to finish and destroy them
      while (pProgramState->pConvertContext->jobs.length != 0)
      {
        for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; i++)
        {
          if (pProgramState->pConvertContext->jobs[i]->status != vcCQS_Running)
          {
            vcConvert_RemoveJob(pProgramState, i);
            --i;
          }
        }
      }
    }

    pProgramState->modelPath[0] = '\0';
    vcProject_RemoveAll(pProgramState);
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

    switch (pProgramState->settings.presentation.styleIndex)
    {
    case 0: ImGui::StyleColorsDark(); ++pProgramState->settings.presentation.styleIndex; break;
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    }
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
        pProgramState->loadList.push_back(udStrdup(event.drop.file));
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
#ifndef GRAPHICS_API_METAL
  udSleep(sleepMS);
#endif
  pProgramState->deltaTime += sleepMS * 0.001; // adjust delta

#ifdef GRAPHICS_API_METAL
  ImGui_ImplMetal_NewFrame(pProgramState->pWindow);
#else
  ImGuiGL_NewFrame(pProgramState->pWindow);
#endif

  vcGizmo_BeginFrame();
  vcGLState_ResetState(true);
  vcRenderWindow(pProgramState);
  ImGui::Render();

#ifdef GRAPHICS_API_METAL
  ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData());
#else
  ImGuiGL_RenderDrawData(ImGui::GetDrawData());
#endif

  ImGui::UpdatePlatformWindows();

  vcGLState_Present(pProgramState->pWindow);

  if (ImGui::GetIO().WantSaveIniSettings)
    vcSettings_Save(&pProgramState->settings);

  ImGui::GetIO().KeysDown[SDL_SCANCODE_BACKSPACE] = false;

  if (pProgramState->hasContext)
  {
    // Load next file in the load list (if there is one and the user has a context)
    bool firstLoad = true;
    bool continueLoading = false;
    do
    {
      continueLoading = false;

      if (pProgramState->loadList.size() > 0)
      {
        const char *pNextLoad = pProgramState->loadList[0];
        pProgramState->loadList.erase(pProgramState->loadList.begin()); // TODO: Proper Exception Handling

        if (pNextLoad != nullptr)
        {
          // test to see if specified filepath is valid
          udFile *pTestFile = nullptr;
          pProgramState->currentError = vE_OpenFailure;
          bool convertDrop = false;

          //TODO: Use ImGui drag and drop on the docks rather than globally here
          if (pProgramState->settings.window.windowsOpen[vcDocks_Convert])
          {
            ImGuiWindow *pConvert = ImGui::FindWindowByName("###convertDock");
            if (pConvert != nullptr && ((pConvert->DockNode != nullptr && pConvert->DockTabIsVisible) || (pConvert->DockNode == nullptr && !pConvert->Collapsed)))
            {
              int x, y;
              SDL_GetMouseState(&x, &y); // ImGui mouse pos is -FLT_MAX during drag/drop operation
              if (x > pConvert->Pos.x && x < pConvert->Pos.x + pConvert->Size.x && y > pConvert->Pos.y && y < pConvert->Pos.y + pConvert->Size.y)
                convertDrop = true;
            }
          }

          if (udFile_Open(&pTestFile, pNextLoad, udFOF_Read) == udR_Success)
          {
            udFile_Close(&pTestFile);
            pProgramState->currentError = vE_Success;

            udFilename loadFile(pNextLoad);
            const char *pExt = loadFile.GetExt();
            if (udStrEquali(pExt, ".uds") || udStrEquali(pExt, ".ssf") || udStrEquali(pExt, ".udg"))
            {
              if (convertDrop)
              {
                vcConvert_AddFile(pProgramState, pNextLoad);
              }
              else
              {
                vdkProjectNode *pNode = nullptr;
                if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "UDS", nullptr, pNextLoad, nullptr) != vE_Success)
                {
                  vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
                }
                else if (firstLoad)
                {
                  udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
                }
                continueLoading = true;
                pProgramState->changeActiveDock = vcDocks_Scene;
              }
            }
            else if (udStrEquali(pExt, ".json"))
            {
              if (firstLoad)
                vcProject_RemoveAll(pProgramState);

              char *pMemory = nullptr;
              if (udFile_Load(pNextLoad, (void**)&pMemory) == udR_Success)
              {
                vdkProject *pProject = nullptr;
                if (vdkProject_LoadFromMemory(&pProject, pMemory) == vE_Success)
                {
                  pProgramState->sceneExplorer.pProjectRoot->Cleanup(pProgramState);
                  delete pProgramState->sceneExplorer.pProjectRoot;
                  vdkProject_Release(&pProgramState->sceneExplorer.pProject);

                  pProgramState->getGeo = true;
                  pProgramState->sceneExplorer.pProject = pProject;
                  vdkProjectNode *pNode = nullptr;
                  vdkProject_GetRootFolder(pProject, &pNode);
                  pProgramState->sceneExplorer.pProjectRoot = new vcFolder(pNode, pProgramState);
                  pNode->pUserData = pProgramState->sceneExplorer.pProjectRoot;
                }
                udFree(pMemory);
              }

              pProgramState->changeActiveDock = vcDocks_Scene;
            }
            else if (udStrEquali(pExt, ".udp"))
            {
              if (firstLoad)
                vcProject_RemoveAll(pProgramState);

              vcUDP_Load(pProgramState, pNextLoad);
              pProgramState->changeActiveDock = vcDocks_Scene;
            }
            else if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg") || udStrEquali(pExt, ".png") || udStrEquali(pExt, ".tga") || udStrEquali(pExt, ".bmp") || udStrEquali(pExt, ".gif"))
            {
              // Use as convert watermark for conversions
              if (convertDrop)
              {
                vcConvert_AddFile(pProgramState, pNextLoad, true);
              }
              else
              {
                const vcSceneItemRef &clicked = pProgramState->sceneExplorer.clickedItem;
                if (clicked.pParent != nullptr && clicked.pItem->itemtype == vdkPNT_Media)
                {
                  vdkProjectNode_SetURI(pProgramState->sceneExplorer.pProject, clicked.pItem, pNextLoad);
                }
                else
                {
                  udDouble3 geolocation = udDouble3::zero();
                  bool hasLocation = false;
                  vcImageType imageType = vcIT_StandardPhoto;

                  // vcTexture *pImage = nullptr;
                  const unsigned char *pFileData = nullptr;
                  int64_t numBytes = 0;

                  if (udFile_Load(pNextLoad, (void**)&pFileData, &numBytes) == udR_Success)
                  {
                    // Many jpg's have exif, let's process that first
                    if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg"))
                    {
                      easyexif::EXIFInfo result;

                      if (result.parseFrom(pFileData, (int)numBytes) == PARSE_EXIF_SUCCESS)
                      {
                        if (result.GeoLocation.Latitude != 0.0 || result.GeoLocation.Longitude != 0.0)
                        {
                          hasLocation = true;
                          geolocation.x = result.GeoLocation.Longitude;
                          geolocation.y = result.GeoLocation.Latitude;
                          geolocation.z = result.GeoLocation.Altitude;
                        }

                        if (result.XMPMetadata != "")
                        {
                          udJSON xmp;
                          if (xmp.Parse(result.XMPMetadata.c_str()) == udR_Success)
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


                  vdkProjectNode *pNode = nullptr;
                  if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "Media", loadFile.GetFilenameWithExt(), pNextLoad, nullptr) == vE_Success)
                  {
                    if (hasLocation && pProgramState->gis.isProjected)
                      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, &geolocation.x);
                    else if (pProgramState->worldMousePos != udDouble3::zero())
                      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, &pProgramState->worldMousePosLongLat.x);
                    else
                      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, &pProgramState->pCamera->positionInLongLat.x);

                    if (imageType == vcIT_PhotoSphere)
                      vdkProjectNode_SetMetadataString(pNode, "imagetype", "photosphere");
                    else if (imageType == vcIT_Panorama)
                      vdkProjectNode_SetMetadataString(pNode, "imagetype", "panorama");
                    else
                      vdkProjectNode_SetMetadataString(pNode, "imagetype", "standard");
                  }
                }
              }
            }
            else if (udStrEquali(pExt, ".slpk"))
            {
              if (convertDrop)
                vcConvert_AddFile(pProgramState, pNextLoad);
              else
                vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, nullptr, "I3S", loadFile.GetFilenameWithExt(), pNextLoad, nullptr);
            }
            else
            {
              if (vcConvert_AddFile(pProgramState, pNextLoad))
                pProgramState->changeActiveDock = vcDocks_Convert;
            }
          }

          udFree(pNextLoad);
        }
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

      firstLoad = false;
    } while (continueLoading);

    // Ping the server every 30 seconds
    if (vcTime_GetEpochSecsF() > pProgramState->lastServerAttempt + 30.0)
    {
      pProgramState->lastServerAttempt = vcTime_GetEpochSecsF();
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcMain_UpdateSessionInfo, pProgramState, false);
    }

    vWorkerThread_DoPostWork(pProgramState->pWorkerPool);

    if (pProgramState->forceLogout)
    {
      vcLogout(pProgramState);
      vcModals_OpenModal(pProgramState, vcMT_LoggedOut);
    }
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
#endif //UDPLATFORM_WINDOWS

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

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
  void *pFileData = nullptr;
  int64_t fileLen = 0;
  unsigned char *pIconData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;

  void *pFontData = nullptr;
  int64_t fontDataLength = 0;

  // default values
  programState.settings.camera.moveMode = vcCMM_Plane;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // TODO: Query device and fill screen
  programState.sceneResolution.x = 1920;
  programState.sceneResolution.y = 1080;
  programState.settings.onScreenControls = true;
#else
  programState.sceneResolution.x = 1280;
  programState.sceneResolution.y = 720;
  programState.settings.onScreenControls = false;
#endif
  vcCamera_Create(&programState.pCamera);

  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

  // Dock setting
  programState.settings.docksLoaded = false;
  programState.settings.window.windowsOpen[vcDocks_Scene] = true;
  programState.settings.window.windowsOpen[vcDocks_Settings] = true;
  programState.settings.window.windowsOpen[vcDocks_SceneExplorer] = true;
  programState.settings.window.windowsOpen[vcDocks_Convert] = true;

  programState.settings.hideIntervalSeconds = 3;
  programState.showUI = true;
  programState.changeActiveDock = vcDocks_Count;
  programState.passFocus = true;
  programState.renaming = -1;
  programState.getGeo = false;
  programState.pGotGeo = nullptr;

  udGeoZone_SetFromSRID(&programState.defaultGeo, 7030);

  programState.sceneExplorer.insertItem.pParent = nullptr;
  programState.sceneExplorer.insertItem.pItem = nullptr;
  programState.sceneExplorer.clickedItem.pParent = nullptr;
  programState.sceneExplorer.clickedItem.pItem = nullptr;

  programState.loadList.reserve(udMax(64, argc));

  vdkProjectNode *pRootFolder = nullptr;
  vdkProject_CreateLocal(&programState.sceneExplorer.pProject, nullptr, nullptr);
  vdkProject_GetRootFolder(programState.sceneExplorer.pProject, &pRootFolder);
  programState.sceneExplorer.pProjectRoot = new vcFolder(pRootFolder, &programState);

  for (int i = 1; i < argc; ++i)
  {
#if UDPLATFORM_OSX
    // macOS assigns a unique process serial number (PSN) to all apps,
    // we should not attempt to load this as a file.
    if (!udStrBeginsWithi(args[i], "-psn"))
#endif
      programState.loadList.push_back(udStrdup(args[i]));
  }

  vWorkerThread_StartThreads(&programState.pWorkerPool);
  vcConvert_Init(&programState);

#if UDPLATFORM_EMSCRIPTEN
  programState.sceneResolution.x = EM_ASM_INT_V({
    return window.innerWidth;
  });
  programState.sceneResolution.y = EM_ASM_INT_V({
    return window.innerHeight;
  });
#endif

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

  if (udFile_Load("asset://assets/icons/EuclideonClientIcon.png", &pFileData, &fileLen) == udR_Success)
  {
    pIconData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

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
        SDL_SetWindowIcon(programState.pWindow, pIcon);

      free(pIconData);
    }

    SDL_free(pIcon);
    udFree(pFileData);
  }

  ImGui::CreateContext();
  ImGui::GetStyle().WindowRounding = 0.0f;

  vcMain_LoadSettings(&programState, false);

  if (!vcGLState_Init(programState.pWindow, &programState.pDefaultFramebuffer))
    goto epilogue;

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // setup watermark for background
  vcTexture_CreateFromFilename(&programState.pCompanyLogo, "asset://assets/textures/logo.png");
  vcTexture_CreateFromFilename(&programState.pBuildingsTexture, "asset://assets/textures/buildings.png", nullptr, nullptr, vcTFM_Nearest, false);

  if (!ImGuiGL_Init(programState.pWindow))
    goto epilogue;

  if (vcRender_Init(&(programState.pRenderContext), &(programState.settings), programState.pCamera, programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  vcFramebuffer_Bind(programState.pDefaultFramebuffer);

  if (udFile_Load("asset://assets/fonts/NotoSansCJKjp-Regular.otf", &pFontData, &fontDataLength) == udR_Success)
  {
    const float FontSize = 16.f;
    ImFontConfig fontCfg = ImFontConfig();
    fontCfg.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFontData, (int)fontDataLength, FontSize, &fontCfg);
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

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFontData, (int)fontDataLength, FontSize, &fontCfg, characterRanges);
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFontData, (int)fontDataLength, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()); // Still need to load Japanese seperately
#else // Debug; Only load required Glyphs
    static ImWchar characterRanges[] =
    {
      0x0020, 0x00FF, // Basic Latin + Latin Supplement
      0x2010, 0x205E, // Punctuations
      0x25A0, 0x25FF, // Geometric Shapes
      0x26A0, 0x26A1, // Exclamation in Triangle
      0
    };

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFontData, (int)fontDataLength, FontSize, &fontCfg, characterRanges);
#endif

    udFree(pFontData);
    fontDataLength = 0;
  }

  SDL_DisableScreenSaver();

  vcString::LoadTable(udTempStr("asset://assets/lang/%s.json", programState.settings.window.languageCode), &programState.languageInfo);
  vcTexture_CreateFromFilename(&programState.pUITexture, "asset://assets/textures/uiDark24.png");

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

#ifdef GRAPHICS_API_METAL
  ImGui_ImplMetal_Shutdown();
#else
  ImGuiGL_DestroyDeviceObjects();
#endif
  ImGui::DestroyContext();

  vcConvert_Deinit(&programState);
  vcCamera_Destroy(&programState.pCamera);
  vcTexture_Destroy(&programState.pCompanyLogo);
  vcTexture_Destroy(&programState.pBuildingsTexture);
  vcTexture_Destroy(&programState.pUITexture);
  for (size_t i = 0; i < programState.loadList.size(); i++)
    udFree(programState.loadList[i]);
  vcRender_Destroy(&programState.pRenderContext);
  vcTexture_Destroy(&programState.tileModal.pServerIcon);
  vcString::FreeTable(&programState.languageInfo);
  vWorkerThread_Shutdown(&programState.pWorkerPool); // This needs to occur before logout
  vcLogout(&programState);
  programState.sceneExplorer.pProjectRoot->Cleanup(&programState);
  vdkProject_Release(&programState.sceneExplorer.pProject);
  delete programState.sceneExplorer.pProjectRoot;
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
            ImGui::Text("%s: %.2f, %.2f, %.2f", vcString::Get("sceneMousePointInfo"), pProgramState->worldMousePos.x, pProgramState->worldMousePos.y, pProgramState->worldMousePos.z);

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
          if (vcGIS_ChangeSpace(&pProgramState->gis, zone, &pProgramState->pCamera->position))
          {
            pProgramState->sceneExplorer.pProjectRoot->ChangeProjection(zone);
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
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneLockAltitude"), vcString::Get("sceneLockAltitudeKey"), vcMBBI_LockAltitude, vcMBBG_FirstItem, (pProgramState->settings.camera.moveMode == vcCMM_Helicopter)))
        pProgramState->settings.camera.moveMode = (pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneCameraInfo"), nullptr, vcMBBI_ShowCameraSettings, vcMBBG_SameGroup, pProgramState->settings.presentation.showCameraInfo))
        pProgramState->settings.presentation.showCameraInfo = !pProgramState->settings.presentation.showCameraInfo;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneProjectionInfo"), nullptr, vcMBBI_ShowGeospatialInfo, vcMBBG_SameGroup, pProgramState->settings.presentation.showProjectionInfo))
        pProgramState->settings.presentation.showProjectionInfo = !pProgramState->settings.presentation.showProjectionInfo;

      // Gizmo Settings
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoTranslate"), vcString::Get("sceneGizmoTranslateKey"), vcMBBI_Translate, vcMBBG_NewGroup, (pProgramState->gizmo.operation == vcGO_Translate)) || (ImGui::IsKeyPressed(SDL_SCANCODE_B, false) && !pProgramState->modalOpen))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Translate ? vcGO_NoGizmo : vcGO_Translate;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoRotate"), vcString::Get("sceneGizmoRotateKey"), vcMBBI_Rotate, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Rotate)) || (ImGui::IsKeyPressed(SDL_SCANCODE_N, false) && !pProgramState->modalOpen))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Rotate ? vcGO_NoGizmo : vcGO_Rotate;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoScale"), vcString::Get("sceneGizmoScaleKey"), vcMBBI_Scale, vcMBBG_SameGroup, (pProgramState->gizmo.operation == vcGO_Scale)) || (!io.KeyCtrl && ImGui::IsKeyPressed(SDL_SCANCODE_M, false) && !pProgramState->modalOpen))
        pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Scale ? vcGO_NoGizmo : vcGO_Scale;

      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneGizmoLocalSpace"), vcString::Get("sceneGizmoLocalSpaceKey"), vcMBBI_UseLocalSpace, vcMBBG_SameGroup, (pProgramState->gizmo.coordinateSystem == vcGCS_Local)) || (ImGui::IsKeyPressed(SDL_SCANCODE_C, false) && !pProgramState->modalOpen))
        pProgramState->gizmo.coordinateSystem = (pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;

      // Fullscreen
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneFullscreen"), vcString::Get("sceneFullscreenKey"), vcMBBI_FullScreen, vcMBBG_NewGroup, pProgramState->settings.window.presentationMode))
        vcMain_PresentationMode(pProgramState);

      // map mode
      if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneMapMode"), vcString::Get("sceneMapModeKey"), vcMBBI_MapMode, vcMBBG_NewGroup, pProgramState->settings.camera.cameraMode == vcCM_OrthoMap))
        vcCamera_SwapMapMode(pProgramState);

      if (pProgramState->settings.presentation.showCameraInfo)
      {
        ImGui::Separator();

        if (ImGui::InputScalarN(vcString::Get("sceneCameraPosition"), ImGuiDataType_Double, &pProgramState->pCamera->position.x, 3))
        {
          // limit the manual entry of camera position to +/- 40000000
          pProgramState->pCamera->position.x = udClamp(pProgramState->pCamera->position.x, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->pCamera->position.y = udClamp(pProgramState->pCamera->position.y, -vcSL_GlobalLimit, vcSL_GlobalLimit);
          pProgramState->pCamera->position.z = udClamp(pProgramState->pCamera->position.z, -vcSL_GlobalLimit, vcSL_GlobalLimit);
        }

        pProgramState->pCamera->eulerRotation = UD_RAD2DEG(pProgramState->pCamera->eulerRotation);
        ImGui::InputScalarN(vcString::Get("sceneCameraRotation"), ImGuiDataType_Double, &pProgramState->pCamera->eulerRotation.x, 3);
        pProgramState->pCamera->eulerRotation = UD_DEG2RAD(pProgramState->pCamera->eulerRotation);

        if (ImGui::SliderFloat(vcString::Get("sceneCameraMoveSpeed"), &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
          pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_GlobalLimitSmallf);

        if (pProgramState->gis.isProjected)
        {
          ImGui::Separator();

          udDouble3 cameraLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->pCamera->matrices.camera.axis.t.toVector3());

          char tmpBuf[128];
          const char *latLongAltStrings[] = { udTempStr("%.7f", cameraLatLong.x), udTempStr("%.7f", cameraLatLong.y), udTempStr("%.2fm", cameraLatLong.z) };
          ImGui::TextUnformatted(vStringFormat(tmpBuf, udLengthOf(tmpBuf), vcString::Get("sceneCameraLatLongAlt"), latLongAltStrings, udLengthOf(latLongAltStrings)));

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

    if (ImGui::Begin(vcString::Get("sceneMapCopyright"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::TextUnformatted(vcString::Get("sceneMapData"));
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
  renderData.pCamera = pProgramState->pCamera;
  renderData.models.Init(32);
  renderData.fences.Init(32);
  renderData.labels.Init(32);
  renderData.waterVolumes.Init(32);
  renderData.polyModels.Init(64);
  renderData.images.Init(32);
  renderData.sceneLayers.Init(32);
  renderData.mouse.position.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.position.y = (uint32_t)(io.MousePos.y - windowPos.y);
  renderData.mouse.clicked = io.MouseClicked[1];

  udDouble3 cameraMoveOffset = udDouble3::zero();

  if (pProgramState->sceneResolution.x != windowSize.x || pProgramState->sceneResolution.y != windowSize.y) //Resize buffers
  {
    pProgramState->sceneResolution = udUInt2::create((uint32_t)windowSize.x, (uint32_t)windowSize.y);
    vcRender_ResizeScene(pProgramState->pRenderContext, pProgramState->sceneResolution.x, pProgramState->sceneResolution.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  }

  if (!pProgramState->modalOpen && (ImGui::IsKeyPressed(SDL_SCANCODE_F5, false) || (io.NavInputs[ImGuiNavInput_TweakFast] && !io.NavInputsDownDuration[ImGuiNavInput_TweakFast]))) // Start Button
    vcMain_PresentationMode(pProgramState);
  if (pProgramState->settings.responsiveUI == vcPM_Show)
    pProgramState->showUI = true;

  // use some data from previous frame
  pProgramState->worldMousePos = pProgramState->previousWorldMousePos;
  pProgramState->pickingSuccess = pProgramState->previousPickingSuccess;
  if (pProgramState->cameraInput.isUsingAnchorPoint)
    renderData.pWorldAnchorPos = &pProgramState->cameraInput.worldAnchorPoint;

  if (!pProgramState->settings.window.presentationMode || pProgramState->settings.responsiveUI == vcPM_Show || pProgramState->showUI)
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

    static bool wasOpenLastFrame = false;

    if (io.MouseDragMaxDistanceSqr[1] < (io.MouseDragThreshold*io.MouseDragThreshold) && ImGui::BeginPopupContextItem("SceneContext"))
    {
      static bool hadMouse = false;
      static udDouble3 mousePosLongLat;

      if (!wasOpenLastFrame || ImGui::IsMouseClicked(1))
      {
        hadMouse = pProgramState->pickingSuccess;
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

            // All nodes of a POI must be in the same projection space
            if ((((pPOI->m_pCurrentProjection == nullptr && !pProgramState->gis.isProjected) || (pPOI->m_pCurrentProjection != nullptr && pPOI->m_pCurrentProjection->srid == pProgramState->gis.SRID)) && ImGui::MenuItem(vcString::Get("scenePOIAddPoint"))))
              pPOI->AddPoint(mousePosLongLat);
          }
        }

        if (ImGui::BeginMenu(vcString::Get("sceneAddMenu")))
        {
          if (ImGui::MenuItem(vcString::Get("sceneAddPOI")))
          {
            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "POI", vcString::Get("scenePOIDefaultName"), nullptr, nullptr) == vE_Success)
              vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, &mousePosLongLat.x);

            ImGui::CloseCurrentPopup();
          }
          if (ImGui::MenuItem(vcString::Get("sceneAddAOI")))
          {
            vcProject_ClearSelection(pProgramState);

            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "POI", vcString::Get("scenePOIAreaDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Polygon, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
            }

            ImGui::CloseCurrentPopup();
          }
          if (ImGui::MenuItem(vcString::Get("sceneAddLine")))
          {
            vcProject_ClearSelection(pProgramState);

            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "POI", vcString::Get("scenePOILineDefaultName"), nullptr, nullptr) == vE_Success)
            {
              vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_LineString, 1, &mousePosLongLat.x);
              udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
            }

            ImGui::CloseCurrentPopup();
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
          pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
          pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
          pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMouseClickedPos;
          pProgramState->cameraInput.progress = 0.0;
        }
      }
      else
      {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
      wasOpenLastFrame = true;
    }
    else
    {
      wasOpenLastFrame = false;
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
        vcGizmoAllowedControls allowedControls = vcGAC_AllUniform;

        if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
          allowedControls = (vcGizmoAllowedControls)(allowedControls & ~vcGAC_RotationAxis);

        vcGizmo_Manipulate(pProgramState->pCamera, pProgramState->gizmo.operation, pProgramState->gizmo.coordinateSystem, temp, &delta, allowedControls, io.KeyShift ? snapAmt : 0.0);

        if (!(delta == udDouble4x4::identity()))
        {
          for (vcSceneItemRef &ref : pProgramState->sceneExplorer.selectedItems)
            ((vcSceneItem*)ref.pItem->pUserData)->ApplyDelta(pProgramState, delta);
        }
      }
    }
  }

  renderData.deltaTime = pProgramState->deltaTime;
  renderData.pGISSpace = &pProgramState->gis;
  renderData.pCameraSettings = &pProgramState->settings.camera;

  pProgramState->sceneExplorer.pProjectRoot->AddToScene(pProgramState, &renderData);

  vcRender_vcRenderSceneImGui(pProgramState->pRenderContext, renderData);

  // Render scene to texture
  vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);
  renderData.models.Deinit();
  renderData.fences.Deinit();
  renderData.labels.Deinit();
  renderData.waterVolumes.Deinit();
  renderData.polyModels.Deinit();
  renderData.images.Deinit();
  renderData.sceneLayers.Deinit();

  // Can only assign longlat positions in projected space
  if (pProgramState->gis.isProjected)
  {
    pProgramState->worldMousePosLongLat = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->worldMousePos, true);
    pProgramState->pCamera->positionInLongLat = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->pCamera->position, true);
  }
  else
  {
    pProgramState->worldMousePosLongLat = pProgramState->worldMousePos;
    pProgramState->pCamera->positionInLongLat = pProgramState->pCamera->position;
  }

  pProgramState->previousWorldMousePos = renderData.worldMousePos;
  pProgramState->previousPickingSuccess = renderData.pickingSuccess;
  pProgramState->pSceneWatermark = renderData.pWatermarkTexture;

  if (renderData.mouse.clicked)
    pProgramState->worldMouseClickedPos = renderData.worldMousePos;
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu(vcString::Get("menuSystem")))
    {
      if (ImGui::MenuItem(vcString::Get("menuLogout")))
        vcLogout(pProgramState);

      if (ImGui::MenuItem(vcString::Get("menuRestoreDefaults"), nullptr))
      {
        vcMain_LoadSettings(pProgramState, true);
        vcRender_ClearTiles(pProgramState->pRenderContext); // refresh map tiles since they just got updated
      }

      if (ImGui::MenuItem(vcString::Get("menuAbout")))
        vcModals_OpenModal(pProgramState, vcMT_About);

      if (ImGui::MenuItem(vcString::Get("menuReleaseNotes")))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

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
        vcProject_RemoveAll(pProgramState);

      if (ImGui::MenuItem(vcString::Get("menuProjectExport"), nullptr, nullptr))
        vcModals_OpenModal(pProgramState, vcMT_ExportProject);

      if (ImGui::MenuItem(vcString::Get("menuProjectImport"), nullptr, nullptr))
        vcModals_OpenModal(pProgramState, vcMT_ImportProject);

      ImGui::Separator();

      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcProject_RemoveAll(pProgramState);
          bool moveTo = true;

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
          {
            vdkProjectNode *pNode = nullptr;
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "UDS", nullptr, pProjectList->GetElement(i)->Get("models[%zu]", j).AsString(), nullptr) != vE_Success)
            {
              vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
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
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "IOT", pFeedName, nullptr, nullptr) != vE_Success)
              vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);

            if (vUUID_IsValid(pProjectList->GetElement(i)->Get("feeds[%zu].groupid", j).AsString()))
              vdkProjectNode_SetMetadataString(pNode, "groupid", pProjectList->GetElement(i)->Get("feeds[%zu].groupid", j).AsString());
          }
        }
      }

      ImGui::EndMenu();
    }

    char endBarInfo[512] = {};
    char tempData[128] = {};

    if (pProgramState->loadList.size() > 0)
    {
      const char *strings[] = { udTempStr("%zu", pProgramState->loadList.size()) };
      udStrcat(endBarInfo, udLengthOf(endBarInfo), vStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFilesQueued"), strings, udLengthOf(strings)));
      udStrcat(endBarInfo, udLengthOf(endBarInfo), " / ");
    }

    if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0)
    {
      udStrcat(endBarInfo, udLengthOf(endBarInfo), vcString::Get("menuBarInactive"));
      udStrcat(endBarInfo, udLengthOf(endBarInfo), " / ");
    }

    if (pProgramState->packageInfo.Get("success").AsBool())
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s [%s] / ", vcString::Get("menuBarUpdateAvailable"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    if (pProgramState->settings.presentation.showDiagnosticInfo)
    {
      const char *strings[] = { udTempStr("%.2f", 1.f / pProgramState->deltaTime), udTempStr("%.3f", pProgramState->deltaTime * 1000.f) };
      udStrcat(endBarInfo, udLengthOf(endBarInfo), vStringFormat(tempData, udLengthOf(tempData), vcString::Get("menuBarFPS"), strings, udLengthOf(strings)));
      udStrcat(endBarInfo, udLengthOf(endBarInfo), " / ");
    }

    int64_t currentTime = vcTime_GetEpochSecs();

    for (int i = 0; i < vdkLT_Count; ++i)
    {
      vdkLicenseInfo info = {};
      if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, (vdkLicenseType)i, &info) == vE_Success)
      {
        if (info.queuePosition < 0 && (uint64_t)currentTime < info.expiresTimestamp)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s (%" PRIu64 "%s) / ", i == vdkLT_Render ? vcString::Get("menuBarRender") : vcString::Get("menuBarConvert"), vcString::Get("menuBarLicense"), (info.expiresTimestamp - currentTime), vcString::Get("menuBarSecondsAbbreviation")));
        else if (info.queuePosition < 0)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s / ", i == vdkLT_Render ? vcString::Get("menuBarRender") : vcString::Get("menuBarConvert"), vcString::Get("menuBarLicenseExpired")));
        else
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s %s (%" PRId64 " %s) / ", i == vdkLT_Render ? vcString::Get("menuBarRender") : vcString::Get("menuBarConvert"), vcString::Get("menuBarLicense"), info.queuePosition, vcString::Get("menuBarLicenseQueued")));
      }
    }

    udStrcat(endBarInfo, udLengthOf(endBarInfo), pProgramState->username);

    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(endBarInfo).x - 25);
    ImGui::TextUnformatted(endBarInfo);

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
        ImGui::TextUnformatted(vcString::Get("menuBarConnectionStatus"));
        ImGui::EndTooltip();
      }
    }

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

void vcRenderWindow(vcState *pProgramState)
{
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  vcGLState_SetViewport(0, 0, pProgramState->settings.window.width, pProgramState->settings.window.height);
  vcFramebuffer_Clear(pProgramState->pDefaultFramebuffer, 0xFF000000);

  ImGuiIO &io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

  ImGui::RenderPlatformWindowsDefault();

  if (pProgramState->settings.responsiveUI == vcPM_Responsive)
  {
    if (io.MouseDelta.x != 0.0 || io.MouseDelta.y != 0.0)
    {
      pProgramState->lastEventTime = vcTime_GetEpochSecs();
      pProgramState->showUI = true;
    }
    else if ((vcTime_GetEpochSecs() - pProgramState->lastEventTime) > pProgramState->settings.hideIntervalSeconds)
    {
      pProgramState->showUI = false;
    }
  }

#if UDPLATFORM_WINDOWS
  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;

  if (io.KeyCtrl && ImGui::IsKeyPressed(SDL_SCANCODE_M))
    vcCamera_SwapMapMode(pProgramState);
#endif

  //end keyboard/mouse handling

  if (pProgramState->hasContext && !pProgramState->settings.window.presentationMode)
  {
    int margin = vcMainMenuGui(pProgramState);

    if (!pProgramState->settings.docksLoaded)
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
    enum vcLoginBackgroundSettings
    {
      vcLBS_LoginBoxY = 40,
      vcLBS_LoginBoxH = 280,
      vcLBS_LogoW = 375,
      vcLBS_LogoH = 577,
    };

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));

    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::SetNextWindowPos(ImVec2(0, size.y), ImGuiCond_Always, ImVec2(0, 1));
    if (ImGui::Begin("LoginScreenPopups", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
    {
      ImGui::PushItemWidth(130.f);

      if (ImGui::Button(vcString::Get("loginReleaseNotes")))
        vcModals_OpenModal(pProgramState, vcMT_ReleaseNotes);

      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("loginAbout")))
        vcModals_OpenModal(pProgramState, vcMT_About);

      ImGui::SameLine();

      vcMain_LangCombo(pProgramState);

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
      const char *pIssueTrackerStr = vStringFormat(vcString::Get("loginSupportIssueTracker"), issueTrackerStrings, udLengthOf(issueTrackerStrings));
      ImGui::TextUnformatted(pIssueTrackerStr);
      udFree(pIssueTrackerStr);
      if (ImGui::IsItemClicked())
        vcWebFile_OpenBrowser("https://github.com/euclideon/vaultclient/issues");
      if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

      const char *pSupportStr = vStringFormat(vcString::Get("loginSupportDirectEmail"), "support@euclideon.com");
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
      const char *pTranslationInfo = vStringFormat(vcString::Get("loginTranslationBy"), translationStrings, udLengthOf(translationStrings));
      ImGui::TextUnformatted(pTranslationInfo);
      udFree(pTranslationInfo);
      if (ImGui::IsItemClicked())
        vcWebFile_OpenBrowser(udTempStr("mailto:%s", pProgramState->languageInfo.pTranslatorContactEmail));
      if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0.f, 0.f));

    if (ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
    {
      ImVec2 p0 = ImVec2((size.x - 2048) / 2, size.y - 512);
      ImVec2 p1 = ImVec2((size.x + 2048) / 2, size.y);

      static float mouseX = 0.f;
      static float mouseY = 0.f;
      if (io.MousePos.x != -FLT_MAX)
      {
        mouseX = (io.MousePos.x / size.x) * 2.f - 1.f;
        mouseY = (io.MousePos.y / size.y);
      }

      ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(0, 0), size, 0xFFB5A245, 0xFFE3D9A8, 0xFFCDBC71, 0xFF998523);

      ImGui::GetWindowDrawList()->AddImage(pProgramState->pBuildingsTexture, ImVec2(p0.x + mouseX * 03.f + 100.f, p0.y + mouseY * 0.f), ImVec2(p1.x + mouseX * 03.f + 100.f, p1.y + mouseY * 0.f), ImVec2(0, 0.75), ImVec2(1, 1.00));
      ImGui::GetWindowDrawList()->AddImage(pProgramState->pBuildingsTexture, ImVec2(p0.x + mouseX * 15.f + 350.f, p0.y + mouseY * 1.f), ImVec2(p1.x + mouseX * 15.f + 350.f, p1.y + mouseY * 1.f), ImVec2(0, 0.50), ImVec2(1, 0.75));
      ImGui::GetWindowDrawList()->AddImage(pProgramState->pBuildingsTexture, ImVec2(p0.x + mouseX * 40.f - 230.f, p0.y + mouseY * 2.f), ImVec2(p1.x + mouseX * 40.f - 230.f, p1.y + mouseY * 2.f), ImVec2(0, 0.25), ImVec2(1, 0.50));
      ImGui::GetWindowDrawList()->AddImage(pProgramState->pBuildingsTexture, ImVec2(p0.x + mouseX * 70.f - 080.f, p0.y + mouseY * 3.f), ImVec2(p1.x + mouseX * 70.f - 080.f, p1.y + mouseY * 3.f), ImVec2(0, 0.00), ImVec2(1, 0.25));

      float scaling = udMin(0.9f * (size.y - vcLBS_LoginBoxH) / vcLBS_LogoH, 1.f);
      float yOff = (size.y - vcLBS_LoginBoxH) / 2.f;
      ImGui::GetWindowDrawList()->AddImage(pProgramState->pCompanyLogo, ImVec2((size.x - vcLBS_LogoW * scaling) / 2.f, yOff - (vcLBS_LogoH * scaling * 0.5f)), ImVec2((size.x + vcLBS_LogoW * scaling) / 2, yOff + (vcLBS_LogoH * scaling * 0.5f)));
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    ImGui::PopStyleColor(); // Border Colour

    ImGui::SetNextWindowSize(ImVec2(500, 160), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(size.x / 2, size.y - vcLBS_LoginBoxY), ImGuiCond_Always, ImVec2(0.5, 1.0));

    const char *loginStatusKeys[] = { "loginMessageCredentials", "loginMessageCredentials", "loginPending", "loginErrorConnection", "loginErrorAuth", "loginErrorTimeSync", "loginErrorSecurity", "loginErrorNegotiate", "loginErrorProxy", "loginErrorProxyAuthPending", "loginErrorProxyAuthPending", "loginErrorProxyAuthFailed", "loginErrorOther" };

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
          pProgramState->passFocus = false;
          pProgramState->loginStatus = vcLS_Pending;
          vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLogin, pProgramState, false);
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
            udStrcpy(pProgramState->settings.loginInfo.userAgent, vcMaxPathLength, UAStrings[UAIndex]);
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
  else
  {
    if (pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer] && !pProgramState->settings.window.presentationMode)
    {
      if (ImGui::Begin(udTempStr("%s###sceneExplorerDock", vcString::Get("sceneExplorerTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_SceneExplorer]))
      {
        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddUDS"), vcString::Get("sceneExplorerAddUDSKey"), vcMBBI_AddPointCloud, vcMBBG_FirstItem) || (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeysDown[SDL_SCANCODE_U]))
          vcModals_OpenModal(pProgramState, vcMT_AddUDS);

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddFolder"), nullptr, vcMBBI_AddFolder, vcMBBG_SameGroup))
        {
          vdkProjectNode *pNode = nullptr;
          if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, &pNode, "Folder", vcString::Get("sceneExplorerFolderDefaultName"), nullptr, nullptr) != vE_Success)
            vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
        }

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerAddViewpoint"), nullptr, vcMBBI_SaveViewport, vcMBBG_SameGroup))
        {
          if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, nullptr, "Camera", vcString::Get("viewpointDefaultName"), nullptr, nullptr))
            vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
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
                pPOI->AddPoint(pProgramState->pCamera->positionInLongLat);
            }
          }

          if (ImGui::MenuItem(vcString::Get("sceneExplorerAddFeed"), nullptr, nullptr))
          {
            if (vdkProjectNode_Create(pProgramState->sceneExplorer.pProject, nullptr, "IOT", vcString::Get("liveFeedDefaultName"), nullptr, nullptr))
              vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
          }

          ImGui::EndPopup();
        }

        if (vcMenuBarButton(pProgramState->pUITexture, vcString::Get("sceneExplorerRemove"), vcString::Get("sceneExplorerRemoveKey"), vcMBBI_Remove, vcMBBG_NewGroup) || (ImGui::GetIO().KeysDown[SDL_SCANCODE_DELETE] && !ImGui::IsAnyItemActive()))
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

                vdkProjectNode_MoveChild(pProgramState->sceneExplorer.pProject, item.pParent, pProgramState->sceneExplorer.insertItem.pParent, pNode, pProgramState->sceneExplorer.insertItem.pItem);

                // Update the selected item information to repeat drag and drop
                pProgramState->sceneExplorer.selectedItems[i].pParent = pProgramState->sceneExplorer.insertItem.pParent;

                pProgramState->sceneExplorer.clickedItem = pProgramState->sceneExplorer.selectedItems[i];
              }
            }
            pProgramState->sceneExplorer.insertItem = { nullptr, nullptr };
          }

          size_t i = 0;
          if (pProgramState->sceneExplorer.pProjectRoot)
            pProgramState->sceneExplorer.pProjectRoot->HandleImGui(pProgramState, &i);
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
    {
      if (ImGui::Begin(udTempStr("%s###settingsDock", vcString::Get("settingsTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_Settings]))
      {
        bool opened = ImGui::CollapsingHeader(vcString::Get("AppearanceID"));
        if (ImGui::BeginPopupContextItem("AppearanceContext"))
        {
          if (ImGui::Selectable(vcString::Get("AppearanceRestore")))
          {
            vcSettings_Load(&pProgramState->settings, true, vcSC_Appearance);
          }
          ImGui::EndPopup();
        }
        if (opened)
        {
          int styleIndex = pProgramState->settings.presentation.styleIndex - 1;

          const char *themeOptions[] = { vcString::Get("settingsAppearanceDark"), vcString::Get("settingsAppearanceLight") };
          if (ImGui::Combo(vcString::Get("settingsAppearanceTheme"), &styleIndex, themeOptions, (int)udLengthOf(themeOptions)))
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

          if (ImGui::SliderFloat(vcString::Get("settingsAppearancePOIDistance"), &pProgramState->settings.presentation.POIFadeDistance, vcSL_POIFaderMin, vcSL_POIFaderMax, "%.3fm", 3.f))
            pProgramState->settings.presentation.POIFadeDistance = udClamp(pProgramState->settings.presentation.POIFadeDistance, vcSL_POIFaderMin, vcSL_GlobalLimitf);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowDiagnostics"), &pProgramState->settings.presentation.showDiagnosticInfo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceAdvancedGIS"), &pProgramState->settings.presentation.showAdvancedGIS);
          ImGui::Checkbox(vcString::Get("settingsAppearanceLimitFPS"), &pProgramState->settings.presentation.limitFPSInBackground);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowCompass"), &pProgramState->settings.presentation.showCompass);

          const char *presentationOptions[] = { vcString::Get("settingsAppearanceHide"), vcString::Get("settingsAppearanceShow"), vcString::Get("settingsAppearanceResponsive") };
          if (ImGui::Combo(vcString::Get("settingsAppearancePresentationUI"), (int*)&pProgramState->settings.responsiveUI, presentationOptions, (int)udLengthOf(presentationOptions)))
            pProgramState->showUI = false;

          const char *anchorOptions[] = { vcString::Get("settingsAppearanceNone"), vcString::Get("settingsAppearanceOrbit"), vcString::Get("settingsAppearanceCompass") };
          ImGui::Combo(vcString::Get("settingsAppearanceMouseAnchor"), (int*)&pProgramState->settings.presentation.mouseAnchor, anchorOptions, (int)udLengthOf(anchorOptions));

          const char *voxelOptions[] = { vcString::Get("settingsAppearanceRectangles"), vcString::Get("settingsAppearanceCubes"), vcString::Get("settingsAppearancePoints") };
          ImGui::Combo(vcString::Get("settingsAppearanceVoxelShape"), &pProgramState->settings.presentation.pointMode, voxelOptions, (int)udLengthOf(voxelOptions));
        }

        bool opened2 = ImGui::CollapsingHeader(vcString::Get("InputControlsID"));
        if (ImGui::BeginPopupContextItem("InputContext"))
        {
          if (ImGui::Selectable(vcString::Get("InputRestore")))
          {
            vcSettings_Load(&pProgramState->settings, true, vcSC_InputControls);
          }
          ImGui::EndPopup();
        }
        if (opened2)
        {
          ImGui::Checkbox(vcString::Get("settingsControlsOSC"), &pProgramState->settings.onScreenControls);
          if (ImGui::Checkbox(vcString::Get("settingsControlsTouchUI"), &pProgramState->settings.window.touchscreenFriendly))
          {
            ImGuiStyle& style = ImGui::GetStyle();
            style.TouchExtraPadding = pProgramState->settings.window.touchscreenFriendly ? ImVec2(4, 4) : ImVec2();
          }

          ImGui::Checkbox(vcString::Get("settingsControlsInvertX"), &pProgramState->settings.camera.invertX);
          ImGui::Checkbox(vcString::Get("settingsControlsInvertY"), &pProgramState->settings.camera.invertY);

          ImGui::TextUnformatted(vcString::Get("settingsControlsMousePivot"));
          const char *mouseModes[] = { vcString::Get("settingsControlsTumble"), vcString::Get("settingsControlsOrbit"), vcString::Get("settingsControlsPan"), vcString::Get("settingsControlsForward") };
          const char *scrollwheelModes[] = { vcString::Get("settingsControlsDolly"), vcString::Get("settingsControlsChangeMoveSpeed") };

          // Checks so the casts below are safe
          UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.cameraMouseBindings[0]) == sizeof(int), "Bindings is no longer sizeof(int)");
          UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.scrollWheelMode) == sizeof(int), "ScrollWheel is no longer sizeof(int)");

          ImGui::Combo(vcString::Get("settingsControlsLeft"), (int*)&pProgramState->settings.camera.cameraMouseBindings[0], mouseModes, (int)udLengthOf(mouseModes));
          ImGui::Combo(vcString::Get("settingsControlsMiddle"), (int*)&pProgramState->settings.camera.cameraMouseBindings[2], mouseModes, (int)udLengthOf(mouseModes));
          ImGui::Combo(vcString::Get("settingsControlsRight"), (int*)&pProgramState->settings.camera.cameraMouseBindings[1], mouseModes, (int)udLengthOf(mouseModes));
          ImGui::Combo(vcString::Get("settingsControlsScrollWheel"), (int*)&pProgramState->settings.camera.scrollWheelMode, scrollwheelModes, (int)udLengthOf(scrollwheelModes));
        }

        bool opened3 = ImGui::CollapsingHeader(vcString::Get("ViewportID"));
        if (ImGui::BeginPopupContextItem("ViewportContext"))
        {
          if (ImGui::Selectable(vcString::Get("ViewportRestore")))
          {
            vcSettings_Load(&pProgramState->settings, true, vcSC_Viewport);
          }
          ImGui::EndPopup();
        }
        if (opened3)
        {
          if (ImGui::SliderFloat(vcString::Get("settingsViewportViewDistance"), &pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
          {
            pProgramState->settings.camera.nearPlane = pProgramState->settings.camera.farPlane * vcSL_CameraFarToNearPlaneRatio;
          }

          //const char *pLensOptions = " Custom FoV\0 7mm\0 11mm\0 15mm\0 24mm\0 30mm\0 50mm\0 70mm\0 100mm\0";
          if (ImGui::Combo(vcString::Get("settingsViewportCameraLens"), &pProgramState->settings.camera.lensIndex, vcCamera_GetLensNames(), vcLS_TotalLenses))
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
            if (ImGui::SliderFloat(vcString::Get("settingsViewportFOV"), &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, vcString::Get("DegreesFormat")))
              pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
          }
        }

        bool opened4 = ImGui::CollapsingHeader(vcString::Get("ElevationFormat"));
        if (ImGui::BeginPopupContextItem("MapsContext"))
        {
          if (ImGui::Selectable(vcString::Get("MapsRestore")))
          {
            vcSettings_Load(&pProgramState->settings, true, vcSC_MapsElevation);
            vcRender_ClearTiles(pProgramState->pRenderContext); // refresh map tiles since they just got updated
          }
          ImGui::EndPopup();
        }
        if (opened4)
        {
          ImGui::Checkbox(vcString::Get("settingsMapsMapTiles"), &pProgramState->settings.maptiles.mapEnabled);

          if (pProgramState->settings.maptiles.mapEnabled)
          {
            ImGui::Checkbox(vcString::Get("settingsMapsMouseLock"), &pProgramState->settings.maptiles.mouseInteracts);

            if (ImGui::Button(vcString::Get("settingsMapsTileServerButton"), ImVec2(-1, 0)))
              vcModals_OpenModal(pProgramState, vcMT_TileServer);

            if (ImGui::SliderFloat(vcString::Get("settingsMapsMapHeight"), &pProgramState->settings.maptiles.mapHeight, vcSL_MapHeightMin, vcSL_MapHeightMax, "%.3fm", 2.f))
              pProgramState->settings.maptiles.mapHeight = udClamp(pProgramState->settings.maptiles.mapHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);

            const char* blendModes[] = { vcString::Get("settingsMapsHybrid"), vcString::Get("settingsMapsOverlay"), vcString::Get("settingsMapsUnderlay") };
            if (ImGui::BeginCombo(vcString::Get("settingsMapsBlending"), blendModes[pProgramState->settings.maptiles.blendMode]))
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

            if (ImGui::SliderFloat(vcString::Get("settingsMapsOpacity"), &pProgramState->settings.maptiles.transparency, vcSL_OpacityMin, vcSL_OpacityMax, "%.3f"))
              pProgramState->settings.maptiles.transparency = udClamp(pProgramState->settings.maptiles.transparency, vcSL_OpacityMin, vcSL_OpacityMax);

            if (ImGui::Button(vcString::Get("settingsMapsSetHeight")))
              pProgramState->settings.maptiles.mapHeight = (float)pProgramState->pCamera->position.z;
          }
        }

        bool opened5 = ImGui::CollapsingHeader(vcString::Get("VisualizationFormat"));
        if (ImGui::BeginPopupContextItem("VisualizationContext"))
        {
          if (ImGui::Selectable(vcString::Get("VisualizationRestore")))
          {
            vcSettings_Load(&pProgramState->settings, true, vcSC_Visualization);
          }
          ImGui::EndPopup();
        }
        if (opened5)
        {
          const char *visualizationModes[] = { vcString::Get("settingsVisModeColour"), vcString::Get("settingsVisModeIntensity"), vcString::Get("settingsVisModeClassification") };
          ImGui::Combo(vcString::Get("settingsVisDisplayMode"), (int*)&pProgramState->settings.visualization.mode, visualizationModes, (int)udLengthOf(visualizationModes));

          if (pProgramState->settings.visualization.mode == vcVM_Intensity)
          {
            vcIGSW_StickyIntSlider(vcString::Get("settingsVisMinIntensity"), &pProgramState->settings.visualization.minIntensity, (int)vcSL_IntensityMin, pProgramState->settings.visualization.maxIntensity, 255);
            vcIGSW_StickyIntSlider(vcString::Get("settingsVisMaxIntensity"), &pProgramState->settings.visualization.maxIntensity, pProgramState->settings.visualization.minIntensity, (int)vcSL_IntensityMax, 255);
          }

          if (pProgramState->settings.visualization.mode == vcVM_Classification)
          {
            ImGui::Checkbox(vcString::Get("settingsVisClassShowColourTable"), &pProgramState->settings.visualization.useCustomClassificationColours);

            if (pProgramState->settings.visualization.useCustomClassificationColours)
            {
              ImGui::SameLine();
              if (ImGui::Button(vcString::Get("RestoreColoursID")))
                memcpy(pProgramState->settings.visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pProgramState->settings.visualization.customClassificationColors));

              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassNeverClassified"), &pProgramState->settings.visualization.customClassificationColors[0], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassUnclassified"), &pProgramState->settings.visualization.customClassificationColors[1], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassGround"), &pProgramState->settings.visualization.customClassificationColors[2], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassLowVegetation"), &pProgramState->settings.visualization.customClassificationColors[3], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassMediumVegetation"), &pProgramState->settings.visualization.customClassificationColors[4], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassHighVegetation"), &pProgramState->settings.visualization.customClassificationColors[5], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassBuilding"), &pProgramState->settings.visualization.customClassificationColors[6], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassLowPoint"), &pProgramState->settings.visualization.customClassificationColors[7], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassKeyPoint"), &pProgramState->settings.visualization.customClassificationColors[8], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassWater"), &pProgramState->settings.visualization.customClassificationColors[9], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassRail"), &pProgramState->settings.visualization.customClassificationColors[10], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassRoadSurface"), &pProgramState->settings.visualization.customClassificationColors[11], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassReserved"), &pProgramState->settings.visualization.customClassificationColors[12], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassWireGuard"), &pProgramState->settings.visualization.customClassificationColors[13], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassWireConductor"), &pProgramState->settings.visualization.customClassificationColors[14], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassTransmissionTower"), &pProgramState->settings.visualization.customClassificationColors[15], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassWireStructureConnector"), &pProgramState->settings.visualization.customClassificationColors[16], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassBridgeDeck"), &pProgramState->settings.visualization.customClassificationColors[17], ImGuiColorEditFlags_NoAlpha);
              vcIGSW_ColorPickerU32(vcString::Get("settingsVisClassHighNoise"), &pProgramState->settings.visualization.customClassificationColors[18], ImGuiColorEditFlags_NoAlpha);

              if (ImGui::TreeNode(vcString::Get("settingsVisClassReservedColours")))
              {
                for (int i = 19; i < 64; ++i)
                  vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, vcString::Get("settingsVisClassReservedLabels")), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                ImGui::TreePop();
              }

              if (ImGui::TreeNode(vcString::Get("settingsVisClassUserDefinable")))
              {
                for (int i = 64; i <= 255; ++i)
                {
                  char buttonID[12], inputID[3];
                  if (pProgramState->settings.visualization.customClassificationColorLabels[i] == nullptr)
                    vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, vcString::Get("settingsVisClassUserDefined")), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                  else
                    vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, pProgramState->settings.visualization.customClassificationColorLabels[i]), &pProgramState->settings.visualization.customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
                  udSprintf(buttonID, 12, "%s##%d", vcString::Get("settingsVisClassRename"), i);
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
                    if (ImGui::Button(vcString::Get("settingsVisClassSet")))
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
          ImGui::Checkbox(vcString::Get("settingsVisEdge"), &pProgramState->settings.postVisualization.edgeOutlines.enable);
          if (pProgramState->settings.postVisualization.edgeOutlines.enable)
          {
            if (ImGui::SliderInt(vcString::Get("settingsVisEdgeWidth"), &pProgramState->settings.postVisualization.edgeOutlines.width, vcSL_EdgeHighlightMin, vcSL_EdgeHighlightMax))
              pProgramState->settings.postVisualization.edgeOutlines.width = udClamp(pProgramState->settings.postVisualization.edgeOutlines.width, vcSL_EdgeHighlightMin, vcSL_EdgeHighlightMax);

            // TODO: Make this less awful. 0-100 would make more sense than 0.0001 to 0.001.
            if (ImGui::SliderFloat(vcString::Get("settingsVisEdgeThreshold"), &pProgramState->settings.postVisualization.edgeOutlines.threshold, vcSL_EdgeHighlightThresholdMin, vcSL_EdgeHighlightThresholdMax, "%.3f", 2))
              pProgramState->settings.postVisualization.edgeOutlines.threshold = udClamp(pProgramState->settings.postVisualization.edgeOutlines.threshold, vcSL_EdgeHighlightThresholdMin, vcSL_EdgeHighlightThresholdMax);
            ImGui::ColorEdit4(vcString::Get("settingsVisEdgeColour"), &pProgramState->settings.postVisualization.edgeOutlines.colour.x);
          }

          // Post visualization - Colour by Height
          ImGui::Checkbox(vcString::Get("settingsVisHeight"), &pProgramState->settings.postVisualization.colourByHeight.enable);
          if (pProgramState->settings.postVisualization.colourByHeight.enable)
          {
            ImGui::ColorEdit4(vcString::Get("settingsVisHeightStartColour"), &pProgramState->settings.postVisualization.colourByHeight.minColour.x);
            ImGui::ColorEdit4(vcString::Get("settingsVisHeightEndColour"), &pProgramState->settings.postVisualization.colourByHeight.maxColour.x);

            // TODO: Set min/max to the bounds of the model? Currently set to 0m -> 1km with accuracy of 1mm
            if (ImGui::SliderFloat(vcString::Get("settingsVisHeightStart"), &pProgramState->settings.postVisualization.colourByHeight.startHeight, vcSL_ColourByHeightMin, vcSL_ColourByHeightMax, "%.3f"))
              pProgramState->settings.postVisualization.colourByHeight.startHeight = udClamp(pProgramState->settings.postVisualization.colourByHeight.startHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
            if (ImGui::SliderFloat(vcString::Get("settingsVisHeightEnd"), &pProgramState->settings.postVisualization.colourByHeight.endHeight, vcSL_ColourByHeightMin, vcSL_ColourByHeightMax, "%.3f"))
              pProgramState->settings.postVisualization.colourByHeight.endHeight = udClamp(pProgramState->settings.postVisualization.colourByHeight.endHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
          }

          // Post visualization - Colour by Depth
          ImGui::Checkbox(vcString::Get("settingsVisDepth"), &pProgramState->settings.postVisualization.colourByDepth.enable);
          if (pProgramState->settings.postVisualization.colourByDepth.enable)
          {
            ImGui::ColorEdit4(vcString::Get("settingsVisDepthColour"), &pProgramState->settings.postVisualization.colourByDepth.colour.x);

            // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
            if (ImGui::SliderFloat(vcString::Get("settingsVisDepthStart"), &pProgramState->settings.postVisualization.colourByDepth.startDepth, vcSL_ColourByDepthMin, vcSL_ColourByDepthMax, "%.3f"))
              pProgramState->settings.postVisualization.colourByDepth.startDepth = udClamp(pProgramState->settings.postVisualization.colourByDepth.startDepth, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
            if (ImGui::SliderFloat(vcString::Get("settingsVisDepthEnd"), &pProgramState->settings.postVisualization.colourByDepth.endDepth, vcSL_ColourByDepthMin, vcSL_ColourByDepthMax, "%.3f"))
              pProgramState->settings.postVisualization.colourByDepth.endDepth = udClamp(pProgramState->settings.postVisualization.colourByDepth.endDepth, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
          }

          // Post visualization - Contours
          ImGui::Checkbox(vcString::Get("settingsVisContours"), &pProgramState->settings.postVisualization.contours.enable);
          if (pProgramState->settings.postVisualization.contours.enable)
          {
            ImGui::ColorEdit4(vcString::Get("settingsVisContoursColour"), &pProgramState->settings.postVisualization.contours.colour.x);

            // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
            if (ImGui::SliderFloat(vcString::Get("settingsVisContoursDistances"), &pProgramState->settings.postVisualization.contours.distances, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax, "%.3f", 2))
              pProgramState->settings.postVisualization.contours.distances = udClamp(pProgramState->settings.postVisualization.contours.distances, vcSL_ContourDistanceMin, vcSL_GlobalLimitSmallf);
            if (ImGui::SliderFloat(vcString::Get("settingsVisContoursBandHeight"), &pProgramState->settings.postVisualization.contours.bandHeight, vcSL_ContourBandHeightMin, vcSL_ContourBandHeightMax, "%.3f", 2))
              pProgramState->settings.postVisualization.contours.bandHeight = udClamp(pProgramState->settings.postVisualization.contours.bandHeight, vcSL_ContourBandHeightMin, vcSL_GlobalLimitSmallf);
          }
        }
      }
      ImGui::End();
    }

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

    if (!pProgramState->settings.docksLoaded)
    {
      vcSettings_Load(&pProgramState->settings, false, vcSC_Docks);

      // Don't show the window in a bad state
      ImGui::EndFrame();
      ImGui::NewFrame();
    }

    if (pProgramState->currentError != vE_Success)
    {
      ImGui::OpenPopup(vcString::Get("errorTitle"));
      ImGui::SetNextWindowSize(ImVec2(250, 60));
    }

    if (ImGui::BeginPopupModal(vcString::Get("errorTitle"), NULL, ImGuiWindowFlags_NoDecoration))
    {
      const char *pMessage;

      switch (pProgramState->currentError)
      {
      case vE_Failure:
        pMessage = vcString::Get("errorUnknown");
        break;
      case vE_OpenFailure:
        pMessage = vcString::Get("errorOpening");
        break;
      case vE_ReadFailure:
        pMessage = vcString::Get("errorReading");
        break;
      case vE_WriteFailure:
        pMessage = vcString::Get("errorWriting");
        break;
      default:
        pMessage = vcString::Get("errorUnknown");
        break;
      }

      ImGui::TextWrapped("%s", pMessage);

      if (ImGui::Button(vcString::Get("errorCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      {
        pProgramState->currentError = vE_Success;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
  vcModals_DrawModals(pProgramState);
}
