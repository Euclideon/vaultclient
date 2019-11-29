#include "vcSettings.h"

#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udFileHandler.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "vcClassificationColours.h"
#include "vcStringFormat.h"
#include "vcHotkey.h"

#if UDPLATFORM_EMSCRIPTEN
# include <emscripten.h>
# include <emscripten/threading.h>
#endif

const char DefaultFilename[] = "asset://defaultsettings.json";

void vcSettings_InitializePrefPath(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage)
    return;

  char *pPath = SDL_GetPrefPath("euclideon", "client");

  if (pPath != nullptr)
  {
    pSettings->pSaveFilePath = pPath;
    udSprintf(pSettings->cacheAssetPath, vcMaxPathLength, "%scache", pSettings->pSaveFilePath);
    if (udCreateDir(pSettings->cacheAssetPath) != udR_Success)
    {
      // Probably already exists
      // TODO: handle other failure conditions
    }
  }
  else
  {
    pSettings->noLocalStorage = true;
  }
}

const char* SDL_fileread(const char* filename)
{
  SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
  if (rw == NULL)
    return NULL;

  Sint64 res_size = SDL_RWsize(rw);
  char *res = udAllocType(char, res_size + 1, udAF_None);

  Sint64 nb_read_total = 0, nb_read = 1;
  char* buf = res;

  while (nb_read_total < res_size && nb_read != 0)
  {
    nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
    nb_read_total += nb_read;
    buf += nb_read;
  }

  SDL_RWclose(rw);

  if (nb_read_total != res_size)
  {
    udFree(res);
    return nullptr;
  }

  res[nb_read_total] = '\0';
  return res;
}

bool SDL_filewrite(const char* filename, const char *pStr, size_t bytes)
{
  bool success = false;

  SDL_RWops *rw = SDL_RWFromFile(filename, "wb");
  if (rw != NULL)
  {
    if (SDL_RWwrite(rw, pStr, 1, bytes) != bytes)
      success = false;
    else
      success = true;

    SDL_RWclose(rw);
  }

  return success;
}

