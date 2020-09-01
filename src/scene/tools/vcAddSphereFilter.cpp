#include "vcAddSphereFilter.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAddSphereFilter vcAddSphereFilter::m_instance;

void vcAddSphereFilter::SceneUI(vcState *pProgramState)
{
  if ((pProgramState->sceneExplorer.selectedItems.size() == 1) && (pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData != nullptr))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData;
    pSceneItem->HandleSceneEmbeddedUI(pProgramState);
  }
}

void vcAddSphereFilter::HandlePicking(vcState* pProgramState, vcRenderData& /*renderData*/, const vcRenderPickResult& /*pickResult*/)
{
  vcQueryNodeFilter_HandleSceneInput(pProgramState, ImGui::IsMouseClicked(0, false));
}
