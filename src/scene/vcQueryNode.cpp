#include "vcQueryNode.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "vdkQuery.h"

#include "imgui.h"


const char *GetNodeName(vcQueryNodeFilterShape shape)
{
  if(shape == vcQNFS_Box)
    return vcString::Get("sceneExplorerFilterBoxDefaultName");
  else if (shape == vcQNFS_Sphere)
    return vcString::Get("sceneExplorerFilterSphereDefaultName");
  else if (shape == vcQNFS_Cylinder)
    return vcString::Get("sceneExplorerFilterCylinderDefaultName");
  else
    return "";
}

const char *GetNodeShape(vcQueryNodeFilterShape shape)
{
  if (shape == vcQNFS_Box)
    return "box";
  else if (shape == vcQNFS_Sphere)
    return "sphere";
  else if (shape == vcQNFS_Cylinder)
    return "cylinder";
  else
    return "";
}

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
    vdkProjectNode_SetMetadataString(m_pNode, "shape", GetNodeShape(m_shape));
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

void vcQueryNode::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  ImGui::Text("%s: %.6f, %.6f, %.6f", vcString::Get("sceneFilterPosition"), m_center.x, m_center.y, m_center.z);
  ImGui::Text("%s: %.6f, %.6f, %.6f", vcString::Get("sceneFilterExtents"), m_extents.x, m_extents.y, m_extents.z);

  if (ImGui::Checkbox(udTempStr("%s##EmbeddedUI", vcString::Get("sceneFilterInverted")), &m_inverted))
    vdkQueryFilter_SetInverted(m_pFilter, m_inverted);

  if (vcQueryNodeFilter_IsDragActive(pProgramState))
  {
    ImGui::Text("%s: %.6f, %.6f, %.6f", vcString::Get("sceneFilterEndPosition"), pProgramState->filterInput.endPoint.x, pProgramState->filterInput.endPoint.y, pProgramState->filterInput.endPoint.z);
    udDouble3 d = pProgramState->filterInput.endPoint - pProgramState->filterInput.pickPoint;
    if (d.x < 0)
      ImGui::Text("%s %f", vcString::Get("sceneFilterWidthNegative"), d.x);

    if (d.y < 0)
      ImGui::Text("%s %f", vcString::Get("sceneFilterLengthNegative"), d.y);
  }
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

static const uint32_t HOLD_TICKS = 10;

void vcQueryNodeFilter_InitFilter(vcQueryNodeFilterInput *pFilter, vcQueryNodeFilterShape shape)
{
  pFilter->shape = shape;
  pFilter->pickPoint = udDouble3::zero();
  pFilter->size = udDouble3::create(2.f, 2.f, 2.f);
  pFilter->endPoint = udDouble3::zero();
  pFilter->pNode = nullptr;
  pFilter->holdCount = 0;
}

void vcQueryNodeFilter_Clear(vcQueryNodeFilterInput *pFilter)
{
  pFilter->shape = vcQNFS_None;
  pFilter->pickPoint = udDouble3::zero();
  pFilter->size = udDouble3::create(2.f, 2.f, 2.f);
  pFilter->endPoint = udDouble3::zero();
  pFilter->pNode = nullptr;
  pFilter->holdCount = 0;
}

vdkProjectNode *vcQueryNodeFilter_CreateNode(vcQueryNodeFilterInput *pFilter, vcState *pProgramState)
{
  vdkProjectNode *pNode = nullptr;
  if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "QFilter", GetNodeName(pFilter->shape), nullptr, nullptr) == vE_Success)
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, vdkPGT_Point, &pProgramState->worldMousePosCartesian, 1);
    vdkProjectNode_SetMetadataString(pNode, "shape", GetNodeShape(pFilter->shape));
    vdkProjectNode_SetMetadataDouble(pNode, "size.x", pFilter->size.x);
    vdkProjectNode_SetMetadataDouble(pNode, "size.y", pFilter->size.y);
    vdkProjectNode_SetMetadataDouble(pNode, "size.z", pFilter->size.z);
    udStrcpy(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID);
  }

  return pNode;
}

void vcQueryNodeFilter_Update(vcQueryNodeFilterInput *pFilter, vcState *pProgramState)
{
  if (pFilter->pNode)
  {
    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, pFilter->pNode, pProgramState->geozone, vdkPGT_Point, &pFilter->pickPoint, 1);
    vdkProjectNode_SetMetadataDouble(pFilter->pNode, "size.x", pFilter->size.x);
    vdkProjectNode_SetMetadataDouble(pFilter->pNode, "size.y", pFilter->size.y);
    vdkProjectNode_SetMetadataDouble(pFilter->pNode, "size.z", pFilter->size.z);
  }
}

void vcQueryNodeFilter_HandleSceneInput(vcState *pProgramState, bool isBtnHeld, bool isBtnReleased)
{
  vcQueryNodeFilterInput *pFilter = &pProgramState->filterInput;
  if (pProgramState->activeTool != vcActiveTool_AddBoxFilter && pProgramState->activeTool != vcActiveTool_AddSphereFilter && pProgramState->activeTool != vcActiveTool_AddCylinderFilter)
  {
    if (pFilter->shape != vcQNFS_None)
      vcQueryNodeFilter_Clear(&pProgramState->filterInput);
    return;
  }

  if (isBtnHeld)
  {
    // create node
    if (pProgramState->pickingSuccess)
    {
      if (!pFilter->pNode)
      {
        pFilter->pNode = vcQueryNodeFilter_CreateNode(pFilter, pProgramState);
        pFilter->pickPoint = pProgramState->worldMousePosCartesian;
      }
    }

    if (pFilter->pNode && !vcQueryNodeFilter_IsDragActive(pProgramState))
      pFilter->holdCount++;
  }

  if (pFilter->holdCount == 0)
    return;

  if (vcQueryNodeFilter_IsDragActive(pProgramState))
  {
    udDouble3 up = vcGIS_GetWorldLocalUp(pProgramState->geozone, pFilter->pickPoint);
    udPlane<double> plane = udPlane<double>::create(pFilter->pickPoint, up);

    pFilter->endPoint = {};
    if (plane.intersects(pProgramState->camera.worldMouseRay, &pFilter->endPoint, nullptr))
    {
      udDouble3 d = pFilter->endPoint - pFilter->pickPoint;
      pFilter->size.x = udMax(d.x, 2.0f) * 2;
      pFilter->size.y = udMax(d.y, 2.0f) * 2;
      pFilter->size.z = udMax(pFilter->size.x, pFilter->size.y);
    }

    vcQueryNodeFilter_Update(pFilter, pProgramState);

    if (isBtnReleased)
      vcQueryNodeFilter_InitFilter(pFilter, pFilter->shape);
  }
  else
  {
    if (isBtnReleased)
    {
      double scaleFactor = udMag3(pProgramState->camera.position - pProgramState->worldMousePosCartesian) / 10.0; // 1/10th of the screen
      pFilter->size = udDouble3::create(scaleFactor, scaleFactor, scaleFactor);
      vcQueryNodeFilter_Update(pFilter, pProgramState);
      vcQueryNodeFilter_InitFilter(pFilter, pFilter->shape);
      return;
    }
  }

}

bool vcQueryNodeFilter_IsDragActive(vcState *pProgramState)
{
  return pProgramState->filterInput.holdCount >= HOLD_TICKS;
}


