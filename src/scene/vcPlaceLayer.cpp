#include "vcPlaceLayer.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcFenceRenderer.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

vcPlaceLayer::vcPlaceLayer(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_places.Init(512);

  m_closeLabels.Init(32);
  m_closeLines.Init(32);

  vcFenceRendererConfig config = {};
  config.visualMode = vcRRVM_Flat;
  config.imageMode = vcRRIM_Glow;
  config.isDualColour = false;
  config.ribbonWidth = 3.0;
  config.primaryColour = udFloat4::one();
  config.textureRepeatScale = 1.0;
  config.textureScrollSpeed = 1.0;

  OnNodeUpdate(pProgramState);
  m_loadStatus = vcSLS_Loaded;
}

void vcPlaceLayer::OnNodeUpdate(vcState *pProgramState)
{
  m_places.Clear();

  udProjectNode_GetMetadataString(m_pNode, "pin", &m_pPinIcon, nullptr);

  udProjectNode_GetMetadataDouble(m_pNode, "labelDistance", &m_labelDistance, 20000);
  udProjectNode_GetMetadataDouble(m_pNode, "pinDistance", &m_pinDistance, 6000000);


  Place place;

  while (udProjectNode_GetMetadataString(m_pNode, udTempStr("places[%zu].name", m_places.length), &place.pName, nullptr) == udE_Success)
  {
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("places[%zu].lla[0]", m_places.length), &place.latLongAlt.x, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("places[%zu].lla[1]", m_places.length), &place.latLongAlt.y, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("places[%zu].lla[2]", m_places.length), &place.latLongAlt.z, 0.0);
    udProjectNode_GetMetadataInt(m_pNode, udTempStr("places[%zu].count", m_places.length), &place.count, 1);

    m_places.PushBack(place);
  }

  ChangeProjection(pProgramState->geozone);
}

void vcPlaceLayer::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  size_t usedLabels = 0;

  for (size_t i = 0; i < m_places.length; ++i)
  {
    double distSq = udMagSq(pProgramState->pActiveViewport->camera.position - m_places[i].localSpace);

    if (distSq < m_labelDistance * m_labelDistance)
    {
      if (m_closeLabels.length <= usedLabels)
      {
        m_closeLabels.PushBack();

        vcLineInstance *pLineInstance = nullptr;
        vcLineRenderer_CreateLine(&pLineInstance);
        m_closeLines.PushBack(pLineInstance);
      }

      udDouble3 normal = vcGIS_GetWorldLocalUp(pProgramState->geozone, m_places[i].localSpace);

      m_closeLabels[usedLabels].worldPosition = m_places[i].localSpace + normal * 200.0;
      m_closeLabels[usedLabels].pText = m_places[i].pName;
      m_closeLabels[usedLabels].backColourRGBA = 0xFF000000;
      m_closeLabels[usedLabels].textColourRGBA = 0xFFFFFFFF;
      m_closeLabels[usedLabels].pSceneItem = this;

      udDouble3 points[] = { m_places[i].localSpace, m_closeLabels[usedLabels].worldPosition };
      vcLineRenderer_UpdatePoints(m_closeLines[usedLabels], points, 2, udFloat4::one(), 5, false);

      pRenderData->labels.PushBack(&m_closeLabels[usedLabels]);
      pRenderData->lines.PushBack(m_closeLines[usedLabels]);

      ++usedLabels;
    }
    else if (distSq < m_pinDistance * m_pinDistance)
    {
      pRenderData->pins.PushBack({ m_places[i].localSpace, m_pPinIcon, m_places[i].count, this });
    }
  }
}

void vcPlaceLayer::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{
  // Does nothing
}

void vcPlaceLayer::HandleSceneExplorerUI(vcState * /*pProgramState*/, size_t *pItemID)
{
  double min = 0;
  double maxLabel = 50000;
  double maxPin = 6000000;

  if (ImGui::SliderScalar(udTempStr("Label Distance##%zu", *pItemID), ImGuiDataType_Double, &m_labelDistance, &min, &maxLabel))
    udProjectNode_SetMetadataDouble(m_pNode, "labelDistance", m_labelDistance);

  if (ImGui::SliderScalar(udTempStr("Pin Distance##%zu", *pItemID), ImGuiDataType_Double, &m_pinDistance, &min, &maxPin))
    udProjectNode_SetMetadataDouble(m_pNode, "pinDistance", m_pinDistance);
}

void vcPlaceLayer::ChangeProjection(const udGeoZone &newZone)
{
  for (size_t i = 0; i < m_places.length; ++i)
    m_places[i].localSpace = udGeoZone_LatLongToCartesian(newZone, m_places[i].latLongAlt);
}

void vcPlaceLayer::Cleanup(vcState * /*pProgramState*/)
{
  m_places.Deinit();
  m_closeLabels.Deinit();

  for (size_t i = 0; i < m_closeLines.length; ++i)
    vcLineRenderer_DestroyLine(&m_closeLines[i]);
  m_closeLines.Deinit();
}
