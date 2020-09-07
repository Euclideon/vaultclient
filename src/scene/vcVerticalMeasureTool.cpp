#include "vcVerticalMeasureTool.h"
#include <stdio.h>

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcFenceRenderer.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"
#include "vcWebFile.h"
#include "vcUnitConversion.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcVerticalMeasureTool::vcVerticalMeasureTool(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
  , m_done(false)
  , m_selectedPoint(-1)
  , m_markDelete(false)
  , m_showAllDistances(false)
  , m_pLineInstance(nullptr)
  , m_flipOrder(false)
{
  ClearPoints();

  vcLineRenderer_CreateLine(&m_pLineInstance);

  m_distStraight = 0.0;
  m_distHoriz = 0.0;
  m_distVert = 0.0;

  for (auto &label: m_labelList)
  {
    label.pText = nullptr;
    label.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.textColour));
    label.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.backgroundColour));
  }
  
  OnNodeUpdate(pProgramState);
  m_loadStatus = vcSLS_Loaded;
}
vcVerticalMeasureTool::~vcVerticalMeasureTool()
{
}

void vcVerticalMeasureTool::Preview(const udDouble3 &position)
{
  m_points[2] = position;
}

void vcVerticalMeasureTool::EndMeasure(vcState *pProgramState, const udDouble3 &position)
{
  m_points[2] = position;
  m_done = true;
  pProgramState->activeTool = vcActiveTool::vcActiveTool_Select;
  udProjectNode_SetMetadataBool(m_pNode, "measureEnd", m_done);
}

void vcVerticalMeasureTool::OnNodeUpdate(vcState *pProgramState)
{
  vcProject_GetNodeMetadata(m_pNode, "measureEnd", &m_done, false);
  vcProject_GetNodeMetadata(m_pNode, "showAllDistances", &m_showAllDistances, false);

  ChangeProjection(pProgramState->geozone);
  UpdateSetting(pProgramState);
}


void vcVerticalMeasureTool::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (pProgramState->activeTool != vcActiveTool::vcActiveTool_MeasureHeight && !m_done && HasLine())
  {
    m_markDelete = true;
    return;
  }

  if (!m_visible)
    return;

  if (m_selected && (m_selectedPoint == -1 || m_selectedPoint == 0 || !m_done))
  {
    vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
    udDouble3 linearDistance = (pProgramState->pActiveViewport->camera.position - m_points[0]);
    pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
    pInstance->worldMat = udDouble4x4::translation(m_points[0]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
    pInstance->pSceneItem = this;
    pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
    pInstance->sceneItemInternalId = 1;
    pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
    pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);
    pInstance->selectable = m_done;
  }
  

  if (HasLine())
  {
    UpdateIntersectionPosition(pProgramState);

    if (m_selected && (m_selectedPoint == -1 || m_selectedPoint == 1 || !m_done))
    {
      vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
      udDouble3 linearDistance = (pProgramState->pActiveViewport->camera.position - m_points[2]);
      pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
      pInstance->worldMat = udDouble4x4::translation(m_points[2]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
      pInstance->pSceneItem = this;
      pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
      pInstance->sceneItemInternalId = 2;
      pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
      pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);
      pInstance->selectable = m_done;
    }

    for (auto &label : m_labelList)
      udFree(label.pText);

    udDouble3 worldUp = vcGIS_GetWorldLocalUp(pProgramState->geozone, m_points[0]);
    if (udDot(m_points[0], worldUp) > udDot(m_points[2], worldUp) && !m_flipOrder) // point 0 is higher than point 2
    {
      m_labelList[0].worldPosition = (m_points[0] + m_points[1]) / 2;
      m_labelList[1].worldPosition = (m_points[1] + m_points[2]) / 2;
    }
    else
    {
      m_labelList[1].worldPosition = (m_points[0] + m_points[1]) / 2;
      m_labelList[0].worldPosition = (m_points[1] + m_points[2]) / 2;
    }
    

    char labelBuf[128] = {};
    char tempBuffer1[128] = {};
    char tempBuffer2[128] = {};

    vcUnitConversion_ConvertAndFormatDistance(tempBuffer1, 128, m_distStraight, vcDistance_Metres, &pProgramState->settings.unitConversionData);
    vcUnitConversion_ConvertAndFormatDistance(tempBuffer2, 128, m_distHoriz, vcDistance_Metres, &pProgramState->settings.unitConversionData);

    // optional straight and horizeontal distance infomation
    if (m_showAllDistances)
    {
      udSprintf(labelBuf, "%s: %s\n%s: %s", vcString::Get("sceneStraightDistance"), tempBuffer1, vcString::Get("sceneHorizontalDistance"), tempBuffer2);
      m_labelList[0].pText = udStrdup(labelBuf);
      m_labelList[0].pSceneItem = this;
      pRenderData->labels.PushBack(&m_labelList[0]);
    }
    
    // vertical distance label
    char labelBufVertical[128] = {};
    vcUnitConversion_ConvertAndFormatDistance(tempBuffer1, 128, m_distVert, vcDistance_Metres, &pProgramState->settings.unitConversionData);
    udSprintf(labelBufVertical, "%s: %s", vcString::Get("sceneVerticalDistance"), tempBuffer1);
    m_labelList[1].pText = udStrdup(labelBufVertical);
    m_labelList[1].pSceneItem = this;
    pRenderData->labels.PushBack(&m_labelList[1]);

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_LineString, m_points, 3);
    vcLineRenderer_UpdatePoints(m_pLineInstance, m_points, 3, vcIGSW_BGRAToImGui(m_lineColour), m_lineWidth, false);
    pRenderData->lines.PushBack(m_pLineInstance);
  }

}