bool vcSettings_Load(vcSettings *pSettings, bool forceReset /*= false*/, vcSettingCategory group /*= vcSC_All*/)
{
  ImGui::GetIO().IniFilename = NULL; // Disables auto save and load

  const char *pSavedData = nullptr;

  if (!pSettings->noLocalStorage && pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  if (!forceReset && pSettings->docksLoaded != vcSettings::vcDockLoaded::vcDL_ForceReset && pSettings->pSaveFilePath != nullptr)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, "%ssettings.json", pSettings->pSaveFilePath);

    if (udFile_Load(buffer, (void **)&pSavedData) != udR_Success)
      udFree(pSavedData); // Didn't load, let's free this in case it was allocated
  }

  // If we didn't load, let's try load the defaults
  if (pSavedData == nullptr && udFile_Load(DefaultFilename, (void **)&pSavedData) != udR_Success)
    udFree(pSavedData); // Didn't load, let's free this in case it was allocated

  udJSON data;
  if (pSavedData != nullptr)
    data.Parse(pSavedData);

  if (group == vcSC_All || group == vcSC_Appearance)
  {
    // Misc Settings
    pSettings->presentation.styleIndex = data.Get("style").AsInt(1); // dark style by default

    pSettings->presentation.showDiagnosticInfo = data.Get("showDiagnosticInfo").AsBool(false);
    pSettings->presentation.showEuclideonLogo = data.Get("showEuclideonLogo").AsBool(false);
    pSettings->presentation.showCameraInfo = data.Get("showCameraInfo").AsBool(true);
    pSettings->presentation.showProjectionInfo = data.Get("showGISInfo").AsBool(true);
    pSettings->presentation.showAdvancedGIS = data.Get("showAdvGISOptions").AsBool(false);
    pSettings->presentation.loginRenderLicense = data.Get("loginRenderLicense").AsBool(false);
    pSettings->presentation.showSkybox = data.Get("showSkybox").AsBool(true);
    pSettings->presentation.skyboxColour = data.Get("skyboxColour").AsFloat4(udFloat4::create(0.39f, 0.58f, 0.93f, 1.f));
    pSettings->presentation.mouseAnchor = (vcAnchorStyle)data.Get("mouseAnchor").AsInt(vcAS_Orbit);
    pSettings->presentation.showCompass = data.Get("showCompass").AsBool(true);
    pSettings->presentation.POIFadeDistance = data.Get("POIfadeDistance").AsFloat(10000.f);
    pSettings->presentation.limitFPSInBackground = data.Get("limitFPSInBackground").AsBool(true);
    pSettings->presentation.pointMode = data.Get("pointMode").AsInt();
    pSettings->responsiveUI = (vcPresentationMode)data.Get("responsiveUI").AsInt(vcPM_Hide);

    pSettings->objectHighlighting.colour = data.Get("objectHighlighting.colour").AsFloat4(udFloat4::create(0.925f, 0.553f, 0.263f, 1.0f));
    pSettings->objectHighlighting.thickness = data.Get("objectHighlighting.thickness").AsFloat(2.0f);

    switch (pSettings->presentation.styleIndex)
    {
    case 0: ImGui::StyleColorsClassic(); break;
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    }
  }

  if (group == vcSC_All || group == vcSC_InputControls)
  {
    pSettings->window.touchscreenFriendly = data.Get("window.touchscreenFriendly").AsBool(false);
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pSettings->onScreenControls = true;
#else
    pSettings->onScreenControls = false;
#endif
    pSettings->camera.invertMouseX = data.Get("camera.invertMouseX").AsBool(false);
    pSettings->camera.invertMouseY = data.Get("camera.invertMouseY").AsBool(false);
    pSettings->camera.invertControllerX = data.Get("camera.invertControllerX").AsBool(false);
    pSettings->camera.invertControllerY = data.Get("camera.invertControllerY").AsBool(false);
    pSettings->camera.cameraMouseBindings[0] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[0]").AsInt(vcCPM_Tumble);
    pSettings->camera.cameraMouseBindings[1] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[1]").AsInt(vcCPM_Pan);
    pSettings->camera.cameraMouseBindings[2] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[2]").AsInt(vcCPM_Orbit);
    pSettings->camera.scrollWheelMode = (vcCameraScrollWheelMode)data.Get("camera.scrollwheelBinding").AsInt(vcCSWM_Dolly);
  }

  if (group == vcSC_All || group == vcSC_Viewport)
  {
    pSettings->camera.nearPlane = data.Get("camera.nearPlane").AsFloat(0.5f);
    pSettings->camera.farPlane = data.Get("camera.farPlane").AsFloat(10000.f);
    pSettings->camera.lensIndex = data.Get("camera.lensId").AsInt(vcLS_30mm);
    pSettings->camera.fieldOfView = data.Get("camera.fieldOfView").AsFloat(vcLens30mm);
  }

  if (group == vcSC_All || group == vcSC_MapsElevation)
  {
    // Map Tiles
    pSettings->maptiles.mapEnabled = data.Get("maptiles.enabled").AsBool(true);
    pSettings->maptiles.mouseInteracts = data.Get("maptiles.mouseInteracts").AsBool(true);
    udStrcpy(pSettings->maptiles.tileServerAddress, data.Get("maptiles.serverURL").AsString("http://slippy.vault.euclideon.com/"));
    if (udStrEquali(pSettings->maptiles.tileServerAddress, "http://20.188.211.58"))
      udStrcpy(pSettings->maptiles.tileServerAddress, "http://slippy.vault.euclideon.com");

    udUUID_GenerateFromString(&pSettings->maptiles.tileServerAddressUUID, pSettings->maptiles.tileServerAddress);

    udStrcpy(pSettings->maptiles.tileServerExtension, data.Get("maptiles.imgExtension").AsString("png"));
    pSettings->maptiles.mapHeight = data.Get("maptiles.mapHeight").AsFloat(0.f);
    pSettings->maptiles.blendMode = (vcMapTileBlendMode)data.Get("maptiles.blendMode").AsInt(1);
    pSettings->maptiles.transparency = data.Get("maptiles.transparency").AsFloat(1.f);

    pSettings->maptiles.mapQuality = (vcTileRendererMapQuality)data.Get("maptiles.mapQuality").AsInt(vcTRMQ_High);
    pSettings->maptiles.mapOptions = (vcTileRendererFlags)data.Get("maptiles.mapOptions").AsInt(vcTRF_None);
  }

  if (group == vcSC_All || group == vcSC_Visualization)
  {
    pSettings->visualization.mode = (vcVisualizatationMode)data.Get("visualization.mode").AsInt(0);
    pSettings->postVisualization.edgeOutlines.enable = data.Get("postVisualization.edgeOutlines.enabled").AsBool(false);
    pSettings->postVisualization.colourByHeight.enable = data.Get("postVisualization.colourByHeight.enabled").AsBool(false);
    pSettings->postVisualization.colourByDepth.enable = data.Get("postVisualization.colourByDepth.enabled").AsBool(false);
    pSettings->postVisualization.contours.enable = data.Get("postVisualization.contours.enabled").AsBool(false);
    pSettings->visualization.minIntensity = data.Get("visualization.minIntensity").AsInt(0);
    pSettings->visualization.maxIntensity = data.Get("visualization.maxIntensity").AsInt(65535);

    memcpy(pSettings->visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pSettings->visualization.customClassificationColors));
    if (data.Get("visualization.classificationColours").IsArray())
    {
      const udJSONArray *pColors = data.Get("visualization.classificationColours").AsArray();

      for (size_t i = 0; i < pColors->length; ++i)
        pSettings->visualization.customClassificationColors[i] = pColors->GetElement(i)->AsInt(GeoverseClassificationColours[i]);
    }
    if (data.Get("visualization.classificationColourLabels").IsArray())
    {
      const udJSONArray *pColorLabels = data.Get("visualization.classificationColourLabels").AsArray();

      for (size_t i = 0; i < pColorLabels->length; ++i)
      {
        int digits = 3;
        const char *buf = pColorLabels->GetElement(i)->AsString();
        pSettings->visualization.customClassificationColorLabels[udStrAtoi(buf, &digits)] = udStrdup(buf + digits);
      }
    }

    // Post visualization - Edge Highlighting
    pSettings->postVisualization.edgeOutlines.width = data.Get("postVisualization.edgeOutlines.width").AsInt(1);
    pSettings->postVisualization.edgeOutlines.threshold = data.Get("postVisualization.edgeOutlines.threshold").AsFloat(0.001f);
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.edgeOutlines.colour[i] = data.Get("postVisualization.edgeOutlines.colour[%d]", i).AsFloat(1.f);

    // Post visualization - Colour by height
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByHeight.minColour[i] = data.Get("postVisualization.colourByHeight.minColour[%d]", i).AsFloat((i < 3) ? 0.f : 1.f); // 0.f, 0.f, 1.f, 1.f
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByHeight.maxColour[i] = data.Get("postVisualization.colourByHeight.maxColour[%d]", i).AsFloat((i % 2) ? 1.f : 0.f); // 0.f, 1.f, 0.f, 1.f
    pSettings->postVisualization.colourByHeight.startHeight = data.Get("postVisualization.colourByHeight.startHeight").AsFloat(30.f);
    pSettings->postVisualization.colourByHeight.endHeight = data.Get("postVisualization.colourByHeight.endHeight").AsFloat(50.f);

    // Post visualization - Colour by depth
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByDepth.colour[i] = data.Get("postVisualization.colourByDepth.colour[%d]", i).AsFloat((i == 0 || i == 3) ? 1.f : 0.f); // 1.f, 0.f, 0.f, 1.f
    pSettings->postVisualization.colourByDepth.startDepth = data.Get("postVisualization.colourByDepth.startDepth").AsFloat(100.f);
    pSettings->postVisualization.colourByDepth.endDepth = data.Get("postVisualization.colourByDepth.endDepth").AsFloat(1000.f);

    // Post visualization - Contours
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.contours.colour[i] = data.Get("postVisualization.contours.colour[%d]", i).AsFloat((i == 3) ? 1.f : 0.f); // 0.f, 0.f, 0.f, 1.f
    pSettings->postVisualization.contours.distances = data.Get("postVisualization.contours.distances").AsFloat(50.f);
    pSettings->postVisualization.contours.bandHeight = data.Get("postVisualization.contours.bandHeight").AsFloat(1.f);
    pSettings->postVisualization.contours.rainbowRepeat = data.Get("postVisualization.contours.rainbowRepeat").AsFloat(1.f);
    pSettings->postVisualization.contours.rainbowIntensity = data.Get("postVisualization.contours.rainbowIntensity").AsFloat(0.f);
  }

  if (group == vcSC_Convert || group == vcSC_All)
  {
    udStrcpy(pSettings->convertdefaults.tempDirectory, data.Get("convert.tempDirectory").AsString(""));
    udStrcpy(pSettings->convertdefaults.watermark.filename, data.Get("convert.watermark").AsString(""));
    pSettings->convertdefaults.watermark.isDirty = true;
    udStrcpy(pSettings->convertdefaults.author, data.Get("convert.author").AsString(""));
    udStrcpy(pSettings->convertdefaults.comment, data.Get("convert.comment").AsString(""));
    udStrcpy(pSettings->convertdefaults.copyright, data.Get("convert.copyright").AsString(""));
    udStrcpy(pSettings->convertdefaults.license, data.Get("convert.license").AsString(""));
  }

  if (group == vcSC_Bindings || group == vcSC_All)
  {
    if (!data.Get("keys").IsObject())
    {
      vcSettings_Load(pSettings, true, vcSC_Bindings);
    }
    else
    {
      for (int i = 0; i < vcB_Count; ++i)
        vcHotkey::Set((vcBind)i, vcHotkey::DecodeKeyString(data.Get("keys.%s", vcHotkey::GetBindName((vcBind)i)).AsString()));
    }
  }

  if (group == vcSC_All)
  {
    // Windows
    pSettings->window.maximized = data.Get("window.maximized").AsBool(false);
    if (pSettings->window.maximized)
    {
      pSettings->window.xpos = SDL_WINDOWPOS_CENTERED;
      pSettings->window.ypos = SDL_WINDOWPOS_CENTERED;
      pSettings->window.width = 1280;
      pSettings->window.height = 720;
    }
    else
    {
      pSettings->window.xpos = data.Get("window.position.x").AsInt(SDL_WINDOWPOS_CENTERED);
      pSettings->window.ypos = data.Get("window.position.y").AsInt(SDL_WINDOWPOS_CENTERED);
      pSettings->window.width = data.Get("window.width").AsInt(1280);
      pSettings->window.height = data.Get("window.height").AsInt(720);
    }

    udStrcpy(pSettings->window.languageCode, data.Get("window.language").AsString("enAU"));

    // Login Info
    pSettings->loginInfo.rememberServer = data.Get("login.rememberServer").AsBool(false);
    if (pSettings->loginInfo.rememberServer)
      udStrcpy(pSettings->loginInfo.serverURL, data.Get("login.serverURL").AsString());

    pSettings->loginInfo.rememberUsername = data.Get("login.rememberUsername").AsBool(false);
    if (pSettings->loginInfo.rememberUsername)
      udStrcpy(pSettings->loginInfo.username, data.Get("login.username").AsString());

    udStrcpy(pSettings->loginInfo.proxy, data.Get("login.proxy").AsString());
    udStrcpy(pSettings->loginInfo.proxyTestURL, data.Get("login.proxyTestURL").AsString("http://vaultmodels.euclideon.com/proxytest"));
    pSettings->loginInfo.autoDetectProxy = data.Get("login.autodetectproxy").AsBool();

    // Camera
    pSettings->camera.moveSpeed = data.Get("camera.moveSpeed").AsFloat(10.f);
    pSettings->camera.lockAltitude = (data.Get("camera.moveMode").AsInt(0) == 1);
  }

  if (forceReset)
  {
    pSettings->docksLoaded = vcSettings::vcDockLoaded::vcDL_ForceReset;
  }
  else if (group == vcSC_Docks)
  {
    if (data.Get("dock").IsArray())
    {
      pSettings->docksLoaded = vcSettings::vcDockLoaded::vcDL_True;

      pSettings->window.windowsOpen[vcDocks_Scene] = data.Get("frames.scene").AsBool(true);
      pSettings->window.windowsOpen[vcDocks_Settings] = data.Get("frames.settings").AsBool(true);
      pSettings->window.windowsOpen[vcDocks_SceneExplorer] = data.Get("frames.explorer").AsBool(true);
      pSettings->window.windowsOpen[vcDocks_Convert] = data.Get("frames.convert").AsBool(false);

      udJSONArray *pDocks = data.Get("dock").AsArray();
      size_t numNodes = pDocks->length;

      ImGui::DockContextClearNodes(GImGui, 0, true);

      ImGuiDockNodeSettings *pDockNodes = udAllocType(ImGuiDockNodeSettings, numNodes, udAF_Zero);

      for (size_t i = 0; i < pDocks->length; ++i)
      {
        udJSON *pDock = pDocks->GetElement(i);

        pDockNodes[i].ID = pDock->Get("id").AsInt();
        pDockNodes[i].Pos = ImVec2ih((short)pDock->Get("x").AsInt(), (short)pDock->Get("y").AsInt());
        pDockNodes[i].Size = ImVec2ih((short)pDock->Get("w").AsInt(), (short)pDock->Get("h").AsInt());
        pDockNodes[i].SizeRef = ImVec2ih((short)pDock->Get("wr").AsInt(), (short)pDock->Get("hr").AsInt());

        if (pDock->Get("parent").IsNumeric())
          pDockNodes[i].ParentID = pDock->Get("parent").AsInt();

        if (pDock->Get("split").IsString())
          pDockNodes[i].SplitAxis = (*(pDock->Get("split").AsString()) == 'X') ? (signed char)ImGuiAxis_X : (signed char)ImGuiAxis_Y;
        else
          pDockNodes[i].SplitAxis = ImGuiAxis_None;

        pDockNodes[i].IsCentralNode = (char)pDock->Get("central").AsInt();
        pDockNodes[i].IsDockSpace = (char)pDock->Get("dockspace").AsInt();
        pDockNodes[i].IsHiddenTabBar = (char)pDock->Get("hiddentabbar").AsInt();
        pDockNodes[i].Depth = (char)pDock->Get("depth").AsInt();

        if (pDock->Get("windows").IsArray())
        {
          udJSONArray *pWindows = pDock->Get("windows").AsArray();

          for (size_t j = 0; j < pWindows->length; ++j)
          {
            udJSON *pJSONWindow = pWindows->GetElement(j);

            ImGuiWindow* pWindow = ImGui::FindWindowByName(pJSONWindow->Get("name").AsString());
            if (pWindow)
            {
              pWindow->DockId = pDockNodes[i].ID;
              pWindow->DockOrder = (short)pJSONWindow->Get("index").AsInt();
              pWindow->Collapsed = pJSONWindow->Get("collapsed").AsBool();
              if (pJSONWindow->Get("visible").AsBool())
              {
                for (size_t k = 0; k < sizeof(pSettings->pActive); ++k)
                {
                  if (pSettings->pActive[k] == nullptr)
                  {
                    pSettings->pActive[k] = pWindow;
                    break;
                  }
                }
              }
            }
          }
        }
      }

      ImGui::DockContextBuildNodesFromSettings(GImGui, pDockNodes, (int)numNodes);
      udFree(pDockNodes);
    }
    else
    {
      vcSettings_Load(pSettings, true, vcSC_Docks);
      goto epilogue;
    }
  }

  if (group == vcSC_Languages || group == vcSC_All)
  {
    const char *pFileContents = nullptr;

    if (udFile_Load(vcSettings_GetAssetPath("assets/lang/languages.json"), (void**)&pFileContents) == udR_Success)
    {
      udJSON languages;

      if (languages.Parse(pFileContents) == udR_Success)
      {
        if (languages.IsArray())
        {
          pSettings->languageOptions.Clear();
          pSettings->languageOptions.ReserveBack(languages.ArrayLength());

          for (size_t i = 0; i < languages.ArrayLength(); ++i)
          {
            vcLanguageOption *pLangOption = pSettings->languageOptions.PushBack();
            udStrcpy(pLangOption->languageName, languages.Get("[%zu].localname", i).AsString());
            udStrcpy(pLangOption->filename, languages.Get("[%zu].filename", i).AsString());
          }
        }
      }
    }

    udFree(pFileContents);
  }

