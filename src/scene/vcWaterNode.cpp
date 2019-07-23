#include "vcWaterNode.h"

#include "vcStrings.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcWater::vcWater(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Failed;
}

void vcWater::OnNodeUpdate(vcState * /*pProgramState*/)
{
  //TODO: Update items
}

void vcWater::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does nothing
}

void vcWater::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{

}

void vcWater::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextUnformatted(vcString::Get("sceneExplorerUnknownCustomNode"));
}

void vcWater::Cleanup(vcState * /*pProgramState*/)
{
  // Do stuff
}

void vcWater::ChangeProjection(const udGeoZone &newZone)
{
  udUnused(newZone);
}

udDouble3 vcWater::GetLocalSpacePivot()
{
  return udDouble3::zero();
}
