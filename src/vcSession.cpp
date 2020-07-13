#include "vcSession.h"

#include "vcState.h"
#include "vcModals.h"
#include "vcRender.h"
#include "vcConvert.h"
#include "vcVersion.h"

#include "vdkContext.h"
#include "vdkServerAPI.h"

#include "udStringUtil.h"

const char* vcSession_GetOSName()
{
#if UDPLATFORM_ANDROID
  return "Android";
#elif UDPLATFORM_EMSCRIPTEN
  return "Emscripten";
#elif UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  return "iOS";
#elif UDPLATFORM_LINUX
  // TODO: Handle other distributions
  return "Ubuntu";
#elif UDPLATFORM_OSX
  return "macOS";
#elif UDPLATFORM_WINDOWS
  return "Windows";
#else
  return "Unknown";
#endif
}


void vcSession_UpdateProjectsWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  const char *pProjData = nullptr;

  char reqBuffer[512];
  double highestTime = pProgramState->highestProjectTime;

  udJSON parsed;

  do
  {
    udSprintf(reqBuffer, "{ \"lastupdated\": %f, \"count\": 50 }", highestTime);

    if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/projects/changed", reqBuffer, &pProjData) == vE_Success)
    {
      parsed.Parse(pProjData);
      vdkServerAPI_ReleaseResult(&pProjData);

      udJSONArray *pItems = parsed.Get("projects").AsArray();
      if (pItems != nullptr && pItems->length > 0)
      {
        for (size_t i = 0; i < pItems->length; ++i)
        {
          const udJSON *pItem = pItems->GetElement(i);

          udUUID groupID = {};
          udUUID projectID = {};

          udUUID_SetFromString(&projectID, pItem->Get("projectid").AsString());
          udUUID_SetFromString(&groupID, pItem->Get("groupid").AsString());

          bool found = false;
          vcProjectInfo *pInfo = nullptr;

          udWriteLockRWLock(pProgramState->pSessionLock);
          for (size_t groupIter = 0; groupIter < pProgramState->groups.length; ++groupIter)
          {
            vcGroupInfo *pGroup = pProgramState->groups.GetElement(groupIter);

            if (pGroup->groupID == groupID)
            {

              for (size_t projectIter = 0; projectIter < pGroup->projects.length; ++projectIter)
              {
                if (pGroup->projects[projectIter].projectID == projectID)
                {
                  pInfo = pGroup->projects.GetElement(projectIter);
                  found = true;
                }
              }

              if (!found)
              {
                pInfo = pGroup->projects.PushBack();
                found = true;
              }
            }
          }

          if (!found)
          {
            vcGroupInfo *pGroupInfo = pProgramState->groups.PushBack();

            pGroupInfo->groupID = groupID;
            pGroupInfo->pGroupName = udStrdup(pItem->Get("groupname").AsString());
            pGroupInfo->pDescription = nullptr;
            pGroupInfo->visibility = vcGroupVisibility_Unknown;
            pGroupInfo->permissionLevel = vcGroupPermissions_Guest;

            pGroupInfo->projects.Init(8);
            pInfo = pGroupInfo->projects.PushBack();
          }

          udFree(pInfo->pProjectName);

          pInfo->projectID = projectID;
          pInfo->pProjectName = udStrdup(pItem->Get("name").AsString());
          pInfo->lastUpdated = pItem->Get("lastupdated").AsDouble();
          pInfo->isShared = pItem->Get("isshared").AsBool();

          highestTime = udMax(highestTime, pInfo->lastUpdated + 0.0001);

          udWriteUnlockRWLock(pProgramState->pSessionLock);
        }
      }

      if (parsed.Get("count").AsInt() == 0)
        break;
    }
    else
    {
      break;
    }
  } while (true);

  pProgramState->highestProjectTime = highestTime;
}

void vcSession_GetGroupsWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  const char *pProjData = nullptr;

  char reqBuffer[512];
  int offset = 0;

  udJSON parsed;

  do
  {
    udSprintf(reqBuffer, "{ \"index\": %d, \"count\": 50 }", offset);

    if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/groups", reqBuffer, &pProjData) == vE_Success)
    {
      parsed.Parse(pProjData);
      vdkServerAPI_ReleaseResult(&pProjData);

      size_t arrayLen = parsed.Get("groups").ArrayLength();

      if (parsed.Get("success").AsBool() && arrayLen > 0)
      {
        offset += parsed.Get("total").AsInt();

        for (size_t i = 0; i < arrayLen; ++i)
        {
          udWriteLockRWLock(pProgramState->pSessionLock);

          vcGroupInfo *pGroupInfo = pProgramState->groups.PushBack();

          pGroupInfo->pGroupName = udStrdup(parsed.Get("groups[%zu].name", i).AsString());
          pGroupInfo->pDescription = udStrdup(parsed.Get("groups[%zu].description", i).AsString());

          const char *pVisString = parsed.Get("groups[%zu].visibility", i).AsString();
          if (udStrEqual(pVisString, "Public"))
            pGroupInfo->visibility = vcGroupVisibility_Public;
          else if (udStrEqual(pVisString, "Internal"))
            pGroupInfo->visibility = vcGroupVisibility_Internal;
          else
            pGroupInfo->visibility = vcGroupVisibility_Private;

          pGroupInfo->permissionLevel = (vcGroupPermissions)parsed.Get("groups[%zu].permissions", i).AsInt();
          udUUID_SetFromString(&pGroupInfo->groupID, parsed.Get("groups[%zu].groupid", i).AsString());

          pGroupInfo->projects.Init(8);

          udWriteUnlockRWLock(pProgramState->pSessionLock);
        }
      }
      else
      {
        break;
      }
    }
    else
    {
      break;
    }
  } while (true);

  vcSession_UpdateProjectsWT(pProgramStatePtr);
}

