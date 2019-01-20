#include "vcPOI.h"

#include "vcScene.h"
#include "vcState.h"

#include "gl/vcFenceRenderer.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

void vcPOI_Cleanup(vcState * /*pProgramState*/, vcSceneItem *pBaseItem)
{
  vcPOI *pPOI = (vcPOI*)pBaseItem;
  udFree(pPOI->pName);

  if (pPOI->pFence != nullptr)
    vcFenceRenderer_Destroy(&pPOI->pFence);
}

void vcPOI_ShowImGui(vcState * /*pProgramState*/, vcSceneItem *pBaseItem)
{
  vcPOI *pPOI = (vcPOI*)pBaseItem;

  vcIGSW_InputTextWithResize("Label Name", &pPOI->pName, &pPOI->nameBufferLength);
  vcIGSW_ColorPickerU32("Label Colour", &pPOI->nameColour, ImGuiColorEditFlags_None);
}

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid)
{
  vcPOI *pPOI = udAllocType(vcPOI, 1, udAF_Zero);
  pPOI->visible = true;

  pPOI->pName = udStrdup(pName);
  pPOI->type = vcSOT_PointOfInterest;

  pPOI->pCleanupFunc = vcPOI_Cleanup;
  pPOI->pImGuiFunc = vcPOI_ShowImGui;

  pPOI->nameColour = nameColour;
  pPOI->namePt = namePt;

  memcpy(&pPOI->line, pLine, sizeof(pPOI->line));

  pPOI->sceneMatrix = udDouble4x4::translation(pLine->pPoints[0]);

  if (pLine->numPoints > 1)
  {
    vcFenceRenderer_Create(&pPOI->pFence);

    //TODO: Find or add a math helper for this
    udFloat4 colours; // RGBA
    colours.x = ((((pLine->lineColour) >> 16) & 0xFF) / 255.f); // Red
    colours.y = ((((pLine->lineColour) >> 8) & 0xFF) / 255.f); // Green
    colours.z = ((((pLine->lineColour) >> 0) & 0xFF) / 255.f); // Blue
    colours.w = ((((pLine->lineColour) >> 24) & 0xFF) / 255.f); // Alpha

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

  pProgramState->sceneList.push_back(pPOI);
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
