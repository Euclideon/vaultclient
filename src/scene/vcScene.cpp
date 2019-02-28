#include "vcScene.h"

#include "vcState.h"
#include "vcRender.h"

vcSceneItem::vcSceneItem() :
  m_loadStatus(0), m_visible(false), m_selected(false), m_expanded(false), m_editName(false),
  m_type(vcSOT_Unknown), m_typeStr(""), m_pMetadata(nullptr), m_pOriginalZone(nullptr),
  m_pZone(nullptr), m_pPath(nullptr), m_pName(nullptr), m_nameBufferLength(0)
{
}

vcSceneItem::~vcSceneItem()
{

}

void vcSceneItem::AddItem(vcState *pProgramState)
{
  vcFolder *pParent = pProgramState->sceneExplorer.clickedItem.pParent;
  vcSceneItem *pChild = nullptr;

  if (pParent)
    pChild = pParent->m_children[pProgramState->sceneExplorer.clickedItem.index];

  // TODO: Proper Exception Handling
  if (pChild != nullptr && pChild->m_type == vcSOT_Folder)
    ((vcFolder*)pChild)->m_children.push_back(this);
  else if (pParent != nullptr)
    pParent->m_children.push_back(this);
  else
    pProgramState->sceneExplorer.pItems->m_children.push_back(this);
}

void vcSceneItem::OnNameChange()
{
  // Does nothing in default version
}

void vcScene_AddItem(vcState *pProgramState, vcSceneItem *pItem, bool select /*= false*/)
{
  vcFolder *pParent = pProgramState->sceneExplorer.clickedItem.pParent;
  vcSceneItem *pChild = nullptr;

  vcFolder *pFolder = nullptr;

  if (pParent)
    pChild = pParent->m_children[pProgramState->sceneExplorer.clickedItem.index];

  // TODO: Proper Exception Handling
  if (pChild != nullptr && pChild->m_type == vcSOT_Folder)
    pFolder = (vcFolder*)pChild;
  else if (pParent != nullptr)
    pFolder = pParent;
  else
    pFolder = pProgramState->sceneExplorer.pItems;

  pFolder->m_children.push_back(pItem);

  if (select)
    vcScene_SelectItem(pProgramState, pFolder, pFolder->m_children.size() - 1);
}

void vcScene_RemoveReference(vcSceneItemRef *pItemRef, vcFolder *pParent, size_t index)
{
  if (pItemRef->pParent == pParent && pItemRef->index == index)
  {
    pItemRef->pParent = nullptr;
    pItemRef->index = SIZE_MAX;
  }
  else if (pItemRef->pParent == pParent && pItemRef->index > index)
  {
    --pItemRef->index;
  }
}

void vcScene_RemoveItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  // Remove references
  vcScene_RemoveReference(&pProgramState->sceneExplorer.insertItem, pParent, index);
  vcScene_RemoveReference(&pProgramState->sceneExplorer.clickedItem, pParent, index);
  for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
  {
    if (pProgramState->sceneExplorer.selectedItems[i].pParent == pParent && pProgramState->sceneExplorer.selectedItems[i].index == index)
    {
      pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
      --i;
    }
    else if (pProgramState->sceneExplorer.selectedItems[i].pParent == pParent && pProgramState->sceneExplorer.selectedItems[i].index > index)
    {
      --pProgramState->sceneExplorer.selectedItems[i].index;
    }
  }

  if (pParent->m_children[index]->m_loadStatus == vcSLS_Pending)
    udInterlockedCompareExchange(&pParent->m_children[index]->m_loadStatus, vcSLS_Unloaded, vcSLS_Pending);

  while (pParent->m_children[index]->m_loadStatus == vcSLS_Loading)
    udYield(); // Spin until other thread stops processing

  if (pParent->m_children[index]->m_loadStatus == vcSLS_Loaded || pParent->m_children[index]->m_loadStatus == vcSLS_OpenFailure || pParent->m_children[index]->m_loadStatus == vcSLS_Failed)
  {
    pParent->m_children[index]->Cleanup(pProgramState);

    if (pParent->m_children[index]->m_pMetadata)
      pParent->m_children[index]->m_pMetadata->Destroy();

    udFree(pParent->m_children[index]->m_pMetadata);
    udFree(pParent->m_children[index]->m_pOriginalZone);
    udFree(pParent->m_children[index]->m_pZone);
  }

  pParent->m_children[index]->m_loadStatus = vcSLS_Unloaded;

  delete pParent->m_children.at(index);
  pParent->m_children.erase(pParent->m_children.begin() + index);
}

