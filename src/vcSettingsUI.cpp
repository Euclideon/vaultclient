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

#include "vdkConfig.h"

#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "stb_image.h"

#include "udStringUtil.h"
#include "udFile.h"

#define MAX_DISPLACEMENT 10000.f

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
    ImGui::Text("Euclideon Vault Client %s", VCVERSION_PRODUCT_STRING);

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
        change |= ImGui::RadioButton(udTempStr("%s##ConvertSettings", vcString::Get("settingsConvert")), &pProgramState->activeSetting, vcSR_ConvertDefaults);
        change |= ImGui::RadioButton(udTempStr("%s##ScreenshotSettings", vcString::Get("settingsScreenshot")), &pProgramState->activeSetting, vcSR_Screenshot);
        change |= ImGui::RadioButton(udTempStr("%s##ConnectionSettings", vcString::Get("settingsConnection")), &pProgramState->activeSetting, vcSR_Connection);

        ImGui::Separator();

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
          if(ImGui::SliderFloat(vcString::Get("settingsAppearanceImageRescale"), &pProgramState->settings.presentation.imageRescaleDistance, vcSL_ImageRescaleMin, vcSL_ImageRescaleMax, "%.3fm", 3.f))
            pProgramState->settings.presentation.imageRescaleDistance = udClamp(pProgramState->settings.presentation.imageRescaleDistance, vcSL_ImageRescaleMin, vcSL_ImageRescaleMax);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowDiagnostics"), &pProgramState->settings.presentation.showDiagnosticInfo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowEuclideonLogo"), &pProgramState->settings.presentation.showEuclideonLogo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceAdvancedGIS"), &pProgramState->settings.presentation.showAdvancedGIS);
          ImGui::Checkbox(vcString::Get("settingsAppearanceLimitFPS"), &pProgramState->settings.presentation.limitFPSInBackground);
          ImGui::Checkbox(vcString::Get("settingsAppearanceLoginRenderLicense"), &pProgramState->settings.presentation.loginRenderLicense);

