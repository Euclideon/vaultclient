#include "vcPolyModelNode.h"

#include "vcPolygonModel.h"
#include "vcStrings.h"
#include "vcRender.h"

#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcPolyModelNode::vcPolyModelNode(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_pModel = nullptr;
  m_matrix = udDouble4x4::identity();

  //TODO: Do this load async
  if (vcPolygonModel_CreateFromURL(&m_pModel, pNode->pURI) == udR_Success)
    m_loadStatus = vcSLS_Loaded;
  else
    m_loadStatus = vcSLS_Failed;
}

void vcPolyModelNode::OnNodeUpdate(vcState * /*pProgramState*/)
{
  // Does nothing
}

void vcPolyModelNode::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (m_loadStatus != vcSLS_Loaded || !m_visible)
    return;

  vcRenderPolyInstance *pModel = pRenderData->polyModels.PushBack();
  pModel->pModel = m_pModel;
  pModel->pSceneItem = this;
  pModel->worldMat = m_matrix;
}

void vcPolyModelNode::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_matrix = delta * m_matrix;
}

void vcPolyModelNode::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  if (m_pModel == nullptr)
    return;

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
