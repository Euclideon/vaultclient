#include "vcViewpoint.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"

vcViewpoint::vcViewpoint(vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pNode)
{
  udUnused(pProgramState);

  if (pNode->pCoordinates && m_pCurrentProjection)
    m_CameraPosition = udGeoZone_ToCartesian(*m_pCurrentProjection, *(udDouble3*)pNode->pCoordinates, true);
  else
    m_CameraPosition = udDouble3::zero();
  m_CameraRotation = udDouble3::zero();
}

void vcViewpoint::AddToScene(vcState *pProgramState, vcRenderData * /*pRenderData*/)
{
  //TODO: Super hacky- remove later
  m_pProject = pProgramState->sceneExplorer.pProject;
}

void vcViewpoint::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_CameraPosition = (delta * udDouble4x4::translation(m_CameraPosition)).axis.t.toVector3();

  udDouble3 sumRotation = delta.extractYPR() + m_CameraRotation;
  m_CameraRotation = udDouble3::create(udMod(sumRotation.x, UD_2PI), udClampWrap(sumRotation.y, -UD_HALF_PI, UD_HALF_PI), udMod(sumRotation.z, UD_2PI));
  // Clamped this to the same limitations as the camera

  if (m_pCurrentProjection == nullptr || m_pNode->pCoordinates == nullptr)
    return; // Can't update if we aren't sure what zone we're in currently

  *(udDouble3*)m_pNode->pCoordinates = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);
}

void vcViewpoint::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  if (ImGui::InputScalarN(udTempStr("%s##ViewpointPosition%zu", vcString::Get("sceneViewpointPosition"), *pItemID), ImGuiDataType_Double, &m_CameraPosition.x, 3))
    *(udDouble3*)m_pNode->pCoordinates = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);

  ImGui::InputScalarN(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), ImGuiDataType_Double, &m_CameraRotation.x, 3);
  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    m_CameraPosition = pProgramState->pCamera->position;
    m_CameraRotation = pProgramState->pCamera->eulerRotation;
    if (m_pCurrentProjection == nullptr)
      m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    *m_pCurrentProjection = pProgramState->gis.zone;
  }
}

void vcViewpoint::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pCurrentProjection == nullptr)
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);

  m_CameraPosition = udGeoZone_ToCartesian(newZone, *(udDouble3*)m_pNode->pCoordinates, true);

  // Call the parent version
  vcSceneItem::ChangeProjection(newZone);
}

void vcViewpoint::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing to clean up
}

void vcViewpoint::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->pCamera->position = m_CameraPosition;
  pProgramState->pCamera->eulerRotation = m_CameraRotation;
}

udDouble4x4 vcViewpoint::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_CameraPosition);
}
