#include "vcModals.h"

#include "vcState.h"
#include "vcPOI.h"
#include "vcTexture.h"
#include "vcRender.h"
#include "vcStrings.h"
#include "vcConvert.h"
#include "vcProxyHelper.h"
#include "vcStringFormat.h"
#include "vcHotkey.h"
#include "vcWebFile.h"
#include "vcSession.h"
#include "imgui_ex/vcMenuButtons.h"

#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "imgui_ex/vcFileDialog.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "imgui_ex/imgui_udValue.h"

#include "vdkServerAPI.h"

#include "stb_image.h"

void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup(vcString::Get("menuLogoutTitle"));

  if (ImGui::BeginPopupModal(vcString::Get("menuLogoutTitle"), nullptr, ImGuiWindowFlags_NoResize))
  {
    if (pProgramState->closeModals & (1 << vcMT_LoggedOut))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    ImGui::TextUnformatted(vcString::Get("menuLogoutMessage"));

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_LoggedOut);
    }

    ImGui::EndPopup();
  }
}

// Presents user with a message if the specified file exists, then returns false if user declines to overwrite the file
bool vcModals_OverwriteExistingFile(vcState *pProgramState, const char *pFilename)
{
  bool result = true;
  const char *pFileExistsMsg = nullptr;
  if (udFileExists(pFilename) == udR_Success)
  {
    const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, vcString::Get("popupConfirmNo") },
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, vcString::Get("popupConfirmYes") },
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
      pProgramState->pWindow,
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

// Presents user with a message if the specified file exists, then returns false if user declines to overwrite the file
bool vcModals_AllowDestructiveAction(vcState *pProgramState, const char *pTitle, const char *pMessage)
{
#if UDPLATFORM_EMSCRIPTEN
  udUnused(pProgramState);
  udUnused(pTitle);

  int result = MAIN_THREAD_EM_ASM_INT(return (confirm(UTF8ToString($0))?1:0), pMessage);
  return (result != 0);
#else
  bool result = false;

  const SDL_MessageBoxButtonData buttons[] = {
    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, vcString::Get("popupConfirmNo") },
    { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, vcString::Get("popupConfirmYes") },
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

  SDL_MessageBoxData messageboxdata = {
    SDL_MESSAGEBOX_WARNING,
    pProgramState->pWindow,
    pTitle,
    pMessage,
    SDL_arraysize(buttons),
    buttons,
    &colorScheme
  };

  int buttonid = 0;
  if (SDL_ShowMessageBox(&messageboxdata, &buttonid) != 0 || buttonid == 0)
    result = false;
  else
    result = true;

  return result;
#endif
}

// Returns true if user accepts ending the session
bool vcModals_ConfirmEndSession(vcState *pProgramState, bool isQuit)
{
  const char *pMessage = udStrdup(isQuit ? vcString::Get("endSessionExitMessage") : vcString::Get("endSessionLogoutMessage"));

  if (pProgramState->hasContext)
  {
#if VC_HASCONVERT
    if (vcConvert_CurrentProgressPercent(pProgramState) > -2)
      udSprintf(&pMessage, "%s\n- %s", pMessage, vcString::Get("endSessionConfirmEndConvert"));
#endif

    if (vdkProject_HasUnsavedChanges(pProgramState->activeProject.pProject) == vE_Success)
      udSprintf(&pMessage, "%s\n- %s", pMessage, vcString::Get("endSessionConfirmProjectUnsaved"));

    if (pProgramState->backgroundWork.exportsRunning.Get() > 0)
      udSprintf(&pMessage, "%s\n- %s", pMessage, vcString::Get("endSessionConfirmExportsRunning"));
  }

  bool retVal = vcModals_AllowDestructiveAction(pProgramState, isQuit ? vcString::Get("endSessionExitTitle") : vcString::Get("endSessionLogoutTitle"), pMessage);

  udFree(pMessage);

  return retVal;
}

