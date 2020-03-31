#include "vcSettings.h"

#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udFileHandler.h"
#include "udStringUtil.h"

#include "imgui.h"

#include "vcClassificationColours.h"
#include "vcStringFormat.h"
#include "vcHotkey.h"

#include "vdkConfig.h"

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

void vcSettings_Cleanup_CustomClassificationColorLabels(vcSettings *pSettings)
{
  size_t lenth = udLengthOf(pSettings->visualization.customClassificationColorLabels);
  for (size_t i = 0; i < lenth; ++i)
  {
    if (pSettings->visualization.customClassificationColorLabels[i] != nullptr)
      udFree(pSettings->visualization.customClassificationColorLabels[i]);
  }
}

bool vcSettings_ApplyConnectionSettings(vcSettings *pSettings)
{
  vdkConfig_ForceProxy(pSettings->loginInfo.proxy);
  vdkConfig_SetUserAgent(pSettings->loginInfo.userAgent);
  vdkConfig_IgnoreCertificateVerification(pSettings->loginInfo.ignoreCertificateVerification);
  vdkConfig_SetProxyAuth(pSettings->loginInfo.proxyUsername, pSettings->loginInfo.proxyPassword);

  return true;
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
    pSettings->window.useNativeUI = data.Get("window.showNativeUI").AsBool();
    udStrcpy(pSettings->window.languageCode, data.Get("window.language").AsString("enAU"));
    pSettings->presentation.styleIndex = data.Get("style").AsInt(1); // dark style by default

    pSettings->presentation.showDiagnosticInfo = data.Get("showDiagnosticInfo").AsBool(false);
    pSettings->presentation.showEuclideonLogo = data.Get("showEuclideonLogo").AsBool(false);
    pSettings->presentation.showCameraInfo = data.Get("showCameraInfo").AsBool(true);
    pSettings->presentation.showProjectionInfo = data.Get("showGISInfo").AsBool(true);
    pSettings->presentation.showAdvancedGIS = data.Get("showAdvancedGISOptions").AsBool(false);
    pSettings->presentation.loginRenderLicense = data.Get("loginRenderLicense").AsBool(false);
    pSettings->presentation.saturation = data.Get("saturation").AsFloat(1.0f);
    pSettings->presentation.mouseAnchor = (vcAnchorStyle)data.Get("mouseAnchor").AsInt(vcAS_Orbit);
    pSettings->presentation.POIFadeDistance = data.Get("POIfadeDistance").AsFloat(10000.f);
    pSettings->presentation.imageRescaleDistance = data.Get("ImageRescaleDistance").AsFloat(10000.f);
    pSettings->presentation.limitFPSInBackground = data.Get("limitFPSInBackground").AsBool(true);
    pSettings->presentation.pointMode = data.Get("pointMode").AsInt();
    pSettings->presentation.layout = (vcWindowLayout)data.Get("layout").AsInt(vcWL_SceneLeft);
    pSettings->presentation.sceneExplorerSize = data.Get("layoutSceneExplorerSize").AsInt(350);
    pSettings->presentation.convertLeftPanelPercentage = data.Get("convertLeftPanelPercentage").AsFloat(0.33f);
    pSettings->presentation.columnSizeCorrect = false;

    pSettings->presentation.skybox.type = (vcSkyboxType)data.Get("skybox.type").AsInt(vcSkyboxType_Simple);
    pSettings->presentation.skybox.colour = data.Get("skybox.colour").AsFloat4(udFloat4::create(0.39f, 0.58f, 0.93f, 1.f));
    pSettings->presentation.skybox.exposure = data.Get("skybox.exposure").AsFloat(7.5f);
    pSettings->presentation.skybox.timeOfDay = data.Get("skybox.timeOfDay").AsFloat(4.5f);

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

    pSettings->mouseSnap.enable = data.Get("mouseSnap.enable").AsBool(false);
    pSettings->mouseSnap.range = data.Get("mouseSnap.range").AsInt(vcSL_MouseSnapRangeMax);
  }

  if (group == vcSC_All || group == vcSC_MapsElevation)
  {
    // Map Tiles
    pSettings->maptiles.mapEnabled = data.Get("maptiles.enabled").AsBool(true);
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
    pSettings->camera.lensIndex = data.Get("camera.lensId").AsInt(vcLS_30mm);
    pSettings->camera.fieldOfView = data.Get("camera.fieldOfView").AsFloat(vcLens30mm);

    pSettings->objectHighlighting.enable = data.Get("objectHighlighting.enable").AsBool(true);
    pSettings->objectHighlighting.colour = data.Get("objectHighlighting.colour").AsFloat4(udFloat4::create(0.925f, 0.553f, 0.263f, 1.0f));
    pSettings->objectHighlighting.thickness = data.Get("objectHighlighting.thickness").AsFloat(2.0f);

    pSettings->visualization.mode = (vcVisualizatationMode)(data.Get("visualization.mode").AsInt(-1) + 1);
    pSettings->postVisualization.edgeOutlines.enable = data.Get("postVisualization.edgeOutlines.enabled").AsBool(false);
    pSettings->postVisualization.colourByHeight.enable = data.Get("postVisualization.colourByHeight.enabled").AsBool(false);
    pSettings->postVisualization.colourByDepth.enable = data.Get("postVisualization.colourByDepth.enabled").AsBool(false);
    pSettings->postVisualization.contours.enable = data.Get("postVisualization.contours.enabled").AsBool(false);
    pSettings->visualization.minIntensity = data.Get("visualization.minIntensity").AsInt(0);
    pSettings->visualization.maxIntensity = data.Get("visualization.maxIntensity").AsInt(65535);

    for (size_t i = 0; i < udLengthOf(pSettings->visualization.customClassificationToggles); i++)
      pSettings->visualization.customClassificationToggles[i] = true;

    if (data.Get("visualization.classificationToggles").IsArray())
    {
      const udJSONArray* pToggles = data.Get("visualization.classificationToggles").AsArray();

      for (size_t i = 0; i < pToggles->length; ++i)
        pSettings->visualization.customClassificationToggles[i] = (pToggles->GetElement(i)->AsInt(1) != 0) ? true : false;
    }

    memcpy(pSettings->visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pSettings->visualization.customClassificationColors));
    if (data.Get("visualization.classificationColours").IsArray())
    {
      const udJSONArray *pColors = data.Get("visualization.classificationColours").AsArray();

      for (size_t i = 0; i < pColors->length; ++i)
        pSettings->visualization.customClassificationColors[i] = pColors->GetElement(i)->AsInt(GeoverseClassificationColours[i]);
    }
    vcSettings_Cleanup_CustomClassificationColorLabels(pSettings);
    if (data.Get("visualization.classificationColourLabels").IsArray())
    {
      const udJSONArray *pColorLabels = data.Get("visualization.classificationColourLabels").AsArray();

      static const int Digits = 3;
      for (size_t i = 0; i < pColorLabels->length; ++i)
      {
        const char *buf = pColorLabels->GetElement(i)->AsString();
        if (udStrlen(buf) > Digits)
        {
          char indexBuffer[Digits + 1] = "";
          udStrncpy(indexBuffer, buf, Digits);
          pSettings->visualization.customClassificationColorLabels[udStrAtoi(indexBuffer)] = udStrdup(buf + Digits);
        }
      }
    }

    // Post visualization - Edge Highlighting
    pSettings->postVisualization.edgeOutlines.width = data.Get("postVisualization.edgeOutlines.width").AsInt(1);
    pSettings->postVisualization.edgeOutlines.threshold = data.Get("postVisualization.edgeOutlines.threshold").AsFloat(2.0f);
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
    vcHotkey::Set(vcB_Forward, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Forward)).AsInt(26));
    vcHotkey::Set(vcB_Backward, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Backward)).AsInt(22));
    vcHotkey::Set(vcB_Left, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Left)).AsInt(4));
    vcHotkey::Set(vcB_Right, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Right)).AsInt(7));
    vcHotkey::Set(vcB_Up, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Up)).AsInt(21));
    vcHotkey::Set(vcB_Down, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Down)).AsInt(9));
    vcHotkey::Set(vcB_DecreaseCameraSpeed, data.Get("keys.%s", vcHotkey::GetBindName(vcB_DecreaseCameraSpeed)).AsInt(91)); // [
    vcHotkey::Set(vcB_IncreaseCameraSpeed, data.Get("keys.%s", vcHotkey::GetBindName(vcB_IncreaseCameraSpeed)).AsInt(93)); // ]
    vcHotkey::Set(vcB_Remove, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Remove)).AsInt(76));
    vcHotkey::Set(vcB_Cancel, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Cancel)).AsInt(41));
    vcHotkey::Set(vcB_LockAltitude, data.Get("keys.%s", vcHotkey::GetBindName(vcB_LockAltitude)).AsInt(44));
    vcHotkey::Set(vcB_GizmoTranslate, data.Get("keys.%s", vcHotkey::GetBindName(vcB_GizmoTranslate)).AsInt(5));
    vcHotkey::Set(vcB_GizmoRotate, data.Get("keys.%s", vcHotkey::GetBindName(vcB_GizmoRotate)).AsInt(17));
    vcHotkey::Set(vcB_GizmoScale, data.Get("keys.%s", vcHotkey::GetBindName(vcB_GizmoScale)).AsInt(16));
    vcHotkey::Set(vcB_GizmoLocalSpace, data.Get("keys.%s", vcHotkey::GetBindName(vcB_GizmoLocalSpace)).AsInt(6));
    vcHotkey::Set(vcB_Fullscreen, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Fullscreen)).AsInt(62));
    vcHotkey::Set(vcB_RenameSceneItem, data.Get("keys.%s", vcHotkey::GetBindName(vcB_RenameSceneItem)).AsInt(60));
    vcHotkey::Set(vcB_Save, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Save)).AsInt(1046));
    vcHotkey::Set(vcB_Load, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Load)).AsInt(1039));
    vcHotkey::Set(vcB_AddUDS, data.Get("keys.%s", vcHotkey::GetBindName(vcB_AddUDS)).AsInt(1048));
    vcHotkey::Set(vcB_BindingsInterface, data.Get("keys.%s", vcHotkey::GetBindName(vcB_BindingsInterface)).AsInt(1029));
    vcHotkey::Set(vcB_Undo, data.Get("keys.%s", vcHotkey::GetBindName(vcB_Undo)).AsInt(1053));
    vcHotkey::Set(vcB_TakeScreenshot, data.Get("keys.%s", vcHotkey::GetBindName(vcB_TakeScreenshot)).AsInt(70));
    vcHotkey::Set(vcB_ToggleSceneExplorer, data.Get("keys.%s", vcHotkey::GetBindName(vcB_ToggleSceneExplorer)).AsInt(61));
  }

  if (group == vcSC_All || group == vcSC_Connection)
  {
    udStrcpy(pSettings->loginInfo.proxy, data.Get("login.proxy").AsString());
    udStrcpy(pSettings->loginInfo.proxyTestURL, data.Get("login.proxyTestURL").AsString("https://az.vault.euclideon.com/proxytest"));

    udStrcpy(pSettings->loginInfo.userAgent, data.Get("login.useragent").AsString());

    pSettings->loginInfo.ignoreCertificateVerification = data.Get("login.ignoreCertificateErrors").AsBool();
    pSettings->loginInfo.requiresProxyAuth = data.Get("login.proxyRequiresAuth").AsBool();

    pSettings->loginInfo.rememberProxyUsername = data.Get("login.rememberProxyUsername").AsBool();
    if (pSettings->loginInfo.rememberProxyUsername)
      udStrcpy(pSettings->loginInfo.proxyUsername, data.Get("login.proxyUsername").AsString());

    vcSettings_ApplyConnectionSettings(pSettings);
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

    // Login Info
    pSettings->loginInfo.rememberServer = data.Get("login.rememberServer").AsBool(false);
    if (pSettings->loginInfo.rememberServer)
      udStrcpy(pSettings->loginInfo.serverURL, data.Get("login.serverURL").AsString());

    pSettings->loginInfo.rememberUsername = data.Get("login.rememberUsername").AsBool(false);
    if (pSettings->loginInfo.rememberUsername)
      udStrcpy(pSettings->loginInfo.username, data.Get("login.username").AsString());

    // Camera
    pSettings->camera.moveSpeed = data.Get("camera.moveSpeed").AsFloat(10.f);
    pSettings->camera.lockAltitude = (data.Get("camera.moveMode").AsInt(0) == 1);
  }

  if (group == vcSC_Languages || group == vcSC_All)
  {
    const char *pFileContents = nullptr;

    if (udFile_Load("asset://assets/lang/languages.json", (void**)&pFileContents) == udR_Success)
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

  if (group == vcSC_Screenshot || group == vcSC_All)
  {
    const char *pTempStr = data.Get("screenshot.outputPath").AsString();
    if (pTempStr == nullptr)
      udStrcpy(pSettings->screenshot.outputPath, pSettings->pSaveFilePath);
    else
      udStrcpy(pSettings->screenshot.outputPath, data.Get("screenshot.outputPath").AsString());
    pSettings->screenshot.resolution.x = data.Get("screenshot.resolution.width").AsInt(4096); // Defaults to 4K
    pSettings->screenshot.resolution.y = data.Get("screenshot.resolution.height").AsInt(2160);
  }

  udFree(pSavedData);
  return true;
}