epilogue:
  udFree(pSavedData);
  return true;
}

void vcSettings_RecurseDocks(ImGuiDockNode *pNode, udJSON &out, int *pDepth)
{
  udJSON data;
  data.Set("id = %d", (int)pNode->ID);
  if (pNode->ParentNode)
    data.Set("parent = %d", (int)pNode->ParentNode->ID);

  data.Set("central = %d", (int)pNode->IsCentralNode());
  data.Set("dockspace = %d", (int)pNode->IsDockSpace());
  data.Set("hiddentabbar = %d", (int)pNode->IsHiddenTabBar());
  data.Set("depth = %d", *pDepth);

  for (int i = 0; i < pNode->Windows.size(); ++i)
  {
    udJSON window;

    // Only want the part '###xxxxDock' so any language works
    const char *pIDName = udStrchr(pNode->Windows[i]->Name, "#");

    window.Set("name = '%s'", pIDName);
    window.Set("index = %d", i);
    window.Set("collapsed = %d", (int)pNode->Windows[i]->Collapsed);
    window.Set("visible = %d", (int)pNode->Windows[i]->DockTabIsVisible);

    data.Set(&window, "windows[]");
  }

  data.Set("x = %f", pNode->Pos.x);
  data.Set("y = %f", pNode->Pos.y);
  data.Set("w = %f", pNode->Size.x);
  data.Set("h = %f", pNode->Size.y);
  data.Set("wr = %f", pNode->SizeRef.x);
  data.Set("hr = %f", pNode->SizeRef.y);

  if (pNode->IsSplitNode())
    data.Set("split = '%s'", (pNode->SplitAxis == 1) ? "Y" : "X");

  out.Set(&data, "dock[]");
  ++(*pDepth);

  for (size_t i = 0; i < udLengthOf(pNode->ChildNodes); ++i)
  {
    if (pNode->ChildNodes[i] != nullptr)
    {
      vcSettings_RecurseDocks(pNode->ChildNodes[i], out, pDepth);
      --(*pDepth);
    }
  }
}

