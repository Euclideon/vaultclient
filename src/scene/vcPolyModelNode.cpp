#include "vcPolyModelNode.h"

#include "vcPolygonModel.h"
#include "vcStrings.h"
#include "vcRender.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

struct vcPolygonModelLoadInfo
{
  vcState *pProgramState;
  vcPolyModelNode *pNode;
};

void vcPolyModelNode_LoadModel(void *pLoadInfoPtr)
{
  vcPolygonModelLoadInfo *pLoadInfo = (vcPolygonModelLoadInfo *)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

  udInterlockedCompareExchange(&pLoadInfo->pNode->m_loadStatus, vcSLS_Loading, vcSLS_Pending);
  udResult result = vcPolygonModel_CreateFromURL(&pLoadInfo->pNode->m_pModel, pLoadInfo->pNode->m_pNode->pURI, pLoadInfo->pProgramState->pWorkerPool);

  if (result == udR_OpenFailure)
    result = vcPolygonModel_CreateFromURL(&pLoadInfo->pNode->m_pModel, udTempStr("%s%s", pLoadInfo->pProgramState->activeProject.pRelativeBase, pLoadInfo->pNode->m_pNode->pURI), pLoadInfo->pProgramState->pWorkerPool);

  udInterlockedCompareExchange(&pLoadInfo->pNode->m_loadStatus, (result == udR_Success ? vcSLS_Loaded : vcSLS_Failed), vcSLS_Loading);
}

vcPolyModelNode::vcPolyModelNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_pModel = nullptr;
  m_matrix = udDouble4x4::identity();
  m_cullFace = vcGLSCM_Back;
  m_ignoreTint = false;
  m_loadStatus = vcSLS_Pending;

  vcPolygonModelLoadInfo *pLoadInfo = udAllocType(vcPolygonModelLoadInfo, 1, udAF_Zero);
  if (pLoadInfo != nullptr)
  {
    // Prepare the load info
    pLoadInfo->pNode = this;
    pLoadInfo->pProgramState = pProgramState;

    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcPolyModelNode_LoadModel, pLoadInfo, true);
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }

  OnNodeUpdate(pProgramState);
}

void vcPolyModelNode::OnNodeUpdate(vcState *pProgramState)
{
  const char *pTemp = nullptr;
  if (vdkProjectNode_GetMetadataString(m_pNode, "culling", &pTemp, "back") == vE_Success)
  {
    if (udStrEquali(pTemp, "none"))
      m_cullFace = vcGLSCM_None;
    else if (udStrEquali(pTemp, "front"))
      m_cullFace = vcGLSCM_Front;
    else // Default to backface
      m_cullFace = vcGLSCM_Back;
  }

  vdkProjectNode_GetMetadataBool(m_pNode, "ignoreTint", &m_ignoreTint, false);

  if (m_pNode->geomCount != 0)
  {
    udDouble3 *pPosition = nullptr;
    udDouble3 scale;
    udDouble3 euler;

    vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, pProgramState->geozone, &pPosition, nullptr);

    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &euler.x, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &euler.y, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.r", &euler.z, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.scale.x", &scale.x, 1.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.scale.y", &scale.y, 1.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.scale.z", &scale.z, 1.0);

    m_matrix = udDouble4x4::rotationYPR(UD_DEG2RAD(euler), *pPosition) * udDouble4x4::scaleNonUniform(scale);

    udFree(pPosition);
  }
}

void vcPolyModelNode::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible || m_pModel == nullptr)
    return;

  vcRenderPolyInstance *pModel = pRenderData->polyModels.PushBack();
  pModel->pModel = m_pModel;
  pModel->pSceneItem = this;
  pModel->worldMat = m_matrix;
  pModel->cullFace = m_cullFace;
  pModel->selectable = true;
  pModel->tint = udFloat4::one();
  if (m_ignoreTint)
    pModel->renderFlags = vcRenderPolyInstance::RenderFlags_IgnoreTint;
}

void vcPolyModelNode::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_matrix = delta * m_matrix;

  // Save it to the node...
  udDouble3 position;
  udDouble3 scale;
  udDoubleQuat orientation;

  m_matrix.extractTransforms(position, scale, orientation);

  udDouble3 eulerRotation = UD_RAD2DEG(orientation.eulerAngles());

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &position, 1);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.x", scale.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.y", scale.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.z", scale.z);
}

