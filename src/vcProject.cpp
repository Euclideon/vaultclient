#include "vcProject.h"

#include "vcSceneItem.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcModals.h"

#include "udFile.h"
#include "udStringUtil.h"

void vcProject_InitBlankScene(vcState *pProgramState)
{
  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  udGeoZone zone = {};
  vcRender_ClearTiles(pProgramState->pRenderContext);
  vcGIS_ChangeSpace(&pProgramState->gis, zone);

  pProgramState->camera.position = udDouble3::zero();

  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};

  vdkProject_CreateLocal(&pProgramState->activeProject.pProject, "New Project");
  vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
  pProgramState->activeProject.pFolder = new vcFolder(pProgramState->activeProject.pProject, pProgramState->activeProject.pRoot, pProgramState);
  pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;
}

bool vcProject_ExtractCameraRecursive(vcState *pProgramState, vdkProjectNode *pParentNode)
{
  vdkProjectNode *pNode = pParentNode->pFirstChild;
  while (pNode != nullptr)
  {
    if (pNode->itemtype == vdkPNT_Viewpoint)
    {
      udDouble3 position = udDouble3::zero();
      udDouble2 headingPitch = udDouble2::zero();

      udDouble3 *pPoint = nullptr;
      int numPoints = 0;

      vcProject_FetchNodeGeometryAsCartesian(pProgramState->activeProject.pProject, pNode, pProgramState->gis.zone, &pPoint, &numPoints);
      if (numPoints == 1)
        position = pPoint[0];

      vdkProjectNode_GetMetadataDouble(pNode, "transform.heading", &headingPitch.x, 0.0);
      vdkProjectNode_GetMetadataDouble(pNode, "transform.pitch", &headingPitch.y, 0.0);

      pProgramState->camera.position = position;
      pProgramState->camera.headingPitch = headingPitch;

      // unset
      memset(pProgramState->sceneExplorer.movetoUUIDWhenPossible, 0, sizeof(pProgramState->sceneExplorer.movetoUUIDWhenPossible));
      return true;
    }
    else if (pNode->itemtype == vdkPNT_PointCloud)
    {
      udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
    }

    if (vcProject_ExtractCameraRecursive(pProgramState, pNode))
      return true;

    pNode = pNode->pNextSibling;
  }

  return false;
}

