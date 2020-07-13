#include "vcProject.h"

#include "vcSceneItem.h"
#include "vcState.h"
#include "vcRender.h"
#include "vcModals.h"

#include "udFile.h"
#include "udStringUtil.h"

const char *vcProject_ErrorToString(vdkError error)
{
  switch (error)
  {
  case vE_InvalidParameter:
    return vcString::Get("errorInvalidParameter");
  case vE_OpenFailure:
    return vcString::Get("errorOpenFailure");
  case vE_NotSupported:
    return vcString::Get("errorUnsupported");
  case vE_WriteFailure:
    return vcString::Get("errorFileExists");
  case vE_ExceededAllowedLimit:
    return vcString::Get("errorExceedsProjectLimit");
  case vE_Failure: // Falls through
  default:
    return vcString::Get("errorUnknown");
  }
}

void vcProject_UpdateProjectHistory(vcState *pProgramState, const char *pFilename, bool isServerProject);

void vcProject_InitScene(vcState *pProgramState, int srid)
{
  udGeoZone zone = {};
  vcGIS_ChangeSpace(&pProgramState->geozone, zone);

  pProgramState->camera.position = udDouble3::zero();

  pProgramState->sceneExplorer.selectedItems.clear();
  pProgramState->sceneExplorer.clickedItem = {};

  vdkProject_GetProjectRoot(pProgramState->activeProject.pProject, &pProgramState->activeProject.pRoot);

  pProgramState->activeProject.pFolder = new vcFolder(&pProgramState->activeProject, pProgramState->activeProject.pRoot, pProgramState);
  pProgramState->activeProject.pRoot->pUserData = pProgramState->activeProject.pFolder;

  udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, srid);

  int cameraProjection = (srid == vcPSZ_StandardGeoJSON) ? vcPSZ_WGS84ECEF : srid; // use ECEF if its default, otherwise the requested zone

  if (cameraProjection != 0)
  {
    udGeoZone cameraZone = {};
    udGeoZone_SetFromSRID(&cameraZone, cameraProjection);

    if (vcGIS_ChangeSpace(&pProgramState->geozone, cameraZone))
      pProgramState->activeProject.pFolder->ChangeProjection(cameraZone);

    if (cameraProjection == vcPSZ_WGS84ECEF || cameraZone.latLongBoundMin == cameraZone.latLongBoundMax)
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

  vdkProjectNode_SetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", pProgramState->geozone.srid);
  vdkProjectNode_SetMetadataInt(pProgramState->activeProject.pRoot, "projectcrs", srid);
  vdkProject_Save(pProgramState->activeProject.pProject);
}

bool vcProject_CreateBlankScene(vcState *pProgramState, const char *pName, int srid)
{
  if (pProgramState == nullptr || pName == nullptr)
    return false;

  vdkProject *pNewProject = nullptr;
  if (vdkProject_CreateInMemory(&pNewProject, pName) != vE_Success)
    return false;

  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  pProgramState->activeProject.pProject = pNewProject;
  vcProject_InitScene(pProgramState, srid);

  return true;
}

vdkError   vcProject_CreateFileScene(vcState *pProgramState, const char *pFileName, const char *pProjectName, int srid)
{
  if (pProgramState == nullptr)
    return vE_Failure;
  if (pFileName == nullptr || pFileName[0] == '\0')
    return vE_InvalidParameter;
  if (pProjectName == nullptr || pProjectName[0] == '\0')
    return vE_InvalidParameter;

  if (udFileExists(pFileName) == udR_Success)
    return vE_WriteFailure;

  vdkProject *pNewProject = nullptr;
  vdkError error = vdkProject_CreateInFile(&pNewProject, pProjectName, pFileName);
  if (error != vE_Success)
    return error;

  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  pProgramState->activeProject.pProject = pNewProject;
  vcProject_InitScene(pProgramState, srid);

  vcProject_UpdateProjectHistory(pProgramState, pFileName, false);

  return error;
}

