#include "vcSHPNode.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h"

#include "udPlatform.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcSHPNode::vcSHPNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_SHP()
{
  m_sceneMatrix = udDouble4x4::identity();

  udResult result = vcSHP_Load(&m_SHP, pNode->pURI);

  if (result == udR_Success)
  {
    m_loadStatus = vcSLS_Loaded;

    //udGeoZone *pInternalZone = vcSceneLayer_GetPreferredZone(m_pSceneRenderer->pSceneLayer);
    //if (pInternalZone != nullptr)
    //{
    //  m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    //  memcpy(m_pPreferredProjection, pInternalZone, sizeof(udGeoZone));
    //}
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }
}

vcSHPNode::~vcSHPNode()
{
  vcSHP_Release(&m_SHP);
}

void vcSHPNode::OnNodeUpdate(vcState * /*pProgramState*/)
{
  //TODO: This should come from m_pNode
}

void vcSHPNode::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible) // Nothing to render
    return;

  //vcRenderPolyInstance instance = {};
  //instance.renderType = vcRenderPolyInstance::RenderType_SceneLayer;
  //instance.pSceneLayer = m_pSceneRenderer;
  //instance.worldMat = m_sceneMatrix;
  //instance.pSceneItem = this;
  //instance.sceneItemInternalId = 0; // TODO: individual node picking
  //instance.selectable = true;
  //instance.tint = udFloat4::one();

  //pRenderData->polyModels.PushBack(instance);
}

void vcSHPNode::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;
}

void vcSHPNode::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);
}

void vcSHPNode::HandleContextMenu(vcState * /*pProgramState*/)
{
  ImGui::Separator();

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetPosition"), false))
  {
    m_sceneMatrix = udDouble4x4::identity();
  }
}

void vcSHPNode::Cleanup(vcState * /*pProgramState*/)
{
  vcSHP_Release(&m_SHP);
}

void vcSHPNode::ChangeProjection(const udGeoZone &newZone)
{
  //TODO
  udGeoZone *pInternalZone = nullptr;

  if (pInternalZone == nullptr)
    return;

  udDouble4x4 prevOrigin = udDouble4x4::translation(GetLocalSpacePivot());
  udDouble4x4 newOffset = udGeoZone_TransformMatrix(prevOrigin, *pInternalZone, newZone);
  m_sceneMatrix = newOffset * udInverse(prevOrigin);
}

udDouble3 vcSHPNode::GetLocalSpacePivot()
{
  //TODO
  return udDouble3::zero();
}

udDouble4x4 vcSHPNode::GetWorldSpaceMatrix()
{
  //TODO
  return udDouble4x4::identity();
}
