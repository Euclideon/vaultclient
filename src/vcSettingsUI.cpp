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

/*
Add errors by placing an entry in the language file with the error message, this entry name goes below in settingsErrors[].
Add a colour in vec4 RGBA format below, and add to the error enum vcSE_ vcSettingsErrors then use vcSettingsUI_Set/Unset/Check Error functions
*/
static const char *settingsErrors[] =
{
  "bindingsErrorBound",
  "bindingsSelectKey"
};

static const ImVec4 settingsErrorColours[] =
{
  ImVec4(1, 0, 0, 1),
  ImVec4(1, 1, 1, 1)
};

static const vcSettingCategory categoryMapping[] =
{
  vcSC_Appearance, //vcSR_Appearance
  vcSC_InputControls, //vcSR_Inputs
  vcSC_Viewport, //vcSR_Viewports
  vcSC_MapsElevation, //vcSR_Maps
  vcSC_Visualization, //vcSR_Visualisations
  vcSC_Bindings, //vcSR_KeyBindings
  vcSC_Convert, //vcSR_ConvertDefaults
  vcSC_Count, //vcSR_Connection
  vcSC_Count, //vcSR_ReleaseNotes
  vcSC_Count, //vcSR_Update
  vcSC_Count //vcSR_About
};
// Entries in categoryMapping above must contain the vcSettingCategory that corresponds to a vcSettingsUIRegion
// if any, in the order they appear in the vcSR_ enum. If no category applies or you don't wish to offer a reset
// defaults option then enter vcSC_Count for the corresponding vcSettingsUIRegion.
UDCOMPILEASSERT(vcSC_Count == 10, "Update the above mapping (in vcSettingsUI.cpp) if necessary, as per the comments");
UDCOMPILEASSERT(vcSR_Count == 11, "Update the above mapping (in vcSettingsUI.cpp) if necessary, as per the comments");

void vcSettingsUI_SetError(vcState *pProgramState, vcSettingsErrors error)
{
  pProgramState->settingsErrors = pProgramState->settingsErrors | error;
}

void vcSettingsUI_UnsetError(vcState *pProgramState, vcSettingsErrors error)
{
  pProgramState->settingsErrors = pProgramState->settingsErrors & ~error;
}

bool vcSettingsUI_CheckError(vcState *pProgramState, vcSettingsErrors error)
{
  return ((pProgramState->settingsErrors & error) == error);
}

