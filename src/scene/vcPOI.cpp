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

void vcPOI::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  // if POI is invisible or if it exceeds maximum visible POI distance
  if (!visible || udMag3(pLabelInfo->worldPosition - pProgramState->pCamera->position) > pProgramState->settings.presentation.POIFadeDistance)
    return;

  if (pFence != nullptr)
    pRenderData->fences.PushBack(pFence);

  if (pLabelInfo != nullptr)
  {
    pLabelInfo->pText = pName;
    pRenderData->labels.PushBack(pLabelInfo);
  }
}

void vcPOI::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  line.pPoints[line.selectedPoint] = (delta * udDouble4x4::translation(line.pPoints[line.selectedPoint])).axis.t.toVector3();

  pLabelInfo->worldPosition = line.pPoints[0];

  if (pFence != nullptr)
  {
    vcFenceRenderer_ClearPoints(pFence);
    vcFenceRenderer_AddPoints(pFence, line.pPoints, line.numPoints, line.closed);
  }
}

void vcPOI::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  bool reConfig = false;

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIColor%zu", vcString::Get("LabelColour"), *pItemID), &nameColour, ImGuiColorEditFlags_None))
    pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(nameColour);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIBackColor%zu", vcString::Get("LabelBackgroundColour"), *pItemID), &backColour, ImGuiColorEditFlags_None))
    pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(backColour);

  bool lines = line.numPoints > 1;

  if (lines)
  {
    if (vcIGSW_ColorPickerU32(vcString::Get("LineColour"), &line.lineColour, ImGuiColorEditFlags_None))
      reConfig = true;

    if (ImGui::BeginCombo(vcString::Get("Points"), udTempStr("%s %zu", vcString::Get("Point"), line.selectedPoint + 1)))
    {
      for (size_t i = 1; i <= line.numPoints; ++i)
        if (ImGui::Selectable(udTempStr("%s %zu", vcString::Get("Point"), i)))
          line.selectedPoint = i - 1;

      ImGui::EndCombo();
    }
  }

  ImGui::TextWrapped("%s: %.2f, %.2f, %.2f", vcString::Get("Position"), line.pPoints[line.selectedPoint].x, line.pPoints[line.selectedPoint].y, line.pPoints[line.selectedPoint].z);

  if (lines)
  {
    // Length
    double length = 0;
    if (line.numPoints > 1)
    {
      if (line.selectedPoint == line.numPoints - 1)
      {
        if (line.closed)
          length = udMag3(line.pPoints[line.selectedPoint] - line.pPoints[0]);
      }
      else
      {
        length = udMag3(line.pPoints[line.selectedPoint] - line.pPoints[line.selectedPoint + 1]);
      }
    }
    ImGui::TextWrapped("%s: %.2f", vcString::Get("Length"), length);

    // Area, ignores Z axis
    if (line.closed)
    {
      double area = 0;
      size_t j = line.numPoints - 1;

      for (size_t i = 0; i < line.numPoints; i++)
      {
        area = area + (line.pPoints[j].x + line.pPoints[i].x) * (line.pPoints[j].y - line.pPoints[i].y);
        j = i;
      }
      area /= 2;

      ImGui::TextWrapped("%s: %.2f", vcString::Get("Area"), area);
    }

    if (ImGui::InputInt(vcString::Get("LineWidth"), (int *)&line.lineWidth))
      reConfig = true;

    const char *lineOptions[] = { vcString::Get("Arrow"), vcString::Get("Glow"), vcString::Get("Solid") };
    if (ImGui::Combo(vcString::Get("LineStyle"), (int *)&line.lineStyle, lineOptions, (int) udLengthOf(lineOptions)))
      reConfig = true;

    if (reConfig)
    {
      vcFenceRendererConfig config;
      udFloat4 colour = vcIGSW_BGRAToImGui(line.lineColour);
      config.visualMode = vcRRVM_Fence;
      config.imageMode = line.lineStyle;
      config.bottomColour = colour;
      config.topColour = colour;
      config.ribbonWidth = (float)line.lineWidth;
      config.textureScrollSpeed = 1.f;
      config.textureRepeatScale = 1.f;

      vcFenceRenderer_SetConfig(pFence, config);
    }

    // TODO: label renderer config too
  }

  // Handle hyperlinks
  const char *pHyperlink = pMetadata->Get("hyperlink").AsString();
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
  udFree(pName);
  udFree(line.pPoints);

  vcFenceRenderer_Destroy(&pFence);
  udFree(pLabelInfo);

  this->vcPOI::~vcPOI();
}

