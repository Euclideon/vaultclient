#include "vcPOI.h"

#include "vcScene.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "gl/vcFenceRenderer.h"

#include "udPlatform/udMath.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "udPlatform/udFile.h"

vcPOI::vcPOI(const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes /*= ""*/)
{
  Init(pName, nameColour, namePt, pLine, srid, pNotes);
}

vcPOI::vcPOI(const char *pName, uint32_t nameColour, double namePt, udDouble3 position, int32_t srid, const char *pNotes /*= ""*/)
{
  vcLineInfo temp;
  temp.numPoints = 1;
  temp.pPoints = &position;
  temp.lineWidth = 1;
  temp.lineColour = 0xFFFFFFFF;

  Init(pName, nameColour, namePt, &temp, srid, pNotes);
}

void vcPOI::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  // if POI is invisible or if it exceeds maximum visible POI distance
  if (!m_visible || udMag3(m_pLabelInfo->worldPosition - pProgramState->pCamera->position) > pProgramState->settings.presentation.POIFadeDistance)
    return;

  if (m_pFence != nullptr)
    pRenderData->fences.PushBack(m_pFence);

  if (m_pLabelInfo != nullptr)
  {
    m_pLabelInfo->pText = m_pName;
    pRenderData->labels.PushBack(m_pLabelInfo);
  }
}

void vcPOI::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_line.pPoints[m_line.selectedPoint] = (delta * udDouble4x4::translation(m_line.pPoints[m_line.selectedPoint])).axis.t.toVector3();

  m_pLabelInfo->worldPosition = m_line.pPoints[0];

  if (m_pFence != nullptr)
  {
    vcFenceRenderer_ClearPoints(m_pFence);
    vcFenceRenderer_AddPoints(m_pFence, m_line.pPoints, m_line.numPoints, m_line.closed);
  }
}

void vcPOI::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  bool reConfig = false;

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIColor%zu", vcString::Get("LabelColour"), *pItemID), &m_nameColour, ImGuiColorEditFlags_None))
    m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIBackColor%zu", vcString::Get("LabelBackgroundColour"), *pItemID), &m_backColour, ImGuiColorEditFlags_None))
    m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);

  bool lines = m_line.numPoints > 1;

  if (lines)
  {
    if (vcIGSW_ColorPickerU32(vcString::Get("LineColour"), &m_line.lineColour, ImGuiColorEditFlags_None))
      reConfig = true;

    if (ImGui::BeginCombo(vcString::Get("Points"), udTempStr("%s %zu", vcString::Get("Point"), m_line.selectedPoint + 1)))
    {
      for (size_t i = 1; i <= m_line.numPoints; ++i)
        if (ImGui::Selectable(udTempStr("%s %zu", vcString::Get("Point"), i)))
          m_line.selectedPoint = i - 1;

      ImGui::EndCombo();
    }
  }

  ImGui::TextWrapped("%s: %.2f, %.2f, %.2f", vcString::Get("Position"), m_line.pPoints[m_line.selectedPoint].x, m_line.pPoints[m_line.selectedPoint].y, m_line.pPoints[m_line.selectedPoint].z);

  if (lines)
  {
    // Length
    double length = 0;
    if (m_line.numPoints > 1)
    {
      if (m_line.selectedPoint == m_line.numPoints - 1)
      {
        if (m_line.closed)
          length = udMag3(m_line.pPoints[m_line.selectedPoint] - m_line.pPoints[0]);
      }
      else
      {
        length = udMag3(m_line.pPoints[m_line.selectedPoint] - m_line.pPoints[m_line.selectedPoint + 1]);
      }
    }
    ImGui::TextWrapped("%s: %.2f", vcString::Get("Length"), length);

    // Area, ignores Z axis
    if (m_line.closed)
    {
      double area = 0;
      size_t j = m_line.numPoints - 1;

      for (size_t i = 0; i < m_line.numPoints; i++)
      {
        area = area + (m_line.pPoints[j].x + m_line.pPoints[i].x) * (m_line.pPoints[j].y - m_line.pPoints[i].y);
        j = i;
      }
      area /= 2;

      ImGui::TextWrapped("%s: %.2f", vcString::Get("Area"), area);
    }

    if (ImGui::InputInt(vcString::Get("LineWidth"), (int *)&m_line.lineWidth))
      reConfig = true;

    const char *lineOptions[] = { vcString::Get("Arrow"), vcString::Get("Glow"), vcString::Get("Solid") };
    if (ImGui::Combo(vcString::Get("LineStyle"), (int *)&m_line.lineStyle, lineOptions, (int) udLengthOf(lineOptions)))
      reConfig = true;

    if (reConfig)
    {
      vcFenceRendererConfig config;
      udFloat4 colour = vcIGSW_BGRAToImGui(m_line.lineColour);
      config.visualMode = vcRRVM_Fence;
      config.imageMode = m_line.lineStyle;
      config.bottomColour = colour;
      config.topColour = colour;
      config.ribbonWidth = (float)m_line.lineWidth;
      config.textureScrollSpeed = 1.f;
      config.textureRepeatScale = 1.f;

      vcFenceRenderer_SetConfig(m_pFence, config);
    }

    // TODO: label renderer config too
  }

  // Handle hyperlinks
  const char *pHyperlink = m_pMetadata->Get("hyperlink").AsString();
  if (pHyperlink != nullptr)
  {
    ImGui::TextWrapped("%s: %s", vcString::Get("LabelHyperlink"), pHyperlink);
    if (udStrEndsWithi(pHyperlink, ".png") || udStrEndsWithi(pHyperlink, ".jpg"))
    {
      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("LabelOpenHyperlink")))
        pProgramState->loadList.push_back(udStrdup(pHyperlink));
    }
  }
}

