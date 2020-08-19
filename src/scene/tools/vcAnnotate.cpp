#include "vcAnnotate.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAnnotate vcAnnotate::m_instance;

void vcAnnotate::SceneUI(vcState *pProgramState)
{
  ImGui::Separator();
  static size_t const bufSize = 64;
  static bool isFirst = true;
  static char buf[bufSize] = {};

  if (isFirst)
  {
    udSprintf(buf, "%s", vcString::Get("toolAnnotateDefaultText"));
    isFirst = false;
  }

  ImGui::InputText(vcString::Get("toolAnnotatePrompt"), buf, bufSize);
  if (pProgramState->sceneExplorer.clickedItem.pItem != nullptr && pProgramState->sceneExplorer.clickedItem.pItem->itemtype == udPNT_PointOfInterest && pProgramState->sceneExplorer.clickedItem.pItem->pUserData != nullptr)
  {
    ImGui::Separator();
    udProjectNode_SetName(pProgramState->activeProject.pProject, pProgramState->sceneExplorer.clickedItem.pItem, buf);
  }
}

void vcAnnotate::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  vcProject_ClearSelection(pProgramState, false);
  udProjectNode *pNode = nullptr;

  if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", vcString::Get("toolAnnotateDefaultText"), nullptr, nullptr) == udE_Success)
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
    udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
  }
}
