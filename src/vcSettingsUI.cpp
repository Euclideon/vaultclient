#include "vcSettingsUI.h"

#include "vcState.h"
#include "vcSettings.h"
#include "vcRender.h"
#include "vcModals.h"
#include "vcClassificationColours.h"
#include "vcHotkey.h"
#include "vcStringFormat.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"
#include "vcWebFile.h"
#include "vcFeatures.h"
#include "vcProxyHelper.h"
#include "vcStringFormat.h"

#include "udConfig.h"

#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "stb_image.h"

#include "udStringUtil.h"
#include "udFile.h"

#define MAX_DISPLACEMENT 10000.f

static struct
{
  const char *pMode;
  const char *pModeStr;
  const char *pServerAddr;
  const char *pCopyright;
  const char *pTileAddressUUID;
  vcTexture *pPreviewTexture;
} s_mapTiles[]{
  { "Open Street Maps", "euc-osm-base", "https://slippy.vault.euclideon.com/{0}/{1}/{2}.png", "\xC2\xA9 OpenStreetMap contributors", "https://slippy.vault.euclideon.com", nullptr },
  { "Azure Aerial", "euc-az-aerial", "https://slippy.vault.euclideon.com/aerial/{0}/{1}/{2}.png", "\xC2\xA9 1992 - 2020 TomTom", "https://slippy.vault.euclideon.com/aerial",  nullptr },
  { "Azure Roads", "euc-az-roads", "https://slippy.vault.euclideon.com/roads/{0}/{1}/{2}.png", "\xC2\xA9 1992 - 2020 TomTom", "https://slippy.vault.euclideon.com/roads", nullptr },
  { "Stamen Toner", "stamen-toner", "https://stamen-tiles.a.ssl.fastly.net/toner/{0}/{1}/{2}.png", "Map tiles by Stamen Design, under CC BY 3.0. Data by OpenStreetMap, under ODbL", nullptr, nullptr },
  { "Stamen Terrain", "stamen-terrain", "https://stamen-tiles.a.ssl.fastly.net/terrain/{0}/{1}/{2}.png", "Map tiles by Stamen Design, under CC BY 3.0. Data by OpenStreetMap, under ODbL", nullptr, nullptr },
  { "Stamen Watercolor", "stamen-watercolor", "https://stamen-tiles.a.ssl.fastly.net/watercolor/{0}/{1}/{2}.png", "Map tiles by Stamen Design, under CC BY 3.0. Data by OpenStreetMap, under CC BY SA", nullptr, nullptr },
  { "Esri WorldImagery", "esri-worldimagery", "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{0}/{2}/{1}", "Tiles \xC2\xA9 Esri - Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community", nullptr, nullptr },
  { "Custom", "custom", nullptr, nullptr, nullptr, nullptr },
};

enum vcLASClassifications
{
  vcLASClassifications_FirstReserved = 19,
  vcLASClassifications_FirstUserDefined = 64,
  vcLASClassifications_LastClassification = 255 // Because they are always stored in a uint8
};

void vcSettingsUI_Cleanup(vcState * /*pProgramState*/)
{
  for (size_t i = 0; i < udLengthOf(s_mapTiles); i++)
    vcTexture_Destroy(&s_mapTiles[i].pPreviewTexture);
}

void vcSettingsUI_ShowHeader(vcState *pProgramState, const char *pSettingTitle, vcSettingCategory category = vcSC_All)
{
  ImGui::Columns(2, nullptr, false);
  ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 125.f);
  ImGui::TextUnformatted(pSettingTitle);
  ImGui::NextColumn();
  if (category != vcSC_All)
  {
    if (ImGui::Button(udTempStr("%s##CategoryRestore", vcString::Get("settingsRestoreDefaults")), ImVec2(-1, 0)))
      vcSettings_Load(&pProgramState->settings, true, category);
  }
  ImGui::Columns(1);
  ImGui::Separator();
}

void vcSettingsUI_CustomClassificationColours(vcState *pProgramState, vcVisualizationSettings *pVisualizationSettings)
{
  if (pVisualizationSettings->useCustomClassificationColours)
  {
    ImGui::SameLine();
    ImGui::SameLine();
    if (ImGui::Button(udTempStr("%s##RestoreClassificationColors", vcString::Get("settingsRestoreDefaults"))))
    {
      memcpy(pProgramState->settings.visualization.customClassificationColors, GeoverseClassificationColours, sizeof(pProgramState->settings.visualization.customClassificationColors));
      for (int i = vcLASClassifications_FirstUserDefined; i <= vcLASClassifications_LastClassification; i++)
      {
        if (pVisualizationSettings->customClassificationColorLabels[i] != nullptr)
          udFree(pVisualizationSettings->customClassificationColorLabels[i]);
      }
    }

    for (uint8_t i = 0; i < vcLASClassifications_FirstReserved; ++i)
      vcIGSW_ColorPickerU32(vcSettingsUI_GetClassificationName(pProgramState, i), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);

    if (ImGui::TreeNode(vcString::Get("settingsVisClassReservedColours")))
    {
      for (uint8_t i = vcLASClassifications_FirstReserved; i < vcLASClassifications_FirstUserDefined; ++i)
        vcIGSW_ColorPickerU32(vcSettingsUI_GetClassificationName(pProgramState, i), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(vcString::Get("settingsVisClassUserDefinable")))
    {
      for (int xi = vcLASClassifications_FirstUserDefined; xi <= vcLASClassifications_LastClassification; ++xi)
      {
        uint8_t i = (uint8_t)xi;

        char buttonID[12], inputID[3];
        vcIGSW_ColorPickerU32(vcSettingsUI_GetClassificationName(pProgramState, i), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
        udSprintf(buttonID, "%s##%d", vcString::Get("settingsVisClassRename"), i);
        udSprintf(inputID, "##I%d", i);
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
            if (pVisualizationSettings->customClassificationColorLabels[i] != nullptr)
              udFree(pVisualizationSettings->customClassificationColorLabels[i]);
            pVisualizationSettings->customClassificationColorLabels[i] = udStrdup(pProgramState->renameText);
            pProgramState->renaming = -1;
          }
        }
      }
      ImGui::TreePop();
    }

    for (size_t i = 0; i < udLengthOf(pVisualizationSettings->customClassificationToggles); i++)
    {
      if (pVisualizationSettings->customClassificationToggles[i])
        pVisualizationSettings->customClassificationColors[i] = pVisualizationSettings->customClassificationColors[i] | 0xFF000000;
      else
        pVisualizationSettings->customClassificationColors[i] = pVisualizationSettings->customClassificationColors[i] & 0x00FFFFFF;
    }
  }
}

