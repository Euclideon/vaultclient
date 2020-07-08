#include "vcI3S.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h"

#include "udPlatform.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcI3S::vcI3S(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_pSceneRenderer(nullptr)
{
  m_sceneMatrix = udDouble4x4::identity();

  udResult result = vcSceneLayerRenderer_Create(&m_pSceneRenderer, pProgramState->pWorkerPool, pNode->pURI);

  if (result == udR_OpenFailure)
    result = vcSceneLayerRenderer_Create(&m_pSceneRenderer, pProgramState->pWorkerPool, udTempStr("%s%s", pProgramState->activeProject.pRelativeBase, pNode->pURI));

  if (result == udR_Success)
  {
    m_loadStatus = vcSLS_Loaded;

    udGeoZone *pInternalZone = vcSceneLayer_GetPreferredZone(m_pSceneRenderer->pSceneLayer);
    if (pInternalZone != nullptr)
    {
      m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
      memcpy(m_pPreferredProjection, pInternalZone, sizeof(udGeoZone));
    }
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }
}

vcI3S::~vcI3S()
{
  vcSceneLayerRenderer_Destroy(&m_pSceneRenderer);
}

void vcI3S::OnNodeUpdate(vcState * /*pProgramState*/)
{
  //TODO: This should come from m_pNode
}

void vcI3S::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible || m_pSceneRenderer == nullptr) // Nothing to render
    return;

  vcRenderPolyInstance instance = {};
  instance.renderType = vcRenderPolyInstance::RenderType_SceneLayer;
  instance.pSceneLayer = m_pSceneRenderer;
  instance.worldMat = m_sceneMatrix;
  instance.pSceneItem = this;
  instance.sceneItemInternalId = 0; // TODO: individual node picking
  instance.selectable = true;
  instance.tint = udFloat4::one();

  pRenderData->polyModels.PushBack(instance);
}

void vcI3S::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;
}

void vcI3S::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);
}

void vcI3S::HandleContextMenu(vcState * /*pProgramState*/)
{
  ImGui::Separator();

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetPosition"), false))
  {
    m_sceneMatrix = udDouble4x4::identity();
  }
}

void vcI3S::Cleanup(vcState * /*pProgramState*/)
{
  vcSceneLayerRenderer_Destroy(&m_pSceneRenderer);
}

void vcI3S::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pSceneRenderer == nullptr)
    return;

  udGeoZone *pInternalZone = vcSceneLayer_GetPreferredZone(m_pSceneRenderer->pSceneLayer);

  if (pInternalZone == nullptr)
    return;

  udDouble4x4 prevOrigin = udDouble4x4::translation(GetLocalSpacePivot());
  udDouble4x4 newOffset = udGeoZone_TransformMatrix(prevOrigin, *pInternalZone, newZone);
  m_sceneMatrix = newOffset * udInverse(prevOrigin);
}

udDouble3 vcI3S::GetLocalSpacePivot()
{
  if (m_pSceneRenderer == nullptr)
    return udDouble3::zero();

  return vcSceneLayer_GetCenter(m_pSceneRenderer->pSceneLayer);
}

udDouble4x4 vcI3S::GetWorldSpaceMatrix()
{
  return m_pSceneRenderer ? m_sceneMatrix : udDouble4x4::identity();
}