void vcModals_DrawAddSceneItem(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_AddSceneItem))
    ImGui::OpenPopup(vcString::Get("sceneExplorerAddSceneItemTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("sceneExplorerAddSceneItemTitle")))
  {
    if (pProgramState->closeModals & (1 << vcMT_AddSceneItem))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProgramState->modelPath, SupportedFileTypes_SceneItems, vcFDT_OpenFile, nullptr);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerLoadButton"), ImVec2(100.f, 0)))
    {
      pProgramState->loadList.PushBack(udStrdup(pProgramState->modelPath));
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();

    //TODO: UI depending on what file is selected

    ImGui::EndPopup();
  }
}

void vcModals_DrawWelcome(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_Welcome))
    ImGui::OpenPopup("###modalWelcome");

  ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("###modalWelcome", nullptr, ImGuiWindowFlags_NoTitleBar))
  {
    if (pProgramState->closeModals & (1 << vcMT_Welcome))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    static int zoneCustomSRID = 84;
    static int creatingNewProjectType = -1;
    static int localOrServerProject = 0;
    static char pProjectPath[vcMaxPathLength] = {};

    static udUUID selectedGroup = {};
    static const char *pGroupName = nullptr;
    static bool availableGroups = true;

    if (ImGui::IsWindowAppearing())
    {
      pGroupName = nullptr;
      availableGroups = true;
      udUUID_Clear(&selectedGroup);
    }

    const char *pNewProjectTypes[] =
    {
      vcString::Get("modalProjectNewGeolocated"),
      vcString::Get("modalProjectNewNonGeolocated"),
      vcString::Get("modalProjectNewGeolocatedSpecificZone")
    };

    const char *pNewProjectDescriptions[] =
    {
      vcString::Get("modalProjectGeolocatedDescription"),
      vcString::Get("modalProjectNonGeolocatedDescription"),
      vcString::Get("modalProjectSpecificZoneDescription")
    };

    vcMenuBarButtonIcon projectIcons[] =
    {
      vcMBBI_Geospatial,
      vcMBBI_Grid,
      vcMBBI_ExpertGrid
    };

    UDCOMPILEASSERT(udLengthOf(pNewProjectTypes) == udLengthOf(pNewProjectDescriptions), "Invalid matching sizes");

    ImVec2 windowSize = ImGui::GetWindowSize();

    // Logo
    {
      const int LogoSize = 400;

      int x = 0;
      int y = 0;

      vcTexture_GetSize(pProgramState->pCompanyLogo, &x, &y);

      float xf = (float)x;
      float yf = (float)y;
      float r = (float)x / (float)y;

      if (r >= 1.0) // X is larger
      {
        xf = (float)udMin(x, LogoSize);
        yf = (xf / r);
      }
      else // Y is larger
      {
        yf = (float)udMin(y, LogoSize);
        xf = (yf * r);
      }

      ImGui::SetCursorPosX((windowSize.x - xf) / 2);
      ImGui::Image(pProgramState->pCompanyLogo, ImVec2(xf, yf));
    }

    // Get Help
    {
      ImGui::SetCursorPosX((windowSize.x - 475) / 2);

      if (ImGui::Selectable("##newProjectGetHelp", false, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 48)))
        vcWebFile_OpenBrowser("https://desk.euclideon.com/");

      float prevPosY = ImGui::GetCursorPosY();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 46);

      ImGui::SetCursorPosX((windowSize.x - 475) / 2);

      udFloat4 iconUV = vcGetIconUV(vcMBBI_Help);
      ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
      ImGui::SameLine();

      // Align text with icon
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

      float textAlignPosX = ImGui::GetCursorPosX();
      ImGui::TextUnformatted(vcString::Get("gettingStarted"));

      // Manually align details text with title text
      ImGui::SetCursorPosX(textAlignPosX);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

      ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      col.w *= 0.65f;
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted(vcString::Get("gettingStartedDesc"));
      ImGui::PopStyleColor();

      ImGui::SetCursorPosY(prevPosY);
      ImGui::Spacing();
    }

    ImGui::Separator();

    if (creatingNewProjectType == -1)
    {
      ImGui::Columns(2);

      ImGui::TextUnformatted(vcString::Get("modalProjectOpenRecent"));
      ImGui::Spacing();
      ImGui::Spacing();

      for (size_t i = 0; i < pProgramState->settings.projectsHistory.projects.length; ++i)
      {
        vcProjectHistoryInfo *pProjectInfo = &pProgramState->settings.projectsHistory.projects[i];

        bool selected = false;
        if (ImGui::Selectable(udTempStr("##projectHistoryItem%zu", i), &selected, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 40)))
        {
          if (pProjectInfo->isServerProject)
            vcProject_LoadFromServer(pProgramState, pProjectInfo->pPath);
          else
            vcProject_LoadFromURI(pProgramState, pProjectInfo->pPath);

          ImGui::CloseCurrentPopup();
        }

        float prevPosY = ImGui::GetCursorPosY();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 37);

        udFloat4 iconUV = vcGetIconUV(pProjectInfo->isServerProject ? vcMBBI_StorageCloud : vcMBBI_StorageLocal);
        ImGui::Image(pProgramState->pUITexture, ImVec2(16, 16), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
        ImGui::SameLine();

        // Align text with icon
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

        float textAlignPosX = ImGui::GetCursorPosX();
        ImGui::Text("%s", pProjectInfo->pName);

        // Manually align details text with title text
        ImGui::SetCursorPosX(textAlignPosX);
        ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        col.w *= 0.65f;
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%s", pProjectInfo->pPath);

        ImGui::PopStyleColor();
        ImGui::SetCursorPosY(prevPosY);
        ImGui::Spacing();
      }

      ImGui::NextColumn();

      ImGui::Text("%s", vcString::Get("modalProjectGetStarted"));
      ImGui::Spacing();
      ImGui::Spacing();

      UDCOMPILEASSERT(udLengthOf(pNewProjectTypes) == udLengthOf(projectIcons), "Invalid matching sizes");

      for (size_t i = 0; i < udLengthOf(pNewProjectTypes); ++i)
      {
        bool selected = false;
        if (ImGui::Selectable(udTempStr("##newProjectType%zu", i), &selected, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 48)))
        {
          creatingNewProjectType = (int)i;
          udStrcpy(pProgramState->modelPath, vcString::Get("modalProjectNewTitle"));

          if (i == 0) // Geolocated
            zoneCustomSRID = 84;
          else if (i == 1)
            zoneCustomSRID = 0; // Non Geolocated
        }

        float prevPosY = ImGui::GetCursorPosY();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 46);

        udFloat4 iconUV = vcGetIconUV(projectIcons[i]);
        ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
        ImGui::SameLine();

        // Align text with icon
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

        float textAlignPosX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(pNewProjectTypes[i]);

        // Manually align details text with title text
        ImGui::SetCursorPosX(textAlignPosX);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

        ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        col.w *= 0.65f;
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(pNewProjectDescriptions[i]);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(prevPosY);
        ImGui::Spacing();
      }

      ImGui::EndColumns();
    }
    else
    {
      ImGui::TextUnformatted(pNewProjectTypes[creatingNewProjectType]);

      vcIGSW_InputText(vcString::Get("modalProjectNewName"), pProgramState->modelPath, udLengthOf(pProgramState->modelPath));
      
      if (creatingNewProjectType == 2)
      {
        ImGui::Indent();
        ImGui::InputInt(vcString::Get("modalProjectNewGeolocatedSpecificZoneID"), &zoneCustomSRID);
      
        udGeoZone zone;
        if (udGeoZone_SetFromSRID(&zone, zoneCustomSRID) != udR_Success)
          ImGui::TextUnformatted(vcString::Get("sceneUnsupportedSRID"));
        else
          ImGui::Text("%s (%s: %d)", zone.zoneName, vcString::Get("sceneSRID"), zoneCustomSRID);
      
        ImGui::Unindent();
      }

      ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
      ImGui::TextUnformatted(vcString::Get("modalProjectLocalOrServerProject"));

      ImGui::Indent();

      ImGui::RadioButton(udTempStr("%s##newProjectLocalServer", vcString::Get("modalProjectLocal")), &localOrServerProject, 0);
      ImGui::SameLine();
      ImGui::RadioButton(udTempStr("%s##newProjectLocalServer", vcString::Get("modalProjectServer")), &localOrServerProject, 1);

      if (localOrServerProject == 0) // local
      {
        vcIGSW_FilePicker(pProgramState, vcString::Get("modalProjectSaveLocation"), pProjectPath, SupportedFileTypes_ProjectsExport, vcFDT_SaveFile, nullptr);
      }
      else
      {
        if (pProgramState->groups.length > 0 && availableGroups)
        {
          if (!udUUID_IsValid(selectedGroup))
          {
            for (auto item : pProgramState->groups)
            {
              if (item.permissionLevel >= 3) // Only >Managers can load projects
              {
                if (ImGui::Selectable(item.pGroupName, selectedGroup == item.groupID) || !udUUID_IsValid(selectedGroup))
                {
                  selectedGroup = item.groupID;
                  pGroupName = item.pGroupName;
                }
              }
            }

            if (!udUUID_IsValid(selectedGroup))
              availableGroups = false;
          }

          if (ImGui::BeginCombo(vcString::Get("modalProjectSaveGroup"), pGroupName))
          {
            for (auto item : pProgramState->groups)
            {
              if (item.permissionLevel >= 3) // Only >Managers can load projects
              {
                if (ImGui::Selectable(item.pGroupName, selectedGroup == item.groupID) || !udUUID_IsValid(selectedGroup))
                {
                  selectedGroup = item.groupID;
                  pGroupName = item.pGroupName;
                }
              }
            }
            ImGui::EndCombo();
          }
        }
        else
        {
          ImGui::TextUnformatted(vcString::Get("modalProjectNoGroups"));
        }
      }

      ImGui::Unindent();
      //TODO: Additional export settings
    }

    static vdkError result = vE_Success;
    if (result != vE_Success)
    {
      ImGui::NextColumn();
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "%s", vcProject_ErrorToString(result));
    }    

    // Position control buttons in the bottom right corner
    if (creatingNewProjectType == -1)
    {
      ImGui::Separator();

      if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      {
        pProgramState->modelPath[0] = '\0';
        creatingNewProjectType = -1;
        result = vE_Success;
        ImGui::CloseCurrentPopup();
      }
    }
    else
    {
      ImGui::SetCursorPos(ImVec2(windowSize.x - 280, windowSize.y - 30));

      if (ImGui::Button("Back", ImVec2(100.f, 0)))
      {        
        creatingNewProjectType = -1;
        result = vE_Success;
      }

      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("modalProjectNewCreate"), ImVec2(150.f, 0)) && vcProject_AbleToChange(pProgramState))
      {
        if (localOrServerProject == 0) // local
        {
          // TODO: CONFIRM FILE OVERRIDE IF EXISTS

          result = vcProject_CreateFileScene(pProgramState, pProjectPath, pProgramState->modelPath, zoneCustomSRID);
        }
        else // server
        {
          result = vcProject_CreateServerScene(pProgramState, pProgramState->modelPath, udUUID_GetAsString(selectedGroup), zoneCustomSRID);
        }

        if (result == vE_Success )
        {
          pProgramState->modelPath[0] = '\0';
          creatingNewProjectType = -1;

          ImGui::CloseCurrentPopup();
        }
      }
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawExportProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ExportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectExportTitle"));

  ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectExportTitle")))
  {
    if (pProgramState->closeModals & (1 << vcMT_ExportProject))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    static const char *pGroupName = nullptr;
    static udUUID selectedGroup = {};
    static bool availableGroups = true;

    if (ImGui::IsWindowAppearing())
    {
      pGroupName = nullptr;
      udUUID_Clear(&selectedGroup);
      availableGroups = true;
    }

    struct
    {
      const char *pName;          vcMenuBarButtonIcon icon;
    } types[] = {
      { "menuProjectExportDisk",  vcMBBI_StorageLocal },
      { "menuProjectExportCloud", vcMBBI_StorageCloud },
    };

    for (size_t i = 0; i < udLengthOf(types); ++i)
    {
      if (ImGui::BeginChild(udTempStr("##saveAsType%zu", i), ImVec2(-1, 90), true))
      {
        udFloat4 iconUV = vcGetIconUV(types[i].icon);
        ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
        ImGui::SameLine();

        // Align text with icon
        //ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

        float textAlignPosX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(vcString::Get(types[i].pName));

        // Manually align details text with title text
        ImGui::SetCursorPosX(textAlignPosX);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

        if (i == 0) // local
        {
          vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProgramState->modelPath, SupportedFileTypes_ProjectsExport, vcFDT_SaveFile, nullptr);

          ImGui::SetCursorPosX(textAlignPosX);
          if (ImGui::Button(vcString::Get("menuProjectExportButton")))
          {
            vcProject_SaveAs(pProgramState, pProgramState->modelPath, false);
            pProgramState->modelPath[0] = '\0';
            ImGui::CloseCurrentPopup();
          }
        }
        else if (i == 1) // cloud
        {
          if (pProgramState->groups.length > 0 && availableGroups)
          {
            if (!udUUID_IsValid(selectedGroup))
            {
              for (auto item : pProgramState->groups)
              {
                if (item.permissionLevel >= 3) // Only >Managers can load projects
                {
                  pGroupName = item.pGroupName;
                  selectedGroup = item.groupID;
                }
              }

              if (!udUUID_IsValid(selectedGroup))
                availableGroups = false;
            }

            if (ImGui::BeginCombo(vcString::Get("modalProjectSaveGroup"), pGroupName))
            {
              for (auto item : pProgramState->groups)
              {
                if (item.permissionLevel >= 3) // Only >Managers can load projects
                {
                  if (ImGui::Selectable(item.pGroupName, selectedGroup == item.groupID))
                  {
                    selectedGroup = item.groupID;
                    pGroupName = item.pGroupName;
                  }
                }
              }

              ImGui::EndCombo();
            }

            ImGui::SetCursorPosX(textAlignPosX);
            if (ImGui::Button(vcString::Get("menuProjectExportButton")))
              vdkProject_SaveToServer(pProgramState->pVDKContext, pProgramState->activeProject.pProject, udUUID_GetAsString(selectedGroup));
          }
          else
          {
            ImGui::TextUnformatted(vcString::Get("modalProjectNoGroups"));
          }
        }

        ImGui::EndChild();
      }
    }

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}


