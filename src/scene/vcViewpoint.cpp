#include "vcViewpoint.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"

vcViewpoint::vcViewpoint(vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pNode, pProgramState)
{
  m_CameraPosition = pProgramState->pCamera->position;
  m_CameraRotation = pProgramState->pCamera->eulerRotation;

  if (pNode->pCoordinates == nullptr)
    pNode->pCoordinates = (double*)udAllocType(udDouble3, 1, udAF_Zero);

  if (pProgramState->gis.isProjected)
  {
    m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    memcpy(m_pPreferredProjection, &pProgramState->gis.zone, sizeof(udGeoZone));
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    memcpy(m_pCurrentProjection, &pProgramState->gis.zone, sizeof(udGeoZone));
    udDouble3 temp = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);
    vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, (double*)&temp);
  }
  else
  {
    udDouble3 temp = udGeoZone_ToLatLong(pProgramState->defaultGeo, m_CameraPosition, true);
    vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pNode, vdkPGT_Point, 1, (double*)&m_CameraPosition);
  }

  m_loadStatus = vcSLS_Loaded;
}

void vcViewpoint::AddToScene(vcState *pProgramState, vcRenderData * /*pRenderData*/)
{
  //TODO: Super hacky- remove later
  m_pProject = pProgramState->sceneExplorer.pProject;
}

void vcViewpoint::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_CameraPosition = (delta * udDouble4x4::translation(m_CameraPosition)).axis.t.toVector3();

  udDouble3 sumRotation = delta.extractYPR() + m_CameraRotation;
  m_CameraRotation = udDouble3::create(udMod(sumRotation.x, UD_2PI), udClampWrap(sumRotation.y, -UD_HALF_PI, UD_HALF_PI), udMod(sumRotation.z, UD_2PI));
  // Clamped this to the same limitations as the camera

  if (m_pCurrentProjection == nullptr)
    return; // Can't update if we aren't sure what zone we're in currently

  udDouble3 temp = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);
  vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, m_pNode, vdkPGT_Point, 1, (double*)&temp);
}

void vcViewpoint::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  if (ImGui::InputScalarN(udTempStr("%s##ViewpointPosition%zu", vcString::Get("sceneViewpointPosition"), *pItemID), ImGuiDataType_Double, &m_CameraPosition.x, 3))
  {
    if (m_pCurrentProjection == nullptr)
    {
      udDouble3 temp = udGeoZone_ToLatLong(pProgramState->defaultGeo, m_CameraPosition, true);
      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, m_pNode, vdkPGT_Point, 1, (double*)&temp);
    }
    else
    {
      udDouble3 temp = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);
      vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, m_pNode, vdkPGT_Point, 1, (double*)&temp);
    }
  }
  ImGui::InputScalarN(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), ImGuiDataType_Double, &m_CameraRotation.x, 3);
  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    m_CameraRotation = pProgramState->pCamera->eulerRotation;
    m_CameraPosition = pProgramState->pCamera->position;

    if (m_pCurrentProjection != nullptr)
      *(udDouble3*)m_pNode->pCoordinates = udGeoZone_ToLatLong(*m_pCurrentProjection, m_CameraPosition, true);

    // If current projection is null, it will be assigned here
    if (pProgramState->gis.isProjected)
      ChangeProjection(pProgramState->gis.zone);
  }
}

void vcViewpoint::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pCurrentProjection != nullptr && m_pCurrentProjection->srid == newZone.srid)
    return;

  udDouble3 *pLatLong;

  if (m_pCurrentProjection == nullptr)
    pLatLong = &m_CameraPosition;
  else
    pLatLong = (udDouble3*)m_pNode->pCoordinates;

  if (pLatLong->y < newZone.latLongBoundMin.x || pLatLong->y > newZone.latLongBoundMax.x || pLatLong->x < newZone.latLongBoundMin.y || pLatLong->x > newZone.latLongBoundMax.y)
    return;

  if (m_pCurrentProjection == nullptr)
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);

  memcpy(m_pCurrentProjection, &newZone, sizeof(udGeoZone));
  m_CameraPosition = udGeoZone_ToCartesian(newZone, *pLatLong, true);
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
