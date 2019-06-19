#include "vcProject.h"

#include "vcSceneItem.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcModals.h"

#include "udFile.h"

void vcProject_InitBlankScene(vcState *pProgramState)
{
  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  udGeoZone zone = {};
  vcRender_ClearTiles(pProgramState->pRenderContext);
  vcGIS_ChangeSpace(&pProgramState->gis, zone);

  if (pProgramState->pCamera != nullptr) // This is destroyed before the scene
    pProgramState->pCamera->position = udDouble3::zero();

  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};

  vdkProject_CreateLocal(&pProgramState->activeProject.pProject, "New Project");
  vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
  pProgramState->activeProject.pFolder = new vcFolder(pProgramState->activeProject.pRoot, pProgramState);
  pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;
}

bool vcProject_InitFromURI(vcState *pProgramState, const char *pFilename)
{
  char *pMemory = nullptr;
  udResult result = udFile_Load(pFilename, (void**)&pMemory);

  if (result == udR_Success)
  {
    vdkProject *pProject = nullptr;
    if (vdkProject_LoadFromMemory(&pProject, pMemory) == vE_Success)
    {
      vcProject_Deinit(pProgramState, &pProgramState->activeProject);

      udGeoZone zone = {};
      vcRender_ClearTiles(pProgramState->pRenderContext);
      vcGIS_ChangeSpace(&pProgramState->gis, zone);

      if (pProgramState->pCamera != nullptr) // This is destroyed before the scene
        pProgramState->pCamera->position = udDouble3::zero();

      pProgramState->sceneExplorer.selectedItems.clear();
      pProgramState->sceneExplorer.clickedItem = {};

      pProgramState->activeProject.pProject = pProject;
      vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
      pProgramState->activeProject.pFolder = new vcFolder(pProgramState->activeProject.pRoot, pProgramState);
      pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;

      pProgramState->getGeo = true;
    }
    else
    {
      // TODO: EVC-671 More descriptive error, code is vE_ParseError
      vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
    }

    udFree(pMemory);
  }
  else
  {
    // TODO: Add to unsupported list in other branch
  }

  return (result == udR_Success);
}

// This won't be required after destroy list works in vdkProject
void vcProject_RecursiveDestroyUserData(vcState *pProgramData, vdkProjectNode *pFirstSibling)
{
  vdkProjectNode *pNode = pFirstSibling;

  do
  {
    if (pNode->pFirstChild)
      vcProject_RecursiveDestroyUserData(pProgramData, pNode->pFirstChild);

    if (pNode->pUserData)
    {
      vcSceneItem *pItem = (vcSceneItem*)pNode->pUserData;

      if (pItem != nullptr)
      {
        if (pItem->m_loadStatus == vcSLS_Pending)
          udInterlockedCompareExchange(&pItem->m_loadStatus, vcSLS_Unloaded, vcSLS_Pending);

        while (pItem->m_loadStatus == vcSLS_Loading)
          udYield(); // Spin until other thread stops processing

        if (pItem->m_loadStatus == vcSLS_Loaded || pItem->m_loadStatus == vcSLS_OpenFailure || pItem->m_loadStatus == vcSLS_Failed)
        {
          pItem->Cleanup(pProgramData);
          delete pItem;
          pNode->pUserData = nullptr;
        }
      }
    }

    pNode = pNode->pNextSibling;
  } while (pNode != nullptr);
}

void vcProject_Deinit(vcState *pProgramData, vcProject *pProject)
{
  vcProject_RecursiveDestroyUserData(pProgramData, pProject->pRoot);
  vdkProject_Release(&pProject->pProject);
}

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

  vdkProjectNode_RemoveChild(pProgramState->activeProject.pProject, pParent, pNode);
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
  vcProject_RemoveSelectedFolder(pProgramState, pProgramState->activeProject.pRoot);

  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};
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
  vcProject_ClearSelection(pProgramState->activeProject.pRoot);
  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};
}

bool vcProject_UseProjectionFromItem(vcState *pProgramState, vcSceneItem *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr || pProgramState->programComplete)
    return false;

  udGeoZone zone = {};

  if (pModel->m_pCurrentProjection == nullptr || pModel->m_pPreferredProjection == nullptr || pModel->m_pCurrentProjection->srid == 0)
    vcGIS_ChangeSpace(&pProgramState->gis, zone);
  else if (vcGIS_ChangeSpace(&pProgramState->gis, *pModel->m_pPreferredProjection)) // Update all models to new zone unless there is no new zone
    pProgramState->activeProject.pFolder->ChangeProjection(*pModel->m_pPreferredProjection);

  // refresh map tiles when geozone changes
  vcRender_ClearTiles(pProgramState->pRenderContext);

  // move camera to the new item's position
  pModel->SetCameraPosition(pProgramState);

  return true;
}
