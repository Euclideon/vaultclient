#include "vcSettings.h"

#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udFileHandler.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "vcClassificationColours.h"
#include "vCore/vStringFormat.h"

#if UDPLATFORM_EMSCRIPTEN
# include <emscripten.h>
#endif

const char DefaultFilename[] = "asset://defaultsettings.json";

const char ImGuiIniDefaults[] = "[Window][Debug##Default]\nPos=60,60\nSize=400,400\nCollapsed=0\n\n[Window][###sceneExplorerDock]\nPos=872,22\nSize=408,259\nCollapsed=0\nDockId=0x00000003,0\n\n[Window][###convertDock]\nPos=0,22\nSize=870,720\nCollapsed=0\nDockId=0x00000001,1\n\n[Window][###settingsDock]\nPos=872,283\nSize=408,459\nCollapsed=0\nDockId=0x00000004,0\n\n[Window][###sceneDock]\nPos=0,22\nSize=870,720\nCollapsed=0\nDockId=0x00000001,0\n\n[Docking][Data]\nDockSpace     ID=0x236DAF44 Pos=0,22 Size=1280,720 Split=X SelectedTab=0x469A2D56\n  DockNode    ID=0x00000001 Parent=0x236DAF44 SizeRef=870,720 CentralNode=1 SelectedTab=0x41CBF0BC\n  DockNode    ID=0x00000002 Parent=0x236DAF44 SizeRef=408,720 Split=Y SelectedTab=0xAD83196B\n    DockNode  ID=0x00000003 Parent=0x00000002 SizeRef=408,259 SelectedTab=0x4073EE43\n    DockNode  ID=0x00000004 Parent=0x00000002 SizeRef=408,459 SelectedTab=0xAD83196B\n\n";

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

  if (!forceReset && pSettings->pSaveFilePath != nullptr)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

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
    pSettings->presentation.showCameraInfo = data.Get("showCameraInfo").AsBool(true);
    pSettings->presentation.showProjectionInfo = data.Get("showGISInfo").AsBool(true);
    pSettings->presentation.showAdvancedGIS = data.Get("showAdvGISOptions").AsBool(false);
    pSettings->presentation.mouseAnchor = (vcAnchorStyle)data.Get("mouseAnchor").AsInt(vcAS_Orbit);
    pSettings->presentation.showCompass = data.Get("showCompass").AsBool(true);
    pSettings->presentation.POIFadeDistance = data.Get("POIfadeDistance").AsFloat(10000.f);
    pSettings->presentation.limitFPSInBackground = data.Get("limitFPSInBackground").AsBool(true);
    pSettings->presentation.pointMode = data.Get("pointMode").AsInt();
    pSettings->responsiveUI = (vcPresentationMode)data.Get("responsiveUI").AsInt(vcPM_Hide);

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
    pSettings->camera.invertX = data.Get("camera.invertX").AsBool(false);
    pSettings->camera.invertY = data.Get("camera.invertY").AsBool(false);
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
    udStrcpy(pSettings->maptiles.tileServerAddress, sizeof(pSettings->maptiles.tileServerAddress), data.Get("maptiles.serverURL").AsString("http://slippy.vault.euclideon.com/"));
    udStrcpy(pSettings->maptiles.tileServerExtension, sizeof(pSettings->maptiles.tileServerExtension), data.Get("maptiles.imgExtension").AsString("png"));
    pSettings->maptiles.mapHeight = data.Get("maptiles.mapHeight").AsFloat(0.f);
    pSettings->maptiles.blendMode = (vcMapTileBlendMode)data.Get("maptiles.blendMode").AsInt(1);
    pSettings->maptiles.transparency = data.Get("maptiles.transparency").AsFloat(1.f);

    if (udStrEquali(pSettings->maptiles.tileServerAddress, "http://20.188.211.58"))
      udStrcpy(pSettings->maptiles.tileServerAddress, sizeof(pSettings->maptiles.tileServerAddress), "http://slippy.vault.euclideon.com");

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
  }

  if (group == vcSC_All)
  {
    // Windows
    pSettings->window.xpos = data.Get("window.position.x").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.ypos = data.Get("window.position.y").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.width = data.Get("window.width").AsInt(1280);
    pSettings->window.height = data.Get("window.height").AsInt(720);
    pSettings->window.maximized = data.Get("window.maximized").AsBool(false);
    udStrcpy(pSettings->window.languageCode, udLengthOf(pSettings->window.languageCode), data.Get("window.language").AsString("enAU"));

    // Dock settings
    pSettings->window.windowsOpen[vcDocks_Scene] = data.Get("frames.scene").AsBool(true);
    pSettings->window.windowsOpen[vcDocks_Settings] = data.Get("frames.settings").AsBool(true);
    pSettings->window.windowsOpen[vcDocks_SceneExplorer] = data.Get("frames.explorer").AsBool(true);
    pSettings->window.windowsOpen[vcDocks_Convert] = data.Get("frames.convert").AsBool(false);

    // Login Info
    pSettings->loginInfo.rememberServer = data.Get("login.rememberServer").AsBool(false);
    if (pSettings->loginInfo.rememberServer)
      udStrcpy(pSettings->loginInfo.serverURL, sizeof(pSettings->loginInfo.serverURL), data.Get("login.serverURL").AsString());

    pSettings->loginInfo.rememberUsername = data.Get("login.rememberUsername").AsBool(false);
    if (pSettings->loginInfo.rememberUsername)
      udStrcpy(pSettings->loginInfo.username, sizeof(pSettings->loginInfo.username), data.Get("login.username").AsString());

    udStrcpy(pSettings->loginInfo.proxy, udLengthOf(pSettings->loginInfo.proxy), data.Get("login.proxy").AsString());
    udStrcpy(pSettings->loginInfo.proxyTestURL, udLengthOf(pSettings->loginInfo.proxyTestURL), data.Get("login.proxyTestURL").AsString("http://vaultmodels.euclideon.com/proxytest"));
    pSettings->loginInfo.autoDetectProxy = data.Get("login.autodetectproxy").AsBool();

    // Camera
    pSettings->camera.moveSpeed = data.Get("camera.moveSpeed").AsFloat(10.f);
    pSettings->camera.moveMode = (vcCameraMoveMode)data.Get("camera.moveMode").AsInt(0);

    const char *pImGuiSettings = data.Get("imgui.ini").AsString(ImGuiIniDefaults);
    ImGui::LoadIniSettingsFromMemory(pImGuiSettings);
  }

  udFree(pSavedData);

  return true;
}

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
  data.Set("showCameraInfo = %s", pSettings->presentation.showCameraInfo ? "true" : "false");
  data.Set("showGISInfo = %s", pSettings->presentation.showProjectionInfo ? "true" : "false");
  data.Set("showAdvancedGISOptions = %s", pSettings->presentation.showAdvancedGIS ? "true" : "false");
  data.Set("mouseAnchor = %d", pSettings->presentation.mouseAnchor);
  data.Set("showCompass = %s", pSettings->presentation.showCompass ? "true" : "false");
  data.Set("limitFPSInBackground = %s", pSettings->presentation.limitFPSInBackground ? "true" : "false");
  data.Set("POIfadeDistance = %f", pSettings->presentation.POIFadeDistance);
  data.Set("pointMode = %d", pSettings->presentation.pointMode);
  data.Set("responsiveUI = %d", pSettings->responsiveUI);

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
  data.Set("camera.invertX = %s", pSettings->camera.invertX ? "true" : "false");
  data.Set("camera.invertY = %s", pSettings->camera.invertY ? "true" : "false");
  data.Set("camera.moveMode = %d", pSettings->camera.moveMode);
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

  const char *pImGuiSettings = ImGui::SaveIniSettingsToMemory();
  tempNode.SetString(pImGuiSettings);
  data.Set(&tempNode, "imgui.ini");

  // Save
  const char *pSettingsStr;

  if (data.Export(&pSettingsStr, udJEO_JSON | udJEO_FormatWhiteSpace) == udR_Success)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

    success = SDL_filewrite(buffer, pSettingsStr, udStrlen(pSettingsStr));

    udFree(pSettingsStr);
  }

  return success;
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
    var url = window.location.href.substr(0, window.location.href.lastIndexOf('/'));
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
  udFree((*ppFile)->pFilenameCopy);
  (*ppFile)->pFilenameCopy = udStrdup(pNewFilename);
  return res;
}

udResult vcSettings_RegisterAssetFileHandler()
{
  return udFile_RegisterHandler(vcSettings_FileHandlerAssetOpen, "asset://");
}
