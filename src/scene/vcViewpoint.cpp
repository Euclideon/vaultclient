#include "vcViewpoint.h"

#include "vcRender.h"
#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcViewpoint::vcViewpoint(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

void vcViewpoint::OnNodeUpdate(vcState *pProgramState)
{
  udError result;

  const char *pDescription = nullptr;
  result = udProjectNode_GetMetadataString(m_pNode, "description", &pDescription, "");
  udStrcpy(m_description, pDescription);

  result = udProjectNode_GetMetadataDouble(m_pNode, "transform.heading", &m_CameraHeadingPitch.x, 0.0);
  result = udProjectNode_GetMetadataDouble(m_pNode, "transform.pitch", &m_CameraHeadingPitch.y, 0.0);

  if (result != udE_Success)
  {
    udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.x", &m_CameraHeadingPitch.x, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &m_CameraHeadingPitch.y, 0.0);
  }

  ChangeProjection(pProgramState->geozone);
}

void vcViewpoint::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does Nothing
}

void vcViewpoint::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_CameraPosition = (delta * udDouble4x4::translation(m_CameraPosition)).axis.t.toVector3();

  udDouble3 sumRotation = delta.extractYPR() + udDouble3::create(m_CameraHeadingPitch, 0.0);

  // Clamped this to the same limitations as the camera
  m_CameraHeadingPitch = udDouble2::create(udMod(sumRotation.x, UD_2PI), udClampWrap(sumRotation.y, -UD_HALF_PI, UD_HALF_PI));

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_CameraPosition, 1);
}

void vcViewpoint::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  bool changed = false;

  ImGui::InputScalarN(udTempStr("%s##ViewpointPosition%zu", vcString::Get("sceneViewpointPosition"), *pItemID), ImGuiDataType_Double, &m_CameraPosition.x, 3);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  vcIGSW_DegreesScalar(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), &m_CameraHeadingPitch);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  if (ImGui::InputTextMultiline(udTempStr("%s##ViewpointDescription", vcString::Get("sceneViewpointDescription")), m_description, udLengthOf(m_description)))
    udProjectNode_SetMetadataString(m_pNode, "description", m_description);

  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    changed = true;
    m_CameraHeadingPitch = pProgramState->pActiveViewport->camera.headingPitch;
    m_CameraPosition = pProgramState->pActiveViewport->camera.position;
  }

  if (ImGui::Button(vcString::Get("sceneViewpointSetCameraAndVisSetting")))
  {
    changed = true;
    m_CameraHeadingPitch = pProgramState->pActiveViewport->camera.headingPitch;
    m_CameraPosition = pProgramState->pActiveViewport->camera.position;
    SaveSettings(pProgramState, m_pNode);
  }

  if (changed)
  {
    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_CameraPosition, 1);

    udProjectNode_SetMetadataDouble(m_pNode, "transform.heading", m_CameraHeadingPitch.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.pitch", m_CameraHeadingPitch.y);
  }
}

void vcViewpoint::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  bool saveCam = ImGui::Button(vcString::Get("sceneViewpointSetCamera"));
  bool saveVis = ImGui::Button(vcString::Get("sceneViewpointSetCameraAndVisSetting"));
  if (saveCam || saveVis)
  {
    m_CameraHeadingPitch = pProgramState->pActiveViewport->camera.headingPitch;
    m_CameraPosition = pProgramState->pActiveViewport->camera.position;

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_CameraPosition, 1);

    udProjectNode_SetMetadataDouble(m_pNode, "transform.heading", m_CameraHeadingPitch.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.pitch", m_CameraHeadingPitch.y);
  }

  if (saveVis)
    SaveSettings(pProgramState, m_pNode);
}

vcMenuBarButtonIcon vcViewpoint::GetSceneExplorerIcon()
{
  return vcMBBI_SaveViewport;
}

void vcViewpoint::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoint = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoint, &numPoints);
  if (numPoints == 1)
    m_CameraPosition = pPoint[0];

  udFree(pPoint);
}

void vcViewpoint::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing to clean up
}

