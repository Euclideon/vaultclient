#include "vcCrossSectionNode.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h"
#include "vcInternalModels.h"
#include "vcProject.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "../imgui_ex/vcImGuiSimpleWidgets.h"

vcCrossSectionNode::vcCrossSectionNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_inverted(false),
  m_currentProjectionHeadingPitch(udDoubleQuat::identity()),
  m_position(),
  m_headingPitch(),
  m_distance(0),
  m_geometry(),
  m_pFilter(nullptr)
{
  m_loadStatus = vcSLS_Loaded;

  OnNodeUpdate(pProgramState);
}

vcCrossSectionNode::~vcCrossSectionNode()
{
  udQueryFilter_Destroy(&m_pFilter);
}

void vcCrossSectionNode::OnNodeUpdate(vcState *pProgramState)
{
  udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.h", &m_headingPitch.x, 0.0);
  udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &m_headingPitch.y, 0.0);

  udProjectNode_GetMetadataDouble(m_pNode, "width", &m_distance, 10.0);
  vcProject_GetNodeMetadata(m_pNode, "inverted", &m_inverted, false);

  ChangeProjection(pProgramState->geozone);
}

void vcCrossSectionNode::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_pFilter != nullptr)
    pRenderData->pQueryFilter = m_pFilter;
}

void vcCrossSectionNode::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  udDouble4x4 matrix = delta * GetWorldSpaceMatrix();

  udDouble3 ypr = matrix.extractYPR();
  m_position = matrix.axis.t.toVector3();

  m_headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, m_position, udDoubleQuat::create(ypr));

  UpdateFilters();

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_position, 1);
  udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.h", m_headingPitch.x);
  udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_headingPitch.y);
}

void vcCrossSectionNode::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  bool changed = false;

  udDouble3 positionTemp = m_position;
  ImGui::InputScalarN(udTempStr("%s##FilterPosition%zu", vcString::Get("sceneFilterPosition"), *pItemID), ImGuiDataType_Double, &positionTemp.x, 3);
  if (ImGui::IsItemDeactivatedAfterEdit())
  {
    for (int i = 0; i < 3; ++i)
    {
      if (isfinite(positionTemp[i]))
        m_position[i] = positionTemp[i];
    }

    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_position, 1);
  }

  if (vcIGSW_DegreesScalar(udTempStr("%s##FilterRotation%zu", vcString::Get("sceneFilterRotation"), *pItemID), &m_headingPitch))
  {
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.h", m_headingPitch.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_headingPitch.y);
  }
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  const double MinDist = 0.0;
  const double MaxDist = 1000.0;
  if (ImGui::SliderScalar(udTempStr("%s##FilterRotation%zu", vcString::Get("sceneCrossSectionDistance"), *pItemID), ImGuiDataType_Double, &m_distance, &MinDist, &MaxDist))
  {
    changed = true;
    udProjectNode_SetMetadataDouble(m_pNode, "width", m_distance);
  }

  if (ImGui::Checkbox(udTempStr("%s##FilterInverted%zu", vcString::Get("sceneFilterInverted"), *pItemID), &m_inverted))
  {
    changed = true;
    udProjectNode_SetMetadataBool(m_pNode, "inverted", m_inverted ? 1 : 0);
  }

  if (changed)
    UpdateFilters();
}

void vcCrossSectionNode::HandleSceneEmbeddedUI(vcState * /*pProgramState*/)
{
  if (ImGui::Checkbox(udTempStr("%s##EmbeddedUI", vcString::Get("sceneFilterInverted")), &m_inverted))
  {
    udProjectNode_SetMetadataBool(m_pNode, "inverted", m_inverted ? 1 : 0);
    UpdateFilters();
  }
}

vcMenuBarButtonIcon vcCrossSectionNode::GetSceneExplorerIcon()
{
  return vcMBBI_AddCrossSectionFilter;
}

void vcCrossSectionNode::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoint = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoint, &numPoints);
  if (numPoints == 1)
    m_position = pPoint[0];

  udFree(pPoint);

  m_currentProjectionHeadingPitch = vcGIS_HeadingPitchToQuaternion(newZone, m_position, m_headingPitch);
  UpdateFilters();
}

void vcCrossSectionNode::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing to clean up
}

udDouble4x4 vcCrossSectionNode::GetWorldSpaceMatrix()
{
  return udDouble4x4::rotationQuat(m_currentProjectionHeadingPitch, m_position) * udDouble4x4::scaleUniform(10000000.0);
}

vcGizmoAllowedControls vcCrossSectionNode::GetAllowedControls()
{
  return (vcGizmoAllowedControls)(vcGAC_Translation | vcGAC_Rotation);
}

inline udGeometryDouble3 &udGeometry_FromMath(const udDouble3 &v) { return *((udGeometryDouble3 *)&v); }

void vcCrossSectionNode::UpdateFilters()
{
  if (m_pFilter == nullptr)
  {
    udQueryFilter_Create(&m_pFilter);
    udQueryFilter_SetFromGeometry(m_pFilter, &m_geometry[0]);
  }

  udDouble3 normalA = m_currentProjectionHeadingPitch.apply({ 0, 1, 0 });
  udDouble3 normalB = -normalA;

  udDouble3 halfPlane2 = (m_position + normalA * m_distance);

  if (!m_inverted)
  {
    udGeometry_InitCSG(&m_geometry[0], &m_geometry[1], &m_geometry[2], udGCSGO_Intersection);
    udGeometry_InitHalfSpace(&m_geometry[1], udGeometry_FromMath(m_position), udGeometry_FromMath(normalA));
    udGeometry_InitHalfSpace(&m_geometry[2], udGeometry_FromMath(halfPlane2), udGeometry_FromMath(normalB));
  }
  else
  {
    udGeometry_InitCSG(&m_geometry[0], &m_geometry[1], &m_geometry[2], udGCSGO_Union);
    udGeometry_InitHalfSpace(&m_geometry[1], udGeometry_FromMath(m_position), udGeometry_FromMath(normalB));
    udGeometry_InitHalfSpace(&m_geometry[2], udGeometry_FromMath(halfPlane2), udGeometry_FromMath(normalA));
  }
}

void vcCrossSectionNode::EndQuery(vcState *pProgramState, const udDouble3 & /*position*/, bool isPreview /*= false*/)
{
  udDouble3 up = vcGIS_GetWorldLocalUp(pProgramState->geozone, m_position);
  udPlane<double> plane = udPlane<double>::create(m_position, up);
  
  udDouble3 endPoint = {};
  if (plane.intersects(pProgramState->pActiveViewport->camera.worldMouseRay, &endPoint, nullptr))
  {
    udDouble3 horizDist = endPoint - m_position; // On horiz plane so vert is always 0

    m_currentProjectionHeadingPitch = udDoubleQuat::create(udDirectionToYPR(horizDist));
    m_headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, m_position, m_currentProjectionHeadingPitch);

    m_distance = udMag3(horizDist);

    UpdateFilters();
  }

  if (!isPreview)
  {
    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, udPGT_Point, &m_position, 1);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.h", m_headingPitch.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_headingPitch.y);
    udProjectNode_SetMetadataDouble(m_pNode, "width", m_distance);
    udProjectNode_SetMetadataBool(m_pNode, "inverted", m_inverted ? 1 : 0);

    pProgramState->activeTool = vcActiveTool::vcActiveTool_Select;
  }
}