void vcSettingsUI_Show(vcState *pProgramState)
{
  if (pProgramState->openSettings)
  {
    pProgramState->openSettings = false;
    ImGui::OpenPopup(udTempStr("%s###settingsDock", vcString::Get("settingsTitle")));
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  }

  if (ImGui::BeginPopupModal(udTempStr("%s###settingsDock", vcString::Get("settingsTitle"))))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 125.f);
    ImGui::Text("udStream %s", VCVERSION_PRODUCT_STRING);

    char strBuf[128];
    if (pProgramState->packageInfo.Get("success").AsBool())
      ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", vcStringFormat(strBuf, udLengthOf(strBuf), vcString::Get("menuAboutPackageUpdate"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      vcSettings_Save(&pProgramState->settings);
      ImGui::CloseCurrentPopup();
      vcHotkey::ClearState();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::BeginChild("__settingsPane"))
    {
      ImGui::Columns(2);

      if (ImGui::IsWindowAppearing())
        ImGui::SetColumnWidth(0, 200.f);

      if (ImGui::BeginChild("__settingsPaneCategories"))
      {
        bool change = false;
        change |= ImGui::RadioButton(udTempStr("%s##AppearanceSettings", vcString::Get("settingsAppearance")), &pProgramState->activeSetting, vcSR_Appearance);
        change |= ImGui::RadioButton(udTempStr("%s##InputSettings", vcString::Get("settingsControls")), &pProgramState->activeSetting, vcSR_Inputs);
        ImGui::Indent();
        change |= ImGui::RadioButton(udTempStr("%s##KeyBindings", vcString::Get("bindingsTitle")), &pProgramState->activeSetting, vcSR_KeyBindings);
        ImGui::Unindent();
        change |= ImGui::RadioButton(udTempStr("%s##MapSettings", vcString::Get("settingsMaps")), &pProgramState->activeSetting, vcSR_Maps);
        change |= ImGui::RadioButton(udTempStr("%s##VisualisationSettings", vcString::Get("settingsVis")), &pProgramState->activeSetting, vcSR_Visualisations);
        change |= ImGui::RadioButton(udTempStr("%s##Tools", vcString::Get("Tools")), &pProgramState->activeSetting, vcSR_Tools);
        //change |= ImGui::RadioButton(udTempStr("%s##UnitsOfMeasurement", vcString::Get("settingsUnitsOfMeasurement")), &pProgramState->activeSetting, vcSR_UnitsOfMeasurement); //TODO The measurement system will eventually be its own section
#if VC_HASCONVERT
        if (pProgramState->branding.convertEnabled)
          change |= ImGui::RadioButton(udTempStr("%s##ConvertSettings", vcString::Get("settingsConvert")), &pProgramState->activeSetting, vcSR_ConvertDefaults);
#endif
        change |= ImGui::RadioButton(udTempStr("%s##ScreenshotSettings", vcString::Get("settingsScreenshot")), &pProgramState->activeSetting, vcSR_Screenshot);
        change |= ImGui::RadioButton(udTempStr("%s##ConnectionSettings", vcString::Get("settingsConnection")), &pProgramState->activeSetting, vcSR_Connection);

        ImGui::Separator();

        if (pProgramState->settings.presentation.showDiagnosticInfo)
        {
          change |= ImGui::RadioButton(udTempStr("%s##MissingStringsSettings", vcString::Get("settingsMissingStrings")), &pProgramState->activeSetting, vcSR_DiagnosticMissingStrings);
          change |= ImGui::RadioButton(udTempStr("%s##TranslationSettings", vcString::Get("settingsTranslation")), &pProgramState->activeSetting, vcSR_DiagnosticTranslation);
        }

        change |= ImGui::RadioButton(udTempStr("%s##ReleaseNotes", vcString::Get("menuReleaseNotes")), &pProgramState->activeSetting, vcSR_ReleaseNotes);

        if (pProgramState->packageInfo.Get("success").AsBool())
          change |= ImGui::RadioButton(udTempStr("%s##UpdateAvailable", vcString::Get("menuNewVersion")), &pProgramState->activeSetting, vcSR_Update);

        change |= ImGui::RadioButton(udTempStr("%s##About", vcString::Get("menuAbout")), &pProgramState->activeSetting, vcSR_About);

        if (change)
          vcHotkey::ClearState();
      }
      ImGui::EndChild();

      ImGui::NextColumn();

      if (ImGui::BeginChild("__settingsPaneDetails"))
      {
        if (pProgramState->activeSetting == vcSR_Appearance)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsAppearance"), vcSC_Appearance);

          vcSettingsUI_LangCombo(pProgramState);
          ImGui::SameLine();
          ImGui::TextUnformatted(vcString::Get("settingsAppearanceLanguage"));

          const char *measurementUnitOptions[] = {vcString::Get("settingsUnitsOfMeasurementMetric"), vcString::Get("settingsUnitsOfMeasurementImperial")};
          static int measurementSystem = -1;

          //This is a hack to determine which system we are using. This will eventually be meaningless as the user will choose each unit separately.
          if (measurementSystem)
            measurementSystem = (pProgramState->settings.unitConversionData.distanceUnit[0].unit == vcDistance_USSurveyInches) ? 1 : 0;

          if (ImGui::Combo(vcString::Get("settingsUnitsOfMeasurement"), &measurementSystem, measurementUnitOptions, (int)udLengthOf(measurementUnitOptions)))
          {
            if (measurementSystem == 0)
              vcUnitConversion_SetMetric(&pProgramState->settings.unitConversionData);
            else if (measurementSystem == 1)
              vcUnitConversion_SetUSSurvey(&pProgramState->settings.unitConversionData);
          }

          ImGui::Checkbox(vcString::Get("sceneCameraInfo"), &pProgramState->settings.presentation.showCameraInfo);
          ImGui::Checkbox(vcString::Get("sceneProjectionInfo"), &pProgramState->settings.presentation.showProjectionInfo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceAdvancedGIS"), &pProgramState->settings.presentation.showAdvancedGIS);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowDiagnostics"), &pProgramState->settings.presentation.showDiagnosticInfo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowEuclideonLogo"), &pProgramState->settings.presentation.showEuclideonLogo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowCameraFrustumInMapMode"), &pProgramState->settings.presentation.showCameraFrustumInMapMode);

          if (ImGui::SliderFloat(vcString::Get("settingsAppearancePOIDistance"), &pProgramState->settings.presentation.POIFadeDistance, vcSL_POIFaderMin, vcSL_POIFaderMax, "%.3fm", ImGuiSliderFlags_Logarithmic))
            pProgramState->settings.presentation.POIFadeDistance = udClamp(pProgramState->settings.presentation.POIFadeDistance, vcSL_POIFaderMin, vcSL_GlobalLimitf);
          if(ImGui::SliderFloat(vcString::Get("settingsAppearanceImageRescale"), &pProgramState->settings.presentation.imageRescaleDistance, vcSL_ImageRescaleMin, vcSL_ImageRescaleMax, "%.3fm", ImGuiSliderFlags_Logarithmic))
            pProgramState->settings.presentation.imageRescaleDistance = udClamp(pProgramState->settings.presentation.imageRescaleDistance, vcSL_ImageRescaleMin, vcSL_ImageRescaleMax);
          ImGui::Checkbox(vcString::Get("settingsAppearanceLimitFPS"), &pProgramState->settings.presentation.limitFPSInBackground);