void vcSession_GetFeaturedProjectsWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  const char *pProjData = nullptr;

  udJSON parsed;

  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/projects/featured", nullptr, &pProjData) == vE_Success)
  {
    parsed.Parse(pProjData);
    vdkServerAPI_ReleaseResult(&pProjData);

    size_t arrayLen = parsed.Get("projects").ArrayLength();

    if (parsed.Get("success").AsBool() && arrayLen > 0)
    {
      for (size_t i = 0; i < arrayLen; ++i)
      {
        udWriteLockRWLock(pProgramState->pSessionLock);

        vcFeaturedProjectInfo *pInfo = pProgramState->featuredProjects.PushBack();

        udUUID_SetFromString(&pInfo->projectID, parsed.Get("projects[%zu].projectid", i).AsString());
        pInfo->pProjectName = udStrdup(parsed.Get("projects[%zu].name", i).AsString());
        vcTexture_AsyncCreateFromFilename(&pInfo->pTexture, pProgramState->pWorkerPool, parsed.Get("projects[%zu].thumb", i).AsString());

        udWriteUnlockRWLock(pProgramState->pSessionLock);
      }
    }
  }
}

void vcSession_GetPackagesWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  const char *pPackageData = nullptr;
  const char *pPostJSON = udTempStr("{ \"packagename\": \"udStream\", \"packagevariant\": \"%s\" }", vcSession_GetOSName());
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/packages/latest", pPostJSON, &pPackageData) == vE_Success)
    pProgramState->packageInfo.Parse(pPackageData);

  vdkServerAPI_ReleaseResult(&pPackageData);
}

void vcSession_GetPackagesMT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  if (pProgramState->packageInfo.Get("success").AsBool())
  {
    if (pProgramState->packageInfo.Get("package.versionnumber").AsInt() <= VCVERSION_BUILD_NUMBER || VCVERSION_BUILD_NUMBER == 0)
    {
      pProgramState->packageInfo.Destroy();
    }
    else
    {
      pProgramState->openSettings = true;
      pProgramState->activeSetting = vcSR_Update;
    }
  }
}

void vcSession_GetProfileInfoWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  const char *pJSONInfo = nullptr;
  udJSON sessionInfo = {};

  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/session/info", nullptr, &pJSONInfo) == vE_Success)
    sessionInfo.Parse(pJSONInfo);
  vdkServerAPI_ReleaseResult(&pJSONInfo);

  udJSON v;
  const char *pExportString = nullptr;
  v.Set("userid = '%s'", sessionInfo.Get("user.userid").AsString(""));
  v.Export(&pExportString, udJEO_JSON); // or udJEO_XML

  if (vdkServerAPI_Query(pProgramState->pVDKContext, "v1/user", pExportString, &pJSONInfo) == vE_Success)
    pProgramState->profileInfo.Parse(pJSONInfo);

  vdkServerAPI_ReleaseResult(&pJSONInfo);
  udFree(pExportString);
  v.Destroy();

  sessionInfo.Destroy();
}

void vcSession_ChangeSession(vcState *pProgramState)
{
  vcRender_SetVaultContext(pProgramState, pProgramState->pRenderContext);
  vdkContext_GetSessionInfo(pProgramState->pVDKContext, &pProgramState->sessionInfo);

  pProgramState->featuredProjects.Init(8);
  pProgramState->groups.Init(8);

  if (!pProgramState->sessionInfo.isOffline)
  {
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetFeaturedProjectsWT, pProgramState, false);
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetGroupsWT, pProgramState, false);
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetPackagesWT, pProgramState, false, vcSession_GetPackagesMT);
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetProfileInfoWT, pProgramState, false);

    pProgramState->lastSync = udGetEpochSecsUTCf();

    //Context Login successful
    memset(pProgramState->password, 0, sizeof(pProgramState->password));
    if (!pProgramState->settings.loginInfo.rememberServer)
      memset(pProgramState->settings.loginInfo.serverURL, 0, sizeof(pProgramState->settings.loginInfo.serverURL));

    if (!pProgramState->settings.loginInfo.rememberEmail)
      memset(pProgramState->settings.loginInfo.email, 0, sizeof(pProgramState->settings.loginInfo.email));
  }

  pProgramState->logoutReason = vE_Success;
  pProgramState->loginStatus = vcLS_NoStatus;
  pProgramState->hasContext = true;

  vcModals_OpenModal(pProgramState, vcMT_Welcome);
}

