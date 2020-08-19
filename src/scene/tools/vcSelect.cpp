#include "vcSelect.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udStringUtil.h"

#include "imgui.h"

vcSelect vcSelect::m_instance;

void vcSelect::SceneUI(vcState *pProgramState)
{
  if ((pProgramState->sceneExplorer.selectedItems.size() == 1) && (pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData != nullptr))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData;
    pSceneItem->HandleSceneEmbeddedUI(pProgramState);
  }
  else
  {
    ImGui::Text("%zu %s", pProgramState->sceneExplorer.selectedItems.size(), vcString::Get("selectedItemInfoPanelitemsSelected"));
  }
}

void vcSelect::HandlePicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult &pickResult)
{
  if (pProgramState->pActiveViewport->pickingSuccess && (pProgramState->pActiveViewport->udModelPickedIndex != -1))
  {
    if (pProgramState->pActiveViewport->udModelPickedIndex < (int)renderData.models.length)
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, renderData.models[pProgramState->pActiveViewport->udModelPickedIndex]->m_pNode->UUID);
  }
  else if (pProgramState->pActiveViewport->pickingSuccess)
  {
    if (pickResult.pSceneItem != nullptr)
    {
      udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pickResult.pSceneItem->m_pNode->UUID);
      pickResult.pSceneItem->SelectSubitem(pickResult.sceneItemInternalId);
    }
    else
    {
      vcProject_ClearSelection(pProgramState);
    }
  }
  else
  {
    vcProject_ClearSelection(pProgramState);
  }
}
