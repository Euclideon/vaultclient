#include "vcViewShed.h"

#include "vcCamera.h"
#include "vcRender.h"
#include "vcInternalModels.h"

#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

const double DistMin = 100.0;
const double DistMax = 1500.0;

vcViewShed::vcViewShed(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_position(udDouble3::zero()),
  m_distance(2500.0),
  m_visibleColour(0x3F00FF00),
  m_hiddenColour(0x3FFF0000)
{
  OnNodeUpdate(pProgramState);
  m_loadStatus = vcSLS_Loaded;
}

void vcViewShed::OnNodeUpdate(vcState *pProgramState)
{
  vdkProjectNode_GetMetadataDouble(m_pNode, "distance", &m_distance, DistMax);
  vdkProjectNode_GetMetadataUint(m_pNode, "visibleColour", &m_visibleColour, 0x3F00FF00);
  vdkProjectNode_GetMetadataUint(m_pNode, "hiddenColour", &m_hiddenColour, 0x3FFF0000);

  ChangeProjection(pProgramState->geozone);
}

void vcViewShed::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  // if POI is invisible or if it exceeds maximum visible POI distance
  if (!m_visible)
    return;

  if (m_selected)
  {
    vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
    pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
    pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);

    udDouble3 linearDistance = (pProgramState->camera.position - m_position);

    pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
    pInstance->worldMat = udDouble4x4::translation(m_position) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
    pInstance->pSceneItem = this;
    pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
    pInstance->selectable = false;
  }

  vcViewShedData *pViewShed = pRenderData->viewSheds.PushBack();
  pViewShed->position = m_position;
  pViewShed->fieldOfView = UD_DEG2RADf(120.0f);
  pViewShed->viewDistance = (float)m_distance;
  pViewShed->visibleColour = vcIGSW_BGRAToImGui(m_visibleColour);
  pViewShed->notVisibleColour = vcIGSW_BGRAToImGui(m_hiddenColour);
}

void vcViewShed::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_position = (delta * udDouble4::create(m_position, 1.0)).toVector3();
  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_position, 1);
}

void vcViewShed::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t *pItemID)
{
  if (ImGui::SliderScalarN(udTempStr("%s##viewShedHiddenColor%zu", vcString::Get("viewShedDistance"), *pItemID), ImGuiDataType_Double, &m_distance, 1, &DistMin, &DistMax))
    vdkProjectNode_SetMetadataDouble(m_pNode, "distance", m_distance);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##viewShedVisibleColor%zu", vcString::Get("viewShedVisibleColour"), *pItemID), &m_visibleColour, ImGuiColorEditFlags_None))
    vdkProjectNode_SetMetadataUint(m_pNode, "visibleColour", m_visibleColour);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##viewShedHiddenColor%zu", vcString::Get("viewShedHiddenColour"), *pItemID), &m_hiddenColour, ImGuiColorEditFlags_None))
    vdkProjectNode_SetMetadataUint(m_pNode, "hiddenColour", m_hiddenColour);
}

void vcViewShed::HandleSceneEmbeddedUI(vcState * /*pProgramState*/)
{
  if (ImGui::SliderScalarN(udTempStr("%s##EmbeddedUIViewShed", vcString::Get("viewShedDistance")), ImGuiDataType_Double, &m_distance, 1, &DistMin, &DistMax))
    vdkProjectNode_SetMetadataDouble(m_pNode, "distance", m_distance);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##EmbeddedUIViewShed", vcString::Get("viewShedVisibleColour")), &m_visibleColour, ImGuiColorEditFlags_None))
    vdkProjectNode_SetMetadataUint(m_pNode, "visibleColour", m_visibleColour);

  if (vcIGSW_ColorPickerU32(udTempStr("%s##EmbeddedUIViewShed", vcString::Get("viewShedHiddenColour")), &m_hiddenColour, ImGuiColorEditFlags_None))
    vdkProjectNode_SetMetadataUint(m_pNode, "hiddenColour", m_hiddenColour);
}

void vcViewShed::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoints = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoints, &numPoints);

  if (numPoints == 1)
    m_position = pPoints[0];

  udFree(pPoints);
}

void vcViewShed::Cleanup(vcState * /*pProgramState*/)
{
  // Do nothing
}

udDouble4x4 vcViewShed::GetWorldSpaceMatrix()
{
    return udDouble4x4::translation(m_position);
}