#if UDPLATFORM_EMSCRIPTEN
void vcSettings_SyncFS()
{
  EM_ASM({
    // Sync from memory into persisted state
    FS.syncfs(false, function(err) { });
  });
}
#endif

bool vcSettings_Save(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage || pSettings->window.isFullscreen)
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
  data.Set("saturation = %f", pSettings->presentation.saturation);
  data.Set("showAdvancedGISOptions = %s", pSettings->presentation.showAdvancedGIS ? "true" : "false");
  data.Set("mouseAnchor = %d", pSettings->presentation.mouseAnchor);
  data.Set("limitFPSInBackground = %s", pSettings->presentation.limitFPSInBackground ? "true" : "false");
  data.Set("POIfadeDistance = %f", pSettings->presentation.POIFadeDistance);
  data.Set("ImageRescaleDistance = %f", pSettings->presentation.imageRescaleDistance);
  data.Set("pointMode = %d", pSettings->presentation.pointMode);
  data.Set("layout = %d", pSettings->presentation.layout);
  data.Set("layoutSceneExplorerSize = %d", pSettings->presentation.sceneExplorerSize);
  data.Set("convertLeftPanelPercentage = %f", pSettings->presentation.convertLeftPanelPercentage);

  data.Set("skybox.type = %d", pSettings->presentation.skybox.type);
  data.Set("skybox.colour = [%f, %f, %f, %f]", pSettings->presentation.skybox.colour.x, pSettings->presentation.skybox.colour.y, pSettings->presentation.skybox.colour.z, pSettings->presentation.skybox.colour.w);
  data.Set("skybox.exposure = %f", pSettings->presentation.skybox.exposure);
  data.Set("skybox.timeOfDay = %f", pSettings->presentation.skybox.timeOfDay);

  data.Set("objectHighlighting.enable = %s", pSettings->objectHighlighting.enable ? "true" : "false");
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
  data.Set("window.showNativeUI = %s", pSettings->window.useNativeUI ? "true" : "false");

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

  tempNode.SetString(pSettings->loginInfo.userAgent);
  data.Set(&tempNode, "login.useragent");

  data.Set("login.ignoreCertificateErrors = %s", pSettings->loginInfo.ignoreCertificateVerification ? "true" : "false");
  data.Set("login.proxyRequiresAuth = %s", pSettings->loginInfo.requiresProxyAuth ? "true" : "false");

  data.Set("login.rememberProxyUsername = %s", pSettings->loginInfo.rememberProxyUsername ? "true" : "false");
  if (pSettings->loginInfo.rememberProxyUsername)
  {
    tempNode.SetString(pSettings->loginInfo.proxyUsername);
    data.Set(&tempNode, "login.proxyUsername");
  }

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
  data.Set("visualization.mode = %d", pSettings->visualization.mode - 1);
  data.Set("visualization.minIntensity = %d", pSettings->visualization.minIntensity);
  data.Set("visualization.maxIntensity = %d", pSettings->visualization.maxIntensity);

  int lastFalseIndex = (int)udLengthOf(pSettings->visualization.customClassificationToggles) - 1;
  for (; lastFalseIndex >= 0; --lastFalseIndex)
  {
    if (!pSettings->visualization.customClassificationToggles[lastFalseIndex])
      break;
  }
  for (int i = 0; i <= lastFalseIndex; ++i)
    data.Set("visualization.classificationToggles[] = %u", pSettings->visualization.customClassificationToggles[i] ? 1 : 0);

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

  // Screenshots
  data.Set("screenshot.resolution.width = %d", pSettings->screenshot.resolution.x);
  data.Set("screenshot.resolution.height = %d", pSettings->screenshot.resolution.y);
  tempNode.SetString(pSettings->screenshot.outputPath);
  data.Set(&tempNode, "screenshot.outputPath");

  // Map Tiles
  data.Set("maptiles.enabled = %s", pSettings->maptiles.mapEnabled ? "true" : "false");
  data.Set("maptiles.blendMode = %d", pSettings->maptiles.blendMode);
  data.Set("maptiles.transparency = %f", pSettings->maptiles.transparency);
  data.Set("maptiles.mapHeight = %f", pSettings->maptiles.mapHeight);
  data.Set("maptiles.mapQuality = %d", int(pSettings->maptiles.mapQuality));
  data.Set("maptiles.mapOptions = %d", int(pSettings->maptiles.mapOptions));

  tempNode.SetString(pSettings->maptiles.tileServerAddress);
  data.Set(&tempNode, "maptiles.serverURL");
  tempNode.SetString(pSettings->maptiles.tileServerExtension);
  data.Set(&tempNode, "maptiles.imgExtension");

  for (size_t i = 0; i < vcB_Count; ++i)
    data.Set("keys.%s = %d", vcHotkey::GetBindName((vcBind)i), vcHotkey::Get((vcBind)i));

  // mouse snap setting
  data.Set("mouseSnap.enable = %s", pSettings->mouseSnap.enable ? "true" : "false");
  data.Set("mouseSnap.range = %d", int(pSettings->mouseSnap.range));

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
  vcSettings_Cleanup_CustomClassificationColorLabels(pSettings);

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