#if UDPLATFORM_EMSCRIPTEN
void vcSettings_SyncFS()
{
  EM_ASM({
    // Sync from memory into persisted state
    FS.syncfs(false, function(err) { assert(!err); });
  });
}
#endif

bool vcSettings_Save(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage || pSettings->window.presentationMode)
    return false;

  bool success = false;

  if (pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  ImGui::GetIO().WantSaveIniSettings = false;

  udJSON data;
  udJSON tempNode;

  // Misc Settings
  data.Set("style = %i", pSettings->presentation.styleIndex);

  data.Set("showDiagnosticInfo = %s", pSettings->presentation.showDiagnosticInfo ? "true" : "false");
  data.Set("showEuclideonLogo = %s", pSettings->presentation.showEuclideonLogo ? "true" : "false");
  data.Set("showCameraInfo = %s", pSettings->presentation.showCameraInfo ? "true" : "false");
  data.Set("showGISInfo = %s", pSettings->presentation.showProjectionInfo ? "true" : "false");
  data.Set("loginRenderLicense = %s", pSettings->presentation.loginRenderLicense ? "true" : "false");
  data.Set("showSkybox = %s", pSettings->presentation.showSkybox ? "true" : "false");
  data.Set("skyboxColour = [%f, %f, %f, %f]", pSettings->presentation.skyboxColour.x, pSettings->presentation.skyboxColour.y, pSettings->presentation.skyboxColour.z, pSettings->presentation.skyboxColour.w);
  data.Set("showAdvancedGISOptions = %s", pSettings->presentation.showAdvancedGIS ? "true" : "false");
  data.Set("mouseAnchor = %d", pSettings->presentation.mouseAnchor);
  data.Set("showCompass = %s", pSettings->presentation.showCompass ? "true" : "false");
  data.Set("limitFPSInBackground = %s", pSettings->presentation.limitFPSInBackground ? "true" : "false");
  data.Set("POIfadeDistance = %f", pSettings->presentation.POIFadeDistance);
  data.Set("pointMode = %d", pSettings->presentation.pointMode);
  data.Set("responsiveUI = %d", pSettings->responsiveUI);

  for (int i = 0; i < 4; i++)
    data.Set("objectHighlighting.colour[] = %f", pSettings->objectHighlighting.colour[i]);
  data.Set("objectHighlighting.thickness = %f", pSettings->objectHighlighting.thickness);

  // Windows
  data.Set("window.position.x = %d", pSettings->window.xpos);
  data.Set("window.position.y = %d", pSettings->window.ypos);
  data.Set("window.width = %d", pSettings->window.width);
  data.Set("window.height = %d", pSettings->window.height);
  data.Set("window.maximized = %s", pSettings->window.maximized ? "true" : "false");
  data.Set("window.touchscreenFriendly = %s", pSettings->window.touchscreenFriendly ? "true" : "false");
  data.Set("window.language = '%s'", pSettings->window.languageCode);

  data.Set("frames.scene = %s", pSettings->window.windowsOpen[vcDocks_Scene] ? "true" : "false");
  data.Set("frames.settings = %s", pSettings->window.windowsOpen[vcDocks_Settings] ? "true" : "false");
  data.Set("frames.explorer = %s", pSettings->window.windowsOpen[vcDocks_SceneExplorer] ? "true" : "false");
  data.Set("frames.convert = %s", pSettings->window.windowsOpen[vcDocks_Convert] ? "true" : "false");

  // Login Info
  data.Set("login.rememberServer = %s", pSettings->loginInfo.rememberServer ? "true" : "false");
  if (pSettings->loginInfo.rememberServer)
  {
    tempNode.SetString(pSettings->loginInfo.serverURL);
    data.Set(&tempNode, "login.serverURL");
  }

  data.Set("login.rememberUsername = %s", pSettings->loginInfo.rememberUsername ? "true" : "false");
  if (pSettings->loginInfo.rememberUsername)
  {
    tempNode.SetString(pSettings->loginInfo.username);
    data.Set(&tempNode, "login.username");
  }

  tempNode.SetString(pSettings->loginInfo.proxy);
  data.Set(&tempNode, "login.proxy");
  data.Set("login.autodetectproxy = %s", pSettings->loginInfo.autoDetectProxy ? "true" : "false");

  // Camera
  data.Set("camera.moveSpeed = %f", pSettings->camera.moveSpeed);
  data.Set("camera.nearPlane = %f", pSettings->camera.nearPlane);
  data.Set("camera.farPlane = %f", pSettings->camera.farPlane);
  data.Set("camera.fieldOfView = %f", pSettings->camera.fieldOfView);
  data.Set("camera.lensId = %i", pSettings->camera.lensIndex);

  data.Set("camera.invertMouseX = %s", pSettings->camera.invertMouseX ? "true" : "false");
  data.Set("camera.invertMouseY = %s", pSettings->camera.invertMouseY ? "true" : "false");
  data.Set("camera.invertControllerX = %s", pSettings->camera.invertControllerX ? "true" : "false");
  data.Set("camera.invertControllerY = %s", pSettings->camera.invertControllerY ? "true" : "false");
  data.Set("camera.moveMode = %d", pSettings->camera.lockAltitude ? 1 : 0);
  data.Set("camera.cameraMouseBindings = [%d, %d, %d]", pSettings->camera.cameraMouseBindings[0], pSettings->camera.cameraMouseBindings[1], pSettings->camera.cameraMouseBindings[2]);
  data.Set("camera.scrollwheelBinding = %d", pSettings->camera.scrollWheelMode);

  // Visualization
  data.Set("visualization.mode = %d", pSettings->visualization.mode);
  data.Set("visualization.minIntensity = %d", pSettings->visualization.minIntensity);
  data.Set("visualization.maxIntensity = %d", pSettings->visualization.maxIntensity);

  int uniqueColoursEnd = 255;
  for (; uniqueColoursEnd >= 0; --uniqueColoursEnd) // Loop from the end so we only write
  {
    if (pSettings->visualization.customClassificationColors[uniqueColoursEnd] != GeoverseClassificationColours[uniqueColoursEnd])
      break;
  }

  for (int i = 0; i <= uniqueColoursEnd; ++i)
    data.Set("visualization.classificationColours[] = %u", pSettings->visualization.customClassificationColors[i]);
  for (int i = 64; i < 256; ++i)
    if (pSettings->visualization.customClassificationColorLabels[i] != nullptr)
      data.Set("visualization.classificationColourLabels[] = '%03d%s'", i, pSettings->visualization.customClassificationColorLabels[i]);

  // Post visualization - Edge Highlighting
  data.Set("postVisualization.edgeOutlines.enabled = %s", pSettings->postVisualization.edgeOutlines.enable ? "true" : "false");
  data.Set("postVisualization.edgeOutlines.width = %d", pSettings->postVisualization.edgeOutlines.width);
  data.Set("postVisualization.edgeOutlines.threshold = %f", pSettings->postVisualization.edgeOutlines.threshold);
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.edgeOutlines.colour[] = %f", pSettings->postVisualization.edgeOutlines.colour[i]);

  // Post visualization - Colour by height
  data.Set("postVisualization.colourByHeight.enabled = %s", pSettings->postVisualization.colourByHeight.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByHeight.minColour[] = %f", pSettings->postVisualization.colourByHeight.minColour[i]);
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByHeight.maxColour[] = %f", pSettings->postVisualization.colourByHeight.maxColour[i]);
  data.Set("postVisualization.colourByHeight.startHeight = %f", pSettings->postVisualization.colourByHeight.startHeight);
  data.Set("postVisualization.colourByHeight.endHeight = %f", pSettings->postVisualization.colourByHeight.endHeight);

  // Post visualization - Colour by depth
  data.Set("postVisualization.colourByDepth.enabled = %s", pSettings->postVisualization.colourByDepth.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByDepth.colour[] = %f", pSettings->postVisualization.colourByDepth.colour[i]);
  data.Set("postVisualization.colourByDepth.startDepth = %f", pSettings->postVisualization.colourByDepth.startDepth);
  data.Set("postVisualization.colourByDepth.endDepth = %f", pSettings->postVisualization.colourByDepth.endDepth);

  // Post visualization - Contours
  data.Set("postVisualization.contours.enabled = %s", pSettings->postVisualization.contours.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.contours.colour[] = %f", pSettings->postVisualization.contours.colour[i]);
  data.Set("postVisualization.contours.distances = %f", pSettings->postVisualization.contours.distances);
  data.Set("postVisualization.contours.bandHeight = %f", pSettings->postVisualization.contours.bandHeight);
  data.Set("postVisualization.contours.rainbowRepeat = %f", pSettings->postVisualization.contours.rainbowRepeat);
  data.Set("postVisualization.contours.rainbowIntensity = %f", pSettings->postVisualization.contours.rainbowIntensity);

  // Convert Settings
  tempNode.SetString(pSettings->convertdefaults.tempDirectory);
  data.Set(&tempNode, "convert.tempDirectory");

  tempNode.SetString(pSettings->convertdefaults.watermark.filename);
  data.Set(&tempNode, "convert.watermark");

  tempNode.SetString(pSettings->convertdefaults.author);
  data.Set(&tempNode, "convert.author");

  tempNode.SetString(pSettings->convertdefaults.comment);
  data.Set(&tempNode, "convert.comment");

  tempNode.SetString(pSettings->convertdefaults.copyright);
  data.Set(&tempNode, "convert.copyright");

  tempNode.SetString(pSettings->convertdefaults.license);
  data.Set(&tempNode, "convert.license");

  // Map Tiles
  data.Set("maptiles.enabled = %s", pSettings->maptiles.mapEnabled ? "true" : "false");
  data.Set("maptiles.blendMode = %d", pSettings->maptiles.blendMode);
  data.Set("maptiles.transparency = %f", pSettings->maptiles.transparency);
  data.Set("maptiles.mapHeight = %f", pSettings->maptiles.mapHeight);
  data.Set("maptiles.mouseInteracts = %s", pSettings->maptiles.mouseInteracts ? "true" : "false");
  data.Set("maptiles.mapQuality = %d", int(pSettings->maptiles.mapQuality));
  data.Set("maptiles.mapOptions = %d", int(pSettings->maptiles.mapOptions));

  tempNode.SetString(pSettings->maptiles.tileServerAddress);
  data.Set(&tempNode, "maptiles.serverURL");
  tempNode.SetString(pSettings->maptiles.tileServerExtension);
  data.Set(&tempNode, "maptiles.imgExtension");

  char keyBuffer[50] = {};
  for (size_t i = 0; i < vcB_Count; ++i)
  {
    vcHotkey::GetKeyName((vcBind)i, keyBuffer, (uint32_t)udLengthOf(keyBuffer));
    data.Set("keys.%s = '%s'", vcHotkey::GetBindName((vcBind)i), keyBuffer);
  }

  int depth = 0;
  ImGuiDockNode *pRootNode = ImGui::DockBuilderGetNode(pSettings->rootDock);
  if (pRootNode != nullptr && !pRootNode->IsEmpty())
    vcSettings_RecurseDocks(ImGui::DockNodeGetRootNode(pRootNode), data, &depth);

  // Save
  const char *pSettingsStr;

  if (data.Export(&pSettingsStr, udJEO_JSON | udJEO_FormatWhiteSpace) == udR_Success)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, "%ssettings.json", pSettings->pSaveFilePath);

    success = SDL_filewrite(buffer, pSettingsStr, udStrlen(pSettingsStr));

    udFree(pSettingsStr);
  }

#if UDPLATFORM_EMSCRIPTEN
  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, vcSettings_SyncFS);
#endif

  return success;
}