void vcVerticalMeasureTool::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  if (m_selectedPoint == -1 || m_selectedPoint == 0)
    m_points[0] = (delta * udDouble4x4::translation(m_points[0])).axis.t.toVector3();

  if (m_selectedPoint == -1 || m_selectedPoint == 1)
    m_points[2] = (delta * udDouble4x4::translation(m_points[2])).axis.t.toVector3();

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_LineString, m_points, 3);
}

void vcVerticalMeasureTool::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  if (ImGui::Checkbox(udTempStr("%s##showAllDistances%zu", vcString::Get("scenePOIMHeightShowAllDistances"), *pItemID), &m_showAllDistances))
    udProjectNode_SetMetadataBool(m_pNode, "showAllDistances", m_showAllDistances);

  if (HasLine())
  {
    if (ImGui::SliderFloat(udTempStr("%s##VerticalLineWidth%zu", vcString::Get("scenePOILineWidth"), *pItemID), &m_lineWidth, 3.f, 15.f, "%.2f", 3.f))
      udProjectNode_SetMetadataDouble(m_pNode, "lineWidth", m_lineWidth);

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLineColour%zu", vcString::Get("scenePOILineColour1"), *pItemID), &m_lineColour, ImGuiColorEditFlags_None))
      udProjectNode_SetMetadataUint(m_pNode, "lineColour", m_lineColour);

    ImGui::SliderInt(udTempStr("%s##SelectedPoint%zu", vcString::Get("scenePOISelectedPoint"), *pItemID), &m_selectedPoint, -1, 1);

    if (m_selectedPoint == 0)
    {
      ImGui::InputScalarN(udTempStr("%s##Start%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_points[0].x, 3);
      if (ImGui::IsItemDeactivatedAfterEdit())
        vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_LineString, m_points, 3);
    }
    else if (m_selectedPoint == 1)
    {
      ImGui::InputScalarN(udTempStr("%s##End%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_points[2].x, 3);
      if (ImGui::IsItemDeactivatedAfterEdit())
        vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_LineString, m_points, 3);
    }

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLabelColour%zu", vcString::Get("scenePOILabelColour"), *pItemID), &m_textColourBGRA, ImGuiColorEditFlags_None))
    {
      for (auto &label : m_labelList)
        label.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textColourBGRA);
      udProjectNode_SetMetadataUint(m_pNode, "nameColour", m_textColourBGRA);
    }

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLabelBackgroundColour%zu", vcString::Get("scenePOILabelBackgroundColour"), *pItemID), &m_textBackgroundBGRA, ImGuiColorEditFlags_None))
    {
      for (auto &label : m_labelList)
        label.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textBackgroundBGRA);
      udProjectNode_SetMetadataUint(m_pNode, "backColour", m_textBackgroundBGRA);
    }

    if (vcIGSW_InputText(vcString::Get("scenePOILabelDescription"), m_description, sizeof(m_description), ImGuiInputTextFlags_EnterReturnsTrue))
      udProjectNode_SetMetadataString(m_pNode, "description", m_description);

    char mBuffer[128] = {};

    vcUnitConversion_ConvertAndFormatDistance(mBuffer, 128, m_distStraight, vcDistance_Metres, &pProgramState->settings.unitConversionData);
    ImGui::Text("%s: %s", vcString::Get("sceneStraightDistance"), mBuffer);

    vcUnitConversion_ConvertAndFormatDistance(mBuffer, 128, m_distHoriz, vcDistance_Metres, &pProgramState->settings.unitConversionData);
    ImGui::Text("%s: %s", vcString::Get("sceneHorizontalDistance"), mBuffer);

    vcUnitConversion_ConvertAndFormatDistance(mBuffer, 128, m_distVert, vcDistance_Metres, &pProgramState->settings.unitConversionData);
    ImGui::Text("%s: %s", vcString::Get("sceneVerticalDistance"), mBuffer);
  }
}

