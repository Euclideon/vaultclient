#include "vcAddSimpleCrossSection.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcCrossSectionNode.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAddSimpleCrossSection vcAddSimpleCrossSection::m_instance;

void vcAddSimpleCrossSection::SceneUI(vcState *pProgramState)
{
  if ((pProgramState->sceneExplorer.selectedItems.size() == 1) && (pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData != nullptr))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData;
    pSceneItem->HandleSceneEmbeddedUI(pProgramState);
  }
}

void vcAddSimpleCrossSection::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->isMouseOver)
    return;

  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && udStrEqual(pProgramState->sceneExplorer.clickedItem.pItem->itemtypeStr, "XSlice"))
  {
    vcCrossSectionNode *pCrossSectionNode = (vcCrossSectionNode*)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pCrossSectionNode->EndQuery(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian);
  }
  else
  {
    vcProject_ClearSelection(pProgramState, false);
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "XSlice", vcString::Get("sceneExplorerFilterCrossSectionDefaultName"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
    }
  }
}

void vcAddSimpleCrossSection::PreviewPicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->isMouseOver)
    return;

  udProjectNode *pItem = pProgramState->sceneExplorer.clickedItem.pItem;
  if (pItem != nullptr && udStrEqual(pItem->itemtypeStr, "XSlice"))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    if (!pSceneItem->m_visible)
      pProgramState->activeTool = vcActiveTool_Select;

    vcCrossSectionNode *pTool = (vcCrossSectionNode *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pTool->EndQuery(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian, true);
  }
}
