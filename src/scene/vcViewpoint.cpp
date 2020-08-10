#include "vcViewpoint.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"

vcViewpoint::vcViewpoint(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

void vcViewpoint::OnNodeUpdate(vcState *pProgramState)
{
  udError result;

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

  ImGui::InputScalarN(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), ImGuiDataType_Double, &m_CameraHeadingPitch.x, 2);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

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
  pProgramState->pActiveViewport->camera.position = m_CameraPosition;
  pProgramState->pActiveViewport->camera.headingPitch = m_CameraHeadingPitch;

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
  uint32_t boolVal = (pProgramState->settings.visualization.useCustomClassificationColours ? 1 : 0);
  udProjectNode_GetMetadataBool(m_pNode, "visualisation.classification.enabled", &boolVal, boolVal);
  pProgramState->settings.visualization.useCustomClassificationColours = (boolVal != 0);

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
}