void vcVerticalMeasureTool::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  char buffer[128];

  ImGui::Text("%s", vcString::Get("sceneStraightDistance"));
  ImGui::PushFont(pProgramState->pBigFont);
  ImGui::Indent();
  vcUnitConversion_ConvertAndFormatDistance(buffer, udLengthOf(buffer), m_distStraight, vcDistance_Metres, &pProgramState->settings.unitConversionData);
  ImGui::Text("%s", buffer);
  ImGui::Unindent();
  ImGui::PopFont();

  ImGui::Text("%s", vcString::Get("sceneHorizontalDistance"));
  ImGui::PushFont(pProgramState->pBigFont);
  ImGui::Indent();
  vcUnitConversion_ConvertAndFormatDistance(buffer, udLengthOf(buffer), m_distHoriz, vcDistance_Metres, &pProgramState->settings.unitConversionData);
  ImGui::Text("%s", buffer);
  ImGui::Unindent();
  ImGui::PopFont();

  ImGui::Text("%s", vcString::Get("sceneVerticalDistance"));
  ImGui::PushFont(pProgramState->pBigFont);
  ImGui::Indent();
  vcUnitConversion_ConvertAndFormatDistance(buffer, udLengthOf(buffer), m_distVert, vcDistance_Metres, &pProgramState->settings.unitConversionData);
  ImGui::Text("%s", buffer);
  ImGui::Unindent();
  ImGui::PopFont();

  if (ImGui::Checkbox(vcString::Get("sceneVerticalFlipOrder"), &m_flipOrder))
    udProjectNode_SetMetadataBool(m_pNode, "flipOrder", m_flipOrder ? 1 : 0);
}

void vcVerticalMeasureTool::Cleanup(vcState *pProgramState)
{
  ClearPoints();

  if (m_pLineInstance)
  {
    vcLineRenderer_DestroyLine(&m_pLineInstance);
    m_pLineInstance = nullptr;
  }

  for (auto &label : m_labelList)
    udFree(label.pText);

  pProgramState->activeTool = vcActiveTool::vcActiveTool_Select;
}

void vcVerticalMeasureTool::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoints = nullptr;
  int number = 0;
  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoints, &number);

  // If still previewing only override the first point, if any
  if (!m_done)
    number = udMin(1, number);

  for (int i = 0; i < number; ++i)
    m_points[i] = pPoints[i];

  udFree(pPoints);
  pPoints = nullptr;
}

udDouble3 vcVerticalMeasureTool::GetLocalSpacePivot()
{
  if (m_selectedPoint == 0)
    return m_points[0];
  else if (m_selectedPoint == 1)
    return m_points[2];

  return m_points[1];
}

void vcVerticalMeasureTool::UpdateIntersectionPosition(vcState *pProgramState)
{
  if (!HasLine())
    return;

  udDouble3 v_20 = m_points[2] - m_points[0];
  udDouble3 worldUp = vcGIS_GetWorldLocalUp(pProgramState->geozone, m_points[0]);
  udDouble3 right = udCross3(v_20, worldUp);
  udDouble3 forward = udCross3(worldUp, right);

  double len = udMag3(forward);
  if (len == 0.0)
  {
    m_points[1] = m_points[0];
    return;
  }

  forward /= len;
  m_distHoriz = udDot(v_20, forward);
  m_distVert = udAbs(udDot(v_20, worldUp));

  if (udDot(m_points[0], worldUp) > udDot(m_points[2], worldUp) && !m_flipOrder)
      m_points[1] = m_points[0] + forward * m_distHoriz;
  else
    m_points[1] = m_points[2] - forward * m_distHoriz;

  m_distStraight = udMag3(m_points[0] - m_points[2]);
}

bool vcVerticalMeasureTool::HasLine()
{
  return m_points[2] != udDouble3::zero();
}

void vcVerticalMeasureTool::ClearPoints()
{
  for (size_t i = 0; i < udLengthOf(m_points); i++)
    m_points[i] = udDouble3::zero();
}

void vcVerticalMeasureTool::UpdateSetting(vcState *pProgramState)
{
  int32_t size = vcLFS_Medium;
  udProjectNode_GetMetadataInt(m_pNode, "textSize", &size, pProgramState->settings.tools.label.textSize);
  udProjectNode_GetMetadataUint(m_pNode, "nameColour", &m_textColourBGRA, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.textColour));
  udProjectNode_GetMetadataUint(m_pNode, "backColour", &m_textBackgroundBGRA, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.backgroundColour));
  vcProject_GetNodeMetadata(m_pNode, "flipOrder", &m_flipOrder, false);

  uint32_t textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textColourBGRA);
  uint32_t backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textBackgroundBGRA);
  for (auto &label : m_labelList)
  {
    label.textColourRGBA = textColourRGBA;
    label.backColourRGBA = backColourRGBA;
  }

  udProjectNode_GetMetadataUint(m_pNode, "lineColour", &m_lineColour, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.line.colour));

  double width = pProgramState->settings.tools.line.width;
  udProjectNode_GetMetadataDouble(m_pNode, "lineWidth", &width, pProgramState->settings.tools.line.width);
  m_lineWidth = (float)width;

  const char *pTemp;
  udProjectNode_GetMetadataString(m_pNode, "description", &pTemp, "");
  udStrcpy(m_description, pTemp);
}

void vcVerticalMeasureTool::HandleToolUI(vcState * pProgramState)
{
  if (HasLine())
    HandleSceneEmbeddedUI(pProgramState);
}