udDouble4x4 vcPOI::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(line.pPoints[0]);
}

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes /*= ""*/)
{
  vcPOI *pPOI = udAllocType(vcPOI, 1, udAF_Zero);
  pPOI = new (pPOI) vcPOI();
  pPOI->visible = true;

  pPOI->pName = udStrdup(pName);
  pPOI->type = vcSOT_PointOfInterest;

  pPOI->nameColour = nameColour;
  pPOI->namePt = namePt;

  memcpy(&pPOI->line, pLine, sizeof(pPOI->line));

  pPOI->line.pPoints = udAllocType(udDouble3, pLine->numPoints, udAF_Zero);
  memcpy(pPOI->line.pPoints, pLine->pPoints, sizeof(udDouble3) * pLine->numPoints);

  pPOI->line.selectedPoint = 0;
  pPOI->line.lineStyle = vcRRIM_Arrow;

  if (pLine->numPoints > 1)
  {
    vcFenceRenderer_Create(&pPOI->pFence);

    udFloat4 colours = vcIGSW_BGRAToImGui(nameColour);

    vcFenceRendererConfig config;
    config.visualMode = vcRRVM_Fence;
    config.imageMode = vcRRIM_Arrow;
    config.bottomColour = colours;
    config.topColour = colours;
    config.ribbonWidth = (float)pLine->lineWidth;
    config.textureScrollSpeed = 1.f;
    config.textureRepeatScale = 1.f;

    vcFenceRenderer_SetConfig(pPOI->pFence, config);
    vcFenceRenderer_AddPoints(pPOI->pFence, pLine->pPoints, pLine->numPoints);
  }

  pPOI->backColour = 0x7F000000;

  pPOI->pLabelInfo = udAllocType(vcLabelInfo, 1, udAF_Zero);
  pPOI->pLabelInfo->pText = pPOI->pName;
  pPOI->pLabelInfo->worldPosition = pLine->pPoints[0];
  pPOI->pLabelInfo->textSize = vcLFS_Medium;
  pPOI->pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(nameColour);
  pPOI->pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(pPOI->backColour);

  if (srid != 0)
  {
    pPOI->pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
    pPOI->pZone = udAllocType(udGeoZone, 1, udAF_Zero);
    udGeoZone_SetFromSRID(pPOI->pOriginalZone, srid);
    memcpy(pPOI->pZone, pPOI->pOriginalZone, sizeof(*pPOI->pZone));
  }

  if (pNotes != nullptr && pNotes[0] != '\0')
  {
    pPOI->pMetadata = udAllocType(udJSON, 1, udAF_Zero);
    pPOI->pMetadata->Set("notes = '%s'", pNotes);
  }

  udStrcpy(pPOI->typeStr, sizeof(pPOI->typeStr), "POI");
  pPOI->loadStatus = vcSLS_Loaded;

  pPOI->AddItem(pProgramState);
}

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, udDouble3 position, int32_t srid, const char *pNotes)
{
  vcLineInfo temp;
  temp.numPoints = 1;
  temp.pPoints = &position;
  temp.lineWidth = 1;
  temp.lineColour = 0xFFFFFFFF;

  vcPOI_AddToList(pProgramState, pName, nameColour, namePt, &temp, srid, pNotes);
}