vdkError vcProject_CreateServerScene(vcState *pProgramState, const char *pName, const char *pGroupUUID, int srid)
{
  if (pProgramState == nullptr || pName == nullptr || pGroupUUID == nullptr)
    return vE_InvalidParameter;

  vdkProject *pNewProject = nullptr;
  vdkError error = vdkProject_CreateInServer(pProgramState->pVDKContext, &pNewProject, pName, pGroupUUID);
  if (error != vE_Success)
    return error;

  if (pProgramState->activeProject.pProject != nullptr)
    vcProject_Deinit(pProgramState, &pProgramState->activeProject);

  pProgramState->activeProject.pProject = pNewProject;
  vcProject_InitScene(pProgramState, srid);

  const char *pProjectUUID = nullptr;
  vdkProject_GetProjectUUID(pNewProject, &pProjectUUID);
  vcProject_UpdateProjectHistory(pProgramState, pProjectUUID, true);

  return error;
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

void vcProject_UpdateProjectHistory(vcState *pProgramState, const char *pFilename, bool isServerProject)
{
  // replace '\\' with '/'
  char *pFormattedPath = udStrdup(pFilename);
  size_t index = 0;
  while (udStrchr(pFormattedPath, "\\", &index) != nullptr)
    pFormattedPath[index] = '/';

  for (size_t i = 0; i < pProgramState->settings.projectsHistory.projects.length; ++i)
  {
    if (udStrEqual(pFormattedPath, pProgramState->settings.projectsHistory.projects[i].pPath))
    {
      vcSettings_CleanupHistoryProjectItem(&pProgramState->settings.projectsHistory.projects[i]);
      pProgramState->settings.projectsHistory.projects.RemoveAt(i);
      break;
    }
  }

  while (pProgramState->settings.projectsHistory.projects.length >= vcMaxProjectHistoryCount)
  {
    vcSettings_CleanupHistoryProjectItem(&pProgramState->settings.projectsHistory.projects[pProgramState->settings.projectsHistory.projects.length - 1]);
    pProgramState->settings.projectsHistory.projects.PopBack();
  }

  const char *pProjectName = udStrdup(pProgramState->activeProject.pRoot->pName);
  pProgramState->settings.projectsHistory.projects.PushFront({ isServerProject, pProjectName, pFormattedPath });
}

void vcProject_RemoveHistoryItem(vcState *pProgramState, size_t itemPosition)
{
  if (pProgramState == nullptr || itemPosition >= pProgramState->settings.projectsHistory.projects.length)
    return;

  vcSettings_CleanupHistoryProjectItem(&pProgramState->settings.projectsHistory.projects[itemPosition]);
  pProgramState->settings.projectsHistory.projects.RemoveAt(itemPosition);
}

bool vcProject_LoadFromServer(vcState *pProgramState, const char *pProjectID)
{
  vdkProject *pProject = nullptr;
  vdkError vResult = vdkProject_LoadFromServer(pProgramState->pVDKContext, &pProject, pProjectID);
  if (vResult == vE_Success)
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

    int32_t projectZone = vcPSZ_StandardGeoJSON; // LongLat
    vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "projectcrs", &projectZone, vcPSZ_StandardGeoJSON);
    if (projectZone > 0 && udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, projectZone) != udR_Success)
      udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, vcPSZ_StandardGeoJSON);

    int32_t recommendedSRID = -1;
    if (vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", &recommendedSRID, pProgramState->activeProject.baseZone.srid) == vE_Success && recommendedSRID >= 0 && ((udGeoZone_SetFromSRID(&zone, recommendedSRID) == udR_Success) || recommendedSRID == 0))
      vcGIS_ChangeSpace(&pProgramState->geozone, zone);

    const char *pInfo = nullptr;
    if (vdkProjectNode_GetMetadataString(pProgramState->activeProject.pRoot, "information", &pInfo, "") == vE_Success)
      vcModals_OpenModal(pProgramState, vcMT_ProjectInfo);

    vcProject_ExtractCamera(pProgramState);
    vcProject_UpdateProjectHistory(pProgramState, pProjectID, true);
  }
  else
  {
    vcState::ErrorItem projectError;
    projectError.source = vcES_ProjectChange;
    projectError.pData = udStrdup(pProjectID);

    if (vResult == vE_NotFound)
      projectError.resultCode = udR_ObjectNotFound;
    else if (vResult == vE_ParseError)
      projectError.resultCode = udR_ParseError;
    else
      projectError.resultCode = udR_Failure_;

    pProgramState->errorItems.PushBack(projectError);

    vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
  }

  return (pProject != nullptr);
}

