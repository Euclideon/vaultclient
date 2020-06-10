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

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcVerticalMeasureTool::vcVerticalMeasureTool(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
  , m_done(false)
  , m_pickStart(true)
  , m_pickEnd(true)
  , m_pLineInstance(nullptr)
{
  ClearPoints();

  vcLineRenderer_CreateLine(&m_pLineInstance);

  m_labelInfo.pText = nullptr;
  m_labelInfo.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.textColour));
  m_labelInfo.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.backgroundColour));
  m_labelInfo.textSize = (vcLabelFontSize)pProgramState->settings.tools.label.textSize;

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
  vdkProjectNode_SetMetadataBool(m_pNode, "measureEnd", m_done);
}

void vcVerticalMeasureTool::OnNodeUpdate(vcState *pProgramState)
{
  vdkProjectNode_GetMetadataBool(m_pNode, "pickStart", &m_pickStart, true);
  vdkProjectNode_GetMetadataBool(m_pNode, "pickEnd", &m_pickEnd, true);
  vdkProjectNode_GetMetadataBool(m_pNode, "measureEnd", &m_done, false);

  if (pProgramState->activeTool != vcActiveTool::vcActiveTool_MeasureHeight && !m_done && HasLine())
  {
    m_points[1] = udDouble3::zero();
    m_points[2] = udDouble3::zero();

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_LineString, m_points, 1);
  }

  ChangeProjection(pProgramState->geozone);
  UpdateSetting(pProgramState);
}


void vcVerticalMeasureTool::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_selected && (m_pickStart || !m_done))
  {
    vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
    udDouble3 linearDistance = (pProgramState->camera.position - m_points[0]);
    pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
    pInstance->worldMat = udDouble4x4::translation(m_points[0]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
    pInstance->pSceneItem = this;
    pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
    pInstance->sceneItemInternalId = 1;
    pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
  }
  

  if (HasLine())
  {
    UpdateIntersectionPosition(pProgramState);

    if (m_selected && (m_pickEnd || !m_done))
    {
      vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
      udDouble3 linearDistance = (pProgramState->camera.position - m_points[2]);
      pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
      pInstance->worldMat = udDouble4x4::translation(m_points[2]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
      pInstance->pSceneItem = this;
      pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
      pInstance->sceneItemInternalId = 2;
      pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
    }

    m_labelInfo.worldPosition = m_points[1];
    char labelBuf[128] = {};
    udStrcat(labelBuf, m_pNode->pName);
    udStrcat(labelBuf, udTempStr("\n%s: %.3f", vcString::Get("scenePOIMHeight"), udAbs(m_points[0].z - m_points[2].z)));
    if(m_labelInfo.pText)
      udFree(m_labelInfo.pText);
    m_labelInfo.pText = udStrdup(labelBuf);
    pRenderData->labels.PushBack(&m_labelInfo);

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_LineString, m_points, 3);
    vcLineRenderer_UpdatePoints(m_pLineInstance, m_points, 3, vcIGSW_BGRAToImGui(m_lineColour), m_lineWidth, false);
    pRenderData->lines.PushBack(m_pLineInstance);
  }

}

void vcVerticalMeasureTool::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  if (m_pickStart)
    m_points[0] = (delta * udDouble4x4::translation(m_points[0])).axis.t.toVector3();

  if (m_pickEnd)
    m_points[2] = (delta * udDouble4x4::translation(m_points[2])).axis.t.toVector3();

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_LineString, m_points, 3);
}

