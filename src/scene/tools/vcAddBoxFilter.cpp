#include "vcAddBoxFilter.h"

#include "vcState.h"
#include "vcStrings.h"

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
