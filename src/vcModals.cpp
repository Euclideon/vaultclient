#include "vcModals.h"

#include "vcState.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"
#include "gl/vcTexture.h"
#include "vcRender.h"

#include "udPlatform/udFile.h"

#include "imgui.h"
#include "imgui_ex/vcFileDialog.h"

#include "stb_image.h"

void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup("Logged Out");

  if (ImGui::BeginPopupModal("Logged Out", nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::Text("You were logged out.");

    if (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_LoggedOut);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawReleaseNotes(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ReleaseNotes))
    ImGui::OpenPopup("Release Notes");

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("Release Notes"))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client Release Notes");

    ImGui::Text("Current Version: %s", VCVERSION_PRODUCT_STRING);

    ImGui::NextColumn();
    if (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    if (pProgramState->pReleaseNotes == nullptr)
      udFile_Load("asset://releasenotes.md", (void**)&pProgramState->pReleaseNotes);

    if (pProgramState->pReleaseNotes != nullptr)
    {
      ImGui::BeginChild("ReleaseNotes");
      ImGui::TextWrapped("%s", pProgramState->pReleaseNotes);
      ImGui::EndChild();
    }
    else
    {
      ImGui::Text("Unable to load release notes from package.");
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawAbout(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_About))
    ImGui::OpenPopup("About");

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("About"))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client Third Party License Information");

    ImGui::Text("Version: %s", VCVERSION_PRODUCT_STRING);

    if (pProgramState->packageInfo.Get("success").AsBool())
      ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "Update Available to %s in your Vault Server.", pProgramState->packageInfo.Get("package.versionstring").AsString());

    ImGui::NextColumn();
    if (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
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
    ImGui::OpenPopup("New Version Available");

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("New Version Available"))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client");

    ImGui::Text("Current Version: %s", VCVERSION_PRODUCT_STRING);
    ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "New Version: %s", pProgramState->packageInfo.Get("package.versionstring").AsString());

    ImGui::Text("Please visit the Vault server in your browser to download the package.");

    ImGui::NextColumn();
    if (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_NewVersionAvailable);
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild("Release Notes");
    ImGui::TextWrapped("%s", pProgramState->packageInfo.Get("package.releasenotes").AsString());
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
    ImGui::OpenPopup("Tile Server");
    if (pProgramState->tileModal.pServerIcon == nullptr)
      vcModals_TileThread(pProgramState);
  }

  ImGui::SetNextWindowSize(ImVec2(300, 342), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("Tile Server"))
  {
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

    if (ImGui::InputText("Tile Server", pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength))
      s_isDirty = true;

    if (ImGui::Combo("Image Format", &s_currentItem, pItems, (int)udLengthOf(pItems)))
    {
      udStrcpy(pProgramState->settings.maptiles.tileServerExtension, udLengthOf(pProgramState->settings.maptiles.tileServerExtension), pItems[s_currentItem]);
      vcModals_TileThread(pProgramState);
      s_isDirty = true;
    }

    if (s_isDirty)
      s_isDirty = !vcModals_TileThread(pProgramState);

    ImGui::SetItemDefaultFocus();

    if (pProgramState->tileModal.loadStatus == -1)
      ImGui::Text("Loading... Please Wait");
    else if (pProgramState->tileModal.loadStatus == -2)
      ImGui::TextColored(ImVec4(255, 0, 0, 255), "Error fetching texture from url");
    else if (pProgramState->tileModal.pServerIcon != nullptr)
      ImGui::Image((ImTextureID)pProgramState->tileModal.pServerIcon, ImVec2(200, 200), ImVec2(0, 0), ImVec2(1, 1));

    if (pProgramState->tileModal.loadStatus != -1 && (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE]))
    {
      ImGui::CloseCurrentPopup();
      udFree(pProgramState->tileModal.pImageData);
      vcRender_ClearTiles(pProgramState->pRenderContext);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawAddUDS(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_AddUDS))
    ImGui::OpenPopup("Add UDS To Scene");

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("Add UDS To Scene"))
  {
    ImGui::InputText("Path/URL:", pProgramState->modelPath, vcMaxPathLength);
    ImGui::SameLine();

    if (ImGui::Button("Load!", ImVec2(100.f, 0)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".uds", ".ssf", ".udg" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawNotImplemented(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_NotYetImplemented))
    ImGui::OpenPopup("Not Implemented");

  if (ImGui::BeginPopupModal("Not Implemented", nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::Text("Sorry, this functionality is not yet available.");

    if (ImGui::Button("Close", ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type)
{
  pProgramState->openModals |= (1 << type);
}

bool vcModals_IsOpening(vcState *pProgramState, vcModalTypes type)
{
  return ((pProgramState->openModals & (1 << type)) != 0);
}

void vcModals_DrawModals(vcState *pProgramState)
{
  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawReleaseNotes(pProgramState);
  vcModals_DrawAbout(pProgramState);
  vcModals_DrawNewVersionAvailable(pProgramState);
  vcModals_DrawTileServer(pProgramState);
  vcModals_DrawAddUDS(pProgramState);
  vcModals_DrawNotImplemented(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_NewVersionAvailable) | (1 << vcMT_LoggedOut));
}