void vcVerticalMeasureTool::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  if (ImGui::Checkbox(udTempStr("%s##Select1%zu", vcString::Get("scenePOIMHeightPickStart"), *pItemID), &m_pickStart))
    vdkProjectNode_SetMetadataBool(m_pNode, "pickStart", m_pickStart);

  ImGui::InputScalarN(udTempStr("%s##Start%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_points[0].x, 3);

  if (HasLine())
  {
    if (ImGui::Checkbox(udTempStr("%s##Select2%zu", vcString::Get("scenePOIMHeightPickEnd"), *pItemID), &m_pickEnd))
      vdkProjectNode_SetMetadataBool(m_pNode, "pickEnd", m_pickEnd);

    ImGui::InputScalarN(udTempStr("%s##End%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_points[2].x, 3);
    if (ImGui::IsItemDeactivatedAfterEdit())
      vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_LineString, m_points, 3);

    if (ImGui::SliderFloat(udTempStr("%s##VerticalLineWidth%zu", vcString::Get("scenePOILineWidth"), *pItemID), &m_lineWidth, 3.f, 15.f, "%.2f", 3.f))
      vdkProjectNode_SetMetadataDouble(m_pNode, "lineWidth", m_lineWidth);

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLineColour%zu", vcString::Get("scenePOILineColour1"), *pItemID), &m_lineColour, ImGuiColorEditFlags_None))
      vdkProjectNode_SetMetadataUint(m_pNode, "lineColour", m_lineColour);

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLabelColour%zu", vcString::Get("scenePOILabelColour"), *pItemID), &m_textColourBGRA, ImGuiColorEditFlags_None))
    {
      m_labelInfo.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textColourBGRA);
      vdkProjectNode_SetMetadataUint(m_pNode, "nameColour", m_textColourBGRA);
    }

    if (vcIGSW_ColorPickerU32(udTempStr("%s##VerticalLabelBackgroundColour%zu", vcString::Get("scenePOILabelBackgroundColour"), *pItemID), &m_textBackgroundBGRA, ImGuiColorEditFlags_None))
    {
      m_labelInfo.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textBackgroundBGRA);
      vdkProjectNode_SetMetadataUint(m_pNode, "backColour", m_textBackgroundBGRA);
    }

    const char *labelSizeOptions[] = { vcString::Get("scenePOILabelSizeNormal"), vcString::Get("scenePOILabelSizeSmall"), vcString::Get("scenePOILabelSizeLarge") };
    int32_t size = m_labelInfo.textSize;
    if (ImGui::Combo(udTempStr("%s##VerticalLabelSize%zu", vcString::Get("scenePOILabelSize"), *pItemID), &size, labelSizeOptions, (int)udLengthOf(labelSizeOptions)))
    {
      m_labelInfo.textSize = (vcLabelFontSize)size;
      vdkProjectNode_SetMetadataInt(m_pNode, "textSize", size);
    }
  }

  ImGui::Text("%s: %.3f", vcString::Get("scenePOIMHeight"), udAbs(m_points[0].z - m_points[2].z));

}

void vcVerticalMeasureTool::Cleanup(vcState *pProgramState)
{
  RemoveMeasureInfo();
  pProgramState->activeTool = vcActiveTool::vcActiveTool_Select;
}

void vcVerticalMeasureTool::ChangeProjection(const udGeoZone &newZone)
{
  ClearPoints();

  udDouble3 *pPoints = nullptr;
  int number = 0;
  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoints, &number);
  for(int i = 0; i < number; i ++)
    m_points[i] = pPoints[i];

  udFree(pPoints);
  pPoints = nullptr;
}

udDouble3 vcVerticalMeasureTool::GetLocalSpacePivot()
{
  if (!HasLine() || !m_done || (m_done && m_pickStart))
    return m_points[0];
  else
    return m_points[2];
}

void vcVerticalMeasureTool::UpdateIntersectionPosition(vcState *pProgramState)
{
  if (!HasLine())
    return;

  udDouble3 localStartPoint = udGeoZone_TransformPoint(m_points[0], pProgramState->geozone, pProgramState->activeProject.baseZone);
  udDouble3 localEndPoint = udGeoZone_TransformPoint(m_points[2], pProgramState->geozone, pProgramState->activeProject.baseZone);
  udDouble3 direction = localEndPoint - localStartPoint;
  udDouble3 middle = udDouble3::zero();
  if (direction.z > 0)
    middle = udDouble3::create(localStartPoint.x, localStartPoint.y, localEndPoint.z);
  else
    middle = udDouble3::create(localEndPoint.x, localEndPoint.y, localStartPoint.z);

  m_points[1] = udGeoZone_TransformPoint(middle, pProgramState->activeProject.baseZone, pProgramState->geozone);

}

bool vcVerticalMeasureTool::HasLine()
{
  return m_points[2] != udDouble3::zero();
}

void vcVerticalMeasureTool::RemoveMeasureInfo()
{
  if (m_pLineInstance)
  {
    vcLineRenderer_DestroyLine(&m_pLineInstance);
    m_pLineInstance = nullptr;
  }

  if (m_labelInfo.pText)
  {
    udFree(m_labelInfo.pText);
    m_labelInfo.pText = nullptr;
  }

}

void vcVerticalMeasureTool::ClearPoints()
{
  for (size_t i = 0; i < POINTSIZE; i++)
    m_points[i] = udDouble3::zero();
}

void vcVerticalMeasureTool::UpdateSetting(vcState *pProgramState)
{
  int32_t size = vcLFS_Medium;
  vdkProjectNode_GetMetadataInt(m_pNode, "textSize", &size, pProgramState->settings.tools.label.textSize);
  m_labelInfo.textSize = (vcLabelFontSize)size;

  vdkProjectNode_GetMetadataUint(m_pNode, "nameColour", &m_textColourBGRA, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.textColour));
  m_labelInfo.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textColourBGRA);

  vdkProjectNode_GetMetadataUint(m_pNode, "backColour", &m_textBackgroundBGRA, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.backgroundColour));
  m_labelInfo.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_textBackgroundBGRA);

  vdkProjectNode_GetMetadataUint(m_pNode, "lineColour", &m_lineColour, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.line.colour));

  double width = pProgramState->settings.tools.line.width;
  vdkProjectNode_GetMetadataDouble(m_pNode, "lineWidth", &width, pProgramState->settings.tools.line.width);
  m_lineWidth = (float)width;
}