void vcScene_RemoveAll(vcState *pProgramState)
{
  if (pProgramState->sceneExplorer.pItems == nullptr)
    return;

  while (pProgramState->sceneExplorer.pItems->m_children.size() > 0)
    vcScene_RemoveItem(pProgramState, pProgramState->sceneExplorer.pItems, 0);
  pProgramState->sceneExplorer.selectedItems.clear();

  udGeoZone zone = {};
  vcRender_ClearTiles(pProgramState->pRenderContext);
  vcGIS_ChangeSpace(&pProgramState->gis, zone);

  if (pProgramState->pCamera != nullptr) // This is destroyed before the scene
    pProgramState->pCamera->position = udDouble3::zero();
}

void vcScene_RemoveSelected(vcState *pProgramState, vcFolder *pFolder)
{
  for (size_t i = 0; i < pFolder->m_children.size(); ++i)
  {
    if (pFolder->m_children[i]->m_selected)
    {
      vcScene_RemoveItem(pProgramState, pFolder, i);
      i--;
      continue;
    }

    if (pFolder->m_children[i]->m_type == vcSOT_Folder)
      vcScene_RemoveSelected(pProgramState, (vcFolder*)pFolder->m_children[i]);
  }
}

void vcScene_RemoveSelected(vcState *pProgramState)
{
  vcScene_RemoveSelected(pProgramState, pProgramState->sceneExplorer.pItems);
  pProgramState->sceneExplorer.selectedItems.clear();
}

bool vcScene_ContainsItem(vcFolder *pParent, vcSceneItem *pItem)
{
  for (size_t i = 0; i < pParent->m_children.size(); ++i)
  {
    if (pParent->m_children[i] == pItem)
      return true;

    if (pParent->m_children[i]->m_type == vcSOT_Folder)
      if (vcScene_ContainsItem((vcFolder*)pParent->m_children[i], pItem))
        return true;
  }

  return pParent == pItem;
}

void vcScene_SelectItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  if (!pParent->m_children[index]->m_selected)
  {
    pParent->m_children[index]->m_selected = true;
    pProgramState->sceneExplorer.selectedItems.push_back({ pParent, index });
  }
}

void vcScene_UnselectItem(vcState *pProgramState, vcFolder *pParent, size_t index)
{
  if (pParent->m_children[index]->m_selected)
  {
    pParent->m_children[index]->m_selected = false;
    for (size_t i = 0; i < pProgramState->sceneExplorer.selectedItems.size(); ++i)
    {
      const vcSceneItemRef &item = pProgramState->sceneExplorer.selectedItems[i];
      if (item.pParent == pParent && item.index == index)
        pProgramState->sceneExplorer.selectedItems.erase(pProgramState->sceneExplorer.selectedItems.begin() + i);
    }
  }
}

void vcScene_ClearSelection(vcFolder *pParent)
{
  for (size_t i = 0; i < pParent->m_children.size(); i++)
  {
    if (pParent->m_children[i]->m_type == vcSOT_Folder)
      vcScene_ClearSelection((vcFolder*)pParent->m_children[i]);
    else
      pParent->m_children[i]->m_selected = false;
  }
  pParent->m_selected = false;
}

void vcScene_ClearSelection(vcState *pProgramState)
{
  vcScene_ClearSelection(pProgramState->sceneExplorer.pItems);
  pProgramState->sceneExplorer.selectedItems.clear();
}

void vcSceneItem::ChangeProjection(vcState * /*pProgramState*/, const udGeoZone &newZone)
{
  if (this->m_pZone != nullptr && newZone.srid != m_pZone->srid)
    memcpy(m_pZone, &newZone, sizeof(newZone));
}

bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  udGeoZone zone = {};

  if (pModel->m_pZone == nullptr || pModel->m_pOriginalZone == nullptr || pModel->m_pOriginalZone->srid == 0)
    vcGIS_ChangeSpace(&pProgramState->gis, zone);
  else if (vcGIS_ChangeSpace(&pProgramState->gis, *pModel->m_pOriginalZone))
    pProgramState->sceneExplorer.pItems->ChangeProjection(pProgramState, *pModel->m_pOriginalZone); // Update all models to new zone unless there is no new zone

  // refresh map tiles when geozone changes
  vcRender_ClearTiles(pProgramState->pRenderContext);

  pProgramState->pCamera->position = pModel->GetWorldSpacePivot();

  return true;
}

udDouble3 vcSceneItem::GetWorldSpacePivot()
{
  return (this->GetWorldSpaceMatrix() * udDouble4::create(this->GetLocalSpacePivot(), 1.0)).toVector3();
}

udDouble3 vcSceneItem::GetLocalSpacePivot()
{
  return udDouble3::zero();
}

udDouble4x4 vcSceneItem::GetWorldSpaceMatrix()
{
  return udDouble4x4::identity();
}