struct vcSDLFile : udFile
{
  SDL_RWops *pSDLHandle;
};

udResult vcSDLFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  vcSDLFile *pFILE = static_cast<vcSDLFile*>(pFile);
  udResult result;
  size_t actualRead;

  UD_ERROR_NULL(pFILE->pSDLHandle, udR_ReadFailure);

  pFILE->pSDLHandle->seek(pFILE->pSDLHandle, seekOffset, RW_SEEK_SET);

  actualRead = pFILE->pSDLHandle->read(pFILE->pSDLHandle, pBuffer, 1, bufferLength);

  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

epilogue:
  return result;
}

udResult vcSDLFile_Close(udFile **ppFile)
{
  udResult result = udR_Success;
  vcSDLFile *pFILE = static_cast<vcSDLFile*>(*ppFile);
  *ppFile = nullptr;

  if (pFILE->pSDLHandle)
    pFILE->pSDLHandle->close(pFILE->pSDLHandle);

  udFree(pFILE);

  return result;
}

udResult vcSettings_FileHandlerAssetOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  size_t fileStart = 8; // length of asset://
  const char *pNewFilename = vcSettings_GetAssetPath(pFilename + fileStart);

#if UDPLATFORM_ANDROID
  udResult result;

  vcSDLFile *pFile = udAllocType(vcSDLFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->pSDLHandle = SDL_RWFromFile(pNewFilename, "rb");
  UD_ERROR_NULL(pFile->pSDLHandle, udR_OpenFailure);

  pFile->fpRead = vcSDLFile_SeekRead;
  pFile->fpClose = vcSDLFile_Close;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
  {
    udFree(pFile->pFilenameCopy);
    udFree(pFile);
  }

  return result;
#else
  udResult res = udFile_Open(ppFile, pNewFilename, flags);
  if (res == udR_Success)
  {
    udFree((*ppFile)->pFilenameCopy);
    (*ppFile)->pFilenameCopy = udStrdup(pNewFilename);
  }
  return res;
#endif
}

udResult vcSettings_RegisterAssetFileHandler()
{
  return udFile_RegisterHandler(vcSettings_FileHandlerAssetOpen, "asset://");
}

udResult vcSettings_UpdateLanguageOptions(vcSettings *pSetting)
{
  udResult result = udR_Unsupported;

#if UDPLATFORM_WINDOWS || UDPLATFORM_OSX || UDPLATFORM_LINUX
  udFindDir *pDir = nullptr;
  bool rewrite = false;

  // Check directory for files not included in languages.json, then update if necessary
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

epilogue:
#else
  udUnused(pSetting);
#endif
  return result;
}