void vcSettings_Cleanup(vcSettings *pSettings)
{
  for (size_t i = 0; i < 256; ++i)
    if (pSettings->visualization.customClassificationColorLabels[i] != nullptr)
      udFree(pSettings->visualization.customClassificationColorLabels[i]);

  pSettings->languageOptions.Deinit();

  vcTexture_Destroy(&pSettings->convertdefaults.watermark.pTexture);
}

#if UDPLATFORM_EMSCRIPTEN
void *vcSettings_GetAssetPathAllocateCallback(size_t length)
{
  return udAlloc(length);
}
#endif

const char *vcSettings_GetAssetPath(const char *pFilename)
{
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  udFilename filename(pFilename);
  return udTempStr("./%s", filename.GetFilenameWithExt());
#elif UDPLATFORM_OSX
  char *pBasePath = SDL_GetBasePath();
  if (pBasePath == nullptr)
    pBasePath = SDL_strdup("./");

  udFilename filename(pFilename);
  const char *pOutput = udTempStr("%s%s", pBasePath, filename.GetFilenameWithExt());
  SDL_free(pBasePath);

  return pOutput;
#elif UDPLATFORM_EMSCRIPTEN
  char *pURL = (char*)EM_ASM_INT({
    var url = self.location.href.substr(0, self.location.href.lastIndexOf('/'));
    var lengthBytes = lengthBytesUTF8(url) + 1;
    var pURL = dynCall_ii($0, lengthBytes);
    stringToUTF8(url, pURL, lengthBytes + 1);
    return pURL;
  }, vcSettings_GetAssetPathAllocateCallback);
  const char *pTempURL = udTempStr("%s/%s", pURL, pFilename);
  udFree(pURL);
  return pTempURL;
#else
  return udTempStr("%s", pFilename);
#endif
}

