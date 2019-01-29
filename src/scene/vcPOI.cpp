#include "vcPOI.h"

#include "vcScene.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "gl/vcFenceRenderer.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

udFloat4 vcPOI_BGRAToImGui(uint32_t lineColour);

void vcPOI::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (pFence != nullptr)
    pRenderData->fences.PushBack(pFence);
}

void vcPOI::ApplyDelta(vcState * /*pProgramState*/)
{

}

void vcPOI::HandleImGui(vcState * /*pProgramState*/, size_t *pItemID)
{
  bool reConfig = false;

  vcIGSW_InputTextWithResize(vcString::Get("LabelName"), &pName, &nameBufferLength);
  vcIGSW_ColorPickerU32(udTempStr("%s##POIColor%zu", vcString::Get("LabelColour"), *pItemID), &nameColour, ImGuiColorEditFlags_None);

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
    if (ImGui::Combo(vcString::Get("LineStyle"), (int *)&line.lineStyle, lineOptions, (int)udLengthOf(lineOptions)))
      reConfig = true;

    if (reConfig)
    {
      vcFenceRendererConfig config;
      udFloat4 colour = vcPOI_BGRAToImGui(line.lineColour);
      config.visualMode = vcRRVM_Fence;
      config.imageMode = line.lineStyle;
      config.bottomColour = colour;
      config.topColour = colour;
      config.ribbonWidth = (float)line.lineWidth;
      config.textureScrollSpeed = 1.f;
      config.textureRepeatScale = 1.f;

      vcFenceRenderer_SetConfig(pFence, config);
    }
  }
}

void vcPOI::Cleanup(vcState * /*pProgramState*/)
{
  udFree(pName);
  udFree(line.pPoints);

  if (pFence != nullptr)
    vcFenceRenderer_Destroy(&pFence);

  this->vcPOI::~vcPOI();
}

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid)
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

  pPOI->sceneMatrix = udDouble4x4::translation(pLine->pPoints[0]);

  pPOI->line.selectedPoint = 0;
  pPOI->line.lineStyle = vcRRIM_Arrow;

  if (pLine->numPoints > 1)
  {
    vcFenceRenderer_Create(&pPOI->pFence);

    udFloat4 colours = vcPOI_BGRAToImGui(nameColour);

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

  if (srid != 0)
  {
    pPOI->pZone = udAllocType(udGeoZone, 1, udAF_Zero);
    udGeoZone_SetFromSRID(pPOI->pZone, srid);
  }

  udStrcpy(pPOI->typeStr, sizeof(pPOI->typeStr), "POI");
  pPOI->loadStatus = vcSLS_Loaded;

  pPOI->AddItem(pProgramState);
}

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, udDouble3 position, int32_t srid)
{
  vcLineInfo temp;
  temp.numPoints = 1;
  temp.pPoints = &position;
  temp.lineWidth = 1;
  temp.lineColour = 0xFFFFFFFF;

  vcPOI_AddToList(pProgramState, pName, nameColour, namePt, &temp, srid);
}

udFloat4 vcPOI_BGRAToImGui(uint32_t lineColour)
{
  //TODO: Find or add a math helper for this
  udFloat4 colours; // RGBA
  colours.x = ((((lineColour) >> 16) & 0xFF) / 255.f); // Red
  colours.y = ((((lineColour) >> 8) & 0xFF) / 255.f); // Green
  colours.z = ((((lineColour) >> 0) & 0xFF) / 255.f); // Blue
  colours.w = ((((lineColour) >> 24) & 0xFF) / 255.f); // Alpha

  return colours;
}