void vcViewpoint::SetCameraPosition(vcState *pProgramState)
{
  // Disable "keep camera above ground" if viewpoint is underground
  if (pProgramState->settings.camera.keepAboveSurface && pProgramState->settings.maptiles.mapEnabled)
  {
    udDouble3 positionUp = vcGIS_GetWorldLocalUp(m_pProject->baseZone, m_CameraPosition);

    udDouble3 surfacePosition = vcRender_QueryMapAtCartesian(pProgramState->pActiveViewport->pRenderContext, m_CameraPosition);

    if (udDot3(positionUp, udNormalize3(m_CameraPosition - surfacePosition)) < 0)
      pProgramState->settings.camera.keepAboveSurface = false;
  }

  for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
  {
    pProgramState->pViewports[viewportIndex].camera.position = m_CameraPosition;
    pProgramState->pViewports[viewportIndex].camera.headingPitch = m_CameraHeadingPitch;
  }

  ApplySettings(pProgramState);
}

udDouble4x4 vcViewpoint::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_CameraPosition);
}

void vcViewpoint::ApplySettings(vcState *pProgramState)
{
  const char *pMode = nullptr;
  udProjectNode_GetMetadataString(m_pNode, "visualisation.mode", &pMode, nullptr);

  if (udStrEqual(pMode, "default"))
    pProgramState->settings.visualization.mode = vcVM_Default;
  else if (udStrEqual(pMode, "colour"))
    pProgramState->settings.visualization.mode = vcVM_Colour;
  else if (udStrEqual(pMode, "intensity"))
    pProgramState->settings.visualization.mode = vcVM_Intensity;
  else if (udStrEqual(pMode, "classification"))
    pProgramState->settings.visualization.mode = vcVM_Classification;
  else if (udStrEqual(pMode, "displacement_distance"))
    pProgramState->settings.visualization.mode = vcVM_DisplacementDistance;
  else if (udStrEqual(pMode, "displacement_direction"))
    pProgramState->settings.visualization.mode = vcVM_DisplacementDirection;
  else if (udStrEqual(pMode, "gps_time"))
    pProgramState->settings.visualization.mode = vcVM_GPSTime;
  else if (udStrEqual(pMode, "scan_angle"))
    pProgramState->settings.visualization.mode = vcVM_ScanAngle;
  else if (udStrEqual(pMode, "point_source_id"))
    pProgramState->settings.visualization.mode = vcVM_PointSourceID;
  else if (udStrEqual(pMode, "return_number"))
    pProgramState->settings.visualization.mode = vcVM_ReturnNumber;
  else if (udStrEqual(pMode, "number_of_returns"))
    pProgramState->settings.visualization.mode = vcVM_NumberOfReturns;

  // Intensity
  udProjectNode_GetMetadataInt(m_pNode, "visualisation.intensity.min", &pProgramState->settings.visualization.minIntensity, pProgramState->settings.visualization.minIntensity);
  udProjectNode_GetMetadataInt(m_pNode, "visualisation.intensity.max", &pProgramState->settings.visualization.maxIntensity, pProgramState->settings.visualization.maxIntensity);

  // Classification
  vcProject_GetNodeMetadata(m_pNode, "visualisation.classification.enabled", &pProgramState->settings.visualization.useCustomClassificationColours, pProgramState->settings.visualization.useCustomClassificationColours);

  // Displacement Distance/Direction
  double doubleVal = pProgramState->settings.visualization.displacement.bounds.x;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.displacement.min", &doubleVal, doubleVal);
  pProgramState->settings.visualization.displacement.bounds.x = (float)doubleVal;
  doubleVal = pProgramState->settings.visualization.displacement.bounds.y;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.displacement.max", &doubleVal, doubleVal);
  pProgramState->settings.visualization.displacement.bounds.y = (float)doubleVal;
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.displacement.colour.max", &pProgramState->settings.visualization.displacement.max, pProgramState->settings.visualization.displacement.max);
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.displacement.colour.min", &pProgramState->settings.visualization.displacement.min, pProgramState->settings.visualization.displacement.min);
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.displacement.colour.mid", &pProgramState->settings.visualization.displacement.mid, pProgramState->settings.visualization.displacement.mid);
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.displacement.colour.error", &pProgramState->settings.visualization.displacement.error, pProgramState->settings.visualization.displacement.error);

  // GPS Time
  const char *pStringVal = nullptr;
  udProjectNode_GetMetadataString(m_pNode, "visualisation.gps_time.format", &pStringVal, nullptr);
  if (udStrEqual(pStringVal, "gps"))
    pProgramState->settings.visualization.GPSTime.inputFormat = vcTimeReference_GPS;
  else if (udStrEqual(pStringVal, "gps_adjusted"))
    pProgramState->settings.visualization.GPSTime.inputFormat = vcTimeReference_GPSAdjusted;

  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.gps_time.min", &pProgramState->settings.visualization.GPSTime.minTime, pProgramState->settings.visualization.GPSTime.minTime);
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.gps_time.max", &pProgramState->settings.visualization.GPSTime.maxTime, pProgramState->settings.visualization.GPSTime.maxTime);

  // Scan Angle
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.scan_angle.min", &pProgramState->settings.visualization.scanAngle.minAngle, pProgramState->settings.visualization.scanAngle.minAngle);
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.scan_angle.max", &pProgramState->settings.visualization.scanAngle.maxAngle, pProgramState->settings.visualization.scanAngle.maxAngle);

  // Point Source ID
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.point_source_id.default_colour", &pProgramState->settings.visualization.pointSourceID.defaultColour, pProgramState->settings.visualization.pointSourceID.defaultColour);
  uint32_t count = 0;
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.point_source_id.colour_map_count", &count, 0);
  for (uint32_t i = 0; i < count; ++i)
  {
    if (i == pProgramState->settings.visualization.pointSourceID.colourMap.length)
      pProgramState->settings.visualization.pointSourceID.colourMap.PushBack();

    vcVisualizationSettings::KV *pKV = pProgramState->settings.visualization.pointSourceID.colourMap.GetElement(i);
    uint32_t key = 0;
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualisation.point_source_id.colour_map[%u].id", i), &key, 0);
    pKV->id = (uint16_t)key;
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualisation.point_source_id.colour_map[%u].colour", i), &pKV->colour, 0);
  }
  while (count != pProgramState->settings.visualization.pointSourceID.colourMap.length)
    pProgramState->settings.visualization.pointSourceID.colourMap.PopBack();

  // Return Number
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.return_number.colours_count", &count, count);
  for (uint32_t i = 0; i < count && i < pProgramState->settings.visualization.s_maxReturnNumbers; ++i)
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualisation.return_number.colours[%i]", i), &pProgramState->settings.visualization.returnNumberColours[i], pProgramState->settings.visualization.returnNumberColours[i]);

  // Number of Returns
  udProjectNode_GetMetadataUint(m_pNode, "visualisation.number_of_returns.colours_count", &count, count);
  for (uint32_t i = 0; i < count && i < pProgramState->settings.visualization.s_maxReturnNumbers; ++i)
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualisation.number_of_returns.colours[%i]", i), &pProgramState->settings.visualization.numberOfReturnsColours[i], pProgramState->settings.visualization.numberOfReturnsColours[i]);

  for (int v = 0; v < vcMaxViewportCount; v++)
  {
    // Camera lens
    pStringVal = nullptr;
    udProjectNode_GetMetadataString(m_pNode, udTempStr("visualisation.camera_lens.preset[%d]", v), &pStringVal, nullptr);
    if (udStrEqual(pStringVal, "custom"))
      pProgramState->settings.camera.lensIndex[v] = vcLS_Custom;
    else if (udStrEqual(pStringVal, "15mm"))
      pProgramState->settings.camera.lensIndex[v] = vcLS_15mm;
    else if (udStrEqual(pStringVal, "24mm"))
      pProgramState->settings.camera.lensIndex[v] = vcLS_24mm;
    else if (udStrEqual(pStringVal, "30mm"))
      pProgramState->settings.camera.lensIndex[v] = vcLS_30mm;
    else if (udStrEqual(pStringVal, "50mm"))
      pProgramState->settings.camera.lensIndex[v] = vcLS_50mm;
    else if (udStrEqual(pStringVal, "70mm"))
       pProgramState->settings.camera.lensIndex[v] = vcLS_70mm;
    else if (udStrEqual(pStringVal, "100mm"))
       pProgramState->settings.camera.lensIndex[v] = vcLS_100mm;
    
    doubleVal = pProgramState->settings.camera.fieldOfView[v];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.camera_lens.custom_radians[%i]", v), &doubleVal, doubleVal);
    pProgramState->settings.camera.fieldOfView[v] = (float)doubleVal;
    
  }

  // Skybox
  pStringVal = nullptr;
  udProjectNode_GetMetadataString(m_pNode, "visualisation.skybox.type", &pStringVal, nullptr);
  if (udStrEqual(pStringVal, "none"))
    pProgramState->settings.presentation.skybox.type = vcSkyboxType_None;
  else if(udStrEqual(pStringVal, "colour"))
    pProgramState->settings.presentation.skybox.type = vcSkyboxType_Colour;
  else if (udStrEqual(pStringVal, "simple"))
    pProgramState->settings.presentation.skybox.type = vcSkyboxType_Simple;
  else if (udStrEqual(pStringVal, "atmosphere"))
    pProgramState->settings.presentation.skybox.type = vcSkyboxType_Atmosphere;

  for (size_t i = 0; i < pProgramState->settings.presentation.skybox.colour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.presentation.skybox.colour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.skybox.colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.presentation.skybox.colour[i] = (float)doubleVal;
  }

  vcProject_GetNodeMetadata(m_pNode, "visualisation.skybox.use_live_time", &pProgramState->settings.presentation.skybox.useLiveTime, pProgramState->settings.presentation.skybox.useLiveTime);
  vcProject_GetNodeMetadata(m_pNode, "visualisation.skybox.lock_sun_position", &pProgramState->settings.presentation.skybox.keepSameTime, pProgramState->settings.presentation.skybox.keepSameTime);
  doubleVal = pProgramState->settings.presentation.skybox.timeOfDay;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.skybox.time_of_day", &doubleVal, doubleVal);
  pProgramState->settings.presentation.skybox.timeOfDay = (float)doubleVal;
  doubleVal = pProgramState->settings.presentation.skybox.month;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.skybox.time_of_year", &doubleVal, doubleVal);
  pProgramState->settings.presentation.skybox.month = (float)doubleVal;
  doubleVal = pProgramState->settings.presentation.skybox.exposure;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.skybox.exposure", &doubleVal, doubleVal);
  pProgramState->settings.presentation.skybox.exposure = (float)doubleVal;

  // Saturation
  doubleVal = pProgramState->settings.presentation.saturation;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.saturation", &doubleVal, doubleVal);
  pProgramState->settings.presentation.saturation = (float)doubleVal;

  // Voxel Shape
  pStringVal = nullptr;
  udProjectNode_GetMetadataString(m_pNode, "visualisation.point_mode", &pStringVal, nullptr);
  if (udStrEqual(pStringVal, "rectangles"))
    pProgramState->settings.presentation.pointMode = udRCPM_Rectangles;
  else if (udStrEqual(pStringVal, "cubes"))
    pProgramState->settings.presentation.pointMode = udRCPM_Cubes;
  else if (udStrEqual(pStringVal, "points"))
    pProgramState->settings.presentation.pointMode = udRCPM_Points;

  // Selected Object Highlighting
  vcProject_GetNodeMetadata(m_pNode, "visualisation.object_highlighting.enable", &pProgramState->settings.objectHighlighting.enable, pProgramState->settings.objectHighlighting.enable);
  for (size_t i = 0; i < pProgramState->settings.objectHighlighting.colour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.objectHighlighting.colour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.object_highlighting.colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.objectHighlighting.colour[i] = (float)doubleVal;
  }
  doubleVal = pProgramState->settings.objectHighlighting.thickness;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.object_highlighting.thickness", &doubleVal, doubleVal);
  pProgramState->settings.objectHighlighting.thickness = (float)doubleVal;

  // Post visualization - Edge Highlighting
  vcProject_GetNodeMetadata(m_pNode, "visualisation.edge_highlighting.enable", &pProgramState->settings.postVisualization.edgeOutlines.enable, pProgramState->settings.postVisualization.edgeOutlines.enable);
  udProjectNode_GetMetadataInt(m_pNode, "visualisation.edge_highlighting.width", &pProgramState->settings.postVisualization.edgeOutlines.width, pProgramState->settings.postVisualization.edgeOutlines.width);
  doubleVal = pProgramState->settings.postVisualization.edgeOutlines.threshold;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.edge_highlighting.threshold", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.edgeOutlines.threshold = (float)doubleVal;
  for (size_t i = 0; i < pProgramState->settings.postVisualization.edgeOutlines.colour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.postVisualization.edgeOutlines.colour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.edge_highlighting.colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.postVisualization.edgeOutlines.colour[i] = (float)doubleVal;
  }

  // Post visualization - Colour by Height
  vcProject_GetNodeMetadata(m_pNode, "visualisation.colour_by_height.enable", &pProgramState->settings.postVisualization.colourByHeight.enable, pProgramState->settings.postVisualization.colourByHeight.enable);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByHeight.minColour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.postVisualization.colourByHeight.minColour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.colour_by_height.min_colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.postVisualization.colourByHeight.minColour[i] = (float)doubleVal;
  }
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByHeight.maxColour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.postVisualization.colourByHeight.maxColour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.colour_by_height.max_colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.postVisualization.colourByHeight.maxColour[i] = (float)doubleVal;
  }
  doubleVal = pProgramState->settings.postVisualization.colourByHeight.startHeight;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.colour_by_height.min_height", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.colourByHeight.startHeight = (float)doubleVal;
  doubleVal = pProgramState->settings.postVisualization.colourByHeight.endHeight;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.colour_by_height.max_height", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.colourByHeight.endHeight = (float)doubleVal;

  // Post visualization - Colour by Depth
  vcProject_GetNodeMetadata(m_pNode, "visualisation.colour_by_depth.enable", &pProgramState->settings.postVisualization.colourByDepth.enable, pProgramState->settings.postVisualization.colourByDepth.enable);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByDepth.colour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.postVisualization.colourByDepth.colour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.colour_by_depth.colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.postVisualization.colourByDepth.colour[i] = (float)doubleVal;
  }
  doubleVal = pProgramState->settings.postVisualization.colourByDepth.startDepth;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.colour_by_depth.min_depth", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.colourByDepth.startDepth = (float)doubleVal;
  doubleVal = pProgramState->settings.postVisualization.colourByDepth.endDepth;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.colour_by_depth.max_depth", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.colourByDepth.endDepth = (float)doubleVal;

  // Post visualization - Contours
  vcProject_GetNodeMetadata(m_pNode, "visualisation.contours.enable", &pProgramState->settings.postVisualization.contours.enable, pProgramState->settings.postVisualization.contours.enable);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByDepth.colour.ElementCount; ++i)
  {
    doubleVal = pProgramState->settings.postVisualization.contours.colour[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("visualisation.contours.colour[%zu]", i), &doubleVal, doubleVal);
    pProgramState->settings.postVisualization.contours.colour[i] = (float)doubleVal;
  }
  doubleVal = pProgramState->settings.postVisualization.contours.distances;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.contours.distances", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.contours.distances = (float)doubleVal;
  doubleVal = pProgramState->settings.postVisualization.contours.bandHeight;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.contours.band_height", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.contours.bandHeight = (float)doubleVal;
  doubleVal = pProgramState->settings.postVisualization.contours.rainbowRepeat;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.contours.rainbow_repeat_rate", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.contours.rainbowRepeat = (float)doubleVal;
  doubleVal = pProgramState->settings.postVisualization.contours.rainbowIntensity;
  udProjectNode_GetMetadataDouble(m_pNode, "visualisation.contours.rainbow_intensity", &doubleVal, doubleVal);
  pProgramState->settings.postVisualization.contours.rainbowIntensity = (float)doubleVal;
}