void vcPolyModelNode::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);

  if (m_pModel == nullptr)
    return;

  const char *uiStrings[] = { vcString::Get("polyModelCullFaceBack"), vcString::Get("polyModelCullFaceFront"), vcString::Get("polyModelCullFaceNone") };
  const char *optStrings[] = { "back", "front", "none" };
  if (ImGui::Combo(udTempStr("%s##%zu", vcString::Get("polyModelCullFace"), *pItemID), (int *)&m_cullFace, uiStrings, (int)udLengthOf(uiStrings)))
    vdkProjectNode_SetMetadataString(m_pNode, "culling", optStrings[m_cullFace]);

  if (ImGui::Checkbox(udTempStr("%s##%zu", vcString::Get("polyModelIgnoreTint"), *pItemID), &m_ignoreTint))
    vdkProjectNode_SetMetadataBool(m_pNode, "ignoreTint", m_ignoreTint);

  // Transform Info
  {
    udDouble3 position;
    udDouble3 scale;
    udDoubleQuat orientation;
    m_matrix.extractTransforms(position, scale, orientation);

    bool repackMatrix = false;
    ImGui::InputScalarN(vcString::Get("sceneModelPosition"), ImGuiDataType_Double, &position.x, 3);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      repackMatrix = true;
      static udDouble3 minDouble = udDouble3::create(-1e7, -1e7, -1e7);
      static udDouble3 maxDouble = udDouble3::create(1e7, 1e7, 1e7);
      position = udClamp(position, minDouble, maxDouble);
    }

    udDouble3 eulerRotation = UD_RAD2DEG(orientation.eulerAngles());
    ImGui::InputScalarN(vcString::Get("sceneModelRotation"), ImGuiDataType_Double, &eulerRotation.x, 3);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      repackMatrix = true;
      orientation = udDoubleQuat::create(UD_DEG2RAD(eulerRotation));
    }

    ImGui::InputScalarN(vcString::Get("sceneModelScale"), ImGuiDataType_Double, &scale.x, 3);
    if (ImGui::IsItemDeactivatedAfterEdit())
      repackMatrix = true;

    if (repackMatrix)
    {
      m_matrix = udDouble4x4::rotationQuat(orientation, position) * udDouble4x4::scaleNonUniform(scale);
      vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &position, 1);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.x", scale.x);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.y", scale.y);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale.z", scale.z);
    }

  }

  if (pProgramState->settings.presentation.showDiagnosticInfo)
  {
    for (int i = 0; i < m_pModel->meshCount; ++i)
    {
      ImGui::Text("%s %d / %s", vcString::Get("polyModelMesh"), i, m_pModel->pMeshes[i].material.pName);
      ImGui::Indent();

      if (m_pModel->pMeshes[i].material.pTexture != nullptr && m_pModel->pMeshes[i].material.pTexture != pProgramState->pWhiteTexture)
      {
        int width = 0;
        int height = 0;
        if (vcTexture_GetSize(m_pModel->pMeshes[i].material.pTexture, &width, &height) == udR_Success)
        {
          ImGui::Text("%s", vcString::Get("polyModelTexture"));
          ImGui::Image((ImTextureID)m_pModel->pMeshes[i].material.pTexture, ImVec2((float)udMin(200, width), (float)udMin(200, height)));
        }
      }

      vcIGSW_ColorPickerU32(udTempStr("%s##matColour#%d_%zu", vcString::Get("polyModelMatColour"), i, *pItemID), &m_pModel->pMeshes[i].material.colour, ImGuiColorEditFlags_None);

      ImGui::Unindent();
    }
  }
}

void vcPolyModelNode::Cleanup(vcState * /*pProgramState*/)
{
  vcPolygonModel_Destroy(&m_pModel);
}

void vcPolyModelNode::ChangeProjection(const udGeoZone & /*newZone*/)
{
  // Improve geolocation support
}

udDouble4x4 vcPolyModelNode::GetWorldSpaceMatrix()
{
  return m_matrix;
}

udDouble3 vcPolyModelNode::GetLocalSpacePivot()
{
  if (m_pModel == nullptr || m_pModel->meshCount == 0)
    return udDouble3::zero();

  return m_pModel->origin;
}
