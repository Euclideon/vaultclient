#include "vcModals.h"

#include "vcState.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"
#include "gl/vcTexture.h"
#include "vcRender.h"

#include "udPlatform/udFile.h"

#include "imgui.h"

#include "stb_image.h"


void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup("Logged Out");

  if (ImGui::BeginPopupModal("Logged Out", nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::Text("You were logged out.");

    if (ImGui::Button("Close", ImVec2(-1, 0)))
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
    if (ImGui::Button("Close", ImVec2(-1, 0)))
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
    if (ImGui::Button("Close", ImVec2(-1, 0)))
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
    ImGui::Text("Euclideon Client");

    ImGui::Text("Current Version: %s", VCVERSION_PRODUCT_STRING);
    ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "New Version: %s", pProgramState->packageInfo.Get("package.versionstring").AsString());

    ImGui::Text("Please visit the Vault server in your browser to download the package.");

    ImGui::NextColumn();
    if (ImGui::Button("Close", ImVec2(-1, 0)))
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

void vcModals_SetTileImage(void *pProgramState)
{
  vcState *programState = (vcState*)pProgramState;
  size_t urlLen = udStrlen(programState->settings.maptiles.tileServerAddress);
  if (urlLen == 0)
    return;

  if (programState->settings.maptiles.tileServerAddress[urlLen - 1] == '/')
    programState->settings.maptiles.tileServerAddress[urlLen - 1] = '\0';

  vcTexture_Destroy(&programState->pTileServerIcon);
  const char *svrSuffix = "/0/0/0.";
  char buf[256];
  udSprintf(buf, sizeof(buf), "%s%s%s", programState->settings.maptiles.tileServerAddress, svrSuffix, programState->settings.maptiles.tileServerExtension);

  if (udFile_Load(buf, &programState->pImageData, &programState->imageSize) != udR_Success)
    programState->tileError = true;
  else
    programState->tileError = false;
}

void vcModals_DrawTileServer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_TileServer))
  {
    ImGui::OpenPopup("Tile Server");
    if (pProgramState->pTileServerIcon == nullptr)
    {
      pProgramState->imageSize = -1;
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);
    }
  }

  ImGui::SetNextWindowSize(ImVec2(300, 342), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("Tile Server"))
  {
    const char *pItems[] = { "png", "jpg" };
    static int s_currentItem = -1;
    if (s_currentItem == -1)
    {
      for (int i = 0; i < (int)udLengthOf(pItems); ++i)
      {
        if (udStrEquali(pItems[i], pProgramState->settings.maptiles.tileServerExtension))
          s_currentItem = i;
      }
    }

    if (ImGui::Combo("Image Format", &s_currentItem, pItems, (int)udLengthOf(pItems)))
    {
      udStrcpy(pProgramState->settings.maptiles.tileServerExtension, 4, pItems[s_currentItem]);
      pProgramState->imageSize = -1;
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);
    }

    if (ImGui::InputText("Tile Server", pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue))
    {
      pProgramState->imageSize = -1;
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);
    }
    ImGui::SetItemDefaultFocus();

    if (ImGui::Button("Load", ImVec2(-1, 0)))
    {
      pProgramState->imageSize = -1;
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);

      if (pProgramState->pTileServerIcon != nullptr)
      {
        ImGui::CloseCurrentPopup();
        vcRender_ClearTiles(pProgramState->pRenderContext);
      }
    }
    uint32_t width, height, channelCount;

    if (pProgramState->imageSize != -1)
    {
      uint8_t *pData = stbi_load_from_memory((stbi_uc*)pProgramState->pImageData, (int)pProgramState->imageSize, (int*)&width, (int*)&height, (int*)&channelCount, 4);
      udFree(pProgramState->pImageData);

      if (pData)
        vcTexture_Create(&pProgramState->pTileServerIcon, width, height, pData, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Repeat, vcTCF_None, 0);

      stbi_image_free(pData);
      pProgramState->imageSize = -1;
    }

    if (pProgramState->tileError)
      ImGui::TextColored(ImVec4(255, 0, 0, 255), "Error fetching or creating texture from url");
    else if (pProgramState->pTileServerIcon != nullptr)
      ImGui::Image((ImTextureID)pProgramState->pTileServerIcon, ImVec2(200, 200), ImVec2(0, 0), ImVec2(1, 1));

    if (ImGui::Button("Close", ImVec2(-1, 0)))
    {
      ImGui::CloseCurrentPopup();
      vcRender_ClearTiles(pProgramState->pRenderContext);
    }
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

  pProgramState->openModals &= ((1 << vcMT_NewVersionAvailable) | (1 << vcMT_LoggedOut));
}