bool vcProject_LoadFromURI(vcState *pProgramState, const char *pFilename)
{
  bool success = false;
  vdkProject *pProject = nullptr;
  if (vdkProject_LoadFromFile(&pProject, pFilename) == vE_Success)
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

    int32_t projectZone = vcPSZ_StandardGeoJSON; // LongLat
    vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "projectcrs", &projectZone, vcPSZ_StandardGeoJSON);
    if (projectZone > 0 && udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, projectZone) != udR_Success)
      udGeoZone_SetFromSRID(&pProgramState->activeProject.baseZone, vcPSZ_StandardGeoJSON);

    int32_t recommendedSRID = -1;
    if (vdkProjectNode_GetMetadataInt(pProgramState->activeProject.pRoot, "defaultcrs", &recommendedSRID, pProgramState->activeProject.baseZone.srid) == vE_Success && recommendedSRID >= 0 && ((udGeoZone_SetFromSRID(&zone, recommendedSRID) == udR_Success) || recommendedSRID == 0))
      vcGIS_ChangeSpace(&pProgramState->geozone, zone);

    const char *pInfo = nullptr;
    if (vdkProjectNode_GetMetadataString(pProgramState->activeProject.pRoot, "information", &pInfo, "") == vE_Success)
      vcModals_OpenModal(pProgramState, vcMT_ProjectInfo);

    vcProject_ExtractCamera(pProgramState);
    vcProject_UpdateProjectHistory(pProgramState, pFilename, false);

    success = true;
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

  return success;
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

bool vcProject_Save(vcState *pProgramState)
{
  if (pProgramState == nullptr)
    return false;

  vdkError status = vdkProject_Save(pProgramState->activeProject.pProject);

  if (status != vE_Success)
  {
    vcState::ErrorItem projectError = {};
    projectError.source = vcES_ProjectChange;
    projectError.pData = nullptr;

    if (status == vE_WriteFailure)
      projectError.resultCode = udR_WriteFailure;
    else
      projectError.resultCode = udR_Failure_;

    pProgramState->errorItems.PushBack(projectError);
    vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
  }
  else
  {
    pProgramState->lastSuccessfulSave = udGetEpochSecsUTCf();
  }

  return (status == vE_Success);
}

void vcProject_AutoCompletedName(udFilename *exportFilename, const char *pFileName, const char *pDefaultName)
{
  udFindDir *pDir = nullptr;
  if (!udStrEquali(pFileName, "") && !udStrEndsWithi(pFileName, "/") && !udStrEndsWithi(pFileName, "\\") && udOpenDir(&pDir, pFileName) == udR_Success)
    exportFilename->SetFromFullPath("%s/%s.json", pFileName, pDefaultName);
  else if (exportFilename->HasFilename())
    exportFilename->SetExtension(".json");
  else
    exportFilename->SetFilenameWithExt(udTempStr("%s.json", pDefaultName));

  udCloseDir(&pDir);
}

bool vcProject_SaveAs(vcState *pProgramState, const char *pPath, bool allowOverride)
{
  if (pProgramState == nullptr || pPath == nullptr)
    return false;
  
  udFilename exportFilename(pPath);
  vcProject_AutoCompletedName(&exportFilename, pPath, pProgramState->activeProject.pRoot->pName);

  // Check if file path exists before writing to disk, and if so, the user will be presented with the option to overwrite or cancel
  if (!allowOverride && !vcModals_OverwriteExistingFile(pProgramState, exportFilename.GetPath()))
    return false;

  vcState::ErrorItem projectError = {};
  projectError.source = vcES_ProjectChange;
  projectError.pData = udStrdup(exportFilename.GetFilenameWithExt());

  if (vdkProject_SaveToFile(pProgramState->activeProject.pProject, exportFilename.GetPath()) == vE_Success)
    projectError.resultCode = udR_Success;
  else
    projectError.resultCode = udR_WriteFailure;

  pProgramState->errorItems.PushBack(projectError);

  vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
  vcProject_UpdateProjectHistory(pProgramState, exportFilename.GetPath(), false);

  return (projectError.resultCode == udR_Success);
}

vdkError vcProject_SaveAsServer(vcState *pProgramState, const char *pProjectID)
{
  if (pProgramState == nullptr || pProjectID == nullptr)
    return vE_InvalidParameter;

  vcState::ErrorItem projectError = {};
  projectError.source = vcES_ProjectChange;
  projectError.pData = udStrdup(pProjectID);

  vdkError result = vdkProject_SaveToServer(pProgramState->pVDKContext, pProgramState->activeProject.pProject, pProjectID);

  if (result == vE_Success)
    projectError.resultCode = udR_Success;
  else if (result == vE_ExceededAllowedLimit)
    projectError.resultCode = udR_ExceededAllowedLimit;
  else
    projectError.resultCode = udR_WriteFailure;

  pProgramState->errorItems.PushBack(projectError);

  vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
  vcProject_UpdateProjectHistory(pProgramState, pProjectID, true);

  return result;
}

