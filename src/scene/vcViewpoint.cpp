#include "vcViewpoint.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"

vcViewpoint::vcViewpoint(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

void vcViewpoint::OnNodeUpdate(vcState *pProgramState)
{
  vdkError result;

  result = vdkProjectNode_GetMetadataDouble(m_pNode, "transform.heading", &m_CameraHeadingPitch.x, 0.0);
  result = vdkProjectNode_GetMetadataDouble(m_pNode, "transform.pitch", &m_CameraHeadingPitch.y, 0.0);

  if (result != vE_Success)
  {
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.x", &m_CameraHeadingPitch.x, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &m_CameraHeadingPitch.y, 0.0);
  }

  ChangeProjection(pProgramState->geozone);
}

void vcViewpoint::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does Nothing
}

void vcViewpoint::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_CameraPosition = (delta * udDouble4x4::translation(m_CameraPosition)).axis.t.toVector3();

  udDouble3 sumRotation = delta.extractYPR() + udDouble3::create(m_CameraHeadingPitch, 0.0);

  // Clamped this to the same limitations as the camera
  m_CameraHeadingPitch = udDouble2::create(udMod(sumRotation.x, UD_2PI), udClampWrap(sumRotation.y, -UD_HALF_PI, UD_HALF_PI));

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_CameraPosition, 1);
}

void vcViewpoint::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  bool changed = false;

  ImGui::InputScalarN(udTempStr("%s##ViewpointPosition%zu", vcString::Get("sceneViewpointPosition"), *pItemID), ImGuiDataType_Double, &m_CameraPosition.x, 3);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  ImGui::InputScalarN(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), ImGuiDataType_Double, &m_CameraHeadingPitch.x, 2);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    changed = true;
    m_CameraHeadingPitch = pProgramState->camera.headingPitch;
    m_CameraPosition = pProgramState->camera.position;
  }

  if (changed)
  {
    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_CameraPosition, 1);

    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.heading", m_CameraHeadingPitch.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.pitch", m_CameraHeadingPitch.y);
  }
}

void vcViewpoint::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    m_CameraHeadingPitch = pProgramState->camera.headingPitch;
    m_CameraPosition = pProgramState->camera.position;

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_CameraPosition, 1);

    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.heading", m_CameraHeadingPitch.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.pitch", m_CameraHeadingPitch.y);
  }
}

void vcViewpoint::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoint = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoint, &numPoints);
  if (numPoints == 1)
    m_CameraPosition = pPoint[0];

  udFree(pPoint);
}

void vcViewpoint::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing to clean up
}

void vcViewpoint::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->camera.position = m_CameraPosition;
  pProgramState->camera.headingPitch = m_CameraHeadingPitch;
}

udDouble4x4 vcViewpoint::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_CameraPosition);
}
