#include "vcProxyHelper.h"
#include "vcState.h"

#include "vdkConfig.h"
#include "vdkWeb.h"

#include "udStringUtil.h"

#if UDPLATFORM_WINDOWS
# include <winhttp.h>
#endif

vdkError vcProxyHelper_AutoDetectProxy(vcState *pProgramState)
{
#if UDPLATFORM_WINDOWS
  HINTERNET pHttpSession = nullptr;
  HINTERNET pConnect = nullptr;
  HINTERNET pRequest = nullptr;

  WINHTTP_AUTOPROXY_OPTIONS  autoProxyOptions;
  ZeroMemory(&autoProxyOptions, sizeof(autoProxyOptions));

  WINHTTP_PROXY_INFO proxyInfo;
  ZeroMemory(&proxyInfo, sizeof(proxyInfo));

  udURL serverURL = {};

  if (pProgramState->settings.loginInfo.serverURL[0] == '\0')
    serverURL.SetURL(pProgramState->settings.loginInfo.proxyTestURL);
  else
    serverURL.SetURL(pProgramState->settings.loginInfo.serverURL);

  pHttpSession = WinHttpOpen(L"udStream AutoProxy Request/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

  if (!pHttpSession)
    goto Exit;

  pConnect = WinHttpConnect(pHttpSession, udOSString(serverURL.GetDomain()), (INTERNET_PORT)serverURL.GetPort(), 0);

  if (!pConnect)
    goto Exit;

  pRequest = WinHttpOpenRequest(pConnect, L"GET", L"index.html", L"HTTP/1.1", WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

  if (!pRequest)
    goto Exit;

  // Use auto-detection because the Proxy Auto-Config URL is not known.
  autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;

  // Use DHCP and DNS-based auto-detection.
  autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;

  // If obtaining the PAC script requires NTLM/Negotiate authentication,
  // then automatically supply the client domain credentials.
  //
  // This is much faster when FALSE, might be worth trying with FALSE first.
  autoProxyOptions.fAutoLogonIfChallenged = TRUE;

  // Get proxy for the URL, otherwise fall back to IE settings.
  if (WinHttpGetProxyForUrl(pHttpSession, udOSString(pProgramState->settings.loginInfo.serverURL), &autoProxyOptions, &proxyInfo))
  {
    if (proxyInfo.dwAccessType != WINHTTP_ACCESS_TYPE_NO_PROXY)
      udStrcpy(pProgramState->settings.loginInfo.autoDetectProxyURL, udOSString(proxyInfo.lpszProxy));
  }
  else
  {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
    ZeroMemory(&proxyConfig, sizeof(proxyConfig));
    if (WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
    {
      // Avoid crashing in udOSString
      if (proxyConfig.lpszProxy != nullptr)
        udStrcpy(pProgramState->settings.loginInfo.autoDetectProxyURL, udOSString(proxyConfig.lpszProxy));
    }

    if (proxyConfig.lpszAutoConfigUrl != nullptr)
      GlobalFree(proxyConfig.lpszAutoConfigUrl);

    if (proxyConfig.lpszProxy != nullptr)
      GlobalFree(proxyConfig.lpszProxy);

    if (proxyConfig.lpszProxyBypass != nullptr)
      GlobalFree(proxyConfig.lpszProxyBypass);
  }

Exit:
  if (proxyInfo.lpszProxy != nullptr)
    GlobalFree(proxyInfo.lpszProxy);

  if (proxyInfo.lpszProxyBypass != nullptr)
    GlobalFree(proxyInfo.lpszProxyBypass);

  if (pRequest != nullptr)
    WinHttpCloseHandle(pRequest);

  if (pConnect != nullptr)
    WinHttpCloseHandle(pConnect);

  if (pHttpSession != nullptr)
    WinHttpCloseHandle(pHttpSession);
#endif

  return vdkConfig_ForceProxy(pProgramState->settings.loginInfo.autoDetectProxyURL);
}

vdkError vcProxyHelper_TestProxy(vcState *pProgramState)
{
  const char *pResult = nullptr;

  const char *pServerURL;

  if (pProgramState->settings.loginInfo.serverURL[0] == '\0')
    pServerURL = pProgramState->settings.loginInfo.proxyTestURL;
  else
    pServerURL = pProgramState->settings.loginInfo.serverURL;

  vdkError vResult = vdkWeb_Request(pServerURL, &pResult, nullptr, nullptr);

  if (vResult == vE_Success)
    vdkWeb_ReleaseResponse(&pResult);

  return vResult;
}

vdkError vcProxyHelper_SetUserAndPass(vcState *pProgramState, const char *pProxyUsername, const char *pProxyPassword)
{
  vdkError vResult = vdkConfig_SetProxyAuth(pProxyUsername, pProxyPassword);

  if (vResult == vE_Success)
    return vcProxyHelper_TestProxy(pProgramState);
  else
    return vResult;
}
