#include "vcAddCylinderFilter.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAddCylinderFilter vcAddCylinderFilter::m_instance;

void vcAddCylinderFilter::SceneUI(vcState *pProgramState)
{
  if ((pProgramState->sceneExplorer.selectedItems.size() == 1) && (pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData != nullptr))
  {
    vcSceneItem *pSceneItem = (vcSceneItem *)pProgramState->sceneExplorer.selectedItems[0].pItem->pUserData;
    pSceneItem->HandleSceneEmbeddedUI(pProgramState);
  }
}
