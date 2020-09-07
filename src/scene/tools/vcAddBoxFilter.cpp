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

  vcQueryNodeFilter_HandleSceneInput(pProgramState, true);
}

void vcAddBoxFilter::PreviewPicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  if (!pProgramState->pActiveViewport->pickingSuccess)
    return;

  vcQueryNodeFilter_HandleSceneInput(pProgramState, false);
}
