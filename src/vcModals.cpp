#include "vcModals.h"

#include "vcState.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"

#include "udPlatform/udFile.h"

#include "imgui.h"

void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup("Logged Out");

  if (ImGui::BeginPopupModal("Logged Out", nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::Text("You were logged out.");

    if (ImGui::Button("Close", ImVec2(-1, 0)))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawReleaseNotes(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ReleaseNotes))
  {
    ImGui::OpenPopup("Release Notes");
    ImGui::SetNextWindowSize(ImVec2(500, 600));
  }

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
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
    {
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
      char ReleaseNotesPath[] = ASSETDIR "/releasenotes.md";
#elif UDPLATFORM_OSX
      char ReleaseNotesPath[vcMaxPathLength] = "";

      {
        char *pBasePath = SDL_GetBasePath();
        if (pBasePath == nullptr)
          pBasePath = SDL_strdup("./");

        udSprintf(ReleaseNotesPath, vcMaxPathLength, "%s%s", pBasePath, "releasenotes.md");
        SDL_free(pBasePath);
      }
#else
      char ReleaseNotesPath[] = "releasenotes.md";
#endif

      udFile_Load(ReleaseNotesPath, (void**)&pProgramState->pReleaseNotes);
    }

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
  {
    ImGui::OpenPopup("About");
    ImGui::SetNextWindowSize(ImVec2(500, 600));
  }

  ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_Appearing);
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
  {
    ImGui::OpenPopup("New Version Available");
    ImGui::SetNextWindowSize(ImVec2(500, 600));
  }

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
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild("Release Notes");
    ImGui::TextUnformatted(pProgramState->packageInfo.Get("package.releasenotes").AsString());
    ImGui::EndChild();

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

  pProgramState->openModals = 0;
}
