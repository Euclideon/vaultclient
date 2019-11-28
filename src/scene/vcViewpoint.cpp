#include "vcViewpoint.h"

#include "vcState.h"
#include "vcStrings.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"

vcViewpoint::vcViewpoint(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

void vcViewpoint::OnNodeUpdate(vcState *pProgramState)
{
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.x", &m_CameraRotation.x, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &m_CameraRotation.y, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.z", &m_CameraRotation.z, 0.0);

  ChangeProjection(pProgramState->gis.zone);
}

void vcViewpoint::AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/)
{
  // Does Nothing
}

void vcViewpoint::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_CameraPosition = (delta * udDouble4x4::translation(m_CameraPosition)).axis.t.toVector3();

  udDouble3 sumRotation = delta.extractYPR() + m_CameraRotation;

  // Clamped this to the same limitations as the camera
  m_CameraRotation = udDouble3::create(udMod(sumRotation.x, UD_2PI), udClampWrap(sumRotation.y, -UD_HALF_PI, UD_HALF_PI), udMod(sumRotation.z, UD_2PI));

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, vdkPGT_Point, &m_CameraPosition, 1);
}

void vcViewpoint::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  bool changed = false;

  changed |= ImGui::InputScalarN(udTempStr("%s##ViewpointPosition%zu", vcString::Get("sceneViewpointPosition"), *pItemID), ImGuiDataType_Double, &m_CameraPosition.x, 3);
  changed |= ImGui::InputScalarN(udTempStr("%s##ViewpointRotation%zu", vcString::Get("sceneViewpointRotation"), *pItemID), ImGuiDataType_Double, &m_CameraRotation.x, 3);

  if (ImGui::Button(vcString::Get("sceneViewpointSetCamera")))
  {
    changed = true;
    m_CameraRotation = pProgramState->camera.eulerRotation;
    m_CameraPosition = pProgramState->camera.position;
  }

  if (changed)
  {
    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, vdkPGT_Point, &m_CameraPosition, 1);

    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.x", m_CameraRotation.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_CameraRotation.y);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.z", m_CameraRotation.z);
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
  pProgramState->camera.eulerRotation = m_CameraRotation;
}

udDouble4x4 vcViewpoint::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_CameraPosition);
}