void vcModals_DrawProjectInfo(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProjectInfo))
    ImGui::OpenPopup(udTempStr("%s###projectInfo", pProgramState->activeProject.pRoot->pName));

  ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(udTempStr("%s###projectInfo", pProgramState->activeProject.pRoot->pName), nullptr, ImGuiWindowFlags_NoSavedSettings))
  {
    if (pProgramState->closeModals & (1 << vcMT_ProjectInfo))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    const char *pInfo = nullptr;

    if (ImGui::BeginChild("##infoModal", ImVec2(-1, -30), true))
    {
      if (vdkProjectNode_GetMetadataString(pProgramState->activeProject.pRoot, "information", &pInfo, "") == vE_Success)
        ImGui::TextWrapped("%s", pInfo);

      ImGui::EndChild();
    }
    
    if (ImGui::Button(vcString::Get("popupOK"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel) || pInfo == nullptr || pInfo[0] == '\0')
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawImportProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ImportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectImportTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectImportTitle")))
  {
    if (pProgramState->closeModals & (1 << vcMT_ImportProject))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProgramState->modelPath, SupportedFileTypes_ProjectsImport, vcFDT_OpenFile, nullptr);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("menuProjectImport"), ImVec2(100.f, 0)) && vcProject_AbleToChange(pProgramState))
    {
      pProgramState->loadList.PushBack(udStrdup(pProgramState->modelPath));
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();

    //TODO: Additional import settings

    ImGui::EndPopup();
  }
}

