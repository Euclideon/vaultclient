#include "vcProject.h"

#include "vcSceneItem.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcModals.h"

#include "udFile.h"
#include "udStringUtil.h"

void vcProject_InitBlankScene(vcState *pProgramState, const char *pName, int srid)
{
  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  udGeoZone zone = {};
  vcGIS_ChangeSpace(&pProgramState->geozone, zone);

  pProgramState->camera.position = udDouble3::zero();

  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};

  vdkProject_CreateLocal(&pProgramState->activeProject.pProject, pName);
  vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
  pProgramState->activeProject.pFolder = new vcFolder(&pProgramState->activeProject, pProgramState->activeProject.pRoot, pProgramState);
  pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;

  udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, srid);

  int cameraProjection = (srid == 84) ? 4978 : srid; // use ECEF if its default, otherwise the requested zone

  if (cameraProjection != 0)
  {
    udGeoZone cameraZone = {};
    udGeoZone_SetFromSRID(&cameraZone, cameraProjection);

    if (vcGIS_ChangeSpace(&pProgramState->geozone, cameraZone))
      pProgramState->activeProject.pFolder->ChangeProjection(cameraZone);

    if (cameraProjection == 4978 || cameraZone.latLongBoundMin == cameraZone.latLongBoundMax)
    {
      double locations[][5] = {
        { 309281.960926, 5640790.149293, 2977479.571028, 55.74, -32.45 }, // Mount Everest
        { 4443919.137517, 556287.927124, 4540116.021340, 21.07, -10.85 }, // Valley in France
        { 6390753.962424, 1173147.659817, 5866300.533479, 3.25, -76.07 }, // Europe High
        { -5345572.793165, 5951831.265765, -4079550.822723, 1.33, -84.59 }, // Australia High
      };

      uint64_t length = (uint64_t)udLengthOf(locations);
      uint64_t seed = udGetEpochMilliSecsUTCd();
      int randomIndex = (int)(seed % length);
      double *pPlace = locations[randomIndex];

      pProgramState->camera.position = udDouble3::create(pPlace[0], pPlace[1], pPlace[2]);
      pProgramState->camera.headingPitch = { UD_DEG2RAD(pPlace[3]), UD_DEG2RAD(pPlace[4]) };
    }
    else
    {
      pProgramState->camera.position = udGeoZone_LatLongToCartesian(cameraZone, udDouble3::create((cameraZone.latLongBoundMin + cameraZone.latLongBoundMax) / 2.0, 10000.0));
      pProgramState->camera.headingPitch = { 0.0, UD_DEG2RAD(-80.0) };
    }
  }
  else
  {
    pProgramState->camera.position = udDouble3::zero();
    pProgramState->camera.headingPitch = udDouble2::zero();
  }
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

      vcProject_FetchNodeGeometryAsCartesian(&pProgramState->activeProject, pNode, pProgramState->geozone, &pPoint, &numPoints);
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
      vcGIS_ChangeSpace(&pProgramState->geozone, zone);

      pProgramState->sceneExplorer.selectedItems.clear();
      pProgramState->sceneExplorer.clickedItem = {};

      pProgramState->activeProject.pProject = pProject;
      vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);
      pProgramState->activeProject.pFolder = new vcFolder(&pProgramState->activeProject, pProgramState->activeProject.pRoot, pProgramState);
      pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;

      udFilename temp(pFilename);
      temp.SetFilenameWithExt("");
      pProgramState->activeProject.pRelativeBase = udStrdup(temp.GetPath());

      int32_t projectZone = 84; // LongLat
      vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "projectcrs", &projectZone, 84);
      if (projectZone > 0 && udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, projectZone) != udR_Success)
        udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, 84);

      int32_t recommendedSRID = -1;
      if (vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", &recommendedSRID, pProgramState->activeProject.baseZone.srid) == vE_Success && recommendedSRID >= 0 && ((udGeoZone_SetFromSRID(&zone, recommendedSRID) == udR_Success) || recommendedSRID == 0))
        vcGIS_ChangeSpace(&pProgramState->geozone, zone);

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

  vdkProjectNode_SetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", pProgramState->geozone.srid);

  if (pProgramState->activeProject.baseZone.srid != 84)
    vdkProjectNode_SetMetadataInt(pProgramState->activeProject.pRoot, "projectcrs", pProgramState->activeProject.baseZone.srid);

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

  if (vcGIS_ChangeSpace(&pProgramState->geozone, *pItem->m_pPreferredProjection))
    pProgramState->activeProject.pFolder->ChangeProjection(*pItem->m_pPreferredProjection);

  // move camera to the new item's position
  pItem->SetCameraPosition(pProgramState);

  return true;
}

bool vcProject_UpdateNodeGeometryFromCartesian(vcProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, vdkProjectGeometryType newType, udDouble3 *pPoints, int numPoints)
{
  if (pProject == nullptr || pNode == nullptr)
    return false;

  udDouble3 *pGeom = nullptr;

  vdkError result = vE_Failure;

  if (zone.srid != 0 && pProject->baseZone.srid != 0) //Geolocated
  {
    pGeom = udAllocType(udDouble3, numPoints, udAF_Zero);

    // Change all points from the projection
    for (int i = 0; i < numPoints; ++i)
      pGeom[i] = udGeoZone_TransformPoint(pPoints[i], zone, pProject->baseZone);

    result = vdkProjectNode_SetGeometry(pProject->pProject, pNode, newType, numPoints, (double*)pGeom);
    udFree(pGeom);
  }
  else
  {
    result = vdkProjectNode_SetGeometry(pProject->pProject, pNode, newType, numPoints, (double*)pPoints);
  }

  return (result == vE_Success);
}

bool vcProject_FetchNodeGeometryAsCartesian(vcProject *pProject, vdkProjectNode *pNode, const udGeoZone &zone, udDouble3 **ppPoints, int *pNumPoints)
{
  if (pProject == nullptr || pNode == nullptr)
    return false;

  udDouble3 *pPoints = udAllocType(udDouble3, pNode->geomCount, udAF_Zero);

  if (pNumPoints != nullptr)
    *pNumPoints = pNode->geomCount;

  if (zone.srid != 0 && pProject->baseZone.srid != 0) // Geolocated
  {
    // Change all points from the projection
    for (int i = 0; i < pNode->geomCount; ++i)
      pPoints[i] = udGeoZone_TransformPoint(((udDouble3*)pNode->pCoordinates)[i], pProject->baseZone, zone);
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
          udSprintf(ppCurrentText, "%s, %s", *ppCurrentText, pAttributionText);
        else
          udSprintf(ppCurrentText, "%s", pAttributionText);
      }
    }

    if (pNode->itemtype == vdkPNT_Folder)
      vcProject_ExtractAttributionText(pNode, ppCurrentText);

    pNode = pNode->pNextSibling;
  }

  
}