void vcSession_Login(void *pProgramStatePtr)
{
  vdkError result;
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  result = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, VCAPPNAME, pProgramState->settings.loginInfo.email, pProgramState->password);
  if (result == vE_ConnectionFailure)
    pProgramState->loginStatus = vcLS_ConnectionError;
  else if (result == vE_AuthFailure)
    pProgramState->loginStatus = vcLS_AuthError;
  else if (result == vE_OutOfSync)
    pProgramState->loginStatus = vcLS_TimeSync;
  else if (result == vE_SecurityFailure)
    pProgramState->loginStatus = vcLS_SecurityError;
  else if (result == vE_ServerFailure || result == vE_ParseError)
    pProgramState->loginStatus = vcLS_NegotiationError;
  else if (result == vE_ProxyError)
    pProgramState->loginStatus = vcLS_ProxyError;
  else if (result == vE_ProxyAuthRequired)
    pProgramState->loginStatus = vcLS_ProxyAuthRequired;
  else if (result != vE_Success)
    pProgramState->loginStatus = vcLS_OtherError;

  if (pProgramState->loginStatus == vcLS_ConnectionError || pProgramState->loginStatus == vcLS_SecurityError || pProgramState->loginStatus == vcLS_ProxyError || pProgramState->loginStatus == vcLS_ProxyAuthRequired)
  {
    pProgramState->openSettings = true;
    pProgramState->activeSetting = vcSR_Connection;
    pProgramState->settings.loginInfo.tested = true;
    pProgramState->settings.loginInfo.testStatus = result;
  }

  pProgramState->logoutReason = result;

  if (result != vE_Success)
    return;

  vcSettings_Save(&pProgramState->settings);
  vcSession_ChangeSession(pProgramState);
}

void vcSession_Logout(vcState *pProgramState)
{
  if (pProgramState->pVDKContext != nullptr)
  {
#if VC_HASCONVERT
    // Cancel all convert jobs
    if (pProgramState->pConvertContext != nullptr)
    {
      // Cancel all jobs
      for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; i++)
      {
        vdkConvert_Cancel(pProgramState->pConvertContext->jobs[i]->pConvertContext);
      }

      // Wait for jobs to finish and destroy them
      while (pProgramState->pConvertContext->jobs.length != 0)
      {
        for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; i++)
        {
          if (pProgramState->pConvertContext->jobs[i]->status != vcCQS_Running)
          {
            vcConvert_RemoveJob(pProgramState, i);
            --i;
          }
        }
      }
    }
#endif //VC_HASCONVERT
    memset(&pProgramState->geozone, 0, sizeof(pProgramState->geozone));

    pProgramState->modelPath[0] = '\0';
    vcProject_CreateBlankScene(pProgramState, "Empty Project", vcPSZ_StandardGeoJSON);
    vcSession_CleanupSession(pProgramState);
    pProgramState->profileInfo.Destroy();

    vcRender_RemoveVaultContext(pProgramState->pRenderContext);
    vdkContext_Disconnect(&pProgramState->pVDKContext, pProgramState->forceLogout);

    vcModals_OpenModal(pProgramState, vcMT_LoggedOut);
  }

  pProgramState->hasContext = false;
  pProgramState->forceLogout = false;
}

void vcSession_Resume(vcState *pProgramState)
{
  bool tryDongle = true;

#if UDPLATFORM_WINDOWS
  tryDongle = !IsDebuggerPresent();
#endif

  if (vdkContext_TryResume(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, VCAPPNAME, pProgramState->settings.loginInfo.email, tryDongle) == vE_Success)
    vcSession_ChangeSession(pProgramState);
}

void vcSession_UpdateInfo(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  vdkError response = vdkContext_GetSessionInfo(pProgramState->pVDKContext, &pProgramState->sessionInfo);

  pProgramState->lastSync = udGetEpochSecsUTCf();

  if (response == vE_SessionExpired)
  {
    pProgramState->logoutReason = response;
    pProgramState->forceLogout = true;
  }
  else if (!pProgramState->sessionInfo.isOffline)
  {
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_UpdateProjectsWT, pProgramState, false);
  }
}

void vcSession_CleanupSession(vcState *pProgramState)
{
  udWriteLockRWLock(pProgramState->pSessionLock);

  pProgramState->highestProjectTime = 0.0;
  pProgramState->lastSync = 0.0;

  if (pProgramState->groups.length > 0)
  {
    for (auto item : pProgramState->groups)
    {
      udFree(item.pGroupName);
      udFree(item.pDescription);

      if (item.projects.length > 0)
      {
        for (auto project : item.projects)
        {
          udFree(project.pProjectName);
        }
      }

      item.projects.Deinit();
    }

    pProgramState->groups.Deinit();
  }

  if (pProgramState->featuredProjects.length > 0)
  {
    for (auto item : pProgramState->featuredProjects)
    {
      udFree(item.pProjectName);
      vcTexture_Destroy(&item.pTexture);
    }
    pProgramState->featuredProjects.Deinit();
  }

  udWriteUnlockRWLock(pProgramState->pSessionLock);
}
