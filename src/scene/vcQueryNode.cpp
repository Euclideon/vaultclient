#include "vcQueryNode.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "vdkQuery.h"

#include "imgui.h"

vcQueryNode::vcQueryNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_shape(vcQNFS_Box),
  m_inverted(false),
  m_currentProjection(udDoubleQuat::identity()),
  m_center(udDouble3::zero()),
  m_extents(udDouble3::one()),
  m_ypr(udDouble3::zero()),
  m_pFilter(nullptr)
{
  m_loadStatus = vcSLS_Loaded;

  vdkQueryFilter_Create(&m_pFilter);
  vdkQueryFilter_SetAsBox(m_pFilter, &m_center.x, &m_extents.x, &m_ypr.x);

  OnNodeUpdate(pProgramState);
}

vcQueryNode::~vcQueryNode()
{
  vdkQueryFilter_Destroy(&m_pFilter);
}

void vcQueryNode::OnNodeUpdate(vcState *pProgramState)
{
  const char *pString = nullptr;

  if (vdkProjectNode_GetMetadataString(m_pNode, "shape", &pString, "box") == vE_Success)
  {
    if (udStrEquali(pString, "box"))
      m_shape = vcQNFS_Box;
    else if (udStrEquali(pString, "sphere"))
      m_shape = vcQNFS_Sphere;
    else if (udStrEquali(pString, "cylinder"))
      m_shape = vcQNFS_Cylinder;
  }

  vdkProjectNode_GetMetadataDouble(m_pNode, "size.x", &m_extents.x, 1.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "size.y", &m_extents.y, 1.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "size.z", &m_extents.z, 1.0);

  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &m_ypr.x, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &m_ypr.y, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.r", &m_ypr.z, 0.0);

  ChangeProjection(pProgramState->geozone);

  switch (m_shape)
  {
  case vcQNFS_Box:
    vdkQueryFilter_SetAsBox(m_pFilter, &m_center.x, &m_extents.x, &m_ypr.x);
    break;
  case vcQNFS_Cylinder:
    vdkQueryFilter_SetAsCylinder(m_pFilter, &m_center.x, m_extents.x, m_extents.z, &m_ypr.x);
    break;
  case vcQNFS_Sphere:
    vdkQueryFilter_SetAsSphere(m_pFilter, &m_center.x, m_extents.x);
    break;
  default:
    //Something went wrong...
    break;
  }
}

void vcQueryNode::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_selected)
  {
    vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();

    if (m_shape == vcQNFS_Box)
      pInstance->pModel = gInternalModels[vcInternalModelType_Cube];
    else if (m_shape == vcQNFS_Cylinder)
      pInstance->pModel = gInternalModels[vcInternalModelType_Cylinder];
    else if (m_shape == vcQNFS_Sphere)
      pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];

    pInstance->worldMat = this->GetWorldSpaceMatrix();
    pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
    pInstance->cullFace = vcGLSCM_Front;
    pInstance->pSceneItem = this;
    pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
    pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);
    pInstance->selectable = false;
  }

  if (m_pFilter != nullptr)
    pRenderData->pQueryFilter = m_pFilter;
}

void vcQueryNode::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  udDouble4x4 matrix = delta * udDouble4x4::rotationYPR(m_ypr, m_center) * udDouble4x4::scaleNonUniform(m_extents);

  m_ypr = matrix.extractYPR();
  m_center = matrix.axis.t.toVector3();
  m_extents = udDouble3::create(udMag3(matrix.axis.x), udMag3(matrix.axis.y), udMag3(matrix.axis.z));
  
  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_center, 1);

  vdkProjectNode_SetMetadataDouble(m_pNode, "size.x", m_extents.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "size.y", m_extents.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "size.z", m_extents.z);

  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_ypr.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_ypr.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", m_ypr.z);
}