#if VC_HASNATIVEFILEPICKER
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowNativeDialogs"), &pProgramState->settings.window.useNativeUI);
#endif

          const char *layoutOptions[] = { vcString::Get("settingsAppearanceWindowLayoutScSx"), vcString::Get("settingsAppearanceWindowLayoutSxSc") };
          if (ImGui::Combo(vcString::Get("settingsAppearanceWindowLayout"), (int*)&pProgramState->settings.presentation.layout, layoutOptions, (int)udLengthOf(layoutOptions)))
            pProgramState->settings.presentation.columnSizeCorrect = false;

          const char *mapModeViewportOptions[] = { vcString::Get("settingsAppearanceMapModeViewportNone"), vcString::Get("settingsAppearanceMapModeViewportLeft"), vcString::Get("settingsAppearanceMapModeViewportRight") };
          if (ImGui::Combo(vcString::Get("settingsAppearanceMapModeViewport"), (int*)&pProgramState->settings.mapModeViewport, mapModeViewportOptions, (int)udLengthOf(mapModeViewportOptions)))
          {
            for (int i = 0; i < vcMaxViewportCount; ++i)
              pProgramState->settings.camera.mapMode[i] = false;

            switch (pProgramState->settings.mapModeViewport)
            {
            case vcMMV_None:
              break;

            case vcMMV_Left:
              pProgramState->settings.camera.mapMode[0] = true;
              break;

            case vcMMV_Right:
              pProgramState->settings.camera.mapMode[1] = true;
              break;
            }
          }
        }

        if (pProgramState->activeSetting == vcSR_Inputs)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsControls"), vcSC_InputControls);

          ImGui::Checkbox(vcString::Get("settingsControlsOSC"), &pProgramState->settings.onScreenControls);
          if (ImGui::Checkbox(vcString::Get("settingsControlsTouchUI"), &pProgramState->settings.window.touchscreenFriendly))
          {
            ImGuiStyle& style = ImGui::GetStyle();
            style.TouchExtraPadding = pProgramState->settings.window.touchscreenFriendly ? ImVec2(4, 4) : ImVec2();
          }

          ImGui::Checkbox(vcString::Get("settingsControlsMouseInvertX"), &pProgramState->settings.camera.invertMouseX);
          ImGui::Checkbox(vcString::Get("settingsControlsMouseInvertY"), &pProgramState->settings.camera.invertMouseY);

          ImGui::Checkbox(vcString::Get("settingsControlsControllerInvertX"), &pProgramState->settings.camera.invertControllerX);
          ImGui::Checkbox(vcString::Get("settingsControlsControllerInvertY"), &pProgramState->settings.camera.invertControllerY);

          ImGui::Checkbox(vcString::Get("settingsControlsMouseSnapToPoints"), &pProgramState->settings.mouseSnap.enable);
          if (pProgramState->settings.mouseSnap.enable)
          {
            if (ImGui::SliderInt(vcString::Get("settingsControlsMouseSnapToPointsRange"), &pProgramState->settings.mouseSnap.range, 1, vcSL_MouseSnapRangeMax))
              pProgramState->settings.mouseSnap.range = udClamp(pProgramState->settings.mouseSnap.range, 1, vcSL_MouseSnapRangeMax);
          }

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

        if (pProgramState->activeSetting == vcSR_Maps)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsMaps"), vcSC_MapsElevation);

          vcSettingsUI_BasicMapSettings(pProgramState);
        }

        if (pProgramState->activeSetting == vcSR_Visualisations)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsVis"), vcSC_Visualization);

          vcSettingsUI_SceneVisualizationSettings(pProgramState);
        }

        if (pProgramState->activeSetting == vcSR_Tools)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("Tools"), vcSC_Tools);
          ImGui::Text("%s", vcString::Get("settingsToolsDefaultValues"));
          ImGui::SliderFloat(vcString::Get("scenePOILineWidth"), &pProgramState->settings.tools.line.width, pProgramState->settings.tools.line.minWidth, pProgramState->settings.tools.line.maxWidth, "%.2f", ImGuiSliderFlags_Logarithmic);
           
          const char *fenceOptions[] = { vcString::Get("scenePOILineOrientationScreenLine"), vcString::Get("scenePOILineOrientationVert"), vcString::Get("scenePOILineOrientationHorz") };
          ImGui::Combo(vcString::Get("scenePOILineOrientation"), &pProgramState->settings.tools.line.fenceMode, fenceOptions, (int)udLengthOf(fenceOptions));
         
          const char *lineOptions[] = { vcString::Get("scenePOILineStyleArrow"), vcString::Get("scenePOILineStyleGlow"), vcString::Get("scenePOILineStyleSolid"), vcString::Get("scenePOILineStyleDiagonal") };
          ImGui::Combo(vcString::Get("scenePOILineStyle"), &pProgramState->settings.tools.line.style, lineOptions, (int)udLengthOf(lineOptions));

          ImGui::ColorEdit4(vcString::Get("scenePOILineColour1"), &pProgramState->settings.tools.line.colour[0], ImGuiColorEditFlags_None);
          ImGui::ColorEdit4(vcString::Get("scenePOIFillColour"), &pProgramState->settings.tools.fill.colour[0], ImGuiColorEditFlags_None);
          ImGui::ColorEdit4(vcString::Get("scenePOILabelColour"), &pProgramState->settings.tools.label.textColour[0], ImGuiColorEditFlags_None);
          ImGui::ColorEdit4(vcString::Get("scenePOILabelBackgroundColour"), &pProgramState->settings.tools.label.backgroundColour[0], ImGuiColorEditFlags_None);
        }

        if (pProgramState->activeSetting == vcSR_KeyBindings)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("bindingsTitle"), vcSC_Bindings);

          vcHotkey::DisplayBindings(pProgramState);
        }
        else
        {
          vcHotkey::ClearState();
        }

        if (pProgramState->activeSetting == vcSR_ConvertDefaults)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsConvert"), vcSC_Convert);

          // Temp directory
          vcIGSW_FilePicker(pProgramState, vcString::Get("convertTempDirectory"), pProgramState->settings.convertdefaults.tempDirectory, udLengthOf(pProgramState->settings.convertdefaults.tempDirectory), nullptr, 0, vcFDT_SelectDirectory, [pProgramState] {
            // Nothing needs to happen here
            udUnused(pProgramState);
          });

          // Metadata
          vcIGSW_InputText(vcString::Get("convertAuthor"), pProgramState->settings.convertdefaults.author, udLengthOf(pProgramState->settings.convertdefaults.author));
          vcIGSW_InputText(vcString::Get("convertComment"), pProgramState->settings.convertdefaults.comment, udLengthOf(pProgramState->settings.convertdefaults.comment));
          vcIGSW_InputText(vcString::Get("convertCopyright"), pProgramState->settings.convertdefaults.copyright, udLengthOf(pProgramState->settings.convertdefaults.copyright));
          vcIGSW_InputText(vcString::Get("convertLicense"), pProgramState->settings.convertdefaults.license, udLengthOf(pProgramState->settings.convertdefaults.license));
        }

        if (pProgramState->activeSetting == vcSR_Screenshot)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsScreenshot"), vcSC_Screenshot);

          static udUInt2 ScreenshotResolutions[] = { { 1280, 720 }, { 1920, 1080 }, { 4096, 2160 } };
          static udUInt2 ScreenshotResolutionsMax = { 4096, 4096 };
          static udUInt2 ScreenshotResolutionsMin = { 32, 32 };
          const char* ResolutionNames[] = { vcString::Get("settingsScreenshotRes720p"), vcString::Get("settingsScreenshotRes1080p"), vcString::Get("settingsScreenshotRes4K"), vcString::Get("settingsScreenshotResCustom") };
          UDCOMPILEASSERT(udLengthOf(ResolutionNames) == udLengthOf(ScreenshotResolutions) + 1, "Update strings!");

          if (pProgramState->settings.screenshot.resolutionIndex != udLengthOf(ScreenshotResolutions))
          {
            for (pProgramState->settings.screenshot.resolutionIndex = 0; pProgramState->settings.screenshot.resolutionIndex < (int)udLengthOf(ScreenshotResolutions); ++pProgramState->settings.screenshot.resolutionIndex)
            {
              if (pProgramState->settings.screenshot.resolution == ScreenshotResolutions[pProgramState->settings.screenshot.resolutionIndex])
                break;
            }
          }

          // Resolution
          if (ImGui::Combo(vcString::Get("settingsScreenshotResLabel"), &pProgramState->settings.screenshot.resolutionIndex, ResolutionNames, (int)udLengthOf(ResolutionNames)))
          {
            if (pProgramState->settings.screenshot.resolutionIndex < (int)udLengthOf(ScreenshotResolutions))
              pProgramState->settings.screenshot.resolution = ScreenshotResolutions[pProgramState->settings.screenshot.resolutionIndex];
          }

          if (pProgramState->settings.screenshot.resolutionIndex == (int)udLengthOf(ScreenshotResolutions))
          {
            ImGui::Indent();
            if (ImGui::InputInt2(vcString::Get("settingsScreenshotResLabel"), (int*)&pProgramState->settings.screenshot.resolution.x))
            {
              pProgramState->settings.screenshot.resolution = udClamp(pProgramState->settings.screenshot.resolution, ScreenshotResolutionsMin, ScreenshotResolutionsMax);
            }
            ImGui::Unindent();
          }

          // Output filename
          vcIGSW_FilePicker(pProgramState, vcString::Get("settingsScreenshotFilename"), pProgramState->settings.screenshot.outputPath, udLengthOf(pProgramState->settings.screenshot.outputPath), nullptr, 0, vcFDT_SelectDirectory, nullptr);

          // Show Picture
          ImGui::Checkbox(vcString::Get("settingsScreenshotView"), &pProgramState->settings.screenshot.viewShot);

          udFindDir* poutputPath(nullptr);
          udResult result = udOpenDir(&poutputPath, pProgramState->settings.screenshot.outputPath);
          if (result != udR_Success)
            ImGui::Text("%s", vcString::Get("settingsScreenshotPathAlert"));
          udCloseDir(&poutputPath);

        }

        if (pProgramState->activeSetting == vcSR_Connection)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsConnection"), vcSC_Connection);

          if (ImGui::Button(vcString::Get("loginProxyTest")) && !pProgramState->settings.loginInfo.testing)
          {
            pProgramState->settings.loginInfo.testing = true;
            udWorkerPool_AddTask(pProgramState->pWorkerPool, [pProgramState] (void*){
              pProgramState->settings.loginInfo.testStatus = vcProxyHelper_TestProxy(pProgramState);
              pProgramState->settings.loginInfo.testing = false;
              pProgramState->settings.loginInfo.tested = true;
            }, nullptr, true, [pProgramState](void*) {
              if (pProgramState->settings.loginInfo.testStatus == udE_ProxyAuthRequired)
                pProgramState->settings.loginInfo.requiresProxyAuth = true;
            });
          }

          if (pProgramState->settings.loginInfo.testing)
          {
            ImGui::SameLine();
            vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
            ImGui::TextUnformatted(vcString::Get("loginProxyTestRunning"));
          }
          else if (pProgramState->settings.loginInfo.tested)
          {
            ImGui::SameLine();
            if (pProgramState->settings.loginInfo.testStatus == udE_Success)
            {
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Success);
              ImGui::TextUnformatted(vcString::Get("loginProxyTestSuccess"));
            }
            else
            {
              char buffer[256];
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Failed);

              if (pProgramState->settings.loginInfo.testStatus == udE_SecurityFailure)
                ImGui::TextUnformatted(vcString::Get("loginErrorSecurity"));
              else if (pProgramState->settings.loginInfo.testStatus == udE_ConnectionFailure)
                ImGui::TextUnformatted(vcString::Get("loginProxyTestCouldNotConnect"));
              else if (pProgramState->settings.loginInfo.testStatus == udE_ProxyError)
                ImGui::TextUnformatted(vcString::Get("loginErrorProxy"));
              else if (pProgramState->settings.loginInfo.testStatus == udE_ProxyAuthRequired)
                ImGui::TextUnformatted(vcString::Get("loginErrorProxyAuthFailed"));
              else
                ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("loginProxyTestFailed"), udTempStr("%d", pProgramState->settings.loginInfo.testStatus)));
            }
          }

          if (!pProgramState->settings.loginInfo.testing)
          {
            ImGui::Separator();

            if (vcIGSW_InputText(vcString::Get("loginProxyAddress"), pProgramState->settings.loginInfo.proxy))
            {
              udConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);
              pProgramState->settings.loginInfo.tested = false;
            }

            ImGui::SameLine();
            if (ImGui::Button(vcString::Get("loginProxyAutodetect")))
            {
              if (vcProxyHelper_AutoDetectProxy(pProgramState) == udE_Success)
              {
                udStrcpy(pProgramState->settings.loginInfo.proxy, pProgramState->settings.loginInfo.autoDetectProxyURL);
                udConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);
              }
            }

            if (vcIGSW_InputText(vcString::Get("loginUserAgent"), pProgramState->settings.loginInfo.userAgent))
            {
              udConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);
              pProgramState->settings.loginInfo.tested = false;
            }

            ImGui::SameLine();

            // TODO: Consider reading user agent strings from a file
            const char *UAOptions[] = { "Firefox on Windows", "Chrome on Windows" };
            const char *UAStrings[] = { "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:54.0) Gecko/20100101 Firefox/73.0", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36" };

            if (ImGui::BeginCombo("###loginUserAgentPresets", nullptr, ImGuiComboFlags_NoPreview))
            {
              for (size_t i = 0; i < udLengthOf(UAOptions); ++i)
              {
                if (ImGui::MenuItem(UAOptions[i]))
                {
                  udStrcpy(pProgramState->settings.loginInfo.userAgent, UAStrings[i]);
                  udConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);
                  pProgramState->settings.loginInfo.tested = false;
                }
              }
              ImGui::EndCombo();
            }

            if (ImGui::Checkbox(vcString::Get("loginIgnoreCert"), &pProgramState->settings.loginInfo.ignoreCertificateVerification))
            {
              udConfig_IgnoreCertificateVerification(pProgramState->settings.loginInfo.ignoreCertificateVerification);
              pProgramState->settings.loginInfo.tested = false;
            }

            if (pProgramState->settings.loginInfo.ignoreCertificateVerification)
            {
              ImGui::SameLine();
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.5f, 0.5f, 1.f));
              ImGui::TextWrapped("%s", vcString::Get("loginIgnoreCertWarning"));
              ImGui::PopStyleColor();
            }

            if (ImGui::Checkbox(vcString::Get("loginProxyRequiresAuth"), &pProgramState->settings.loginInfo.requiresProxyAuth))
              pProgramState->settings.loginInfo.tested = false;

            if (pProgramState->settings.loginInfo.requiresProxyAuth)
            {
              ImGui::Indent();

              bool updateInfo = false;

              updateInfo |= vcIGSW_InputText(vcString::Get("modalProxyUsername"), pProgramState->settings.loginInfo.proxyUsername, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
              if (ImGui::IsItemDeactivatedAfterEdit())
                updateInfo = true;

              ImGui::SameLine();
              ImGui::Checkbox(udTempStr("%s##rememberProxyUser", vcString::Get("loginRememberUser")), &pProgramState->settings.loginInfo.rememberProxyUsername);

              updateInfo |= vcIGSW_InputText(vcString::Get("modalProxyPassword"), pProgramState->settings.loginInfo.proxyPassword, udLengthOf(pProgramState->settings.loginInfo.proxyPassword), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
              if (ImGui::IsItemDeactivatedAfterEdit())
                updateInfo = true;

              if (updateInfo)
              {
                pProgramState->settings.loginInfo.tested = false;
                udConfig_SetProxyAuth(pProgramState->settings.loginInfo.proxyUsername, pProgramState->settings.loginInfo.proxyPassword);
              }

              ImGui::Unindent();
            }
          }
        }

        if (pProgramState->activeSetting == vcSR_ReleaseNotes)
        {
          if (pProgramState->pReleaseNotes == nullptr)
            udFile_Load("asset://releasenotes.md", (void**)&pProgramState->pReleaseNotes);

          if (pProgramState->pReleaseNotes != nullptr)
          {
            ImGui::BeginChild(vcString::Get("menuReleaseNotesShort"));
            vcIGSW_Markdown(pProgramState, pProgramState->pReleaseNotes);
            ImGui::EndChild();
          }
          else
          {
            ImGui::TextUnformatted(vcString::Get("menuReleaseNotesFail"));
          }
        }

        if (pProgramState->activeSetting == vcSR_Update)
        {
          ImGui::TextWrapped("%s", vcString::Get("menuNewVersionDownloadPrompt"));

          ImGui::BeginChild(vcString::Get("menuReleaseNotesTitle"), ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
          ImGui::TextUnformatted(pProgramState->packageInfo.Get("package.releasenotes").AsString(""));
          ImGui::EndChild();
        }

        if (pProgramState->activeSetting == vcSR_About)
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

          ImGui::Separator();

          ImGui::BeginChild("Licenses");
          for (int i = 0; i < (int)udLengthOf(ThirdPartyLicenses); i++)
          {
            // ImGui::Text has a limitation of 3072 bytes.
            ImGui::TextUnformatted(ThirdPartyLicenses[i].pName);
            ImGui::TextUnformatted(ThirdPartyLicenses[i].pLicense);
            ImGui::Separator();
          }
          ImGui::EndChild();
        }

        if (pProgramState->activeSetting == vcSR_DiagnosticMissingStrings)
        {
          ImGui::BeginChild("MissingStrings");
          vcString::ShowMissingStringUI();
          ImGui::EndChild();
        }

        if (pProgramState->activeSetting == vcSR_DiagnosticTranslation)
        {
          ImGui::BeginChild("Translation");
          vcString::ShowTranslationHelperUI(pProgramState);
          ImGui::EndChild();
        }
      }
      ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::EndPopup();
  }
}

