#include "vcUnsupportedNode.h"

#include "vcStrings.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcUnsupportedNode::vcUnsupportedNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Failed;
}

void vcUnsupportedNode::OnNodeUpdate(vcState * /*pProgramState*/)
{
  // Does nothing
}

void vcUnsupportedNode::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does nothing
}

void vcUnsupportedNode::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{
  // Does nothing
}

void vcUnsupportedNode::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextUnformatted(vcString::Get("sceneExplorerUnknownCustomNode"));
}

void vcUnsupportedNode::Cleanup(vcState * /*pProgramState*/)
{
  // Does Nothing
}

void vcUnsupportedNode::ChangeProjection(const udGeoZone & /*newZone*/)
{
  // Does Nothing
}

udDouble3 vcUnsupportedNode::GetLocalSpacePivot()
{
  return udDouble3::zero();
}
