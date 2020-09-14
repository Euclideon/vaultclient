#include "vcAddBoxFilter.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcQueryNode.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAddBoxFilter vcAddBoxFilter::m_instance;

void vcAddBoxFilter::SceneUI(vcState *pProgramState)
{
  if ((pProgramState->sceneExplorer.selectedItems.size() == 1) && (pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData != nullptr))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData;
    pSceneItem->HandleSceneEmbeddedUI(pProgramState);
  }
}

void vcAddBoxFilter::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->pickingSuccess)
    return;

  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && pProgramState->sceneExplorer.clickedItem.pItem->itemtype == udPNT_QueryFilter)
  {
    vcQueryNode *pQueryNode = (vcQueryNode *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pQueryNode->EndQuery(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian);
  }
  else
  {
    vcProject_ClearSelection(pProgramState, false);
    udProjectNode *pNode = nullptr;
    if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "QFilter", vcString::Get("sceneExplorerFilterBoxDefaultName"), nullptr, nullptr) == udE_Success)
    {
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
      udProjectNode_SetMetadataString(pNode, "shape", "box");
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
    }
  }
}

void vcAddBoxFilter::PreviewPicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->pickingSuccess)
    return;

  udProjectNode *pItem = pProgramState->sceneExplorer.clickedItem.pItem;
  if (pItem != nullptr && udStrEqual(pItem->itemtypeStr, "QFilter"))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    if (!pSceneItem->m_visible)
      pProgramState->activeTool = vcActiveTool_Select;

    vcQueryNode *pTool = (vcQueryNode *)pProgramState->sceneExplorer.clickedItem.pItem->pUserData;
    pTool->EndQuery(pProgramState, pProgramState->pActiveViewport->worldMousePosCartesian, true);
  }
}