bool vcSettingsUI_LangCombo(vcState *pProgramState)
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

const char *vcSettingsUI_GetClassificationName(vcState *pProgramState, uint8_t classification)
{
  if (classification < vcLASClassifications_FirstReserved)
  {
    const char *localizations[] = {
      "settingsVisClassNeverClassified",
      "settingsVisClassUnclassified",
      "settingsVisClassGround",
      "settingsVisClassLowVegetation",
      "settingsVisClassMediumVegetation",
      "settingsVisClassHighVegetation",
      "settingsVisClassBuilding",
      "settingsVisClassLowPoint",
      "settingsVisClassKeyPoint",
      "settingsVisClassWater",
      "settingsVisClassRail",
      "settingsVisClassRoadSurface",
      "settingsVisClassReserved",
      "settingsVisClassWireGuard",
      "settingsVisClassWireConductor",
      "settingsVisClassTransmissionTower",
      "settingsVisClassWireStructureConnector",
      "settingsVisClassBridgeDeck",
      "settingsVisClassHighNoise"
    };

    return udTempStr("%d. %s", classification, vcString::Get(localizations[classification]));
  }
  else if (classification < vcLASClassifications_FirstUserDefined)
  {
    return udTempStr("%d. %s", classification, vcString::Get("settingsVisClassReservedLabels"));
  }
  else // User defined
  {
    if (pProgramState->settings.visualization.customClassificationColorLabels[classification] == nullptr)
      return udTempStr("%d. %s", classification, vcString::Get("settingsVisClassUserDefined"));
    else
      return udTempStr("%d. %s", classification, pProgramState->settings.visualization.customClassificationColorLabels[classification]);
  }
}

