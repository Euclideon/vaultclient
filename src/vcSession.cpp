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

#ifdef GIT_TAG
#  define TAG_SUFFIX VCVERSION_VERSION_STRING
#elif VCVERSION_BUILD_NUMBER > 0
#  define TAG_SUFFIX VCVERSION_VERSION_STRING " Internal Build"
#else
#  define TAG_SUFFIX VCSTRINGIFY(VCVERSION_VERSION_ARRAY_PARTIAL) " Developer Build"
#endif

void vcSession_GetProjectsWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  const char *pProjData = nullptr;
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "dev/projects", nullptr, &pProjData) == vE_Success)
    pProgramState->projects.Parse(pProjData);
  vdkServerAPI_ReleaseResult(&pProjData);
}

void vcSession_GetPackagesWT(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  const char *pPackageData = nullptr;
  const char *pPostJSON = udTempStr("{ \"packagename\": \"EuclideonVaultClient\", \"packagevariant\": \"%s\" }", vcSession_GetOSName());
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

void vcSession_GetProfileInfo(void *pProgramStatePtr)
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

  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetProjectsWT, pProgramState, false);
  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetPackagesWT, pProgramState, false, vcSession_GetPackagesMT);

  if (pProgramState->settings.presentation.loginRenderLicense)
    vdkContext_RequestLicense(pProgramState->pVDKContext, vdkLT_Render);

  //Context Login successful
  memset(pProgramState->password, 0, sizeof(pProgramState->password));
  if (!pProgramState->settings.loginInfo.rememberServer)
    memset(pProgramState->settings.loginInfo.serverURL, 0, sizeof(pProgramState->settings.loginInfo.serverURL));

  if (!pProgramState->settings.loginInfo.rememberUsername)
    memset(pProgramState->settings.loginInfo.username, 0, sizeof(pProgramState->settings.loginInfo.username));

  vdkContext_GetSessionInfo(pProgramState->pVDKContext, &pProgramState->sessionInfo);

  pProgramState->logoutReason = vE_Success;
  pProgramState->loginStatus = vcLS_NoStatus;
  pProgramState->hasContext = true;

  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcSession_GetProfileInfo, pProgramState, false);
}

void vcSession_Login(void *pProgramStatePtr)
{
  vdkError result;
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  result = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, "Euclideon Vault Client / " TAG_SUFFIX, pProgramState->settings.loginInfo.username, pProgramState->password);
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

  pProgramState->logoutReason = result;

  if (result != vE_Success)
    return;

  vcSession_ChangeSession(pProgramState);
}

void vcSession_Logout(vcState *pProgramState)
{
  pProgramState->hasContext = false;
  pProgramState->forceLogout = false;

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

    pProgramState->modelPath[0] = '\0';
    vcProject_InitBlankScene(pProgramState);
    pProgramState->projects.Destroy();
    pProgramState->profileInfo.Destroy();
    vcRender_ClearPoints(pProgramState->pRenderContext);

    memset(&pProgramState->gis, 0, sizeof(pProgramState->gis));
    vdkContext_Disconnect(&pProgramState->pVDKContext);

    vcModals_OpenModal(pProgramState, vcMT_LoggedOut);
  }
}

void vcSession_Resume(vcState *pProgramState)
{
  bool tryDongle = true;

#if UDPLATFORM_WINDOWS
  tryDongle = !IsDebuggerPresent();
#endif

  if (vdkContext_TryResume(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, "Euclideon Vault Client / " TAG_SUFFIX, pProgramState->settings.loginInfo.username, tryDongle) == vE_Success)
    vcSession_ChangeSession(pProgramState);
}

void vcSession_UpdateInfo(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;
  vdkError response = vdkContext_KeepAlive(pProgramState->pVDKContext);

  pProgramState->logoutReason = response;

  if (response == vE_SessionExpired)
    pProgramState->forceLogout = true;
  else
    vdkContext_GetSessionInfo(pProgramState->pVDKContext, &pProgramState->sessionInfo);
}