#if VC_HASNATIVEFILEPICKER
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowNativeDialogs"), &pProgramState->settings.window.useNativeUI);
#endif

          ImGui::Checkbox(vcString::Get("settingsAppearanceShowSkybox"), &pProgramState->settings.presentation.showSkybox);

          if (!pProgramState->settings.presentation.showSkybox)
          {
            ImGui::Indent();
            ImGui::ColorEdit3(vcString::Get("settingsAppearanceSkyboxColour"), &pProgramState->settings.presentation.skyboxColour.x);
            ImGui::Unindent();
          }

          // limit the value between 0-5.
          if (ImGui::SliderFloat(vcString::Get("settingsAppearanceSaturation"), &pProgramState->settings.presentation.saturation, 0.0f, 5.0f))
            pProgramState->settings.presentation.saturation = udClamp(pProgramState->settings.presentation.saturation, 0.0f, 5.0f);

          const char *presentationOptions[] = { vcString::Get("settingsAppearanceHide"), vcString::Get("settingsAppearanceShow"), vcString::Get("settingsAppearanceResponsive") };
          if (ImGui::Combo(vcString::Get("settingsAppearancePresentationUI"), (int*)&pProgramState->settings.responsiveUI, presentationOptions, (int)udLengthOf(presentationOptions)))
            pProgramState->showUI = false;

          const char *anchorOptions[] = { vcString::Get("settingsAppearanceNone"), vcString::Get("settingsAppearanceOrbit"), vcString::Get("settingsAppearanceCompass") };
          ImGui::Combo(vcString::Get("settingsAppearanceMouseAnchor"), (int*)&pProgramState->settings.presentation.mouseAnchor, anchorOptions, (int)udLengthOf(anchorOptions));

          const char *voxelOptions[] = { vcString::Get("settingsAppearanceRectangles"), vcString::Get("settingsAppearanceCubes"), vcString::Get("settingsAppearancePoints") };
          ImGui::Combo(vcString::Get("settingsAppearanceVoxelShape"), &pProgramState->settings.presentation.pointMode, voxelOptions, (int)udLengthOf(voxelOptions));

          const char *layoutOptions[] = { vcString::Get("settingsAppearanceWindowLayoutScSx"), vcString::Get("settingsAppearanceWindowLayoutSxSc") };
          if (ImGui::Combo(vcString::Get("settingsAppearanceWindowLayout"), (int*)&pProgramState->settings.presentation.layout, layoutOptions, (int)udLengthOf(layoutOptions)))
            pProgramState->settings.presentation.columnSizeCorrect = false;
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
              for (size_t n = 0; n < udLengthOf(blendModes); ++n)
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
              pProgramState->settings.maptiles.mapHeight = (float)pProgramState->camera.position.z;
          }
        }

        if (pProgramState->activeSetting == vcSR_Visualisations)
        {
          vcSettingsUI_ShowHeader(pProgramState, vcString::Get("settingsVis"), vcSC_Visualization);

          vcSettingsUI_VisualizationSettings(pProgramState, &pProgramState->settings.visualization);

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

          if (ImGui::Combo(vcString::Get("settingsViewportCameraLens"), &pProgramState->settings.camera.lensIndex, lensNameArray, (int)udLengthOf(lensNameArray)))
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
            if (ImGui::SliderFloat(vcString::Get("settingsViewportFOV"), &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "%.0f°"))
              pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
          }

          // Selected Object Highlighting
          ImGui::Checkbox(vcString::Get("settingsVisObjectHighlight"), &pProgramState->settings.objectHighlighting.enable);
          if (pProgramState->settings.objectHighlighting.enable)
          {
            ImGui::ColorEdit4(vcString::Get("settingsVisHighlightColour"), &pProgramState->settings.objectHighlighting.colour.x);
            ImGui::SliderFloat(vcString::Get("settingsVisHighlightThickness"), &pProgramState->settings.objectHighlighting.thickness, 1.0f, 3.0f);
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
            if (ImGui::SliderFloat(vcString::Get("settingsVisContoursRainbowRepeatRate"), &pProgramState->settings.postVisualization.contours.rainbowRepeat, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax, "%.3f", 2))
              pProgramState->settings.postVisualization.contours.rainbowRepeat = udClamp(pProgramState->settings.postVisualization.contours.rainbowRepeat, vcSL_ContourDistanceMin, vcSL_ContourDistanceMax);
            if (ImGui::SliderFloat(vcString::Get("settingsVisContoursRainbowIntensity"), &pProgramState->settings.postVisualization.contours.rainbowIntensity, 0.f, 1.f, "%.3f", 2))
              pProgramState->settings.postVisualization.contours.rainbowIntensity = udClamp(pProgramState->settings.postVisualization.contours.rainbowIntensity, 0.f, 1.f);
          }
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

          vcIGSW_FilePicker(pProgramState, vcString::Get("convertChangeDefaultWatermark"), pProgramState->settings.convertdefaults.watermark.filename, SupportedFileTypes_Images, vcFDT_OpenFile, [pProgramState] {
            //reload stuff
            udFilename filename = pProgramState->settings.convertdefaults.watermark.filename;
            uint8_t *pData = nullptr;
            int64_t dataLength = 0;
            if (udFile_Load(pProgramState->settings.convertdefaults.watermark.filename, (void**)&pData, &dataLength) == udR_Success)
            {
              // TODO: Resize watermark to the same dimensions as vdkConvert does - maybe requires additional VDK functionality?
              filename.SetFolder(pProgramState->settings.pSaveFilePath);
              udFile_Save(filename, pData, (size_t)dataLength);
              udFree(pData);
            }
            udStrcpy(pProgramState->settings.convertdefaults.watermark.filename, filename.GetFilenameWithExt());
            pProgramState->settings.convertdefaults.watermark.isDirty = true;
          });

          ImGui::Indent();
          {
            if (pProgramState->settings.convertdefaults.watermark.isDirty)
            {
              pProgramState->settings.convertdefaults.watermark.isDirty = false;
              vcTexture_Destroy(&pProgramState->settings.convertdefaults.watermark.pTexture);
              uint8_t *pData = nullptr;
              int64_t dataSize = 0;
              char buffer[vcMaxPathLength];
              udStrcpy(buffer, pProgramState->settings.pSaveFilePath);
              udStrcat(buffer, pProgramState->settings.convertdefaults.watermark.filename);
              if (udFile_Load(buffer, (void**)&pData, &dataSize) == udR_Success)
              {
                int comp;
                stbi_uc *pImg = stbi_load_from_memory(pData, (int)dataSize, &pProgramState->settings.convertdefaults.watermark.width, &pProgramState->settings.convertdefaults.watermark.height, &comp, 4);

                vcTexture_Create(&pProgramState->settings.convertdefaults.watermark.pTexture, pProgramState->settings.convertdefaults.watermark.width, pProgramState->settings.convertdefaults.watermark.height, pImg);

                stbi_image_free(pImg);
              }

              udFree(pData);
            }

            if (pProgramState->settings.convertdefaults.watermark.pTexture != nullptr)
            {
              //Since we're allowing images of any dimensions, we need to make sure it fits in the UI.
              udInt2 dimension = {pProgramState->settings.convertdefaults.watermark.width, pProgramState->settings.convertdefaults.watermark.height};
              udInt2 maxDimension = {512, 256};
              int minDimension = 2;

              for (int i = 0; i < udInt2::ElementCount; i++)
              {
                if (dimension[i] > maxDimension[i])
                {
                  float factor = float(dimension[i]) / maxDimension[i];
                  dimension /= factor;
                  int j = (i + 1) % udInt2::ElementCount;

                  //Is the other dimension too thin now?
                  if (dimension[j] < minDimension)
                    dimension[j] = minDimension;
                }
              }

              ImGui::Image(pProgramState->settings.convertdefaults.watermark.pTexture, ImVec2(float(dimension[0]), float(dimension[1])));

              if (ImGui::Button(vcString::Get("convertRemoveWatermark")))
              {
                memset(pProgramState->settings.convertdefaults.watermark.filename, 0, sizeof(pProgramState->settings.convertdefaults.watermark.filename));
                pProgramState->settings.convertdefaults.watermark.width = 0;
                pProgramState->settings.convertdefaults.watermark.height = 0;
                vcTexture_Destroy(&pProgramState->settings.convertdefaults.watermark.pTexture);
              }
            }
          }
          ImGui::Unindent();

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
              if (pProgramState->settings.loginInfo.testStatus == vE_ProxyAuthRequired)
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
            if (pProgramState->settings.loginInfo.testStatus == vE_Success)
            {
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Success);
              ImGui::TextUnformatted(vcString::Get("loginProxyTestSuccess"));
            }
            else
            {
              char buffer[256];
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Failed);

              if (pProgramState->settings.loginInfo.testStatus == vE_SecurityFailure)
                ImGui::TextUnformatted(vcString::Get("loginErrorSecurity"));
              else if (pProgramState->settings.loginInfo.testStatus == vE_ConnectionFailure)
                ImGui::TextUnformatted(vcString::Get("loginProxyTestCouldNotConnect"));
              else if (pProgramState->settings.loginInfo.testStatus == vE_ProxyError)
                ImGui::TextUnformatted(vcString::Get("loginErrorProxy"));
              else if (pProgramState->settings.loginInfo.testStatus == vE_ProxyAuthRequired)
                ImGui::TextUnformatted(vcString::Get("loginErrorProxyAuthFailed"));
              else
                ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("loginProxyTestFailed"), udTempStr("%d", pProgramState->settings.loginInfo.testStatus)));
            }
          }

          if (!pProgramState->settings.loginInfo.testing)
          {
            ImGui::Separator();

            if (vcIGSW_InputText(vcString::Get("loginProxyAddress"), pProgramState->settings.loginInfo.proxy, vcMaxPathLength))
            {
              vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);
              pProgramState->settings.loginInfo.tested = false;
            }

            ImGui::SameLine();
            if (ImGui::Button(vcString::Get("loginProxyAutodetect")))
            {
              if (vcProxyHelper_AutoDetectProxy(pProgramState) == vE_Success)
              {
                udStrcpy(pProgramState->settings.loginInfo.proxy, pProgramState->settings.loginInfo.autoDetectProxyURL);
                vdkConfig_ForceProxy(pProgramState->settings.loginInfo.proxy);
              }
            }

            if (vcIGSW_InputText(vcString::Get("loginUserAgent"), pProgramState->settings.loginInfo.userAgent, vcMaxPathLength))
            {
              vdkConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);
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
                  vdkConfig_SetUserAgent(pProgramState->settings.loginInfo.userAgent);
                  pProgramState->settings.loginInfo.tested = false;
                }
              }
              ImGui::EndCombo();
            }

            if (ImGui::Checkbox(vcString::Get("loginIgnoreCert"), &pProgramState->settings.loginInfo.ignoreCertificateVerification))
            {
              vdkConfig_IgnoreCertificateVerification(pProgramState->settings.loginInfo.ignoreCertificateVerification);
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
                vdkConfig_SetProxyAuth(pProgramState->settings.loginInfo.proxyUsername, pProgramState->settings.loginInfo.proxyPassword);
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
            ImGui::TextUnformatted(pProgramState->pReleaseNotes);
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

          const char *pSupportStr = vcStringFormat(vcString::Get("loginSupportDirectEmail"), "support@euclideon.com");
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

void vcSettingsUI_VisualizationSettings(vcState *pProgramState, vcVisualizationSettings *pVisualizationSettings)
{
  const char *visualizationModes[] = { vcString::Get("settingsVisModeDefault"), vcString::Get("settingsVisModeColour"), vcString::Get("settingsVisModeIntensity"), vcString::Get("settingsVisModeClassification"), vcString::Get("settingsVisModeDisplacement") };
  ImGui::Combo(vcString::Get("settingsVisDisplayMode"), (int *)&pVisualizationSettings->mode, visualizationModes, (int)udLengthOf(visualizationModes));

  if (pVisualizationSettings->mode == vcVM_Intensity)
  {
    ImGui::SliderInt(vcString::Get("settingsVisMinIntensity"), &pVisualizationSettings->minIntensity, (int)vcSL_IntensityMin, pVisualizationSettings->maxIntensity);
    ImGui::SliderInt(vcString::Get("settingsVisMaxIntensity"), &pVisualizationSettings->maxIntensity, pVisualizationSettings->minIntensity, (int)vcSL_IntensityMax);
  }

  if (pVisualizationSettings->mode == vcVM_Classification)
  {
    ImGui::Checkbox(vcString::Get("settingsVisClassShowColourTable"), &pVisualizationSettings->useCustomClassificationColours);

    if (pVisualizationSettings->useCustomClassificationColours)
    {
      ImGui::SameLine();
      if (ImGui::Button(udTempStr("%s##RestoreClassificationColors", vcString::Get("settingsRestoreDefaults"))))
        memcpy(pVisualizationSettings->customClassificationColors, GeoverseClassificationColours, sizeof(pVisualizationSettings->customClassificationColors));

      static const char *s_customClassifications[] =
      {
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

      if (ImGui::Button(vcString::Get("settingsVisClassShowAll")))
      {
        for (size_t i = 0; i < udLengthOf(pVisualizationSettings->customClassificationToggles); i++)
          pVisualizationSettings->customClassificationToggles[i] = true;
      }

      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("settingsVisClassDisableAll")))
      {
        for (size_t i = 0; i < udLengthOf(pVisualizationSettings->customClassificationToggles); i++)
          pVisualizationSettings->customClassificationToggles[i] = false;
      }

      for (size_t i = 0; i < udLengthOf(s_customClassifications); i++)
      {
        ImGui::PushID((int)i);
        ImGui::Checkbox("", &pVisualizationSettings->customClassificationToggles[i]);
        ImGui::PopID();
        ImGui::SameLine();
        vcIGSW_ColorPickerU32(vcString::Get(s_customClassifications[i]), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
      }

      if (ImGui::TreeNode(vcString::Get("settingsVisClassReservedColours")))
      {
        for (int i = 19; i < 64; ++i)
        {
          ImGui::PushID(i);
          ImGui::Checkbox("", &pVisualizationSettings->customClassificationToggles[i]);
          ImGui::PopID();
          ImGui::SameLine();
          vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, vcString::Get("settingsVisClassReservedLabels")), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
        }
        ImGui::TreePop();
      }

      if (ImGui::TreeNode(vcString::Get("settingsVisClassUserDefinable")))
      {
        for (int i = 64; i <= 255; ++i)
        {
          ImGui::PushID(i);
          ImGui::Checkbox("", &pVisualizationSettings->customClassificationToggles[i]);
          ImGui::PopID();
          ImGui::SameLine();

          char buttonID[12], inputID[3];
          if (pVisualizationSettings->customClassificationColorLabels[i] == nullptr)
            vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, vcString::Get("settingsVisClassUserDefined")), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
          else
            vcIGSW_ColorPickerU32(udTempStr("%d. %s", i, pVisualizationSettings->customClassificationColorLabels[i]), &pVisualizationSettings->customClassificationColors[i], ImGuiColorEditFlags_NoAlpha);
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
            vcIGSW_InputText(inputID, pProgramState->renameText, 30, ImGuiInputTextFlags_AutoSelectAll);
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

  if (pVisualizationSettings->mode == vcVM_Displacement)
  {
    if (ImGui::InputFloat2(vcString::Get("settingsVisDisplacementRange"), &pVisualizationSettings->displacement.x))
    {
      pVisualizationSettings->displacement.x = udClamp(pVisualizationSettings->displacement.x, 0.f, MAX_DISPLACEMENT);
      pVisualizationSettings->displacement.y = udClamp(pVisualizationSettings->displacement.y, pVisualizationSettings->displacement.x, MAX_DISPLACEMENT);
    }
  }
}