void vcSettingsUI_BasicMapSettings(vcState *pProgramState, bool alwaysShowOptions /*= false*/)
{
  ImGui::Checkbox(vcString::Get("settingsMapsMapTiles"), &pProgramState->settings.maptiles.mapEnabled);

  if (alwaysShowOptions || pProgramState->settings.maptiles.mapEnabled)
  {
    ImGui::Checkbox(vcString::Get("settingsMapsDEM"), &pProgramState->settings.maptiles.demEnabled);
    ImGui::Checkbox(vcString::Get("sceneCameraKeepAboveSurface"), &pProgramState->settings.camera.keepAboveSurface);

    if (pProgramState->settings.presentation.showDiagnosticInfo)
    {
      if (pProgramState->geozone.srid != vcPSZ_NotGeolocated)
      {
        int32_t newSRID = -1;
        char buffer[512];

        if (pProgramState->geozone.srid != vcPSZ_WGS84ECEF && ImGui::Button(vcString::Get("settingsMapECEFMode")))
        {
          pProgramState->previousSRID = pProgramState->geozone.srid;
          newSRID = vcPSZ_WGS84ECEF;
        }

        if (pProgramState->geozone.srid == vcPSZ_WGS84ECEF && pProgramState->previousSRID != -1 && ImGui::Button(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("settingsMapFlatMode"), udTempStr("%d", pProgramState->previousSRID))))
          newSRID = pProgramState->previousSRID;

        if (newSRID != -1)
        {
          udGeoZone zone = {};
          udGeoZone_SetFromSRID(&zone, newSRID);

          for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
            vcGIS_ChangeSpace(&pProgramState->geozone, zone, &pProgramState->pViewports[viewportIndex].camera.position, viewportIndex + 1 == pProgramState->settings.activeViewportCount);

          pProgramState->activeProject.pFolder->ChangeProjection(zone);
        }
      }
    }

    if (ImGui::BeginChild("__settingsMapServerSelection", ImVec2(512, 384)))
    {
      ImGui::TextUnformatted(vcString::Get("settingsMapMapLayers"));

      char buffer[128] = {};
      char mapLayerStr[16] = {};

      if (pProgramState->settings.maptiles.mapEnabled)
      {
        pProgramState->settings.maptiles.activeLayerCount = 0;
        for (int mapLayer = 0; mapLayer < vcMaxTileLayerCount; ++mapLayer)
        {
          if (ImGui::Button(udTempStr(pProgramState->settings.maptiles.layers[mapLayer].enabled ? "-##mapLayer%d" : "+##mapLayer%d", mapLayer), ImVec2(24, 24)))
            pProgramState->settings.maptiles.layers[mapLayer].enabled = !pProgramState->settings.maptiles.layers[mapLayer].enabled;

          ImGui::SameLine();
          udStrItoa(mapLayerStr, mapLayer + 1);
          ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("settingsMapLayer"), mapLayerStr));

          if (!pProgramState->settings.maptiles.layers[mapLayer].enabled)
            break;

          pProgramState->settings.maptiles.activeLayerCount++;

          ImGui::Indent();
          ImGui::TextUnformatted(vcString::Get("settingsMapType"));

          ImGui::BeginChild(udTempStr("mapSelection%d", mapLayer), ImVec2(512, 200));

          ImGuiStyle &style = ImGui::GetStyle();
          ImVec2 button_sz(92, 92);
          float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

          bool customServerSpecified = pProgramState->settings.maptiles.layers[mapLayer].customServer.tileServerAddress[0] != '\0';
          bool customServerSelected = udStrEqual(pProgramState->settings.maptiles.layers[mapLayer].mapType, "custom");

          for (size_t i = 0; i < udLengthOf(s_mapTiles); i++)
          {
            ImGui::PushID((int)i);

            bool pop = udStrEqual(pProgramState->settings.maptiles.layers[mapLayer].mapType, s_mapTiles[i].pModeStr);
            
            if (s_mapTiles[i].pPreviewTexture == nullptr)
            {
              vcTexture_AsyncCreateFromFilename(&s_mapTiles[i].pPreviewTexture, pProgramState->pWorkerPool, udTempStr("asset://assets/textures/mapservers/%s.png", s_mapTiles[i].pModeStr), vcTFM_Linear);
              s_mapTiles[i].pPreviewTexture = pProgramState->pWhiteTexture;
            }

            if (!udStrEqual(s_mapTiles[i].pModeStr, "custom") || customServerSelected || customServerSpecified || !alwaysShowOptions)
            {
              if (pop)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

              if (ImGui::ImageButton(s_mapTiles[i].pPreviewTexture, button_sz))
              {
                udStrcpy(pProgramState->settings.maptiles.layers[mapLayer].mapType, s_mapTiles[i].pModeStr);
                vcSettings_ApplyMapChange(&pProgramState->settings, mapLayer);
                for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
                  vcRender_ClearTiles(pProgramState->pViewports[viewportIndex].pRenderContext);
              }

              if (pop)
                ImGui::PopStyleColor();

              if (ImGui::IsItemHovered())
              {
                ImGui::BeginTooltip();
                if (udStrEqual(s_mapTiles[i].pMode, "Custom"))
                  ImGui::TextUnformatted(vcString::Get("settingsMapTypeCustom"));
                else
                  ImGui::TextUnformatted(s_mapTiles[i].pMode);
                ImGui::EndTooltip();
              }

              float last_button_x2 = ImGui::GetItemRectMax().x;
              float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line

              if (i + 1 < udLengthOf(s_mapTiles) && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
            }
            ImGui::PopID();
          }

          ImGui::EndChild();

          if (pProgramState->settings.maptiles.mapEnabled)
          {
            if (customServerSelected)
            {
              ImGui::Indent();

              bool changed = false;

              ImGui::TextWrapped("%s", vcString::Get("settingsMapsTileServerInstructions"));

              changed |= vcIGSW_InputText(udTempStr("%s##mapLayer%d", vcString::Get("settingsMapsTileServer"), mapLayer), pProgramState->settings.maptiles.layers[mapLayer].customServer.tileServerAddress);
              changed |= vcIGSW_InputText(udTempStr("%s##mapLayer%d", vcString::Get("settingsMapsAttribution"), mapLayer), pProgramState->settings.maptiles.layers[mapLayer].customServer.attribution);

              if (changed)
              {
                vcSettings_ApplyMapChange(&pProgramState->settings, mapLayer);
                for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
                  vcRender_ClearTiles(pProgramState->pViewports[viewportIndex].pRenderContext);
              }

              ImGui::Unindent();
            }
            if (ImGui::SliderFloat(udTempStr("%s##%d", vcString::Get("settingsMapsMapHeight"), mapLayer), &pProgramState->settings.maptiles.layers[mapLayer].mapHeight, vcSL_MapHeightMin, vcSL_MapHeightMax, "%.3fm", ImGuiSliderFlags_Logarithmic))
              pProgramState->settings.maptiles.layers[mapLayer].mapHeight = udClamp(pProgramState->settings.maptiles.layers[mapLayer].mapHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);

            if (ImGui::SliderFloat(udTempStr("%s##mapLayer%d", vcString::Get("settingsMapsOpacity"), mapLayer), &pProgramState->settings.maptiles.layers[mapLayer].transparency, vcSL_OpacityMin, vcSL_OpacityMax, "%.3f"))
              pProgramState->settings.maptiles.layers[mapLayer].transparency = udClamp(pProgramState->settings.maptiles.layers[mapLayer].transparency, vcSL_OpacityMin, vcSL_OpacityMax);

            if (ImGui::InputInt(udTempStr("%s##mapLayer%d", vcString::Get("settingsMapsMaxDepth"), mapLayer), &pProgramState->settings.maptiles.layers[mapLayer].maxDepth, vcSL_MaxDepthMin, vcSL_MaxDepthMax))
              pProgramState->settings.maptiles.layers[mapLayer].maxDepth = udClamp(pProgramState->settings.maptiles.layers[mapLayer].maxDepth, vcSL_MaxDepthMin, vcSL_MaxDepthMax);
          }
          ImGui::Unindent();
        }
      }
    }
    ImGui::EndChild();
  }
}