ImVec4 vcSettingsUI_GetErrorColour(vcSettingsErrors error)
{
  return settingsErrorColours[int(udLog2((float)error))];
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

    ImGui::NextColumn();
    ImGui::Separator();

    if (pProgramState->settingsErrors != 0)
    {
      for (int i = 0; i < vcSE_Count; ++i)
      {
        if (pProgramState->settingsErrors & (1 << i))
          ImGui::TextColored(settingsErrorColours[i], "%s", vcString::Get(settingsErrors[i]));
      }
    }
    else
    {
      ImGui::Text("%s", "");
    }

    ImGui::NextColumn();

    if (categoryMapping[pProgramState->activeSetting] != vcSC_Count && (ImGui::Button(udTempStr("%s##CategoryRestore", vcString::Get("settingsRestoreDefaults")), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Load)))
    {
      vcSettings_Load(&pProgramState->settings, true, categoryMapping[pProgramState->activeSetting]);
      if (categoryMapping[pProgramState->activeSetting] == vcSC_MapsElevation)
      {
        if (pProgramState->tileModal.pServerIcon != nullptr)
          vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);
        vcRender_ClearTiles(pProgramState->pRenderContext); // refresh map tiles since they just got updated
      }
      vcHotkey::ClearState();
    }

    ImGui::EndColumns();
    ImGui::Separator();

    if (ImGui::BeginChild("__settingsPane"))
    {
      ImGui::Columns(2);

      ImGui::SetColumnWidth(0, 200.f);

      if (ImGui::BeginChild("__settingsPaneCategories"))
      {
        bool change = false;
        change |= ImGui::RadioButton(udTempStr("%s##AppearanceSettings", vcString::Get("settingsAppearance")), &pProgramState->activeSetting, vcSR_Appearance);
        change |= ImGui::RadioButton(udTempStr("%s##InputSettings", vcString::Get("settingsControls")), &pProgramState->activeSetting, vcSR_Inputs);
        change |= ImGui::RadioButton(udTempStr("%s##ViewportSettings", vcString::Get("settingsViewport")), &pProgramState->activeSetting, vcSR_Viewports);
        change |= ImGui::RadioButton(udTempStr("%s##MapSettings", vcString::Get("settingsMaps")), &pProgramState->activeSetting, vcSR_Maps);
        change |= ImGui::RadioButton(udTempStr("%s##VisualisationSettings", vcString::Get("settingsVis")), &pProgramState->activeSetting, vcSR_Visualisations);
        change |= ImGui::RadioButton(udTempStr("%s##KeyBindings", vcString::Get("bindingsTitle")), &pProgramState->activeSetting, vcSR_KeyBindings);
        change |= ImGui::RadioButton(udTempStr("%s##ConvertSettings", vcString::Get("settingsConvert")), &pProgramState->activeSetting, vcSR_ConvertDefaults);
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
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowDiagnostics"), &pProgramState->settings.presentation.showDiagnosticInfo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowEuclideonLogo"), &pProgramState->settings.presentation.showEuclideonLogo);
          ImGui::Checkbox(vcString::Get("settingsAppearanceAdvancedGIS"), &pProgramState->settings.presentation.showAdvancedGIS);
          ImGui::Checkbox(vcString::Get("settingsAppearanceLimitFPS"), &pProgramState->settings.presentation.limitFPSInBackground);
          ImGui::Checkbox(vcString::Get("settingsAppearanceShowCompass"), &pProgramState->settings.presentation.showCompass);
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
        }

        if (pProgramState->activeSetting == vcSR_Inputs)
        {
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

        if (pProgramState->activeSetting == vcSR_Viewports)
        {
          if (ImGui::SliderFloat(vcString::Get("settingsViewportViewDistance"), &pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
            pProgramState->settings.camera.nearPlane = pProgramState->settings.camera.farPlane * vcSL_CameraFarToNearPlaneRatio;

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
        }

        if (pProgramState->activeSetting == vcSR_Maps)
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
          const char *visualizationModes[] = { vcString::Get("settingsVisModeColour"), vcString::Get("settingsVisModeIntensity"), vcString::Get("settingsVisModeClassification"), vcString::Get("settingsVisModeDisplacement") };
          ImGui::Combo(vcString::Get("settingsVisDisplayMode"), (int*)&pProgramState->settings.visualization.mode, visualizationModes, (int)udLengthOf(visualizationModes));

          if (pProgramState->settings.visualization.mode == vcVM_Intensity)
          {
            ImGui::SliderInt(vcString::Get("settingsVisMinIntensity"), &pProgramState->settings.visualization.minIntensity, (int)vcSL_IntensityMin, pProgramState->settings.visualization.maxIntensity);
            ImGui::SliderInt(vcString::Get("settingsVisMaxIntensity"), &pProgramState->settings.visualization.maxIntensity, pProgramState->settings.visualization.minIntensity, (int)vcSL_IntensityMax);
          }

          if (pProgramState->settings.visualization.mode == vcVM_Classification)
          {
            ImGui::Checkbox(vcString::Get("settingsVisClassShowColourTable"), &pProgramState->settings.visualization.useCustomClassificationColours);

            if (pProgramState->settings.visualization.useCustomClassificationColours)
            {
              ImGui::SameLine();
              if (ImGui::Button(udTempStr("%s##RestoreClassificationColors", vcString::Get("settingsRestoreDefaults"))))
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

          if (pProgramState->settings.visualization.mode == vcVM_Displacement)
          {
            if (ImGui::InputFloat2(vcString::Get("settingsVisDisplacementRange"), &pProgramState->settings.visualization.displacement.x))
            {
              pProgramState->settings.visualization.displacement.x = udClamp(pProgramState->settings.visualization.displacement.x, 0.f, MAX_DISPLACEMENT);
              pProgramState->settings.visualization.displacement.y = udClamp(pProgramState->settings.visualization.displacement.y, pProgramState->settings.visualization.displacement.x, MAX_DISPLACEMENT);
            }
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
          vcHotkey::DisplayBindings(pProgramState);
        else
          vcHotkey::ClearState();

        if (pProgramState->activeSetting == vcSR_ConvertDefaults)
        {
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
              ImGui::Image(pProgramState->settings.convertdefaults.watermark.pTexture, ImVec2(256, 256));

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
          ImGui::InputText(vcString::Get("convertAuthor"), pProgramState->settings.convertdefaults.author, udLengthOf(pProgramState->settings.convertdefaults.author));
          ImGui::InputText(vcString::Get("convertComment"), pProgramState->settings.convertdefaults.comment, udLengthOf(pProgramState->settings.convertdefaults.comment));
          ImGui::InputText(vcString::Get("convertCopyright"), pProgramState->settings.convertdefaults.copyright, udLengthOf(pProgramState->settings.convertdefaults.copyright));
          ImGui::InputText(vcString::Get("convertLicense"), pProgramState->settings.convertdefaults.license, udLengthOf(pProgramState->settings.convertdefaults.license));
        }

        if (pProgramState->activeSetting == vcSR_Connection)
        {
          // Make sure its actually off before doing the auto-proxy check
          if (ImGui::Checkbox(vcString::Get("loginProxyAutodetect"), &pProgramState->settings.loginInfo.autoDetectProxy) && pProgramState->settings.loginInfo.autoDetectProxy)
            vcProxyHelper_AutoDetectProxy(pProgramState);

          if (vcIGSW_InputText(vcString::Get("loginProxyAddress"), pProgramState->settings.loginInfo.proxy, vcMaxPathLength, pProgramState->settings.loginInfo.autoDetectProxy ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None) && !pProgramState->settings.loginInfo.autoDetectProxy)
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

          if (vcIGSW_InputText(vcString::Get("loginUserAgent"), pProgramState->settings.loginInfo.userAgent, vcMaxPathLength))
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
