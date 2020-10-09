#include "vc12dxmlCommon.h"
#include "vcState.h"

void vc12DXML_SuperString::AddToProject(vcState *pProgramState, udProjectNode *pParent)
{
  if (m_pName == nullptr || m_points.size() == 0)
    return;

  udProjectNode *pNode = nullptr;
  if (m_points.size() == 1)
  {
    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParent, "POI", m_pName, nullptr, nullptr);
    vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Point, &m_points[0], 1);
  }
  else
  {
    udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParent, "POI", m_pName, nullptr, nullptr);
    if (m_isClosed)
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_Polygon, &m_points[0], (int)m_points.size());
    else
      vcProject_UpdateNodeGeometryFromCartesian(pProgramState, &pProgramState->activeProject, pNode, pProgramState->geozone, udPGT_MultiPoint, &m_points[0], (int)m_points.size());
  }

  udProjectNode_SetMetadataDouble(pNode, "lineWidth", m_weight);
  udProjectNode_SetMetadataUint(pNode, "lineColourPrimary", m_colour);
  udProjectNode_SetMetadataUint(pNode, "lineColourSecondary", m_colour);
  udProjectNode_SetMetadataString(pNode, "textSize", "Medium");
}

void vc12DXML_Model::AddToProject(vcState *pProgramState, udProjectNode *pParent)
{
  udProjectNode *pNode = nullptr;
  udProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pParent, "Folder", m_pName, nullptr, nullptr);

  for (auto pEle : m_elements)
    pEle->AddToProject(pProgramState, pNode);
}

void vc12DXML_Project::AddToProject(vcState *pProgramState, udProjectNode *pParent)
{
  // TODO Can a 12dxml file be geolocated? For now, set as non-geolocated.
  udGeoZone zone;
  if (udGeoZone_SetFromSRID(&zone, 0) == udR_Success)
    pProgramState->geozone = zone;

  for (const auto &model : m_models)
    model->AddToProject(pProgramState, pParent);
}