void vcViewpoint::SaveSettings(vcState *pProgramState, udProjectNode *pNode)
{
  switch (pProgramState->settings.visualization.mode)
  {
  case vcVM_Default:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "default");
    break;
  case vcVM_Colour:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "colour");
    break;
  case vcVM_Intensity:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "intensity");
    break;
  case vcVM_Classification:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "classification");
    break;
  case vcVM_DisplacementDistance:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "displacement_distance");
    break;
  case vcVM_DisplacementDirection:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "displacement_direction");
    break;
  case vcVM_GPSTime:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "gps_time");
    break;
  case vcVM_ScanAngle:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "scan_angle");
    break;
  case vcVM_PointSourceID:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "point_source_id");
    break;
  case vcVM_ReturnNumber:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "return_number");
    break;
  case vcVM_NumberOfReturns:
    udProjectNode_SetMetadataString(pNode, "visualisation.mode", "number_of_returns");
    break;
  case vcVM_Count:
    break;
  }

  // Intensity
  udProjectNode_SetMetadataInt(pNode, "visualisation.intensity.min", pProgramState->settings.visualization.minIntensity);
  udProjectNode_SetMetadataInt(pNode, "visualisation.intensity.max", pProgramState->settings.visualization.maxIntensity);

  // Classification
  udProjectNode_SetMetadataBool(pNode, "visualisation.classification.enabled", pProgramState->settings.visualization.useCustomClassificationColours);

  // Displacement Distance/Direction
  udProjectNode_SetMetadataDouble(pNode, "visualisation.displacement.min", pProgramState->settings.visualization.displacement.bounds.x);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.displacement.max", pProgramState->settings.visualization.displacement.bounds.y);
  udProjectNode_SetMetadataUint(pNode, "visualisation.displacement.colour.max", pProgramState->settings.visualization.displacement.max);
  udProjectNode_SetMetadataUint(pNode, "visualisation.displacement.colour.min", pProgramState->settings.visualization.displacement.min);
  udProjectNode_SetMetadataUint(pNode, "visualisation.displacement.colour.mid", pProgramState->settings.visualization.displacement.mid);
  udProjectNode_SetMetadataUint(pNode, "visualisation.displacement.colour.error", pProgramState->settings.visualization.displacement.error);

  // GPS Time
  if (pProgramState->settings.visualization.GPSTime.inputFormat == vcTimeReference_GPS)
    udProjectNode_SetMetadataString(pNode, "visualisation.gps_time.format", "gps");
  else if (pProgramState->settings.visualization.GPSTime.inputFormat == vcTimeReference_GPSAdjusted)
    udProjectNode_SetMetadataString(pNode, "visualisation.gps_time.format", "gps_adjusted");

  udProjectNode_SetMetadataDouble(pNode, "visualisation.gps_time.min", pProgramState->settings.visualization.GPSTime.minTime);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.gps_time.max", pProgramState->settings.visualization.GPSTime.maxTime);

  // Scan Angle
  udProjectNode_SetMetadataDouble(pNode, "visualisation.scan_angle.min", pProgramState->settings.visualization.scanAngle.minAngle);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.scan_angle.max", pProgramState->settings.visualization.scanAngle.maxAngle);

  // Point Source ID
  udProjectNode_SetMetadataUint(pNode, "visualisation.point_source_id.default_colour", pProgramState->settings.visualization.pointSourceID.defaultColour);
  udProjectNode_SetMetadataUint(pNode, "visualisation.point_source_id.colour_map_count", (uint32_t)pProgramState->settings.visualization.pointSourceID.colourMap.length);
  for (vcVisualizationSettings::KV kv : pProgramState->settings.visualization.pointSourceID.colourMap)
  {
    udProjectNode_SetMetadataUint(pNode, "visualisation.point_source_id.colour_map[-1].id", kv.id);
    udProjectNode_SetMetadataUint(pNode, "visualisation.point_source_id.colour_map[-1].colour", kv.colour);
  }

  // Return Number
  udProjectNode_SetMetadataUint(pNode, "visualisation.return_number.colours_count", pProgramState->settings.visualization.s_maxReturnNumbers);
  for (uint32_t i = 0; i < pProgramState->settings.visualization.s_maxReturnNumbers; ++i)
    udProjectNode_SetMetadataUint(pNode, udTempStr("visualisation.return_number.colours[%i]", i), pProgramState->settings.visualization.returnNumberColours[i]);

  // Number of Returns
  udProjectNode_SetMetadataUint(pNode, "visualisation.number_of_returns.colours_count", pProgramState->settings.visualization.s_maxReturnNumbers);
  for (uint32_t i = 0; i < pProgramState->settings.visualization.s_maxReturnNumbers; ++i)
    udProjectNode_SetMetadataUint(pNode, udTempStr("visualisation.number_of_returns.colours[%i]", i), pProgramState->settings.visualization.numberOfReturnsColours[i]);

  // Camera lens
  const char *lensNameArray[] = {
    "custom",
    "15mm",
    "24mm",
    "30mm",
    "50mm",
    "70mm",
    "100mm",
  };
  UDCOMPILEASSERT(udLengthOf(lensNameArray) == vcLS_TotalLenses, "Lens name array length mismatch");

  for (int v = 0; v < vcMaxViewportCount; v++)
  {
    udProjectNode_SetMetadataString(pNode, udTempStr("visualisation.camera_lens.preset[%d]", v), lensNameArray[pProgramState->settings.camera.lensIndex[v]]);
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.camera_lens.custom_radians[%d]", v), pProgramState->settings.camera.fieldOfView[v]);
  }

  // Skybox
  const char *skyboxOptions[] = { "none", "colour", "simple", "atmosphere" };
  udProjectNode_SetMetadataString(pNode, "visualisation.skybox.type", skyboxOptions[pProgramState->settings.presentation.skybox.type]);
  for (size_t i = 0; i < pProgramState->settings.presentation.skybox.colour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.skybox.colour[%zu]", i), pProgramState->settings.presentation.skybox.colour[i]);
  udProjectNode_SetMetadataBool(pNode, "visualisation.skybox.use_live_time", pProgramState->settings.presentation.skybox.useLiveTime ? 1 : 0);
  udProjectNode_SetMetadataBool(pNode, "visualisation.skybox.lock_sun_position", pProgramState->settings.presentation.skybox.keepSameTime ? 1 : 0);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.skybox.time_of_day", pProgramState->settings.presentation.skybox.timeOfDay);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.skybox.time_of_year", pProgramState->settings.presentation.skybox.month);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.skybox.exposure", pProgramState->settings.presentation.skybox.exposure);

  // Saturation
  udProjectNode_SetMetadataDouble(pNode, "visualisation.saturation", pProgramState->settings.presentation.saturation);

  // Voxel Shape
  const char *voxelOptions[] = { "rectangles", "cubes", "points" };
  udProjectNode_SetMetadataString(pNode, "visualisation.point_mode", voxelOptions[pProgramState->settings.presentation.pointMode]);

  // Selected Object Highlighting
  udProjectNode_SetMetadataBool(pNode, "visualisation.object_highlighting.enable", pProgramState->settings.objectHighlighting.enable ? 1 : 0);
  for (size_t i = 0; i < pProgramState->settings.objectHighlighting.colour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.object_highlighting.colour[%zu]", i), pProgramState->settings.objectHighlighting.colour[i]);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.object_highlighting.thickness", pProgramState->settings.objectHighlighting.thickness);

  // Post visualization - Edge Highlighting
  udProjectNode_SetMetadataBool(pNode, "visualisation.edge_highlighting.enable", pProgramState->settings.postVisualization.edgeOutlines.enable ? 1 : 0);
  udProjectNode_SetMetadataInt(pNode, "visualisation.edge_highlighting.width", pProgramState->settings.postVisualization.edgeOutlines.width);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.edge_highlighting.threshold", pProgramState->settings.postVisualization.edgeOutlines.threshold);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.edgeOutlines.colour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.edge_highlighting.colour[%zu]", i), pProgramState->settings.postVisualization.edgeOutlines.colour[i]);

  // Post visualization - Colour by Height
  udProjectNode_SetMetadataBool(pNode, "visualisation.colour_by_height.enable", pProgramState->settings.postVisualization.colourByHeight.enable ? 1 : 0);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByHeight.minColour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.colour_by_height.min_colour[%zu]", i), pProgramState->settings.postVisualization.colourByHeight.minColour[i]);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByHeight.maxColour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.colour_by_height.max_colour[%zu]", i), pProgramState->settings.postVisualization.colourByHeight.maxColour[i]);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.colour_by_height.min_height", pProgramState->settings.postVisualization.colourByHeight.startHeight);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.colour_by_height.max_height", pProgramState->settings.postVisualization.colourByHeight.endHeight);

  // Post visualization - Colour by Depth
  udProjectNode_SetMetadataBool(pNode, "visualisation.colour_by_depth.enable", pProgramState->settings.postVisualization.colourByDepth.enable ? 1 : 0);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.colourByDepth.colour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.colour_by_depth.colour[%zu]", i), pProgramState->settings.postVisualization.colourByDepth.colour[i]);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.colour_by_depth.min_depth", pProgramState->settings.postVisualization.colourByDepth.startDepth);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.colour_by_depth.max_depth", pProgramState->settings.postVisualization.colourByDepth.endDepth);

  // Post visualization - Contours
  udProjectNode_SetMetadataBool(pNode, "visualisation.contours.enable", pProgramState->settings.postVisualization.contours.enable ? 1 : 0);
  for (size_t i = 0; i < pProgramState->settings.postVisualization.contours.colour.ElementCount; ++i)
    udProjectNode_SetMetadataDouble(pNode, udTempStr("visualisation.contours.colour[%zu]", i), pProgramState->settings.postVisualization.contours.colour[i]);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.contours.distances", pProgramState->settings.postVisualization.contours.distances);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.contours.band_height", pProgramState->settings.postVisualization.contours.bandHeight);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.contours.rainbow_repeat_rate", pProgramState->settings.postVisualization.contours.rainbowRepeat);
  udProjectNode_SetMetadataDouble(pNode, "visualisation.contours.rainbow_intensity", pProgramState->settings.postVisualization.contours.rainbowIntensity);
}
