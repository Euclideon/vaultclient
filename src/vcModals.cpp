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
#include "vcStringFormat.h"
#include "vcHotkey.h"

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

    if (ImGui::Button(vcString::Get("menuLogoutCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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

    if (ImGui::Button(vcString::Get("modalProxyAuthCancelButton")) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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
    if (ImGui::Button(vcString::Get("menuReleaseNotesCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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
      ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", vcStringFormat(strBuf, udLengthOf(strBuf), vcString::Get("menuAboutPackageUpdate"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("menuAboutCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild("Licenses");
    for (int i = 0; i < (int)udLengthOf(ThirdPartyLicenses); i++)
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
    if (ImGui::Button(vcString::Get("menuNewVersionCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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
  udSprintf(buf, "%s/0/0/0.%s", pProgramState->settings.maptiles.tileServerAddress, pProgramState->settings.maptiles.tileServerExtension);

  int64_t imageSize;
  void *pLocalData = nullptr;

  if (udFile_Load(buf, &pLocalData, &imageSize) != udR_Success)
    imageSize = -2;

  pProgramState->tileModal.loadStatus = imageSize;
  udFree(pProgramState->tileModal.pImageData);
  udInterlockedExchangePointer(&pProgramState->tileModal.pImageData, pLocalData);
}

void vcModals_SetTileTexture(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

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

  if (!pProgramState->modalOpen)
  {
    udFree(pProgramState->tileModal.pImageData);
    vcRender_ClearTiles(pProgramState->pRenderContext);
  }
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

  udUUID_GenerateFromString(&pProgramState->settings.maptiles.tileServerAddressUUID, pProgramState->settings.maptiles.tileServerAddress);

  udWorkerPool_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false, vcModals_SetTileTexture);

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
      udStrcpy(pProgramState->settings.maptiles.tileServerExtension, pItems[s_currentItem]);
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

    if (ImGui::Button(vcString::Get("settingsMapsTileServerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
    {
      if (pProgramState->tileModal.loadStatus == 0)
      {
        udFree(pProgramState->tileModal.pImageData);
        vcRender_ClearTiles(pProgramState->pRenderContext);
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

// Presents user with a message if the specified file exists, then returns false if user declines to overwrite the file
bool vcModals_OverwriteExistingFile(const char *pFilename)
{
  bool result = true;
  const char *pFileExistsMsg = nullptr;
  if (udFileExists(pFilename) == udR_Success)
  {
    const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
    };
    SDL_MessageBoxColorScheme colorScheme = {
      {
        { 255, 0, 0 },
        { 0, 255, 0 },
        { 255, 255, 0 },
        { 0, 0, 255 },
        { 255, 0, 255 }
      }
    };
    pFileExistsMsg = vcStringFormat(vcString::Get("convertFileExistsMessage"), pFilename);
    SDL_MessageBoxData messageboxdata = {
      SDL_MESSAGEBOX_INFORMATION,
      NULL,
      vcString::Get("convertFileExistsTitle"),
      pFileExistsMsg,
      SDL_arraysize(buttons),
      buttons,
      &colorScheme
    };
    int buttonid = 0;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) != 0 || buttonid == 0)
      result = false;
    udFree(pFileExistsMsg);
  }
  return result;
}

void vcModals_DrawFileModal(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_AddUDS))
    ImGui::OpenPopup(vcString::Get("sceneExplorerAddUDSTitle"));
  if (pProgramState->openModals & (1 << vcMT_ImportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectImportTitle"));
  if (pProgramState->openModals & (1 << vcMT_ExportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectExportTitle"));
  if (pProgramState->openModals & (1 << vcMT_ConvertAdd))
    ImGui::OpenPopup(vcString::Get("convertAddFileTitle"));
  if (pProgramState->openModals & (1 << vcMT_ConvertOutput))
    ImGui::OpenPopup(vcString::Get("convertSetOutputTitle"));
  if (pProgramState->openModals & (1 << vcMT_ConvertTempDirectory))
    ImGui::OpenPopup(vcString::Get("convertSetTempDirectoryTitle"));

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

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertAddFileTitle")))
    mode = vcMT_ConvertAdd;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertSetOutputTitle")))
    mode = vcMT_ConvertOutput;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertSetTempDirectoryTitle")))
    mode = vcMT_ConvertTempDirectory;

  if (mode < vcMT_Count)
  {
    pProgramState->modalOpen = true;
    bool pressedEnter = ImGui::InputText(vcString::Get("sceneExplorerPathURL"), pProgramState->modelPath, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    bool loadFile = false;
    bool saveFile = false;
    if (mode == vcMT_ExportProject)
      saveFile = (ImGui::Button(vcString::Get("sceneExplorerExportButton"), ImVec2(100.f, 0)) || pressedEnter);
    else if (mode == vcMT_ConvertOutput || mode == vcMT_ConvertTempDirectory)
      loadFile = (ImGui::Button(vcString::Get("sceneExplorerSetButton"), ImVec2(100.f, 0)) || pressedEnter);
    else
      loadFile = (ImGui::Button(vcString::Get("sceneExplorerLoadButton"), ImVec2(100.f, 0)) || pressedEnter);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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
    else if (mode == vcMT_ConvertAdd)
    {
      // TODO: List all supported conversion filetypes
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), false, nullptr, 0)) // No extensions means show every file
        loadFile = true;
    }
    else if (mode == vcMT_ConvertOutput)
    {
      const char *fileExtensions[] = { ".uds" };
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), false, fileExtensions, udLengthOf(fileExtensions)))
        loadFile = true;
    }
    else if (mode == vcMT_ConvertTempDirectory)
    {
      const char *fileExtensions[] = { "/", "\\" };
      if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), false, fileExtensions, udLengthOf(fileExtensions)))
        loadFile = true;
    }

    if (loadFile)
    {
      if (mode == vcMT_ConvertAdd)
      {
        vcConvert_QueueFile(pProgramState, pProgramState->modelPath);
      }
      else if (mode == vcMT_ConvertOutput)
      {
        // Set output path and filename
        udFilename loadFilename(pProgramState->modelPath);
        loadFilename.SetExtension(".uds");
        vdkConvertContext *pConvertContext = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext;
        vdkConvert_SetOutputFilename(pConvertContext, loadFilename.GetPath());
        // SetOutputFilename() overwrites the temp directory automatically, unless the user has modified it
      }
      else if (mode == vcMT_ConvertTempDirectory)
      {
        // Set temporary directory
        vdkConvertContext *pConvertContext = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext;
        vdkConvert_SetTempDirectory(pConvertContext, pProgramState->modelPath);
      }
      else
      {
        pProgramState->loadList.PushBack(udStrdup(pProgramState->modelPath));
      }
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    else if (saveFile)
    {
      vcProject_Save(pProgramState, pProgramState->modelPath, false);
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void vcModals_DrawLoadWatermark(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoadWatermark))
    ImGui::OpenPopup(vcString::Get("convertLoadWatermark"));
  if (pProgramState->openModals & (1 << vcMT_ChangeDefaultWatermark))
    ImGui::OpenPopup(vcString::Get("convertChangeDefaultWatermark"));

  vcModalTypes mode = vcMT_Count;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertLoadWatermark")))
    mode = vcMT_LoadWatermark;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("convertChangeDefaultWatermark")))
    mode = vcMT_ChangeDefaultWatermark;

  if (mode < vcMT_Count)
  {
    pProgramState->modalOpen = true;
    bool loadFile = ImGui::InputText(vcString::Get("convertPathURL"), pProgramState->modelPath, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("convertLoadButton"), ImVec2(100.f, 0)))
      loadFile = true;
    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("convertCancelButton"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".jpg", ".png", ".tga", ".bmp", ".gif" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
      loadFile = true;

    if (loadFile)
    {
      if (mode == vcMT_LoadWatermark)
      {
        vdkConvert_AddWatermark(pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext, pProgramState->modelPath);
        pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->watermark.isDirty = true;
      }
      else
      {
        udFilename filename = pProgramState->modelPath;
        uint8_t *pData = nullptr;
        int64_t dataLength = 0;
        if (udFile_Load(pProgramState->modelPath, (void**)&pData, &dataLength) == udR_Success)
        {
          // TODO: Resize watermark to the same dimensions as vdkConvert does - maybe requires additional VDK functionality?
          filename.SetFolder(pProgramState->settings.pSaveFilePath);
          udFile_Save(filename, pData, (size_t)dataLength);
          udFree(pData);
        }
        udStrcpy(pProgramState->settings.convertdefaults.watermark.filename, filename.GetFilenameWithExt());
        pProgramState->settings.convertdefaults.watermark.isDirty = true;
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawProjectChangeResult(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProjectChange))
    ImGui::OpenPopup("###ProjectChange");

  if (ImGui::BeginPopupModal("###ProjectChange", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
  {
    pProgramState->modalOpen = true;

    for (uint32_t i = 0; i < pProgramState->errorItems.length; ++i)
    {
      const char *pMessage = nullptr;

      if (i > 0)
        ImGui::NewLine();

      if (pProgramState->errorItems[i].source == vcES_ProjectChange)
      {
        ImGui::TextUnformatted(pProgramState->errorItems[i].pData);
        switch (pProgramState->errorItems[i].resultCode)
        {
        case udR_WriteFailure:
          pMessage = vcString::Get("sceneExplorerProjectChangeFailedWrite");
          break;
        case udR_ParseError:
          pMessage = vcString::Get("sceneExplorerProjectChangeFailedParse");
          break;
        case udR_Success:
          pMessage = vcString::Get("sceneExplorerProjectChangeSucceededMessage");
          break;
        case udR_ReadFailure:
          pMessage = vcString::Get("sceneExplorerProjectChangeFailedRead");
          break;
        case udR_Failure_: // Falls through
        default:
          pMessage = vcString::Get("sceneExplorerProjectChangeFailedMessage");
          break;
        }
        ImGui::TextUnformatted(pMessage);
      }
    }

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
    {
      for (uint32_t i = 0; i < pProgramState->errorItems.length; ++i)
      {
        if (pProgramState->errorItems[i].source == vcES_ProjectChange)
        {
          udFree(pProgramState->errorItems[i].pData);
          pProgramState->errorItems.RemoveAt(i);
          --i;
        }
      }
      ImGui::CloseCurrentPopup();
    }

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

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawUnsupportedFiles(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_UnsupportedFile))
    ImGui::OpenPopup(vcString::Get("sceneExplorerUnsupportedFilesTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerUnsupportedFilesTitle"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
  {
    pProgramState->modalOpen = true;
    ImGui::TextUnformatted(vcString::Get("sceneExplorerUnsupportedFilesMessage"));

    // Clear and close buttons
    if (ImGui::Button(vcString::Get("sceneExplorerClearAllButton")))
    {
      for (uint32_t i = 0; i < pProgramState->errorItems.length; ++i)
      {
        if (pProgramState->errorItems[i].source == vcES_File)
        {
          udFree(pProgramState->errorItems[i].pData);
          pProgramState->errorItems.RemoveAt(i);
          --i;
        }
      }
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerCloseButton")) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    // Actual Content
    ImGui::BeginChild("unsupportedFilesChild");
    ImGui::Columns(2);

    for (size_t i = 0; i < pProgramState->errorItems.length; ++i)
    {
      if (pProgramState->errorItems[i].source != vcES_File)
        continue;

      bool removeItem = ImGui::Button(udTempStr("X##errorFileRemove%zu", i));
      ImGui::SameLine();
      // Get the offset so the next column is offset by the same value to keep alignment
      float offset = ImGui::GetCurrentWindow()->DC.CurrentLineTextBaseOffset;
      ImGui::TextUnformatted(pProgramState->errorItems[i].pData);
      ImGui::NextColumn();

      ImGui::GetCurrentWindow()->DC.CurrentLineTextBaseOffset = offset;
      ImGui::TextUnformatted(udResultAsString(pProgramState->errorItems[i].resultCode));
      ImGui::NextColumn();

      if (removeItem)
      {
        udFree(pProgramState->errorItems[i].pData);
        pProgramState->errorItems.RemoveAt(i);
        --i;
      }
    }

    ImGui::EndColumns();
    ImGui::EndChild();

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
    if (ImGui::Button(vcString::Get("sceneImageViewerCloseButton"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
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

void vcModals_DrawBindings(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_Bindings))
    ImGui::OpenPopup(vcString::Get("bindingsModalTitle"));

  ImGui::SetNextWindowSize(ImVec2(915, 605));
  if (ImGui::BeginPopupModal(vcString::Get("bindingsModalTitle"), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::BeginChild("bindingsInterfaceChild", ImVec2(-1, -1));

    vcHotkey::DisplayBindings(pProgramState);

    ImGui::Columns(3, "ButtonRow");
    if (ImGui::Button(udTempStr("%s###bindingsClose" , vcString::Get("bindingsModalClose")), ImVec2(-1, 25)) || ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_Close)])
      ImGui::CloseCurrentPopup();

    ImGui::NextColumn();
    if (ImGui::Button(udTempStr("%s###bindingsLoad", vcString::Get("bindingsModalDefaults")), ImVec2(-1, 25)) || vcHotkey::IsPressed(vcB_Load))
      vcSettings_Load(&pProgramState->settings, false, vcSC_Bindings);

    ImGui::NextColumn();
    if (ImGui::Button(udTempStr("%s###bindingsSave", vcString::Get("bindingsModalSave")), ImVec2(-1, 25)) || vcHotkey::IsPressed(vcB_Save))
      vcSettings_Save(&pProgramState->settings);

    ImGui::EndColumns();

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
  vcModals_DrawBindings(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_NewVersionAvailable) | (1 << vcMT_LoggedOut) | (1 << vcMT_ProxyAuth));
}
