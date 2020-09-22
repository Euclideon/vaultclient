#include "vcMeasureHeight.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcHotkey.h"
#include "vcStringFormat.h"
#include "vcVerticalMeasureTool.h"

#include "udStringUtil.h"

#include "imgui.h"

vcMeasureHeight vcMeasureHeight::m_instance;

void vcMeasureHeight::SceneUI(vcState *pProgramState)
{
  udProjectNode *pItem = pProgramState->sceneExplorer.clickedItem.pItem;
  if (pItem == nullptr)
  {
    ImGui::PushFont(pProgramState->pMidFont);
    ImGui::TextWrapped("%s", vcString::Get("toolMeasureStart"));
    ImGui::PopFont();
  }
  else if (pItem != nullptr && udStrEqual(pItem->itemtypeStr, "MHeight") && pProgramState->sceneExplorer.clickedItem.pItem->pUserData != nullptr)
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
  ImGui::Separator();
}

void vcMeasureHeight::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->pickingSuccess)
    return;

  udProjectNode *pItem = pProgramState->sceneExplorer.clickedItem.pItem;
  if (pItem != nullptr && udStrEqual(pItem->itemtypeStr, "MHeight"))
  {
    vcVerticalMeasureTool *pTool = (vcVerticalMeasureTool *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pTool->EndMeasure(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian);
  }
  else
  {
    vcProject_ClearSelection(pProgramState, false);
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "MHeight", vcString::Get("sceneVerticalMeasurementTool"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_LineString, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
    }
  }
}

void vcMeasureHeight::PreviewPicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  udProjectNode *pItem = pProgramState->sceneExplorer.clickedItem.pItem;
  if (pItem != nullptr && udStrEqual(pItem->itemtypeStr, "MHeight"))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    if (!pSceneItem->m_visible)
      pProgramState->activeTool = vcActiveTool_Select;

    vcVerticalMeasureTool *pTool = (vcVerticalMeasureTool *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pTool->Preview(pProgramState->pActiveViewport->worldMousePosCartesian);
  }
}