// Try extract a valid viewpoint from the project, based on available nodes
void vcProject_ExtractCamera(vcState *pProgramState)
{
  pProgramState->camera.position = udDouble3::zero();
  pProgramState->camera.headingPitch = udDouble2::zero();

  vcProject_ExtractCameraRecursive(pProgramState, pProgramState->activeProject.pRoot);
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

      pProgramState->sceneExplorer.selectedItems.clear();
      pProgramState->sceneExplorer.clickedItem = {};

      pProgramState->activeProject.pProject = pProject;
      vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
      pProgramState->activeProject.pFolder = new vcFolder(pProgramState->activeProject.pProject, pProgramState->activeProject.pRoot, pProgramState);
      pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;

      udFilename temp(pFilename);
      temp.SetFilenameWithExt("");
      pProgramState->activeProject.pRelativeBase = udStrdup(temp.GetPath());

      int32_t recommendedSRID = -1;
      if (vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", &recommendedSRID, -1) == vE_Success && recommendedSRID >= 0 && udGeoZone_SetFromSRID(&zone, recommendedSRID) == udR_Success)
        vcGIS_ChangeSpace(&pProgramState->gis, zone);

      vcProject_ExtractCamera(pProgramState);
    }
    else
    {
      vcState::ErrorItem projectError;
      projectError.source = vcES_ProjectChange;
      projectError.pData = udStrdup(pFilename);
      projectError.resultCode = udR_ParseError;

      pProgramState->errorItems.PushBack(projectError);

      vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
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
  if (pProject == nullptr || pProject->pProject == nullptr)
    return;

  udFree(pProject->pRelativeBase);
  vcProject_RecursiveDestroyUserData(pProgramData, pProject->pRoot);
  vdkProject_Release(&pProject->pProject);
}

void vcProject_Save(vcState *pProgramState, const char *pPath, bool allowOverride)
{
  if (pProgramState == nullptr || pPath == nullptr)
    return;

  const char *pOutput = nullptr;

  if (pProgramState->gis.isProjected)
    vdkProjectNode_SetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", pProgramState->gis.SRID);

  if (vdkProject_WriteToMemory(pProgramState->activeProject.pProject, &pOutput) == vE_Success)
  {
    udFindDir *pDir = nullptr;
    udFilename exportFilename(pPath);

    if (!udStrEquali(pPath, "") && !udStrEndsWithi(pPath, "/") && !udStrEndsWithi(pPath, "\\") && udOpenDir(&pDir, pPath) == udR_Success)
      exportFilename.SetFromFullPath("%s/untitled_project.json", pPath);
    else if (exportFilename.HasFilename())
      exportFilename.SetExtension(".json");
    else
      exportFilename.SetFilenameWithExt("untitled_project.json");

    // Check if file path exists before writing to disk, and if so, the user will be presented with the option to overwrite or cancel
    if (allowOverride || vcModals_OverwriteExistingFile(exportFilename.GetPath()))
    {
      vcState::ErrorItem projectError = {};
      projectError.source = vcES_ProjectChange;
      projectError.pData = udStrdup(exportFilename.GetFilenameWithExt());

      if (udFile_Save(exportFilename.GetPath(), (void *)pOutput, udStrlen(pOutput)) == udR_Success)
        projectError.resultCode = udR_Success;
      else
        projectError.resultCode = udR_WriteFailure;

      pProgramState->errorItems.PushBack(projectError);

      vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
    }

    udCloseDir(&pDir);
  }
}

bool vcProject_AbleToChange(vcState *pProgramState)
{
  if (pProgramState == nullptr || !pProgramState->hasContext)
    return false;

  if (vdkProject_HasUnsavedChanges(pProgramState->activeProject.pProject) == vE_NotFound)
    return true;

  return vcModals_AllowDestructiveAction(vcString::Get("menuChangeScene"), vcString::Get("menuChangeSceneConfirm"));
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
  if (pParentNode == pItem)
    return true;

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
  vcSceneItem *pItem = (vcSceneItem *)pNode->pUserData;

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

void vcProject_ClearSelection(vcState *pProgramState, bool clearToolState /*= true*/)
{
  vcProject_ClearSelection(pProgramState->activeProject.pRoot);
  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};

  if (clearToolState)
    pProgramState->activeTool = vcActiveTool_Select;
}

bool vcProject_UseProjectionFromItem(vcState *pProgramState, vcSceneItem *pItem)
{
  if (pProgramState == nullptr || pItem == nullptr || pProgramState->programComplete || pItem->m_pPreferredProjection == nullptr)
    return false;

  if (vcGIS_ChangeSpace(&pProgramState->gis, *pItem->m_pPreferredProjection))
    pProgramState->activeProject.pFolder->ChangeProjection(*pItem->m_pPreferredProjection);

  // refresh map tiles when geozone changes
  vcRender_ClearTiles(pProgramState->pRenderContext);

  // move camera to the new item's position
  pItem->SetCameraPosition(pProgramState);

  return true;
}

bool vcProject_UpdateNodeGeometryFromCartesian(vdkProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, vdkProjectGeometryType newType, udDouble3 *pPoints, int numPoints)
{
  if (pProject == nullptr || pNode == nullptr)
    return false;

  udDouble3 *pGeom = nullptr;

  vdkError result = vE_Failure;

  if (zone.srid != 0) //Geolocated
  {
    pGeom = udAllocType(udDouble3, numPoints, udAF_Zero);

    // Change all points from the projection
    for (int i = 0; i < numPoints; ++i)
      pGeom[i] = udGeoZone_CartesianToLatLong(zone, pPoints[i], true);

    result = vdkProjectNode_SetGeometry(pProject, pNode, newType, numPoints, (double*)pGeom);
    udFree(pGeom);
  }
  else
  {
    result = vdkProjectNode_SetGeometry(pProject, pNode, newType, numPoints, (double*)pPoints);
  }

  return (result == vE_Success);
}

bool vcProject_FetchNodeGeometryAsCartesian(vdkProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, udDouble3 **ppPoints, int *pNumPoints)
{
  if (pProject == nullptr || pNode == nullptr)
    return false;

  udDouble3 *pPoints = udAllocType(udDouble3, pNode->geomCount, udAF_Zero);

  if (pNumPoints != nullptr)
    *pNumPoints = pNode->geomCount;

  if (zone.srid != 0) // Geolocated
  {
    // Change all points from the projection
    for (int i = 0; i < pNode->geomCount; ++i)
      pPoints[i] = udGeoZone_LatLongToCartesian(zone, ((udDouble3*)pNode->pCoordinates)[i], true);
  }
  else
  {
    for (int i = 0; i < pNode->geomCount; ++i)
      pPoints[i] = ((udDouble3*)pNode->pCoordinates)[i];
  }

  *ppPoints = pPoints;

  return true;
}

void vcProject_ExtractAttributionText(vdkProjectNode *pFolderNode, const char **ppCurrentText)
{
  vdkProjectNode *pNode = pFolderNode->pFirstChild;

  while (pNode != nullptr)
  {
    if (pNode->pUserData != nullptr && pNode->itemtype == vdkProjectNodeType::vdkPNT_PointCloud)
    {
      vcModel *pModel = nullptr;
      const char *pAttributionText = nullptr;

      pModel = (vcModel *)pNode->pUserData;

      //Priority: Author -> License -> Copyright
      pAttributionText = pModel->m_metadata.Get("Author").AsString(pModel->m_metadata.Get("License").AsString(pModel->m_metadata.Get("Copyright").AsString()));
      if (pAttributionText)
      {
        if (*ppCurrentText != nullptr)
          udSprintf(ppCurrentText, "%s       %s      ", *ppCurrentText, pAttributionText);
        else
          udSprintf(ppCurrentText, "%s      ", pAttributionText);
      }
    }

    if (pNode->itemtype == vdkPNT_Folder)
      vcProject_ExtractAttributionText(pNode, ppCurrentText);

    pNode = pNode->pNextSibling;
  }

  
}
