#include "vcMeasureArea.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcHotkey.h"
#include "vcStringFormat.h"
#include "vcPOI.h"

#include "udStringUtil.h"

#include "imgui.h"

vcMeasureArea vcMeasureArea::m_instance;

void vcMeasureArea::SceneUI(vcState *pProgramState)
{
  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && pProgramState->sceneExplorer.clickedItem.pItem->itemtype == udPNT_PointOfInterest && pProgramState->sceneExplorer.clickedItem.pItem->pUserData != nullptr)
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;

    char bufferA[128];
    char bufferB[128];
    vcHotkey::GetKeyName(vcB_Cancel, bufferB);

    ImGui::PushFont(pProgramState->pMidFont);
    ImGui::TextWrapped("%s", vcStringFormat(bufferA, udLengthOf(bufferA), vcString::Get("toolMeasureNext"), bufferB));
    ImGui::PopFont();

    ImGui::Separator();

    pSceneItem->HandleToolUI(pProgramState);
  }
  else
  {
    ImGui::PushFont(pProgramState->pMidFont);
    ImGui::TextWrapped("%s", vcString::Get("toolMeasureStart"));
    ImGui::PopFont();
  }
}

void vcMeasureArea::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  //Are these checks necessary? If a tool is being used would that not suggest we indeed have a valid clickedItem?
  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && pProgramState->sceneExplorer.clickedItem.pItem->itemtype == udPNT_PointOfInterest)
  {
    vcPOI *pPOI = (vcPOI *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pPOI->AddPoint(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian);
  }
  else
  {
    vcProject_ClearSelection(pProgramState, false);
    udProjectNode *pNode = nullptr;

    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("scenePOIAreaDefaultName"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
      udProjectNode_SetMetadataBool(pNode, "showArea", true);
      udProjectNode_SetMetadataBool(pNode, "showFill", true);
    }
  }
}

void vcMeasureArea::PreviewPicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && pProgramState->sceneExplorer.clickedItem.pItem->itemtype == udPNT_PointOfInterest)
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;

    if (!pSceneItem->m_visible)
      pProgramState->activeTool = vcActiveTool_Select;

    // Preview Point
    vcPOI *pPOI = (vcPOI *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pPOI->AddPoint(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian, true);
  }
}