udResult vcSettings_FileHandlerAssetOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  size_t fileStart = 8; // length of asset://
  const char *pNewFilename = vcSettings_GetAssetPath(pFilename + fileStart);
  udResult res = udFile_Open(ppFile, pNewFilename, flags);
  if (res == udR_Success)
  {
    udFree((*ppFile)->pFilenameCopy);
    (*ppFile)->pFilenameCopy = udStrdup(pNewFilename);
  }
  return res;
}

udResult vcSettings_RegisterAssetFileHandler()
{
  return udFile_RegisterHandler(vcSettings_FileHandlerAssetOpen, "asset://");
}

udResult vcSettings_UpdateLanguageOptions(vcSettings *pSetting)
{
  udResult result = udR_Unsupported;
  udFindDir *pDir = nullptr;
  bool rewrite = false;

  // Check directory for files not included in languages.json, then update if necessary
#if defined(UDPLATFORM_WINDOWS) || defined(UDPLATFORM_OSX) || defined(UDPLATFORM_LINUX)
  UD_ERROR_CHECK(udOpenDir(&pDir, vcSettings_GetAssetPath("assets/lang")));

  do
  {
    // Skip system '$*', this '.', parent '..' and misc 'hidden' folders '.*'
    if (udStrBeginsWith(pDir->pFilename, ".") || udStrBeginsWith(pDir->pFilename, "$") || udStrcmpi(pDir->pFilename, "languages.json") == 0)
      continue;

    bool found = false;
    for (size_t i = 0; i < pSetting->languageOptions.length; ++i)
    {
      if (udStrcmpi(pSetting->languageOptions[i].filename, pDir->pFilename) == 0)
      {
        found = true;
        break;
      }
    }

    if (found)
      continue;

    vcLanguageOption *pNewLang = pSetting->languageOptions.PushBack();

    udFilename tempFile(pDir->pFilename);
    tempFile.ExtractFilenameOnly(pNewLang->filename, (int)udLengthOf(pNewLang->filename));

    //TODO: Load the file and get the local name out
    udStrcpy(pNewLang->languageName, pDir->pFilename);

    rewrite = true;

  } while (udReadDir(pDir) == udR_Success);

  udCloseDir(&pDir);

  if (rewrite)
  {
    //TODO: Write the file back if possible (most non-windows platforms will not be possible)

    //const char *pLanguageStr;
    //
    //if (languages.Export(&pLanguageStr, udJEO_JSON | udJEO_FormatWhiteSpace) == udR_Success)
    //{
    //  SDL_filewrite(vcSettings_GetAssetPath("assets/lang/languages.json"), pLanguageStr, udStrlen(pLanguageStr));
    //  udFree(pLanguageStr);
    //}
  }
#endif

epilogue:
  return result;
}