void vcModals_DrawProjectSettings(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ProjectSettings))
    ImGui::OpenPopup(vcString::Get("menuProjectSettingsTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectSettingsTitle")))
  {
    static char information[1024] = {};

    if (pProgramState->closeModals & (1 << vcMT_ProjectSettings))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    if (ImGui::IsWindowAppearing())
    {
      udStrcpy(pProgramState->modelPath, pProgramState->activeProject.pRoot->pName);
      const char *pCurrentInformation = nullptr;
      vdkProjectNode_GetMetadataString(pProgramState->activeProject.pRoot, "information", &pCurrentInformation, "");
      udStrcpy(information, pCurrentInformation);
    }

    ImGui::InputText(vcString::Get("menuProjectName"), pProgramState->modelPath, udLengthOf(pProgramState->modelPath));

    ImVec2 size = ImGui::GetWindowSize();
    ImGui::InputTextMultiline(vcString::Get("menuProjectInfo"), information, udLengthOf(information), ImVec2(0, size.y - 90));

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      vdkProjectNode_SetName(pProgramState->activeProject.pProject, pProgramState->activeProject.pRoot, pProgramState->modelPath);
      vdkProjectNode_SetMetadataString(pProgramState->activeProject.pRoot, "information", information);

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
    if (pProgramState->closeModals & (1 << vcMT_ProjectChange))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    for (uint32_t i = 0; i < pProgramState->errorItems.length; ++i)
    {
      const char *pMessage = nullptr;

      if (i > 0)
        ImGui::NewLine();

      if (pProgramState->errorItems[i].source == vcES_ProjectChange)
      {
        if (pProgramState->errorItems[i].pData != nullptr)
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

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
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
    if (pProgramState->closeModals & (1 << vcMT_ProjectReadOnly))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    ImGui::TextUnformatted(vcString::Get("sceneExplorerProjectReadOnlyMessage"));

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
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
    if (pProgramState->closeModals & (1 << vcMT_UnsupportedFile))
      ImGui::CloseCurrentPopup();
    else
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

    if (ImGui::Button(vcString::Get("popupClose")) || vcHotkey::IsPressed(vcB_Cancel))
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
      float offset = ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset;
      const char *pFileName = pProgramState->errorItems[i].pData;
      ImGui::TextUnformatted(pFileName);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", pFileName);
      ImGui::NextColumn();

      ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = offset;

      int errorCode = pProgramState->errorItems[i].resultCode;
      const char *pErrorString = nullptr;
      switch (errorCode)
      {
      case udR_CorruptData:
        pErrorString = vcString::Get("errorCorruptData");
        break;
      case udR_Unsupported:
        pErrorString = vcString::Get("errorUnsupported");
        break;
      case udR_OpenFailure:
        pErrorString = vcString::Get("errorOpenFailure");
        break;
      case udR_ReadFailure:
        pErrorString = vcString::Get("errorReadFailure");
        break;
      case udR_WriteFailure:
        pErrorString = vcString::Get("errorWriteFailure");
        break;
      case udR_CloseFailure:
        pErrorString = vcString::Get("errorCloseFailure");
        break;
      default:
        pErrorString = vcString::Get("errorUnknown");
        break;
      }
      ImGui::TextUnformatted(pErrorString);
      ImGui::NextColumn();

      if (removeItem)
      {
        udFree(pProgramState->errorItems[i].pData);
        pProgramState->errorItems.RemoveAt(i);
        --i;
      }
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_DrawImageViewer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ImageViewer))
    ImGui::OpenPopup(vcString::Get("sceneImageViewerTitle"));

  ImGui::SetNextWindowSizeConstraints(ImVec2(50.f, 50.f), ImVec2((float)pProgramState->settings.window.width, (float)pProgramState->settings.window.height));
  ImGui::SetNextWindowSize(ImVec2((float)pProgramState->image.width + 25, (float)pProgramState->image.height + 50), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("sceneImageViewerTitle"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
  {
    if (pProgramState->closeModals & (1 << vcMT_ImageViewer))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      ImGui::CloseCurrentPopup();

    if (ImGui::BeginChild("ImageViewerImage", ImVec2(-1, 0), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
      ImGuiIO io = ImGui::GetIO();
      ImVec2 window = ImGui::GetWindowSize();
      ImVec2 windowPos = ImGui::GetWindowPos();

#if GRAPHICS_API_OPENGL
      ImVec2 uvs[2] = { {0,1}, {1,0} };
#else
      ImVec2 uvs[2] = { {0,0}, {1,1} };
#endif
      
      ImGui::Image(pProgramState->image.pImage, ImVec2((float)pProgramState->image.width, (float)pProgramState->image.height), uvs[0], uvs[1]);

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

void vcModals_DrawProfile(vcState* pProgramState)
{ 
  const char *profile = vcString::Get("modalProfileTitle");

  if (pProgramState->openModals & (1 << vcMT_Profile))
    ImGui::OpenPopup(profile);

  float width = 300.f;
  float height = 250.f;
  float buttonWidth = 80.f;
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

  if (ImGui::BeginPopupModal(profile, nullptr, ImGuiWindowFlags_NoResize))
  {
    if (pProgramState->closeModals & (1 << vcMT_Profile))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    if (pProgramState->sessionInfo.isOffline)
    {
      ImGui::TextUnformatted(vcString::Get("modalProfileOffline"));

      ImGui::InputText(vcString::Get("modalProfileDongle"), pProgramState->sessionInfo.displayName, udLengthOf(pProgramState->sessionInfo.displayName), ImGuiInputTextFlags_ReadOnly);
    }
    else
    {
      ImGui::InputText(vcString::Get("modalProfileRealName"), pProgramState->sessionInfo.displayName, udLengthOf(pProgramState->sessionInfo.displayName), ImGuiInputTextFlags_ReadOnly);

      const char *pEmail = pProgramState->profileInfo.Get("user.email").AsString("");
      ImGui::InputText(vcString::Get("modalProfileEmail"), (char*)pEmail, udStrlen(pEmail), ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::Separator();

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(buttonWidth, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawChangePassword(vcState *pProgramState)
{
  const char *pProfile = vcString::Get("modalChangePasswordTitle");

  if (pProgramState->openModals & (1 << vcMT_ChangePassword))
    ImGui::OpenPopup(pProfile);

  float width = 390.f;
  float height = 150.f;
  float buttonWidth = 80.f;
  if (pProgramState->changePassword.message[0] != '\0')
    height += ImGui::GetFontSize();

  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

  if (ImGui::BeginPopupModal(pProfile, nullptr, ImGuiWindowFlags_NoResize))
  {
    if (pProgramState->closeModals & (1 << vcMT_ChangePassword))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    if (pProgramState->sessionInfo.isOffline)
    {
      ImGui::TextUnformatted(vcString::Get("modalProfileOffline"));
    }
    else
    {
      if (ImGui::InputText(vcString::Get("modalProfileCurrentPassword"), pProgramState->changePassword.currentPassword, vcMaxPathLength, ImGuiInputTextFlags_Password))
        memset(pProgramState->changePassword.message, 0, sizeof(pProgramState->changePassword.message));

      ImGui::Separator();
      if (ImGui::InputText(vcString::Get("modalProfileNewPassword"), pProgramState->changePassword.newPassword, vcMaxPathLength, ImGuiInputTextFlags_Password))
        memset(pProgramState->changePassword.message, 0, sizeof(pProgramState->changePassword.message));

      if (ImGui::InputText(vcString::Get("modalProfileReEnterNewPassword"), pProgramState->changePassword.newPasswordConfirm, vcMaxPathLength, ImGuiInputTextFlags_Password))
        memset(pProgramState->changePassword.message, 0, sizeof(pProgramState->changePassword.message));
    }

    ImGui::Separator();

    bool newPasswordsAreSet = (pProgramState->changePassword.newPassword[0] != '\0') && (pProgramState->changePassword.newPasswordConfirm[0] != '\0');
    bool tryChangePassword = (pProgramState->changePassword.currentPassword[0] != '\0') && newPasswordsAreSet;

    if (pProgramState->changePassword.newPassword[0] != '\0' && udStrlen(pProgramState->changePassword.newPassword) < 8)
    {
      udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordRequirements"));
      tryChangePassword = false;
    }
    else if (newPasswordsAreSet && !udStrEqual(pProgramState->changePassword.newPassword, pProgramState->changePassword.newPasswordConfirm))
    {
      udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordNoMatch"));
      tryChangePassword = false;
    }

    if (pProgramState->changePassword.message[0] != '\0')
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", pProgramState->changePassword.message);

    bool submitPressed = ImGui::Button(vcString::Get("modalProfileConfirmNewPassword"), ImVec2(130.0f, 0));
    tryChangePassword = tryChangePassword && submitPressed;

    if (tryChangePassword)
    {
      udJSON changePasswordData;
      
      udJSON temp;

      temp.SetString(pProgramState->settings.loginInfo.email);
      changePasswordData.Set(&temp, "username");
      
      temp.SetString(pProgramState->changePassword.currentPassword);
      changePasswordData.Set(&temp, "oldpassword");

      temp.SetString(pProgramState->changePassword.newPassword);
      changePasswordData.Set(&temp, "password");

      temp.SetString(pProgramState->changePassword.newPasswordConfirm);
      changePasswordData.Set(&temp, "passwordConfirm");

      const char *pUpdatePasswordString = nullptr;
      changePasswordData.Export(&pUpdatePasswordString);

      const char *pResult = nullptr;
      vdkError result = vdkServerAPI_Query(pProgramState->pVDKContext, "v1/user/updatepassword", pUpdatePasswordString, &pResult);
      if (result == vE_Success)
      {
        udJSON resultData;
        resultData.Parse(pResult);

        if (resultData.Get("success").AsBool())
        {
          memset(pProgramState->changePassword.currentPassword, 0, sizeof(pProgramState->changePassword.currentPassword));
          memset(pProgramState->changePassword.newPassword, 0, sizeof(pProgramState->changePassword.newPassword));
          memset(pProgramState->changePassword.newPasswordConfirm, 0, sizeof(pProgramState->changePassword.newPasswordConfirm));
          memset(pProgramState->changePassword.message, 0, sizeof(pProgramState->changePassword.message));
          ImGui::CloseCurrentPopup();
        }
        else
        {
          const char *pMessage = resultData.Get("message").AsString();

          if (udStrEqual(pMessage, "Current password incorrect."))
            udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordIncorrect"));
          else if (udStrEqual(pMessage, "Passwords don't match."))
            udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordNoMatch"));
          else if (udStrEqual(pMessage, "Password is less than 8 character minimum."))
            udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordRequirements"));
          else
            udStrcpy(pProgramState->changePassword.message, vcString::Get("modalChangePasswordUnknownError"));
        }
      }

      vdkServerAPI_ReleaseResult(&pResult);
      udFree(pUpdatePasswordString);
    }
    
    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(buttonWidth, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      memset(pProgramState->changePassword.currentPassword, 0, sizeof(pProgramState->changePassword.currentPassword));
      memset(pProgramState->changePassword.newPassword, 0, sizeof(pProgramState->changePassword.newPassword));
      memset(pProgramState->changePassword.newPasswordConfirm, 0, sizeof(pProgramState->changePassword.newPasswordConfirm));
      memset(pProgramState->changePassword.message, 0, sizeof(pProgramState->changePassword.message));
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawConvert(vcState* pProgramState)
{
#if VC_HASCONVERT
  const char *pModalName = udTempStr("%s###convertDock", vcString::Get("convertTitle"));
  
  if (pProgramState->openModals & (1 << vcMT_Convert))
  {
    ImGui::OpenPopup(pModalName);
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
  }

  if (ImGui::BeginPopupModal(pModalName))
  {
    if (pProgramState->closeModals & (1 << vcMT_Convert))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 125.f);

    ImGui::TextUnformatted(vcString::Get("convertTitle"));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      ImGui::CloseCurrentPopup();

    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("convertSafeToCloseTooltip"));

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::BeginChild("__convertPane"))
      vcConvert_ShowUI(pProgramState);
    ImGui::EndChild();

    ImGui::EndPopup();
  }
#else
  udUnused(pProgramState);
#endif
}

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type)
{
  pProgramState->openModals |= (1 << type);
}

void vcModals_CloseModal(vcState *pProgramState, vcModalTypes type)
{
  pProgramState->closeModals |= (1 << type);
}

void vcModals_DrawModals(vcState *pProgramState)
{
  pProgramState->modalOpen = false;
  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawAddSceneItem(pProgramState);
  vcModals_DrawWelcome(pProgramState);
  vcModals_DrawExportProject(pProgramState);
  vcModals_DrawImportProject(pProgramState);
  vcModals_DrawProjectSettings(pProgramState);
  vcModals_DrawProjectChangeResult(pProgramState);
  vcModals_DrawProjectReadOnly(pProgramState);
  vcModals_DrawProjectInfo(pProgramState);
  vcModals_DrawImageViewer(pProgramState);
  vcModals_DrawUnsupportedFiles(pProgramState);
  vcModals_DrawProfile(pProgramState);
  vcModals_DrawChangePassword(pProgramState);
  vcModals_DrawConvert(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_LoggedOut));
  pProgramState->closeModals = 0;
}