void vcPOI::Cleanup(vcState * /*pProgramState*/)
{
  udFree(m_pName);
  udFree(m_line.pPoints);

  vcFenceRenderer_Destroy(&m_pFence);
  udFree(m_pLabelInfo);
}

udDouble4x4 vcPOI::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_line.pPoints[0]);
}

void vcPOI::Init(const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes /*= ""*/)
{
  m_visible = true;
  m_pName = udStrdup(pName);
  m_type = vcSOT_PointOfInterest;
  m_nameColour = nameColour;
  m_namePt = namePt;
  memcpy(&m_line, pLine, sizeof(m_line));

  m_line.pPoints = udAllocType(udDouble3, pLine->numPoints, udAF_Zero);
  memcpy(m_line.pPoints, pLine->pPoints, sizeof(udDouble3) * pLine->numPoints);

  m_line.selectedPoint = 0;
  m_line.lineStyle = vcRRIM_Arrow;

  m_pFence = nullptr;
  if (pLine->numPoints > 1)
  {
    vcFenceRenderer_Create(&m_pFence);

    udFloat4 colours = vcIGSW_BGRAToImGui(nameColour);

    vcFenceRendererConfig config;
    config.visualMode = vcRRVM_Fence;
    config.imageMode = vcRRIM_Arrow;
    config.bottomColour = colours;
    config.topColour = colours;
    config.ribbonWidth = (float)pLine->lineWidth;
    config.textureScrollSpeed = 1.f;
    config.textureRepeatScale = 1.f;

    vcFenceRenderer_SetConfig(m_pFence, config);
    vcFenceRenderer_AddPoints(m_pFence, pLine->pPoints, pLine->numPoints);
  }

  m_backColour = 0x7F000000;

  m_pLabelInfo = udAllocType(vcLabelInfo, 1, udAF_Zero);
  m_pLabelInfo->pText = m_pName;
  m_pLabelInfo->worldPosition = pLine->pPoints[0];
  m_pLabelInfo->textSize = vcLFS_Medium;
  m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(nameColour);
  m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);

  if (srid != 0)
  {
    m_pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
    m_pZone = udAllocType(udGeoZone, 1, udAF_Zero);
    udGeoZone_SetFromSRID(m_pOriginalZone, srid);
    memcpy(m_pZone, m_pOriginalZone, sizeof(*m_pZone));
  }

  if (pNotes != nullptr && pNotes[0] != '\0')
  {
    m_pMetadata = udAllocType(udJSON, 1, udAF_Zero);
    m_pMetadata->Set("notes = '%s'", pNotes);
  }

  udStrcpy(m_typeStr, sizeof(m_typeStr), "POI");
  m_loadStatus = vcSLS_Loaded;
}