bool vcProject_AbleToChange(vcState *pProgramState)
{
  if (pProgramState == nullptr || !pProgramState->hasContext)
    return false;

  if (vdkProject_HasUnsavedChanges(pProgramState->activeProject.pProject) == vE_NotFound)
    return true;

  return vcModals_AllowDestructiveAction(pProgramState, vcString::Get("menuChangeScene"), vcString::Get("menuChangeSceneConfirm"));
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

  pProgramState->sceneExplorer.selectStartItem = {};
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

static void vcProject_SelectRange(vcState *pProgramState, vdkProjectNode *pNode1, vdkProjectNode *pNode2)
{
  udChunkedArray<vdkProjectNode *> stack;
  stack.Init(32);
  stack.PushBack(pProgramState->activeProject.pRoot);

  vdkProjectNode *pChild = pProgramState->activeProject.pRoot->pFirstChild;
  bool select = false;
  do
  {
    // If the current node is either of the end nodes in the range, toggle the selection
    bool toggleSelect = (pChild == pNode1 || pChild == pNode2);
    if (toggleSelect)
      select = !select;

    // If selecting (or toggling to include last item), select the current node
    if (select || toggleSelect)
    {
      ((vcSceneItem *)pChild->pUserData)->m_selected = true;
      pProgramState->sceneExplorer.selectedItems.push_back({ stack[stack.length - 1], pChild });
    }

    // If no longer selecting, break out of the loop
    if (toggleSelect && !select)
      break;

    // Add child to the stack to simplify the code below
    stack.PushBack(pChild);

    // Depth first search for the nodes
    if (pChild->pFirstChild)
    {
      pChild = pChild->pFirstChild;
      continue;
    }

    // Try the sibling of the current node (pChild)
    if (pChild->pNextSibling)
    {
      pChild = pChild->pNextSibling;
      stack.PopBack();
      continue;
    }

    // There are no children or siblings, pop up the stack until there's a sibling
    do
    {
      stack.PopBack();
      pChild = stack[stack.length - 1];
    } while (pChild->pNextSibling == nullptr);

    // Pop the current node and try the sibling
    stack.PopBack();
    pChild = pChild->pNextSibling;
  } while (stack.length > 0);

  stack.Deinit();
}

void vcProject_SelectItem(vcState *pProgramState, vdkProjectNode *pParent, vdkProjectNode *pNode)
{
  vcSceneItem *pItem = (vcSceneItem*)pNode->pUserData;

  // If we're doing range selection, else normal selection
  if (pItem != nullptr && pProgramState->sceneExplorer.selectStartItem.pItem != nullptr && pProgramState->sceneExplorer.selectStartItem.pItem != pNode)
  {
    vdkProjectNode *pSelectNode = pProgramState->sceneExplorer.selectStartItem.pItem;
    vcProject_SelectRange(pProgramState, pNode, pSelectNode);
  }
  else if (pItem != nullptr && !pItem->m_selected)
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

bool vcProject_UpdateNodeGeometryFromLatLong(vcProject *pProject, vdkProjectNode *pNode, vdkProjectGeometryType newType, udDouble3 *pPoints, int numPoints)
{
  //TODO: Optimise this
  udGeoZone zone;
  udGeoZone_SetFromSRID(&zone, 4326);

  return vcProject_UpdateNodeGeometryFromCartesian(pProject, pNode, zone, newType, pPoints, numPoints);
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
  //TODO: Cache attribution text; if nothing was added/removed from the scene then it doesn't need to be updated
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
        if (*ppCurrentText == nullptr || udStrstr(*ppCurrentText, 0, pAttributionText) == nullptr)
        {
          if (*ppCurrentText != nullptr)
            udSprintf(ppCurrentText, "%s, %s", *ppCurrentText, pAttributionText);
          else
            udSprintf(ppCurrentText, "%s", pAttributionText);
        }
      }
    }

    if (pNode->itemtype == vdkPNT_Folder)
      vcProject_ExtractAttributionText(pNode, ppCurrentText);

    pNode = pNode->pNextSibling;
  }

  
}