void vcQueryNode::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  bool changed = false;

  const char *filterShapeNames[] = { vcString::Get("sceneFilterShapeBox"), vcString::Get("sceneFilterShapeCylinder"), vcString::Get("sceneFilterShapeSphere") };
  int shape = m_shape;
  if (ImGui::Combo(udTempStr("%s##FilterShape%zu", vcString::Get("sceneFilterShape"), *pItemID), &shape, filterShapeNames, (int)udLengthOf(filterShapeNames)))
  {
    changed = true;
    m_shape = (vcQueryNodeFilterShape)shape;

    if (m_shape == vcQNFS_Box)
      vdkProjectNode_SetMetadataString(m_pNode, "shape", "box");
    else if (m_shape == vcQNFS_Sphere)
      vdkProjectNode_SetMetadataString(m_pNode, "shape", "sphere");
    else if (m_shape == vcQNFS_Cylinder)
      vdkProjectNode_SetMetadataString(m_pNode, "shape", "cylinder");
  }

  ImGui::InputScalarN(udTempStr("%s##FilterPosition%zu", vcString::Get("sceneFilterPosition"), *pItemID), ImGuiDataType_Double, &m_center.x, 3);
  changed |= ImGui::IsItemDeactivatedAfterEdit();

  if (m_shape != vcQNFS_Sphere)
  {
    ImGui::InputScalarN(udTempStr("%s##FilterRotation%zu", vcString::Get("sceneFilterRotation"), *pItemID), ImGuiDataType_Double, &m_ypr.x, 3);
    changed |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::InputScalarN(udTempStr("%s##FilterExtents%zu", vcString::Get("sceneFilterExtents"), *pItemID), ImGuiDataType_Double, &m_extents.x, 3);
    changed |= ImGui::IsItemDeactivatedAfterEdit();
  }
  else // Is a sphere
  {
    ImGui::InputDouble(udTempStr("%s##FilterExtents%zu", vcString::Get("sceneFilterExtents"), *pItemID), &m_extents.x);
    changed |= ImGui::IsItemDeactivatedAfterEdit();
  }

  if (ImGui::Checkbox(udTempStr("%s##FilterInverted%zu", vcString::Get("sceneFilterInverted"), *pItemID), &m_inverted))
  {
    changed = true;
    vdkQueryFilter_SetInverted(m_pFilter, m_inverted);
  }

  if (changed)
  {
    vdkProjectNode_SetMetadataDouble(m_pNode, "size.x", m_extents.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "size.y", m_extents.y);
    vdkProjectNode_SetMetadataDouble(m_pNode, "size.z", m_extents.z);

    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_ypr.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_ypr.y);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", m_ypr.z);

    this->ApplyDelta(pProgramState, udDouble4x4::identity());

    //TODO: Save extents and rotation
  }
}

void vcQueryNode::HandleSceneEmbeddedUI(vcState * /*pProgramState*/)
{
  if (ImGui::Checkbox(udTempStr("%s##EmbeddedUI", vcString::Get("sceneFilterInverted")), &m_inverted))
    vdkQueryFilter_SetInverted(m_pFilter, m_inverted);
}

void vcQueryNode::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoint = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoint, &numPoints);
  if (numPoints == 1)
    m_center = pPoint[0];

  udFree(pPoint);

  udDoubleQuat qNewProjection = vcGIS_GetQuaternion(newZone, m_center);
  udDoubleQuat qScene = udDoubleQuat::create(m_ypr);
  m_ypr = (qNewProjection * (m_currentProjection.inverse() * qScene)).eulerAngles();
  m_currentProjection = qNewProjection;

  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_ypr.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_ypr.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", m_ypr.z);

  switch (m_shape)
  {
  case vcQNFS_Box:
    vdkQueryFilter_SetAsBox(m_pFilter, &m_center.x, &m_extents.x, &m_ypr.x);
    break;
  case vcQNFS_Cylinder:
    vdkQueryFilter_SetAsCylinder(m_pFilter, &m_center.x, m_extents.x, m_extents.z, &m_ypr.x);
    break;
  case vcQNFS_Sphere:
    vdkQueryFilter_SetAsSphere(m_pFilter, &m_center.x, m_extents.x);
    break;
  default:
    //Something went wrong...
    break;
  }
}

void vcQueryNode::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing to clean up
}

udDouble4x4 vcQueryNode::GetWorldSpaceMatrix()
{
  if (m_shape == vcQNFS_Sphere)
    return udDouble4x4::rotationYPR(m_ypr, m_center) * udDouble4x4::scaleUniform(m_extents.x);
  else if (m_shape == vcQNFS_Cylinder)
    return udDouble4x4::rotationYPR(m_ypr, m_center) * udDouble4x4::scaleNonUniform(udDouble3::create(m_extents.x, m_extents.x, m_extents.z));

  return udDouble4x4::rotationYPR(m_ypr, m_center) * udDouble4x4::scaleNonUniform(m_extents);
}

vcGizmoAllowedControls vcQueryNode::GetAllowedControls()
{
  if (m_shape == vcQNFS_Sphere)
    return (vcGizmoAllowedControls)(vcGAC_ScaleUniform | vcGAC_Translation | vcGAC_Rotation);
  else
    return vcGAC_All;
}
