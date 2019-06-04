#include "vcUnsupportedNode.h"

#include "vcStrings.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcUnsupportedNode::vcUnsupportedNode(vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pNode, pProgramState)
{
  m_loadStatus = vcSLS_Failed;
}

void vcUnsupportedNode::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does nothing
}

void vcUnsupportedNode::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{

}

void vcUnsupportedNode::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextUnformatted(vcString::Get("sceneExplorerUnknownCustomNode"));
}

void vcUnsupportedNode::Cleanup(vcState * /*pProgramState*/)
{
  // Do stuff
}

void vcUnsupportedNode::ChangeProjection(const udGeoZone &newZone)
{
  udUnused(newZone);
}

udDouble3 vcUnsupportedNode::GetLocalSpacePivot()
{
  return udDouble3::zero();
}