void vcSettingsUI_SceneVisualizationSettings(vcState *pProgramState)
{
  vcSettingsUI_VisualizationSettings(&pProgramState->settings.visualization);

  if (pProgramState->settings.visualization.mode == vcVM_Classification)
    vcSettingsUI_CustomClassificationColours(pProgramState, &pProgramState->settings.visualization);

  const char *lensNameArray[] = {
    vcString::Get("settingsViewportCameraLensCustom"),
    vcString::Get("settingsViewportCameraLens15mm"),
    vcString::Get("settingsViewportCameraLens24mm"),
    vcString::Get("settingsViewportCameraLens30mm"),
    vcString::Get("settingsViewportCameraLens50mm"),
    vcString::Get("settingsViewportCameraLens70mm"),
    vcString::Get("settingsViewportCameraLens100mm"),
  };
  UDCOMPILEASSERT(udLengthOf(lensNameArray) == vcLS_TotalLenses, "Lens name array length mismatch");
  for (int v = 0; v < vcMaxViewportCount; v++)
  {
    if (v < pProgramState->settings.activeViewportCount)
    {
      if (ImGui::Combo(udTempStr("%s %d", vcString::Get("settingsViewportCameraLens"), v), &pProgramState->settings.camera.lensIndex[v], lensNameArray, (int)udLengthOf(lensNameArray)))
      {
        switch (pProgramState->settings.camera.lensIndex[v])
        {
        case vcLS_Custom:
          /*Custom FoV*/
          break;
        case vcLS_15mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens15mm;
          break;
        case vcLS_24mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens24mm;
          break;
        case vcLS_30mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens30mm;
          break;
        case vcLS_50mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens50mm;
          break;
        case vcLS_70mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens70mm;
          break;
        case vcLS_100mm:
          pProgramState->settings.camera.fieldOfView[v] = vcLens100mm;
          break;
        }
      }

      if (pProgramState->settings.camera.lensIndex[v] == vcLS_Custom)
      {
        float fovDeg = UD_RAD2DEGf(pProgramState->settings.camera.fieldOfView[v]);
        ImGui::Indent();
        if (ImGui::SliderFloat(udTempStr("%s %d", vcString::Get("settingsViewportFOV"), v), &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "%.0f°"))
          pProgramState->settings.camera.fieldOfView[v] = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
        ImGui::Unindent();
      }
    }
  }

  const char *skyboxOptions[] = { vcString::Get("settingsAppearanceSkyboxTypeNone"), vcString::Get("settingsAppearanceSkyboxTypeColour"), vcString::Get("settingsAppearanceSkyboxTypeSimple"), vcString::Get("settingsAppearanceSkyboxTypeAtmosphere") };
  ImGui::Combo(vcString::Get("settingsAppearanceSkyboxType"), (int*)&pProgramState->settings.presentation.skybox.type, skyboxOptions, (int)udLengthOf(skyboxOptions));
  if (pProgramState->settings.presentation.skybox.type == vcSkyboxType_Colour)
  {
    ImGui::Indent();
    ImGui::ColorEdit3(vcString::Get("settingsAppearanceSkyboxColour"), &pProgramState->settings.presentation.skybox.colour.x);
    ImGui::Unindent();
  }
  else if (pProgramState->settings.presentation.skybox.type == vcSkyboxType_Atmosphere)
  {
    ImGui::Indent();
    ImGui::Checkbox(vcString::Get("settingsAppearanceSkyboxUseLiveTime"), &pProgramState->settings.presentation.skybox.useLiveTime);

    if (!pProgramState->settings.presentation.skybox.useLiveTime)
    {
      ImGui::Checkbox(vcString::Get("settingsAppearanceSkyboxLockSunPosition"), &pProgramState->settings.presentation.skybox.keepSameTime);
      ImGui::SliderFloat(vcString::Get("settingsAppearanceSkyboxTimeOfDay"), &pProgramState->settings.presentation.skybox.timeOfDay, 0, 24, "%.3f", ImGuiSliderFlags_ClampOnInput);
      ImGui::SliderFloat(vcString::Get("settingsAppearanceSkyboxTimeOfYear"), &pProgramState->settings.presentation.skybox.month, 1, 12, "%.3f", ImGuiSliderFlags_ClampOnInput);
    }

    ImGui::SliderFloat(vcString::Get("settingsAppearanceSkyboxExposure"), &pProgramState->settings.presentation.skybox.exposure, 0.0f, 100.0f);

    ImGui::Unindent();
  }

  // limit the value between 0-5.
  if (ImGui::SliderFloat(vcString::Get("settingsAppearanceSaturation"), &pProgramState->settings.presentation.saturation, 0.0f, 5.0f))
    pProgramState->settings.presentation.saturation = udClamp(pProgramState->settings.presentation.saturation, 0.0f, 5.0f);

  const char *voxelOptions[] = { vcString::Get("settingsAppearanceRectangles"), vcString::Get("settingsAppearanceCubes"), vcString::Get("settingsAppearancePoints") };
  ImGui::Combo(vcString::Get("settingsAppearanceVoxelShape"), &pProgramState->settings.presentation.pointMode, voxelOptions, (int)udLengthOf(voxelOptions));

  // Selected Object Highlighting
  ImGui::Checkbox(vcString::Get("settingsVisObjectHighlight"), &pProgramState->settings.objectHighlighting.enable);
  if (pProgramState->settings.objectHighlighting.enable)
  {
    ImGui::Indent();

    // colour[] need to be between 0-1
    pProgramState->settings.objectHighlighting.colour = udClamp(pProgramState->settings.objectHighlighting.colour, udFloat4::create(0.0f), udFloat4::create(1.0f));

    ImGui::ColorEdit4(vcString::Get("settingsVisHighlightColour"), &pProgramState->settings.objectHighlighting.colour.x);
    ImGui::SliderFloat(vcString::Get("settingsVisHighlightThickness"), &pProgramState->settings.objectHighlighting.thickness, 1.0f, 3.0f);
    ImGui::Unindent();
  }

  // Post visualization - Edge Highlighting
  ImGui::Checkbox(vcString::Get("settingsVisEdge"), &pProgramState->settings.postVisualization.edgeOutlines.enable);
  if (pProgramState->settings.postVisualization.edgeOutlines.enable)
  {
    ImGui::Indent();

    if (ImGui::SliderInt(vcString::Get("settingsVisEdgeWidth"), &pProgramState->settings.postVisualization.edgeOutlines.width, vcSL_EdgeHighlightMin, vcSL_EdgeHighlightMax))
      pProgramState->settings.postVisualization.edgeOutlines.width = udClamp(pProgramState->settings.postVisualization.edgeOutlines.width, vcSL_EdgeHighlightMin, vcSL_EdgeHighlightMax);

    // TODO: Make this less awful. 0-100 would make more sense than 0.0001 to 0.001.
    if (ImGui::SliderFloat(vcString::Get("settingsVisEdgeThreshold"), &pProgramState->settings.postVisualization.edgeOutlines.threshold, vcSL_EdgeHighlightThresholdMin, vcSL_EdgeHighlightThresholdMax, "%.3f", ImGuiSliderFlags_Logarithmic))
      pProgramState->settings.postVisualization.edgeOutlines.threshold = udClamp(pProgramState->settings.postVisualization.edgeOutlines.threshold, vcSL_EdgeHighlightThresholdMin, vcSL_EdgeHighlightThresholdMax);
    ImGui::ColorEdit4(vcString::Get("settingsVisEdgeColour"), &pProgramState->settings.postVisualization.edgeOutlines.colour.x);

    ImGui::Unindent();
  }

  // Post visualization - Colour by Height
  ImGui::Checkbox(vcString::Get("settingsVisHeight"), &pProgramState->settings.postVisualization.colourByHeight.enable);
  if (pProgramState->settings.postVisualization.colourByHeight.enable)
  {
    ImGui::Indent();

    ImGui::ColorEdit4(vcString::Get("settingsVisHeightStartColour"), &pProgramState->settings.postVisualization.colourByHeight.minColour.x);
    ImGui::ColorEdit4(vcString::Get("settingsVisHeightEndColour"), &pProgramState->settings.postVisualization.colourByHeight.maxColour.x);

    // TODO: Set min/max to the bounds of the model? Currently set to 0m -> 1km with accuracy of 1mm
    if (ImGui::SliderFloat(vcString::Get("settingsVisHeightStart"), &pProgramState->settings.postVisualization.colourByHeight.startHeight, vcSL_ColourByHeightMin, vcSL_ColourByHeightMax, "%.3f"))
      pProgramState->settings.postVisualization.colourByHeight.startHeight = udClamp(pProgramState->settings.postVisualization.colourByHeight.startHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
    if (ImGui::SliderFloat(vcString::Get("settingsVisHeightEnd"), &pProgramState->settings.postVisualization.colourByHeight.endHeight, vcSL_ColourByHeightMin, vcSL_ColourByHeightMax, "%.3f"))
      pProgramState->settings.postVisualization.colourByHeight.endHeight = udClamp(pProgramState->settings.postVisualization.colourByHeight.endHeight, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);

    ImGui::Unindent();
  }

  // Post visualization - Colour by Depth
  ImGui::Checkbox(vcString::Get("settingsVisDepth"), &pProgramState->settings.postVisualization.colourByDepth.enable);
  if (pProgramState->settings.postVisualization.colourByDepth.enable)
  {
    ImGui::Indent();

    ImGui::ColorEdit4(vcString::Get("settingsVisDepthColour"), &pProgramState->settings.postVisualization.colourByDepth.colour.x);

    // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
    if (ImGui::SliderFloat(vcString::Get("settingsVisDepthStart"), &pProgramState->settings.postVisualization.colourByDepth.startDepth, vcSL_ColourByDepthMin, vcSL_ColourByDepthMax, "%.3f"))
      pProgramState->settings.postVisualization.colourByDepth.startDepth = udClamp(pProgramState->settings.postVisualization.colourByDepth.startDepth, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);
    if (ImGui::SliderFloat(vcString::Get("settingsVisDepthEnd"), &pProgramState->settings.postVisualization.colourByDepth.endDepth, vcSL_ColourByDepthMin, vcSL_ColourByDepthMax, "%.3f"))
      pProgramState->settings.postVisualization.colourByDepth.endDepth = udClamp(pProgramState->settings.postVisualization.colourByDepth.endDepth, -vcSL_GlobalLimitf, vcSL_GlobalLimitf);

    ImGui::Unindent();
  }

  // Post visualization - Contours
  ImGui::Checkbox(vcString::Get("settingsVisContours"), &pProgramState->settings.postVisualization.contours.enable);
  if (pProgramState->settings.postVisualization.contours.enable)
  {
    ImGui::Indent();

    ImGui::ColorEdit4(vcString::Get("settingsVisContoursColour"), &pProgramState->settings.postVisualization.contours.colour.x);

    // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
    if (ImGui::SliderFloat(vcString::Get("settingsVisContoursDistances"), &pProgramState->settings.postVisualization.contours.distances, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax, "%.3f", ImGuiSliderFlags_Logarithmic))
      pProgramState->settings.postVisualization.contours.distances = udClamp(pProgramState->settings.postVisualization.contours.distances, vcSL_ContourDistanceMin, vcSL_GlobalLimitSmallf);
    if (ImGui::SliderFloat(vcString::Get("settingsVisContoursBandHeight"), &pProgramState->settings.postVisualization.contours.bandHeight, vcSL_ContourBandHeightMin, vcSL_ContourBandHeightMax, "%.3f", ImGuiSliderFlags_Logarithmic))
      pProgramState->settings.postVisualization.contours.bandHeight = udClamp(pProgramState->settings.postVisualization.contours.bandHeight, vcSL_ContourBandHeightMin, vcSL_GlobalLimitSmallf);
    if (ImGui::SliderFloat(vcString::Get("settingsVisContoursRainbowRepeatRate"), &pProgramState->settings.postVisualization.contours.rainbowRepeat, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax, "%.3f", ImGuiSliderFlags_Logarithmic))
      pProgramState->settings.postVisualization.contours.rainbowRepeat = udClamp(pProgramState->settings.postVisualization.contours.rainbowRepeat, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax);
    if (ImGui::SliderFloat(vcString::Get("settingsVisContoursRainbowIntensity"), &pProgramState->settings.postVisualization.contours.rainbowIntensity, 0.f, 1.f, "%.3f", ImGuiSliderFlags_Logarithmic))
      pProgramState->settings.postVisualization.contours.rainbowIntensity = udClamp(pProgramState->settings.postVisualization.contours.rainbowIntensity, 0.f, 1.f);

    ImGui::Unindent();
  }
}

void vcSettings_ApplyMapChange(vcSettings *pSettings, int affectedMapLayer)
{
  for (int mapLayer = 0; mapLayer < vcMaxTileLayerCount; ++mapLayer)
  {
    if (mapLayer != affectedMapLayer && affectedMapLayer != -1)
      continue;

    bool found = false;
    for (size_t i = 0; i < udLengthOf(s_mapTiles); ++i)
    {
      if (udStrEqual(pSettings->maptiles.layers[mapLayer].mapType, s_mapTiles[i].pModeStr) && !udStrEqual(s_mapTiles[i].pModeStr, "custom"))
      {
        udStrcpy(pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddress, s_mapTiles[i].pServerAddr);
        udStrcpy(pSettings->maptiles.layers[mapLayer].activeServer.attribution, s_mapTiles[i].pCopyright);

        if (s_mapTiles[i].pTileAddressUUID != nullptr)
          udUUID_GenerateFromString(&pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddressUUID, s_mapTiles[i].pTileAddressUUID);
        else
          udUUID_GenerateFromString(&pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddressUUID, s_mapTiles[i].pServerAddr);

        found = true;
        break;
      }
    }

    if (!found)// `custom` or not supported
    {
      udStrcpy(pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddress, pSettings->maptiles.layers[mapLayer].customServer.tileServerAddress);
      udStrcpy(pSettings->maptiles.layers[mapLayer].activeServer.attribution, pSettings->maptiles.layers[mapLayer].customServer.attribution);
      udUUID_GenerateFromString(&pSettings->maptiles.layers[mapLayer].customServer.tileServerAddressUUID, pSettings->maptiles.layers[mapLayer].customServer.tileServerAddress);

      pSettings->maptiles.layers[mapLayer].activeServer.tileServerAddressUUID = pSettings->maptiles.layers[mapLayer].customServer.tileServerAddressUUID;
    }
  }
}

bool vcSettingsUI_VisualizationSettings(vcVisualizationSettings *pVisualizationSettings, bool isGlobal /*= true*/, udAttributeSet *pAttributes /*= nullptr*/)
{
  bool retVal = false;

  const char *visualizationModes[] = {vcString::Get("settingsVisModeDefault"), vcString::Get("settingsVisModeColour"), vcString::Get("settingsVisModeIntensity"), vcString::Get("settingsVisModeClassification"), vcString::Get("settingsVisModeDisplacementDistance"), vcString::Get("settingsVisModeDisplacementDirection"), vcString::Get("settingsVisModeGPSTime"), vcString::Get("settingsVisModeScanAngle"), vcString::Get("settingsVisModePointSourceID"), vcString::Get("settingsVisModeReturnNumber"), vcString::Get("settingsVisModeNumberOfReturns")};
  UDCOMPILEASSERT(udLengthOf(visualizationModes) == vcVM_Count, "Update combo box!");

  bool hasDisplacementDistance = false;
  bool hasDisplacementDirection = false;
  if (pAttributes)
  {
    for (uint32_t i = 0; i < pAttributes->count; ++i)
    {
      if (udStrEqual(pAttributes->pDescriptors[i].name, "udDisplacement"))
        hasDisplacementDistance = true;
      else if (udStrEqual(pAttributes->pDescriptors[i].name, "udDisplacementDirectionX"))
        hasDisplacementDirection = true;
    }
  }

  if (ImGui::BeginCombo(vcString::Get("settingsVisDisplayMode"), visualizationModes[pVisualizationSettings->mode]))
  {
    retVal = true;
    if (ImGui::Selectable(visualizationModes[vcVM_Default], pVisualizationSettings->mode == vcVM_Default))
      pVisualizationSettings->mode = vcVM_Default;

    if ((isGlobal || (pAttributes->content & udSAC_ARGB) != 0) && ImGui::Selectable(visualizationModes[vcVM_Colour], pVisualizationSettings->mode == vcVM_Colour))
      pVisualizationSettings->mode = vcVM_Colour;

    if ((isGlobal || (pAttributes->content & udSAC_Intensity) != 0) && ImGui::Selectable(visualizationModes[vcVM_Intensity], pVisualizationSettings->mode == vcVM_Intensity))
      pVisualizationSettings->mode = vcVM_Intensity;

    if ((isGlobal || (pAttributes->content & udSAC_Classification) != 0) && ImGui::Selectable(visualizationModes[vcVM_Classification], pVisualizationSettings->mode == vcVM_Classification))
      pVisualizationSettings->mode = vcVM_Classification;

    if ((isGlobal || hasDisplacementDistance) && ImGui::Selectable(visualizationModes[vcVM_DisplacementDistance], pVisualizationSettings->mode == vcVM_DisplacementDistance))
      pVisualizationSettings->mode = vcVM_DisplacementDistance;

    if ((isGlobal || hasDisplacementDirection) && ImGui::Selectable(visualizationModes[vcVM_DisplacementDirection], pVisualizationSettings->mode == vcVM_DisplacementDirection))
      pVisualizationSettings->mode = vcVM_DisplacementDirection;

    if ((isGlobal || (pAttributes->content & udSAC_GPSTime) != 0) && ImGui::Selectable(visualizationModes[vcVM_GPSTime], pVisualizationSettings->mode == vcVM_GPSTime))
      pVisualizationSettings->mode = vcVM_GPSTime;

    if ((isGlobal || (pAttributes->content & udSAC_ScanAngle) != 0) && ImGui::Selectable(visualizationModes[vcVM_ScanAngle], pVisualizationSettings->mode == vcVM_ScanAngle))
      pVisualizationSettings->mode = vcVM_ScanAngle;

    if ((isGlobal || (pAttributes->content & udSAC_PointSourceID) != 0) && ImGui::Selectable(visualizationModes[vcVM_PointSourceID], pVisualizationSettings->mode == vcVM_PointSourceID))
      pVisualizationSettings->mode = vcVM_PointSourceID;

    if ((isGlobal || (pAttributes->content & udSAC_ReturnNumber) != 0) && ImGui::Selectable(visualizationModes[vcVM_ReturnNumber], pVisualizationSettings->mode == vcVM_ReturnNumber))
      pVisualizationSettings->mode = vcVM_ReturnNumber;

    if ((isGlobal || (pAttributes->content & udSAC_NumberOfReturns) != 0) && ImGui::Selectable(visualizationModes[vcVM_NumberOfReturns], pVisualizationSettings->mode == vcVM_NumberOfReturns))
      pVisualizationSettings->mode = vcVM_NumberOfReturns;

    ImGui::EndCombo();
  }

  ImGui::Indent();

  switch (pVisualizationSettings->mode)
  {
  case vcVM_Intensity:
  {
    retVal |= ImGui::SliderInt(vcString::Get("settingsVisMinIntensity"), &pVisualizationSettings->minIntensity, (int)vcSL_IntensityMin, pVisualizationSettings->maxIntensity);
    retVal |= ImGui::SliderInt(vcString::Get("settingsVisMaxIntensity"), &pVisualizationSettings->maxIntensity, pVisualizationSettings->minIntensity, (int)vcSL_IntensityMax);
  }
  break;
  case vcVM_Classification:
    if (isGlobal)
      retVal |= ImGui::Checkbox(vcString::Get("settingsVisClassShowColourTable"), &pVisualizationSettings->useCustomClassificationColours);

    break;
  case vcVM_DisplacementDistance: // Fall Through
  case vcVM_DisplacementDirection:
  {
    ImGui::Indent();

    if (ImGui::InputFloat2(vcString::Get("settingsVisDisplacementRange"), &pVisualizationSettings->displacement.bounds.x))
    {
      retVal = true;

      pVisualizationSettings->displacement.bounds.x = udClamp(pVisualizationSettings->displacement.bounds.x, 0.f, MAX_DISPLACEMENT);
      pVisualizationSettings->displacement.bounds.y = udClamp(pVisualizationSettings->displacement.bounds.y, pVisualizationSettings->displacement.bounds.x, MAX_DISPLACEMENT);
    }

    vcIGSW_ColorPickerU32(vcString::Get("settingsVisDisplacementColourMax"), &pVisualizationSettings->displacement.max, ImGuiColorEditFlags_None);
    vcIGSW_ColorPickerU32(vcString::Get("settingsVisDisplacementColourMin"), &pVisualizationSettings->displacement.min, ImGuiColorEditFlags_None);
    vcIGSW_ColorPickerU32(vcString::Get("settingsVisDisplacementColourMid"), &pVisualizationSettings->displacement.mid, ImGuiColorEditFlags_None);
    vcIGSW_ColorPickerU32(vcString::Get("settingsVisDisplacementColourNoMatch"), &pVisualizationSettings->displacement.error, ImGuiColorEditFlags_None);

    ImGui::Unindent();
    break;
  }
  case vcVM_GPSTime:
  {
    vcTimeReference GPSTimeInIntToEnum[] = {vcTimeReference_GPS, vcTimeReference_GPSAdjusted};
    int GPSTimeInEnumToInt[] = {0, 0, 0, 1, 0, 0};

    static int GPSTimeFormatIn = GPSTimeInEnumToInt[pVisualizationSettings->GPSTime.inputFormat];
    const char *GPSTimeFormatInOptions[] = {"GPS", "GPS Adjusted"};

    retVal |= ImGui::Combo("Input GPS Time format", &GPSTimeFormatIn, GPSTimeFormatInOptions, (int)udLengthOf(GPSTimeFormatInOptions));

    bool minEdited = ImGui::InputDouble(vcString::Get("settingsVisGPSTimeMin"), &pVisualizationSettings->GPSTime.minTime, 0.0, 0.0, "%.1f");
    bool maxEdited = ImGui::InputDouble(vcString::Get("settingsVisGPSTimeMax"), &pVisualizationSettings->GPSTime.maxTime, 0.0, 0.0, "%.1f");

    pVisualizationSettings->GPSTime.inputFormat = GPSTimeInIntToEnum[GPSTimeFormatIn];

    retVal |= (minEdited || maxEdited);

    const double minGPSTime = -1e9;        //06/01/1980 GPS adjusted time
    const double maxGPSTime = 32188147218; //06/01/3000 GPS time

    pVisualizationSettings->GPSTime.minTime = udClamp(pVisualizationSettings->GPSTime.minTime, minGPSTime, maxGPSTime);
    pVisualizationSettings->GPSTime.maxTime = udClamp(pVisualizationSettings->GPSTime.maxTime, minGPSTime, maxGPSTime);

    if (minEdited && (pVisualizationSettings->GPSTime.minTime > pVisualizationSettings->GPSTime.maxTime))
      pVisualizationSettings->GPSTime.maxTime = pVisualizationSettings->GPSTime.minTime;

    if (maxEdited && pVisualizationSettings->GPSTime.maxTime < pVisualizationSettings->GPSTime.minTime)
      pVisualizationSettings->GPSTime.minTime = pVisualizationSettings->GPSTime.maxTime;

    break;
  }
  case vcVM_ScanAngle:
  {
    retVal |= ImGui::InputDouble(vcString::Get("settingsVisScanAngleMinAngle"), &pVisualizationSettings->scanAngle.minAngle, 0.0, 0.0, "%.1f");
    retVal |= ImGui::InputDouble(vcString::Get("settingsVisScanAngleMaxAngle"), &pVisualizationSettings->scanAngle.maxAngle, 0.0, 0.0, "%.1f");
    break;
  }
  case vcVM_PointSourceID:
  {
    vcIGSW_ColorPickerU32(vcString::Get("settingsVisPointSourceIDNoID"), &pVisualizationSettings->pointSourceID.defaultColour, ImGuiColorEditFlags_None);
    static int newPointSourceID = 0;
    if (ImGui::InputInt(vcString::Get("settingsVisPointSourceIDNewID"), &newPointSourceID))
    {
      retVal = true;

      if (newPointSourceID < 0) newPointSourceID = 0;
      if (newPointSourceID > 0xFFFF) newPointSourceID = 0xFFFF;
    }

    static uint32_t newPointSourceColour = 0;
    vcIGSW_ColorPickerU32(vcString::Get("settingsVisPointSourceIDColour"), &newPointSourceColour, ImGuiColorEditFlags_None);
    if (ImGui::Button(vcString::Get("settingsVisPointSourceIDAdd")))
    {
      retVal = true;

      uint16_t id16 = uint16_t(newPointSourceID);

      size_t index = size_t(-1);
      for (size_t i = 0; i < pVisualizationSettings->pointSourceID.colourMap.length; ++i)
      {
        if (pVisualizationSettings->pointSourceID.colourMap[i].id == id16)
        {
          index = i;
          break;
        }
      }

      if (index == size_t(-1))
        pVisualizationSettings->pointSourceID.colourMap.PushBack({id16, newPointSourceColour});
      else
        pVisualizationSettings->pointSourceID.colourMap[index].colour = newPointSourceColour;

      newPointSourceColour = 0;
      newPointSourceID++;
    }

    ImGui::SameLine();
    if (ImGui::Button(vcString::Get("settingsVisPointSourceIDRemoveAll")))
    {
      retVal = true;

      newPointSourceID = 0;
      pVisualizationSettings->pointSourceID.colourMap.Clear();
    }

    size_t removeIndex = size_t(-1);
    for (size_t i = 0; i < pVisualizationSettings->pointSourceID.colourMap.length; ++i)
    {
      if (ImGui::Button(udTempStr(" X ##PointSourceID%u", pVisualizationSettings->pointSourceID.colourMap[i].id)))
        removeIndex = i;
      ImGui::SameLine(0);
      vcIGSW_ColorPickerU32(udTempStr("%u##PointSourceID", pVisualizationSettings->pointSourceID.colourMap[i].id), &pVisualizationSettings->pointSourceID.colourMap[i].colour, ImGuiColorEditFlags_None);
    }

    if (removeIndex != size_t(-1))
      pVisualizationSettings->pointSourceID.colourMap.RemoveAt(removeIndex);
    break;
  }
  case vcVM_ReturnNumber:
  {
    for (uint32_t i = 0; i < pVisualizationSettings->s_maxReturnNumbers; ++i)
      retVal |= vcIGSW_ColorPickerU32(udTempStr("%i", i + 1), &pVisualizationSettings->returnNumberColours[i], ImGuiColorEditFlags_None);
    break;
  }
  case vcVM_NumberOfReturns:
  {
    for (uint32_t i = 0; i < pVisualizationSettings->s_maxReturnNumbers; ++i)
      vcIGSW_ColorPickerU32(udTempStr("%i", i + 1), &pVisualizationSettings->numberOfReturnsColours[i], ImGuiColorEditFlags_None);
    break;
  }
  default:
    break;
  }

  ImGui::Unindent();

  return retVal;
}
