#include "vcModals.h"

#include "vcState.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"
#include "vcPOI.h"
#include "gl/vcTexture.h"
#include "vcRender.h"
#include "vcStrings.h"
#include "vcConvert.h"
#include "vcProxyHelper.h"

#include "vCore/vStringFormat.h"

#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcFileDialog.h"

#include "stb_image.h"

void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup(vcString::Get("menuLogoutTitle"));

  if (ImGui::BeginPopupModal(vcString::Get("menuLogoutTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("menuLogoutMessage"));

    if (ImGui::Button(vcString::Get("menuLogoutCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_LoggedOut);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawProxyAuth(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProxyAuth))
    ImGui::OpenPopup("modalProxy");

  if (ImGui::BeginPopupModal("modalProxy", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
  {
    pProgramState->modalOpen = true;

    // These are here so they don't leak
    static char proxyUsername[256];
    static char proxyPassword[256];

    bool closePopup = false;
    bool tryLogin = false;
    static bool detailsGood = true;

    // Info
    ImGui::TextUnformatted(vcString::Get("modalProxyInfo"));

    if (!detailsGood)
      ImGui::TextUnformatted(vcString::Get("modalProxyBadCreds"));

    // Username
    tryLogin |= ImGui::InputText(vcString::Get("modalProxyUsername"), proxyUsername, udLengthOf(proxyUsername), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SetItemDefaultFocus();

    // Password
    tryLogin |= ImGui::InputText(vcString::Get("modalProxyPassword"), proxyPassword, udLengthOf(proxyPassword), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

    if (ImGui::Button(vcString::Get("modalProxyAuthContinueButton")) || tryLogin)
    {
      if (vcProxyHelper_SetUserAndPass(pProgramState, proxyUsername, proxyPassword) == vE_Success)
        closePopup = true;
      else
        detailsGood = false;
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("modalProxyAuthCancelButton")) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      pProgramState->loginStatus = vcLS_ProxyAuthFailed; // Change the error so it doesn't try login when the modal closes
      closePopup = true;
    }

    if (closePopup)
    {
      detailsGood = true;

      memset(proxyUsername, 0, sizeof(proxyUsername));
      memset(proxyPassword, 0, sizeof(proxyPassword));

      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_ProxyAuth);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawReleaseNotes(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ReleaseNotes))
    ImGui::OpenPopup(vcString::Get("menuReleaseNotesTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuReleaseNotesTitle")))
  {
    pProgramState->modalOpen = true;
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client / %s", vcString::Get("menuReleaseNotesTitle"));

    ImGui::Text("%s: %s", vcString::Get("menuCurrentVersion"), VCVERSION_PRODUCT_STRING);

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("menuReleaseNotesCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    if (pProgramState->pReleaseNotes == nullptr)
      udFile_Load("asset://releasenotes.md", (void**)&pProgramState->pReleaseNotes);

    if (pProgramState->pReleaseNotes != nullptr)
    {
      ImGui::BeginChild(vcString::Get("menuReleaseNotesShort"));
      ImGui::TextUnformatted(pProgramState->pReleaseNotes);
      ImGui::EndChild();
    }
    else
    {
      ImGui::TextUnformatted(vcString::Get("menuReleaseNotesFail"));
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawAbout(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_About))
    ImGui::OpenPopup(vcString::Get("menuAboutTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuAboutTitle")))
  {
    pProgramState->modalOpen = true;
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client / %s", vcString::Get("menuAboutLicenseInfo"));

    ImGui::Text("%s: %s", vcString::Get("menuAboutVersion"), VCVERSION_PRODUCT_STRING);

    char strBuf[128];
    if (pProgramState->packageInfo.Get("success").AsBool())
      ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", vStringFormat(strBuf, udLengthOf(strBuf), vcString::Get("menuAboutPackageUpdate"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("menuAboutCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild("Licenses");
    for (int i = 0; i < (int)UDARRAYSIZE(ThirdPartyLicenses); i++)
    {
      // ImGui::Text has a limitation of 3072 bytes.
      ImGui::TextUnformatted(ThirdPartyLicenses[i].pName);
      ImGui::TextUnformatted(ThirdPartyLicenses[i].pLicense);
      ImGui::Separator();
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_DrawNewVersionAvailable(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_NewVersionAvailable))
    ImGui::OpenPopup(vcString::Get("menuNewVersionAvailableTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuNewVersionAvailableTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client");

    ImGui::Text("%s: %s", vcString::Get("menuCurrentVersion"), VCVERSION_PRODUCT_STRING);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.f, 0.5f, 1.f));
    ImGui::TextWrapped("%s: %s", vcString::Get("menuNewVersion"), pProgramState->packageInfo.Get("package.versionstring").AsString());
    ImGui::PopStyleColor();

    ImGui::TextWrapped("%s", vcString::Get("menuNewVersionDownloadPrompt"));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("menuNewVersionCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_NewVersionAvailable);
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild(vcString::Get("menuReleaseNotesTitle"), ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(pProgramState->packageInfo.Get("package.releasenotes").AsString(""));
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_SetTileImage(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  char buf[256];
  udSprintf(buf, udLengthOf(buf), "%s/0/0/0.%s", pProgramState->settings.maptiles.tileServerAddress, pProgramState->settings.maptiles.tileServerExtension);

  int64_t imageSize;
  void *pLocalData = nullptr;

  if (udFile_Load(buf, &pLocalData, &imageSize) != udR_Success)
    imageSize = -2;

  pProgramState->tileModal.loadStatus = imageSize;
  udFree(pProgramState->tileModal.pImageData);
  udInterlockedExchangePointer(&pProgramState->tileModal.pImageData, pLocalData);
}

inline bool vcModals_TileThread(vcState *pProgramState)
{
  if (pProgramState->tileModal.loadStatus == -1)
    return false; // Avoid the thread starting if its already running

  pProgramState->tileModal.loadStatus = -1; // Loading
  vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);

  size_t urlLen = udStrlen(pProgramState->settings.maptiles.tileServerAddress);
  if (urlLen == 0)
  {
    pProgramState->tileModal.loadStatus = -2;
    return true;
  }

  if (pProgramState->settings.maptiles.tileServerAddress[urlLen - 1] == '/')
    pProgramState->settings.maptiles.tileServerAddress[urlLen - 1] = '\0';

  vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);

  return true;
}

void vcModals_DrawTileServer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_TileServer))
  {
    ImGui::OpenPopup(vcString::Get("settingsMapsTileServerTitle"));
    if (pProgramState->tileModal.pServerIcon == nullptr)
      vcModals_TileThread(pProgramState);
  }

  ImGui::SetNextWindowSize(ImVec2(300, 342), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("settingsMapsTileServerTitle")))
  {
    pProgramState->modalOpen = true;
    // If there is loaded data, we turn it into a texture:
    if (pProgramState->tileModal.loadStatus > 0)
    {
      if (pProgramState->tileModal.pServerIcon != nullptr)
        vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);

      uint32_t width, height, channelCount;
      uint8_t *pData = stbi_load_from_memory((stbi_uc*)pProgramState->tileModal.pImageData, (int)pProgramState->tileModal.loadStatus, (int*)&width, (int*)&height, (int*)&channelCount, 4);
      udFree(pProgramState->tileModal.pImageData);

      if (pData)
        vcTexture_Create(&pProgramState->tileModal.pServerIcon, width, height, pData, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Repeat, vcTCF_None, 0);

      stbi_image_free(pData);
      pProgramState->tileModal.loadStatus = 0;
    }

    static bool s_isDirty = false;
    static int s_currentItem = -1;

    const char *pItems[] = { "png", "jpg" };
    if (s_currentItem == -1)
    {
      for (int i = 0; i < (int)udLengthOf(pItems); ++i)
      {
        if (udStrEquali(pItems[i], pProgramState->settings.maptiles.tileServerExtension))
          s_currentItem = i;
      }
    }

    if (ImGui::InputText(vcString::Get("settingsMapsTileServer"), pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength))
      s_isDirty = true;

    if (ImGui::Combo(vcString::Get("settingsMapsTileServerImageFormat"), &s_currentItem, pItems, (int)udLengthOf(pItems)))
    {
      udStrcpy(pProgramState->settings.maptiles.tileServerExtension, udLengthOf(pProgramState->settings.maptiles.tileServerExtension), pItems[s_currentItem]);
      vcModals_TileThread(pProgramState);
      s_isDirty = true;
    }

    if (s_isDirty)
      s_isDirty = !vcModals_TileThread(pProgramState);

    ImGui::SetItemDefaultFocus();

    if (pProgramState->tileModal.loadStatus == -1)
      ImGui::TextUnformatted(vcString::Get("settingsMapsTileServerLoading"));
    else if (pProgramState->tileModal.loadStatus == -2)
      ImGui::TextColored(ImVec4(255, 0, 0, 255), "%s", vcString::Get("settingsMapsTileServerErrorFetching"));
    else if (pProgramState->tileModal.pServerIcon != nullptr)
      ImGui::Image((ImTextureID)pProgramState->tileModal.pServerIcon, ImVec2(200, 200), ImVec2(0, 0), ImVec2(1, 1));

    if (pProgramState->tileModal.loadStatus != -1 && (ImGui::Button(vcString::Get("settingsMapsTileServerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE]))
    {
      ImGui::CloseCurrentPopup();
      udFree(pProgramState->tileModal.pImageData);
      vcRender_ClearTiles(pProgramState->pRenderContext);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawFileModal(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_AddUDS))
    ImGui::OpenPopup(vcString::Get("sceneExplorerAddUDSTitle"));
  if (pProgramState->openModals & (1 << vcMT_ImportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectImportTitle"));
  if (pProgramState->openModals & (1 << vcMT_ExportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectExportTitle"));

  vcModalTypes mode = vcMT_Count;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerAddUDSTitle")))
    mode = vcMT_AddUDS;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectImportTitle")))
    mode = vcMT_ImportProject;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectExportTitle")))
    mode = vcMT_ExportProject;

  if (mode < vcMT_Count)
  {
    pProgramState->modalOpen = true;
    bool pressedEnter = ImGui::InputText(vcString::Get("sceneExplorerPathURL"), pProgramState->modelPath, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    bool loadFile = false;
    bool saveFile = false;
    if (mode == vcMT_ExportProject)
      saveFile = (ImGui::Button(vcString::Get("sceneExplorerExportButton"), ImVec2(100.f, 0)) || pressedEnter);
    else
      loadFile = (ImGui::Button(vcString::Get("sceneExplorerLoadButton"), ImVec2(100.f, 0)) || pressedEnter);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();

    if (mode == vcMT_AddUDS)
    {
      const char *fileExtensions[] = { ".uds", ".ssf", ".udg" };
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
        loadFile = true;
    }
    else if (mode == vcMT_ImportProject)
    {
      const char *fileExtensions[] = { ".json", ".udp" };
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
        loadFile = true;
    }
    else if (mode == vcMT_ExportProject)
    {
      const char *fileExtensions[] = { ".json" };
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), false, fileExtensions, udLengthOf(fileExtensions)))
        saveFile = true;
    }

    if (loadFile)
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    if (saveFile)
    {
      const char *pOutput = nullptr;
      if (vdkProject_WriteToMemory(pProgramState->sceneExplorer.pProject, &pOutput) == vE_Success)
      {
        const char *pExportFilename;
        if (udStrEndsWithi(pProgramState->modelPath, ".json"))  // If user has entered a path and filename, or selected a file to overwrite e.g. C:/Data/proj.json
          pExportFilename = udTempStr("%s", pProgramState->modelPath);
        else if (udStrEndsWithi(pProgramState->modelPath, "/")) // If user has entered a path to a folder (will fail if doesn't exist)      e.g. C:/Data/
          pExportFilename = udTempStr("%svc_saved_project.json", pProgramState->modelPath);
        else if (udFileExists(pProgramState->modelPath) == udR_Success) // If user has entered a path to a folder with no slash             e.g. C:/Data
          pExportFilename = udTempStr("%s/vc_saved_project.json", pProgramState->modelPath);
        else                                                          // If user has entered a path and filename but no valid extension     e.g. C:/Data/proj
          pExportFilename = udTempStr("%s.json", pProgramState->modelPath);

        if (udFile_Save(pExportFilename, (void*)pOutput, udStrlen(pOutput)) != udR_Success)
          vcModals_OpenModal(pProgramState, vcMT_ProjectChangeFailed);
        else
          vcModals_OpenModal(pProgramState, vcMT_ProjectChangeSucceeded);
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawLoadWatermark(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoadWatermark))
    ImGui::OpenPopup(vcString::Get("convertLoadWatermark"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertLoadWatermark")))
  {
    pProgramState->modalOpen = true;
    bool pressedEnter = ImGui::InputText(vcString::Get("convertPathURL"), pProgramState->modelPath, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("convertLoadButton"), ImVec2(100.f, 0)) || pressedEnter)
    {
      vdkConvert_AddWatermark(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext, pProgramState->modelPath);
      pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->watermark.isDirty = true;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("convertCancelButton"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".jpg", ".png", ".tga", ".bmp", ".gif" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
    {
      vdkConvert_AddWatermark(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext, pProgramState->modelPath);
      pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->watermark.isDirty = true;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawProjectChangeResult(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProjectChangeFailed))
    ImGui::OpenPopup(vcString::Get("sceneExplorerProjectChangeFailedTitle"));
  else if (pProgramState->openModals & (1 << vcMT_ProjectChangeSucceeded))
    ImGui::OpenPopup(vcString::Get("sceneExplorerProjectChangeSucceededTitle"));

  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerProjectChangeFailedTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("sceneExplorerProjectChangeFailedMessage"));

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
  else if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerProjectChangeSucceededTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("sceneExplorerProjectChangeSucceededMessage"));

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawProjectReadOnly(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProjectReadOnly))
    ImGui::OpenPopup(vcString::Get("sceneExplorerProjectReadOnlyTitle"));

  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerProjectReadOnlyTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("sceneExplorerProjectReadOnlyMessage"));

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawUnsupportedFiles(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_UnsupportedFile))
    ImGui::OpenPopup(vcString::Get("sceneExplorerUnsupportedFilesTitle"));

  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerUnsupportedFilesTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("sceneExplorerUnsupportedFilesMessage"));

    for (int i = 0; i < (int)pProgramState->pConvertContext->jobs.length; ++i)
    {
      if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_NoFile)
        ImGui::TextUnformatted(pProgramState->pConvertContext->jobs[i]->pFilename);
    }

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      for (int i = (int)pProgramState->pConvertContext->jobs.length - 1; i >= 0; --i)
      {
        if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_NoFile)
          vcConvert_RemoveJob(pProgramState, i);
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawImageViewer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ImageViewer))
    ImGui::OpenPopup(vcString::Get("sceneImageViewerTitle"));

  // Use 75% of the window
  int maxX, maxY;
  SDL_GetWindowSize(pProgramState->pWindow, &maxX, &maxY);
  ImGui::SetNextWindowSize(ImVec2(maxX * 0.75f, maxY * 0.75f), ImGuiCond_Always);

  if (ImGui::BeginPopupModal(vcString::Get("sceneImageViewerTitle"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
  {
    pProgramState->modalOpen = true;
    if (ImGui::Button(vcString::Get("sceneImageViewerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    if (ImGui::BeginChild("ImageViewerImage", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
      ImGuiIO io = ImGui::GetIO();
      ImVec2 window = ImGui::GetWindowSize();
      ImVec2 windowPos = ImGui::GetWindowPos();

      ImGui::Image(pProgramState->image.pImage, ImVec2((float)pProgramState->image.width, (float)pProgramState->image.height));

      if (ImGui::IsWindowHovered())
      {
        io.MouseWheel += ImGui::IsMouseDoubleClicked(0);
        if (io.MouseWheel != 0 && (io.MouseWheel > 0 || (pProgramState->image.width > window.x || pProgramState->image.height > window.y + 15)))
        {
          float scaleFactor = io.MouseWheel / 10;

          float ratio = (float)pProgramState->image.width / (float)pProgramState->image.height;
          float xRatio = float((io.MousePos.x - windowPos.x) / window.x - .5);
          float yRatio = float((io.MousePos.y - windowPos.y) / window.y - .5);

          float deltaX = pProgramState->image.width * scaleFactor;
          float deltaY = pProgramState->image.height * scaleFactor;

          ImGui::SetScrollX(ImGui::GetScrollX() + (deltaX / 2) + (deltaX * xRatio));
          ImGui::SetScrollY(ImGui::GetScrollY() + (deltaY / 2) + (deltaY * yRatio));

          pProgramState->image.width = int(pProgramState->image.width * (1 + scaleFactor));
          pProgramState->image.height = int(pProgramState->image.height * (1 + scaleFactor));

          if (pProgramState->image.width > pProgramState->image.height)
          {
            if (pProgramState->image.width < (int)window.x)
            {
              pProgramState->image.width = (int)window.x;
              pProgramState->image.height = int(pProgramState->image.width / ratio);
            }
          }
          else
          {
            if (pProgramState->image.height < (int)window.y)
            {
              pProgramState->image.height = (int)window.y;
              pProgramState->image.width = int(pProgramState->image.height * ratio);
            }
          }
        }

        if (io.MouseDown[0])
        {
          ImGui::SetScrollX(ImGui::GetScrollX() - (ImGui::GetScrollMaxX() * (io.MouseDelta.x / window.x * 2)));
          ImGui::SetScrollY(ImGui::GetScrollY() - (ImGui::GetScrollMaxY() * (io.MouseDelta.y / window.y * 2)));
        }
      }
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type)
{
  pProgramState->openModals |= (1 << type);
}

void vcModals_DrawModals(vcState *pProgramState)
{
  pProgramState->modalOpen = false;
  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawProxyAuth(pProgramState);
  vcModals_DrawReleaseNotes(pProgramState);
  vcModals_DrawAbout(pProgramState);
  vcModals_DrawNewVersionAvailable(pProgramState);
  vcModals_DrawTileServer(pProgramState);
  vcModals_DrawFileModal(pProgramState);
  vcModals_DrawLoadWatermark(pProgramState);
  vcModals_DrawProjectChangeResult(pProgramState);
  vcModals_DrawProjectReadOnly(pProgramState);
  vcModals_DrawImageViewer(pProgramState);
  vcModals_DrawUnsupportedFiles(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_NewVersionAvailable) | (1 << vcMT_LoggedOut) | (1 << vcMT_ProxyAuth));
}
