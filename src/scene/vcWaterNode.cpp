#include "vcWaterNode.h"

#include "vcStrings.h"
#include "vcState.h"
#include "udStringUtil.h"

#include "vcWaterRenderer.h"
#include "vcRender.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcWater::vcWater(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_pWaterRenderer = nullptr;
  m_altitude = 0.0;

  vcWaterRenderer_Create(&m_pWaterRenderer, pProgramState->pWorkerPool); // TODO: This creates a water renderer for each water node...probably don't want to do this!
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

void vcWater::OnNodeUpdate(vcState *pProgramState)
{
  vdkProjectNode_GetMetadataDouble(m_pNode, "altitude", &m_altitude, 0.0);
  ChangeProjection(pProgramState->geozone);
}

void vcWater::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  pRenderData->waterVolumes.PushBack(m_pWaterRenderer);
}

void vcWater::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{

}

void vcWater::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t *pItemID)
{
  double min = -100.0;
  double max = 3500.0;
  if (ImGui::SliderScalar(udTempStr("%s##%zu", vcString::Get("waterAltitude"), *pItemID), ImGuiDataType_Double, &m_altitude, &min, &max))
    vdkProjectNode_SetMetadataDouble(m_pNode, "altitude", m_altitude);
}

void vcWater::Cleanup(vcState * /*pProgramState*/)
{
  vcWaterRenderer_Destroy(&m_pWaterRenderer);
}

void vcWater::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pWaterRenderer == nullptr)
    return;

  vcWaterRenderer_ClearAllVolumes(m_pWaterRenderer);

  m_pivot = udGeoZone_LatLongToCartesian(newZone, ((udDouble3*)m_pNode->pCoordinates)[0] + udDouble3::create(0.0, 0.0, m_altitude), true);

  // load islands
  std::vector< std::pair<const udDouble3 *, size_t> > islandPoints;
  if (m_pNode->pFirstChild != nullptr)
  {
    vdkProjectNode *pIslandNode = m_pNode->pFirstChild;

    do
    {
      islandPoints.push_back(std::make_pair(((udDouble3*)pIslandNode->pCoordinates), pIslandNode->geomCount));
      pIslandNode = pIslandNode->pNextSibling;
    } while (pIslandNode != nullptr);
  }

  vcWaterRenderer_AddVolume(m_pWaterRenderer, newZone, m_altitude, (udDouble3*)m_pNode->pCoordinates, m_pNode->geomCount, islandPoints);
}

udDouble3 vcWater::GetLocalSpacePivot()
{
  return m_pivot;
}
