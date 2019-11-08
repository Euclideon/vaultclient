#include "vcSettingsUI.h"

#include "vcState.h"
#include "vcSettings.h"
#include "vcRender.h"
#include "vcModals.h"
#include "vcClassificationColours.h"

#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "stb_image.h"

#include "udStringUtil.h"
#include "udFile.h"

void vcSettingsUI_Show(vcState *pProgramState)
{
  bool openedHeader = false;

  if (ImGui::Begin(udTempStr("%s###settingsDock", vcString::Get("settingsTitle")), &pProgramState->settings.window.windowsOpen[vcDocks_Settings]))
  {
    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##AppearanceSettings", vcString::Get("settingsAppearance")));

    if (ImGui::BeginPopupContextItem("AppearanceContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##AppearanceRestore", vcString::Get("settingsAppearanceRestoreDefaults"))))
        vcSettings_Load(&pProgramState->settings, true, vcSC_Appearance);

      ImGui::EndPopup();
    }

    if (openedHeader)
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
      ImGui::Checkbox(vcString::Get("settingsAppearanceShowEuclideonLogo"), &pProgramState->settings.presentation.showEuclideonLogo);
      ImGui::Checkbox(vcString::Get("settingsAppearanceAdvancedGIS"), &pProgramState->settings.presentation.showAdvancedGIS);
      ImGui::Checkbox(vcString::Get("settingsAppearanceLimitFPS"), &pProgramState->settings.presentation.limitFPSInBackground);
      ImGui::Checkbox(vcString::Get("settingsAppearanceShowCompass"), &pProgramState->settings.presentation.showCompass);
      ImGui::Checkbox(vcString::Get("settingsAppearanceLoginRenderLicense"), &pProgramState->settings.presentation.loginRenderLicense);

      ImGui::Checkbox(vcString::Get("settingsAppearanceShowSkybox"), &pProgramState->settings.presentation.showSkybox);

      if (!pProgramState->settings.presentation.showSkybox)
      {
        ImGui::Indent();
        ImGui::ColorEdit3(vcString::Get("settingsAppearanceSkyboxColour"), &pProgramState->settings.presentation.skyboxColour.x);
        ImGui::Unindent();
      }

      const char *presentationOptions[] = { vcString::Get("settingsAppearanceHide"), vcString::Get("settingsAppearanceShow"), vcString::Get("settingsAppearanceResponsive") };
      if (ImGui::Combo(vcString::Get("settingsAppearancePresentationUI"), (int*)&pProgramState->settings.responsiveUI, presentationOptions, (int)udLengthOf(presentationOptions)))
        pProgramState->showUI = false;

      const char *anchorOptions[] = { vcString::Get("settingsAppearanceNone"), vcString::Get("settingsAppearanceOrbit"), vcString::Get("settingsAppearanceCompass") };
      ImGui::Combo(vcString::Get("settingsAppearanceMouseAnchor"), (int*)&pProgramState->settings.presentation.mouseAnchor, anchorOptions, (int)udLengthOf(anchorOptions));

      const char *voxelOptions[] = { vcString::Get("settingsAppearanceRectangles"), vcString::Get("settingsAppearanceCubes"), vcString::Get("settingsAppearancePoints") };
      ImGui::Combo(vcString::Get("settingsAppearanceVoxelShape"), &pProgramState->settings.presentation.pointMode, voxelOptions, (int)udLengthOf(voxelOptions));
    }

    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##InputSettings", vcString::Get("settingsControls")));
    if (ImGui::BeginPopupContextItem("InputContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##ControlsRestore", vcString::Get("settingsControlsRestoreDefaults"))))
        vcSettings_Load(&pProgramState->settings, true, vcSC_InputControls);

      ImGui::EndPopup();
    }

    if (openedHeader)
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

    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##ViewportSettings", vcString::Get("settingsViewport")));
    if (ImGui::BeginPopupContextItem("ViewportContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##ViewportRestore", vcString::Get("settingsViewportRestoreDefaults"))))
        vcSettings_Load(&pProgramState->settings, true, vcSC_Viewport);

      ImGui::EndPopup();
    }

    if (openedHeader)
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

    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##MapSettings", vcString::Get("settingsMaps")));
    if (ImGui::BeginPopupContextItem("MapsContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##MapRestore", vcString::Get("settingsMapsRestoreDefaults"))))
      {
        vcSettings_Load(&pProgramState->settings, true, vcSC_MapsElevation);
        vcRender_ClearTiles(pProgramState->pRenderContext); // refresh map tiles since they just got updated
      }

      ImGui::EndPopup();
    }

    if (openedHeader)
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

    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##VisualisationSettings", vcString::Get("settingsVis")));
    if (ImGui::BeginPopupContextItem("VisualizationContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##VisRestore", vcString::Get("settingsVisRestoreDefaults"))))
      {
        vcSettings_Load(&pProgramState->settings, true, vcSC_Visualization);
      }
      ImGui::EndPopup();
    }

    if (openedHeader)
    {
      ImGui::ColorEdit4(vcString::Get("settingsVisHighlightColour"), &pProgramState->settings.objectHighlighting.colour.x);
      ImGui::SliderFloat(vcString::Get("settingsVisHighlightThickness"), &pProgramState->settings.objectHighlighting.thickness, 1.0f, 3.0f);

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
          if (ImGui::Button(udTempStr("%s##RestoreClassificationColors", vcString::Get("settingsVisClassRestoreDefaults"))))
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
        ImGui::InputFloat2(vcString::Get("settingsVisDisplacementRange"), &pProgramState->settings.visualization.displacement.x);

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

    openedHeader = ImGui::CollapsingHeader(udTempStr("%s##ConvertSettings", vcString::Get("settingsConvert")));
    if (ImGui::BeginPopupContextItem("ConvertContext"))
    {
      if (ImGui::Selectable(udTempStr("%s##ConvertRestore", vcString::Get("settingsConvertRestoreDefaults"))))
        vcSettings_Load(&pProgramState->settings, true, vcSC_Convert);

      ImGui::EndPopup();
    }

    if (openedHeader)
    {
      // Temp directory
      ImGui::InputText(vcString::Get("convertTempDirectory"), pProgramState->settings.convertdefaults.tempDirectory, udLengthOf(pProgramState->settings.convertdefaults.tempDirectory));

      if (ImGui::Button(vcString::Get("convertChangeDefaultWatermark")))
        vcModals_OpenModal(pProgramState, vcMT_ChangeDefaultWatermark);

      if (pProgramState->settings.convertdefaults.watermark.pTexture != nullptr)
      {
        ImGui::SameLine();
        if (ImGui::Button(vcString::Get("convertRemoveWatermark")))
        {
          memset(pProgramState->settings.convertdefaults.watermark.filename, 0, sizeof(pProgramState->settings.convertdefaults.watermark.filename));
          pProgramState->settings.convertdefaults.watermark.width = 0;
          pProgramState->settings.convertdefaults.watermark.height = 0;
          vcTexture_Destroy(&pProgramState->settings.convertdefaults.watermark.pTexture);
        }
      }

      if (pProgramState->settings.convertdefaults.watermark.isDirty)
      {
        pProgramState->settings.convertdefaults.watermark.isDirty = false;
        vcTexture_Destroy(&pProgramState->settings.convertdefaults.watermark.pTexture);
        uint8_t *pData = nullptr;
        int64_t dataSize = 0;
        char buffer[vcMaxPathLength];
        udStrcpy(buffer, pProgramState->settings.pSaveFilePath);
        udStrcat(buffer, pProgramState->settings.convertdefaults.watermark.filename);
        if (udFile_Load(buffer, (void**)&pData, &dataSize)== udR_Success)
        {
          int comp;
          stbi_uc *pImg = stbi_load_from_memory(pData, (int)dataSize, &pProgramState->settings.convertdefaults.watermark.width, &pProgramState->settings.convertdefaults.watermark.height, &comp, 4);

          vcTexture_Create(&pProgramState->settings.convertdefaults.watermark.pTexture, pProgramState->settings.convertdefaults.watermark.width, pProgramState->settings.convertdefaults.watermark.height, pImg);

          stbi_image_free(pImg);
        }

        udFree(pData);
      }

      if (pProgramState->settings.convertdefaults.watermark.pTexture != nullptr)
        ImGui::Image(pProgramState->settings.convertdefaults.watermark.pTexture, ImVec2(256, 256));

      // Metadata
      ImGui::InputText(vcString::Get("convertAuthor"), pProgramState->settings.convertdefaults.author, udLengthOf(pProgramState->settings.convertdefaults.author));
      ImGui::InputText(vcString::Get("convertComment"), pProgramState->settings.convertdefaults.comment, udLengthOf(pProgramState->settings.convertdefaults.comment));
      ImGui::InputText(vcString::Get("convertCopyright"), pProgramState->settings.convertdefaults.copyright, udLengthOf(pProgramState->settings.convertdefaults.copyright));
      ImGui::InputText(vcString::Get("convertLicense"), pProgramState->settings.convertdefaults.license, udLengthOf(pProgramState->settings.convertdefaults.license));
    }
  }

  ImGui::End();
}
