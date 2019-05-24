#include "vcProject.h"

#include "vcSceneItem.h"
#include "vcState.h"
#include "vcRender.h"

void vcProject_RemoveItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode)
{
  for (int32_t i = 0; i < (int32_t)pProgramState->sceneExplorer.selectedItems.size(); ++i)
  {
    if (pProgramState->sceneExplorer.selectedItems[i].pParent == pParent && pProgramState->sceneExplorer.selectedItems[i].pItem == pNode)
    {
      pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
      --i;
    }
  }

  vcSceneItem *pItem = pNode == nullptr ? nullptr : (vcSceneItem*)pNode->pUserData;

  if (pItem != nullptr)
  {
    if (pItem->m_loadStatus == vcSLS_Pending)
      udInterlockedCompareExchange(&pItem->m_loadStatus, vcSLS_Unloaded, vcSLS_Pending);

    while (pItem->m_loadStatus == vcSLS_Loading)
      udYield(); // Spin until other thread stops processing

    if (pItem->m_loadStatus == vcSLS_Loaded || pItem->m_loadStatus == vcSLS_OpenFailure || pItem->m_loadStatus == vcSLS_Failed)
    {
      pItem->Cleanup(pProgramState);
      delete pItem;
      pNode->pUserData = nullptr;
    }
  }

  vdkProjectNode_RemoveChild(pProgramState->sceneExplorer.pProject, pParent, pNode);
}

void vcProject_RemoveAll(vcState *pProgramState)
{
  if (pProgramState->sceneExplorer.pProjectRoot == nullptr)
    return;

  //TODO: Just destroy the project and recreate it

  while (pProgramState->sceneExplorer.pProjectRoot->m_pNode->pFirstChild != nullptr)
    vcProject_RemoveItem(pProgramState, pProgramState->sceneExplorer.pProjectRoot->m_pNode, pProgramState->sceneExplorer.pProjectRoot->m_pNode->pFirstChild);
  pProgramState->sceneExplorer.selectedItems.clear();

  udGeoZone zone = {};
  vcRender_ClearTiles(pProgramState->pRenderContext);
  vcGIS_ChangeSpace(&pProgramState->gis, zone);

  if (pProgramState->pCamera != nullptr) // This is destroyed before the scene
    pProgramState->pCamera->position = udDouble3::zero();
}

void vcProject_RemoveSelectedFolder(vcState *pProgramState, vdkProjectNode *pFolderNode)
{
  vdkProjectNode *pNode = pFolderNode->pFirstChild;

  while (pNode != nullptr)
  {
    vcSceneItem *pItem = (vcSceneItem*)pNode->pUserData;

    if (pItem != nullptr && pItem->m_selected)
      vcProject_RemoveItem(pProgramState, pFolderNode, pNode);

    if (pNode->itemtype == vdkPNT_Folder)
      vcProject_RemoveSelectedFolder(pProgramState, pNode);

    pNode = pNode->pNextSibling;
  }
}

void vcProject_RemoveSelected(vcState *pProgramState)
{
  vcProject_RemoveSelectedFolder(pProgramState, pProgramState->sceneExplorer.pProjectRoot->m_pNode);
  pProgramState->sceneExplorer.selectedItems.clear();
}

bool vcProject_ContainsItem(vdkProjectNode *pParentNode, vdkProjectNode *pItem)
{
  vdkProjectNode *pNode = pParentNode->pFirstChild;

  while (pNode != nullptr)
  {
    if (pNode == pItem)
      return true;

    if (pNode->itemtype == vdkPNT_Folder && vcProject_ContainsItem(pNode, pItem))
      return true;

    pNode = pNode->pNextSibling;
  }

  return false;
}

void vcProject_SelectItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode)
{
  vcSceneItem *pItem = (vcSceneItem*)pNode->pUserData;

  if (pItem != nullptr && !pItem->m_selected)
  {
    pItem->m_selected = true;
    pProgramState->sceneExplorer.selectedItems.push_back({ pParent, pNode });
  }
}

void vcProject_UnselectItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode)
{
  vcSceneItem *pItem = (vcSceneItem*)pNode->pUserData;

  if (pItem != nullptr && pItem->m_selected)
  {
    pItem->m_selected = false;
    for (int32_t i = 0; i < (int32_t)pProgramState->sceneExplorer.selectedItems.size(); ++i)
    {
      const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
      if (item.pParent == pParent && item.pItem == pNode)
      {
        pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
        --i;
      }
    }
  }
}

void vcProject_ClearSelection(vdkProjectNode *pParentNode)
{
  vdkProjectNode *pNode = pParentNode->pFirstChild;

  while (pNode != nullptr)
  {
    if (pNode->itemtype == vdkPNT_Folder)
      vcProject_ClearSelection(pNode);
    else if (pNode->pUserData != nullptr)
      ((vcSceneItem*)pNode->pUserData)->m_selected = false;

    pNode = pNode->pNextSibling;
  }

  if (pParentNode->pUserData != nullptr)
    ((vcSceneItem*)pParentNode->pUserData)->m_selected = false;
}

void vcProject_ClearSelection(vcState *pProgramState)
{
  vcProject_ClearSelection(pProgramState->sceneExplorer.pProjectRoot->m_pNode);
  pProgramState->sceneExplorer.selectedItems.clear();
}

bool vcProject_UseProjectionFromItem(vcState *pProgramState, vcSceneItem *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr || pProgramState->programComplete)
    return false;

  udGeoZone zone = {};

  if (pModel->m_pCurrentProjection == nullptr || pModel->m_pPreferredProjection == nullptr || pModel->m_pCurrentProjection->srid == 0)
    vcGIS_ChangeSpace(&pProgramState->gis, zone);
  else if (vcGIS_ChangeSpace(&pProgramState->gis, *pModel->m_pPreferredProjection)) // Update all models to new zone unless there is no new zone
    pProgramState->sceneExplorer.pProjectRoot->ChangeProjection(*pModel->m_pPreferredProjection);

  // refresh map tiles when geozone changes
  vcRender_ClearTiles(pProgramState->pRenderContext);

  // move camera to the new item's position
  pModel->SetCameraPosition(pProgramState);

  return true;
}
