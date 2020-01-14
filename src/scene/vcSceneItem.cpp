#include "vcSceneItem.h"

#include "vcState.h"
#include "vcRender.h"

vdkProjectNode* vcSceneItem_CreateNodeInProject(vdkProject *pProject, const char *pType, const char *pName)
{
  vdkProjectNode *pNode = nullptr;
  vdkProjectNode *pRootNode = nullptr;

  vdkProject_GetProjectRoot(pProject, &pRootNode);

  vdkProjectNode_Create(pProject, &pNode, pRootNode, pType, pName, nullptr, nullptr);

  UDASSERT(pNode != nullptr, "Remove Path to this- memory alloc failed.");

  return pNode;
}

vcSceneItem::vcSceneItem(vdkProject *pProject, vdkProjectNode *pNode, vcState * /*pProgramState*/) :
  m_pProject(pProject),
  m_loadStatus(0),
  m_visible(true),
  m_selected(false),
  m_expanded(false),
  m_editName(false),
  m_pName(nullptr),
  m_nameCapacity(0),
  m_pPreferredProjection(nullptr)
{
  m_metadata.SetVoid();
  m_pNode = pNode;
}

vcSceneItem::vcSceneItem(vcState *pProgramState, const char *pType, const char *pName) :
  vcSceneItem(pProgramState->activeProject.pProject, vcSceneItem_CreateNodeInProject(pProgramState->activeProject.pProject, pType, pName), pProgramState)
{
  // Do nothing
}

vcSceneItem::~vcSceneItem()
{
  m_metadata.Destroy();
  udFree(m_pPreferredProjection);
}

void vcSceneItem::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->pCamera->position = GetWorldSpacePivot();
}

udDouble3 vcSceneItem::GetWorldSpacePivot()
{
  return (this->GetWorldSpaceMatrix() * udDouble4::create(this->GetLocalSpacePivot(), 1.0)).toVector3();
}

udDouble3 vcSceneItem::GetLocalSpacePivot()
{
  return udDouble3::zero(); //TODO: Consider somehow defaulting to Camera Position instead
}

udDouble4x4 vcSceneItem::GetWorldSpaceMatrix()
{
  return udDouble4x4::identity();
}

void vcSceneItem::UpdateNode(vcState *pProgramState)
{
  this->OnNodeUpdate(pProgramState);
  m_lastUpdateTime = m_pNode->lastUpdate;
};

bool vcSceneItem::IsSceneSelected(uint64_t internalId)
{
  udUnused(internalId);

  return m_selected;
}

vcGizmoAllowedControls vcSceneItem::GetAllowedControls()
{
  return vcGAC_All;
}
