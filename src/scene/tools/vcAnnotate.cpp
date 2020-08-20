#include "vcAnnotate.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udStringUtil.h"

#include "imgui.h"

vcAnnotate vcAnnotate::m_instance;

vcAnnotate::vcAnnotate()
  : vcSceneTool(vcActiveTool_Annotate)
  , m_buffer("")
{

}

void vcAnnotate::SceneUI(vcState * /*pProgramState*/)
{
  ImGui::Separator();

  static bool isFirst = true;
  if (isFirst)
  {
    udSprintf(m_buffer, "%s", vcString::Get("toolAnnotateDefaultText"));
    isFirst = false;
  }

  ImGui::InputText(vcString::Get("toolAnnotatePrompt"), m_buffer, m_bufferSize);
}

void vcAnnotate::HandlePicking(vcState *pProgramState, vcRenderData & /*renderData*/, const vcRenderPickResult & /*pickResult*/)
{
  vcProject_ClearSelection(pProgramState, false);
  udProjectNode *pNode = nullptr;

  if (udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "POI", m_buffer, nullptr, nullptr) == udE_Success)
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &pProgramState->pActiveViewport->worldMousePosCartesian, 1);
    udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
  }
}
