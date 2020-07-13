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

bool gShowInputControlsNextHack = false;

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
    static bool showInputControls = true;
    // Don't set to false if it's set to true (could be put in the block with OpenPopup, but it could be forced open for some other reason)
    gShowInputControlsNextHack = gShowInputControlsNextHack || showInputControls;
    showInputControls = false;

    if (pProgramState->closeModals & (1 << vcMT_Welcome))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

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

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

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

    udReadLockRWLock(pProgramState->pSessionLock);
    if (pProgramState->featuredProjects.length > 0)
    {
      ImVec2 textSize = ImGui::CalcTextSize(vcString::Get("welcomeFeaturedProjects"));
      ImGui::SetCursorPosX((windowSize.x - textSize.x) / 2);
      ImGui::TextUnformatted(vcString::Get("welcomeFeaturedProjects"));

      ImGuiStyle style = ImGui::GetStyle();

      float totalSize = pProgramState->featuredProjects.length * (128 + style.FramePadding.x);
      if (pProgramState->featuredProjects.length > 1)
        totalSize += (pProgramState->featuredProjects.length * style.ItemSpacing.x);

      ImGui::SetCursorPosX((windowSize.x - totalSize) / 2);

      bool first = true;
      for (auto item : pProgramState->featuredProjects)
      {
        if (!first)
          ImGui::SameLine();
        first = false;

        if (ImGui::ImageButton(item.pTexture, ImVec2(128, 128)))
        {
          vcProject_LoadFromServer(pProgramState, udUUID_GetAsString(item.projectID));
          ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("%s", item.pProjectName);
      }

      ImGui::Separator();
    }
    udReadUnlockRWLock(pProgramState->pSessionLock);

    struct
    {
      const char *pAction;                                      const char *pDescription;                             vcMenuBarButtonIcon icon;
    } items[] = {
      { vcString::Get("menuProjectImport"),                     vcString::Get("menuProjectImportDesc"),               vcMBBI_Open },
      { vcString::Get("modalProjectNewCreate"),                 vcString::Get("modalProjectNewDesc"),                 vcMBBI_NewProject },
#if VC_HASCONVERT
      { vcString::Get("convertTitle"),                          vcString::Get("convertDesc"),                         vcMBBI_Convert },
#endif // VC_HASCONVERT
    };

    {
      ImGui::Columns(2);

      ImGui::TextUnformatted(vcString::Get("modalProjectOpenRecent"));
      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::BeginChild("historyItems", ImVec2(0, -40)))
      {
        for (size_t i = 0; i < pProgramState->settings.projectsHistory.projects.length; ++i)
        {
          vcProjectHistoryInfo *pProjectInfo = &pProgramState->settings.projectsHistory.projects[i];

          bool selectableClicked = ImGui::Selectable(udTempStr("##projectHistoryItem%zu", i), false, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 40));

          ImVec2 itemRect = ImGui::GetItemRectSize();
          ImVec2 activeCursorPos = ImGui::GetCursorPos();
          bool selectableHovered = ImGui::IsItemHovered();

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

          // This is quite complicated and was written in a rush.
          // Due to some interesting behaviour in ImGui::Selectable, the delete button has to be process using the click of the selectable but with the hover of the button
          // We then continue so the selectable isn't processed
          if (selectableHovered)
          {
            ImGui::SetCursorPos(ImVec2(activeCursorPos.x + itemRect.x - 50.f, activeCursorPos.y - 37.f));
            vcMenuBarButton(pProgramState->pUITexture, vcString::Get("modalProjectClearRecentItem"), nullptr, vcMBBI_Remove, vcMBBG_FirstItem);

            if (selectableClicked && ImGui::IsItemHovered())
            {
              vcProject_RemoveHistoryItem(pProgramState, i);
              --i;
              continue;
            }
          }

          if (selectableClicked)
          {
            if (pProjectInfo->isServerProject)
              vcProject_LoadFromServer(pProgramState, pProjectInfo->pPath);
            else
              vcProject_LoadFromURI(pProgramState, pProjectInfo->pPath);

            ImGui::CloseCurrentPopup();
          }

          ImGui::SetCursorPos(activeCursorPos);
          ImGui::Spacing();
        }

        ImGui::EndChild();
      }

      ImGui::NextColumn();

      ImGui::Text("%s", vcString::Get("modalProjectGetStarted"));
      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::BeginChild("gettingStartedItems", ImVec2(0, -40)))
      {
        for (size_t i = 0; i < udLengthOf(items); ++i)
        {
          bool selected = false;
          if (ImGui::Selectable(udTempStr("##newProjectType%zu", i), &selected, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 40)))
          {
            ImGui::CloseCurrentPopup();

            if (items[i].icon == vcMBBI_NewProject)
              vcModals_OpenModal(pProgramState, vcMT_CreateProject);
            else if (items[i].icon == vcMBBI_Convert)
              vcModals_OpenModal(pProgramState, vcMT_Convert);
            else if (items[i].icon == vcMBBI_Open)
              vcModals_OpenModal(pProgramState, vcMT_LoadProject);
          }

          float prevPosY = ImGui::GetCursorPosY();
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 38);

          udFloat4 iconUV = vcGetIconUV(items[i].icon);
          ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
          ImGui::SameLine();

          // Align text with icon
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

          float textAlignPosX = ImGui::GetCursorPosX();
          ImGui::TextUnformatted(items[i].pAction);

          // Manually align details text with title text
          ImGui::SetCursorPosX(textAlignPosX);
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

          ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
          col.w *= 0.65f;
          ImGui::PushStyleColor(ImGuiCol_Text, col);
          ImGui::TextUnformatted(items[i].pDescription);
          ImGui::PopStyleColor();

          ImGui::SetCursorPosY(prevPosY);
          ImGui::Spacing();
        }

        ImGui::EndChild();
      }

      ImGui::EndColumns();
    }
    
    // Position control buttons in the bottom right corner
    {
      ImGui::Separator();

      const float DismissButtonSize = 200.f;

      ImGui::Spacing();
      ImGui::Spacing();

      ImGui::SetCursorPosX((windowSize.x - DismissButtonSize) / 2.f);

      if (ImGui::Button(vcString::Get("popupDismiss"), ImVec2(DismissButtonSize, 0)) || vcHotkey::IsPressed(vcB_Cancel))
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawExportProject(vcState *pProgramState)
{
  static char ErrorText[128] = {};

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
#if UDPLATFORM_EMSCRIPTEN
      // TODO: Handle downloading local copy for Emscripten instead
      if (i == 0)
        continue;
#endif

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
          udReadLockRWLock(pProgramState->pSessionLock);
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
            {
              vdkError result = vcProject_SaveAsServer(pProgramState, udUUID_GetAsString(selectedGroup));
              if (result == vE_Success)
                ImGui::CloseCurrentPopup();
              else
                udSprintf(ErrorText, "%s: %s", vcString::Get("errorServerCommunication"), vcProject_ErrorToString(result));
            }
          }
          else
          {
            ImGui::TextUnformatted(vcString::Get("modalProjectNoGroups"));
          }
          udReadUnlockRWLock(pProgramState->pSessionLock);
        }

        ImGui::EndChild();
      }
    }

    if (ErrorText[0] != 0)
    {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "%s", ErrorText);
      ImGui::Spacing();
      ImGui::Spacing();
    }

    if (ImGui::Button(vcString::Get("sceneExplorerCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
    {
      pProgramState->modelPath[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
  else
  {
    ErrorText[0] = 0;
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

void vcModals_DrawCreateProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_CreateProject))
    ImGui::OpenPopup(vcString::Get("menuProjectCreateButton"));

  ImGui::SetNextWindowSize(ImVec2(475, 450), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectCreateButton"), 0, ImGuiWindowFlags_NoResize))
  {
    if (pProgramState->closeModals & (1 << vcMT_CreateProject))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    ImVec2 windowSize = ImGui::GetWindowSize();

    static int creatingNewProjectType = -1;
    static int zoneCustomSRID = 84;
    static char pProjectPath[vcMaxPathLength] = {};
    static udUUID selectedGroup = {};
    static const char *pGroupName = nullptr;
    static bool availableGroups = true;
    static vdkError result = vE_Success;

    if (ImGui::IsWindowAppearing())
    {
      creatingNewProjectType = -1;
      pGroupName = nullptr;
      availableGroups = true;
      udUUID_Clear(&selectedGroup);
      result = vE_Success;
    }

    struct
    {
      const char *pAction;                                      const char *pDescription;                             vcMenuBarButtonIcon icon;
    } items[] = {
      { vcString::Get("modalProjectNewGeolocated"),             vcString::Get("modalProjectGeolocatedDescription"),   vcMBBI_Geospatial },
      { vcString::Get("modalProjectNewNonGeolocated"),          vcString::Get("modalProjectNonGeolocatedDescription"),vcMBBI_Grid },
      { vcString::Get("modalProjectNewGeolocatedSpecificZone"), vcString::Get("modalProjectSpecificZoneDescription"), vcMBBI_ExpertGrid },
    };


    if (creatingNewProjectType == -1)
    {
      for (size_t i = 0; i < udLengthOf(items); ++i)
      {
        bool selected = false;
        if (ImGui::Selectable(udTempStr("##newProjectType%zu", i), &selected, ImGuiSelectableFlags_DontClosePopups, ImVec2(475, 48)))
        {
          creatingNewProjectType = (int)i;
          udStrcpy(pProgramState->modelPath, vcString::Get("modalProjectNewTitle"));

          if (items[i].icon == vcMBBI_Geospatial)
            zoneCustomSRID = 84; // Geolocated
          else if (items[i].icon == vcMBBI_Grid)
            zoneCustomSRID = 0; // Non Geolocated
        }

        float prevPosY = ImGui::GetCursorPosY();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 46);

        udFloat4 iconUV = vcGetIconUV(items[i].icon);
        ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
        ImGui::SameLine();

        // Align text with icon
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);

        float textAlignPosX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(items[i].pAction);

        // Manually align details text with title text
        ImGui::SetCursorPosX(textAlignPosX);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

        ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        col.w *= 0.65f;
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(items[i].pDescription);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(prevPosY);
        ImGui::Spacing();
      }

      ImGui::SetCursorPosY(windowSize.y - 30);
      if (ImGui::Button(vcString::Get("popupDismiss"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      {
        pProgramState->modelPath[0] = '\0';
        creatingNewProjectType = -1;
        result = vE_Success;
        ImGui::CloseCurrentPopup();
      }
    }
    else
    {
      ImGui::TextUnformatted(items[creatingNewProjectType].pAction);

      vcIGSW_InputText(vcString::Get("modalProjectNewName"), pProgramState->modelPath, udLengthOf(pProgramState->modelPath));

      if (items[creatingNewProjectType].icon == vcMBBI_ExpertGrid)
      {
        ImGui::InputInt(vcString::Get("modalProjectNewGeolocatedSpecificZoneID"), &zoneCustomSRID);

        udGeoZone zone;
        if (udGeoZone_SetFromSRID(&zone, zoneCustomSRID) != udR_Success)
          ImGui::TextUnformatted(vcString::Get("sceneUnsupportedSRID"));
        else
          ImGui::Text("%s (%s: %d)", zone.displayName, vcString::Get("sceneSRID"), zoneCustomSRID);
      }

      struct
      {
        const char *pName;
        vcMenuBarButtonIcon icon;
        float height;
      } types[] = {
        { "menuProjectExportDisk",  vcMBBI_StorageLocal, 85.f },
        { "menuProjectExportCloud", vcMBBI_StorageCloud, 85.f },
        { "menuProjectExportMemory", vcMBBI_Visualization, 60.f },
      };

      for (size_t i = 0; i < udLengthOf(types); ++i)
      {
        if (ImGui::BeginChild(udTempStr("##saveAsType%zu", i), ImVec2(-1, types[i].height), true))
        {
          udFloat4 iconUV = vcGetIconUV(types[i].icon);
          ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
          ImGui::SameLine();

          float textAlignPosX = ImGui::GetCursorPosX();
          ImGui::TextUnformatted(vcString::Get(types[i].pName));

          // Manually align details text with title text
          ImGui::SetCursorPosX(textAlignPosX);
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

          if (types[i].icon == vcMBBI_StorageLocal) // local
          {
            vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProjectPath, SupportedFileTypes_ProjectsExport, vcFDT_SaveFile, nullptr);

            ImGui::SetCursorPosX(textAlignPosX);
            if (ImGui::Button(vcString::Get("menuProjectCreateButton")) && vcProject_AbleToChange(pProgramState))
            {
              udFilename exportFilename(pProjectPath);
              vcProject_AutoCompletedName(&exportFilename, pProjectPath, pProgramState->modelPath);
              result = vcProject_CreateFileScene(pProgramState, exportFilename, pProgramState->modelPath, zoneCustomSRID);
              if (result == vE_Success)
              {
                pProgramState->modelPath[0] = '\0';
                ImGui::CloseCurrentPopup();
              }
            }
          }
          else if (types[i].icon == vcMBBI_StorageCloud) // cloud
          {
            udReadLockRWLock(pProgramState->pSessionLock);
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
              if (ImGui::Button(vcString::Get("menuProjectCreateButton")) && vcProject_AbleToChange(pProgramState))
              {
                result = vcProject_CreateServerScene(pProgramState, pProgramState->modelPath, udUUID_GetAsString(selectedGroup), zoneCustomSRID);
                if (result == vE_Success)
                {
                  pProgramState->modelPath[0] = '\0';
                  ImGui::CloseCurrentPopup();
                }
              }
            }
            else
            {
              ImGui::TextUnformatted(vcString::Get("modalProjectNoGroups"));
            }
            udReadUnlockRWLock(pProgramState->pSessionLock);
          }
          else if (types[i].icon == vcMBBI_Visualization)
          {
            if (ImGui::Button(vcString::Get("menuProjectCreateButton")) && vcProject_AbleToChange(pProgramState))
            {
              if (vcProject_CreateBlankScene(pProgramState, pProgramState->modelPath, zoneCustomSRID))
              {
                pProgramState->modelPath[0] = '\0';
                ImGui::CloseCurrentPopup();
              }
              else
              {
                result = vE_Failure;
              }
            }
          }

          ImGui::EndChild();
        }
      }

      ImGui::SetCursorPosY(windowSize.y - 30);

      if (ImGui::Button("Back", ImVec2(100.f, 0)))
      {
        creatingNewProjectType = -1;
        result = vE_Success;
      }

      if (result != vE_Success)
      {
        ImGui::NextColumn();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "%s", vcProject_ErrorToString(result));
      }
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawLoadProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoadProject))
    ImGui::OpenPopup(vcString::Get("menuProjectImportTitle"));

  ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectImportTitle")))
  {
    if (pProgramState->closeModals & (1 << vcMT_LoadProject))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;
    
    udFloat4 openUV = vcGetIconUV(vcMBBI_Open);
    udFloat4 shareUV = vcGetIconUV(vcMBBI_Share);
    udFloat4 ownerUV = vcGetIconUV(vcMBBI_NewProject);
    udFloat4 publicUV = vcGetIconUV(vcMBBI_Visualization);

    struct
    {
      const char *pName;
      vcMenuBarButtonIcon icon;
      float size;
    } types[] = {
      { "menuProjectImportShared", vcMBBI_Share, 90.f },
      { "menuProjectImportDisk", vcMBBI_StorageLocal, 90.f },
      { "menuProjectImportCloud", vcMBBI_StorageCloud, -30.f },
    };

    for (size_t i = 0; i < udLengthOf(types); ++i)
    {
#if UDPLATFORM_EMSCRIPTEN
      // TODO: Handle uploading local copy for Emscripten instead
      if (types[i].icon == vcMBBI_StorageLocal)
        continue;
#endif

      if (ImGui::BeginChild(udTempStr("##loadFromType%zu", i), ImVec2(-1, types[i].size), true))
      {
        float startingPos = ImGui::GetCursorPosX();

        udFloat4 iconUV = vcGetIconUV(types[i].icon);
        ImGui::Image(pProgramState->pUITexture, ImVec2(24, 24), ImVec2(iconUV.x, iconUV.y), ImVec2(iconUV.z, iconUV.w));
        ImGui::SameLine();

        float textAlignPosX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(vcString::Get(types[i].pName));

        // Manually align details text with title text
        ImGui::SetCursorPosX(textAlignPosX);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9);

        if (types[i].icon == vcMBBI_Share) // Shared
        {
          static char shareModelPath[vcMaxPathLength];
          ImGui::InputText(vcString::Get("shareLinkBrowserLoad"), shareModelPath, udLengthOf(shareModelPath));
          ImGui::SetCursorPosX(textAlignPosX);
          if (ImGui::Button(vcString::Get("menuProjectImport")) && vcProject_AbleToChange(pProgramState))
          {
            if (udStrBeginsWith(shareModelPath, "euclideon:project/"))
            {
              vcProject_LoadFromServer(pProgramState, &shareModelPath[18]);
              shareModelPath[0] = '\0';
              ImGui::CloseCurrentPopup();
            }
            else
            {
              // Show error code here?
            }
          }
        }
        else if (types[i].icon == vcMBBI_StorageLocal) // local
        {
          vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProgramState->modelPath, SupportedFileTypes_ProjectsExport, vcFDT_OpenFile, [pProgramState] {
            if (vcProject_AbleToChange(pProgramState))
            {
              vcProject_LoadFromURI(pProgramState, pProgramState->modelPath);
              pProgramState->modelPath[0] = '\0';
              ImGui::CloseCurrentPopup();
            }
          });

          ImGui::SetCursorPosX(textAlignPosX);
          if (ImGui::Button(vcString::Get("menuProjectImport")) && vcProject_AbleToChange(pProgramState))
          {
            vcProject_LoadFromURI(pProgramState, pProgramState->modelPath);
            pProgramState->modelPath[0] = '\0';

            ImGui::CloseCurrentPopup();
          }
        }
        else if (types[i].icon == vcMBBI_StorageCloud) // cloud
        {
          udReadLockRWLock(pProgramState->pSessionLock);
          if (pProgramState->groups.length > 0)
          {
            ImGui::Indent(textAlignPosX - startingPos);

            float spacing = ImGui::GetStyle().IndentSpacing * 2;

            for (vcGroupInfo &group : pProgramState->groups)
            {
              if (group.projects.length > 0)
              {
                ImGui::Image(pProgramState->pUITexture, ImVec2(16, 16), ImVec2(openUV.x, openUV.y), ImVec2(openUV.z, openUV.w));
                ImGui::SameLine();

                ImGui::TextUnformatted(group.pGroupName);

                if (group.permissionLevel == vcGroupPermissions_Owner)
                {
                  ImGui::SameLine();
                  ImGui::Image(pProgramState->pUITexture, ImVec2(16, 16), ImVec2(ownerUV.x, ownerUV.y), ImVec2(ownerUV.z, ownerUV.w));
                  if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", vcString::Get("modalGroupOwner"));
                }

                if (group.visibility == vcGroupVisibility_Public || group.visibility == vcGroupVisibility_Internal)
                {
                  ImGui::SameLine();
                  ImGui::Image(pProgramState->pUITexture, ImVec2(16, 16), ImVec2(publicUV.x, publicUV.y), ImVec2(publicUV.z, publicUV.w));
                  if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", vcString::Get("modalGroupPublic"));
                }

                ImGui::Indent(spacing);

                for (vcProjectInfo &project : group.projects)
                {
                  if (project.isDeleted)
                    continue;

                  float selectablePosY = ImGui::GetCursorPosY();

                  if (ImGui::Selectable(udTempStr("##prj_%s", udUUID_GetAsString(project.projectID)), false) && vcProject_AbleToChange(pProgramState))
                  {
                    vcProject_LoadFromServer(pProgramState, udUUID_GetAsString(project.projectID));
                    ImGui::CloseCurrentPopup();
                  }

                  if (group.permissionLevel >= vcGroupPermissions_Manager)
                  {
                    if (ImGui::BeginPopupContextItem())
                    {
                      if (group.permissionLevel == vcGroupPermissions_Owner)
                      {
                        if (ImGui::MenuItem(vcString::Get("modalProjectDelete")) && vcModals_AllowDestructiveAction(pProgramState, vcString::Get("modalProjectDelete"), vcString::Get("modalProjectDeleteDesc")))
                        {
                          if (vdkProject_DeleteServerProject(pProgramState->pVDKContext, udUUID_GetAsString(project.projectID)) == vE_Success)
                            project.isDeleted = true;

                          //TODO: Handle when it doesn't work?
                        }
                      }

                      if (project.isShared)
                      {
                        if (ImGui::MenuItem(vcString::Get("shareMakeUnshare")))
                        {
                          if (vdkProject_SetLinkShareStatus(pProgramState->pVDKContext, udUUID_GetAsString(project.projectID), false) == vE_Success)
                            project.isShared = false;

                          //TODO: Handle when it doesn't work?
                        }
                      }

                      if (!project.isShared)
                      {
                        if (ImGui::MenuItem(vcString::Get("shareMakeShare")))
                        {
                          if (vdkProject_SetLinkShareStatus(pProgramState->pVDKContext, udUUID_GetAsString(project.projectID), true) == vE_Success)
                            project.isShared = true;

                          //TODO: Handle when it doesn't work?
                        }
                      }

                      ImGui::EndPopup();
                    }
                  }

                  float prevPosY = ImGui::GetCursorPosY();
                  ImGui::SetCursorPosY(selectablePosY);

                  ImGui::TextUnformatted(project.pProjectName);

                  if (project.isShared)
                  {
                    ImGui::SameLine();
                    ImGui::Image(pProgramState->pUITexture, ImVec2(16, 16), ImVec2(shareUV.x, shareUV.y), ImVec2(shareUV.z, shareUV.w));
                    if (ImGui::IsItemHovered())
                      ImGui::SetTooltip("%s", vcString::Get("shareProjectShared"));
                  }

                  ImGui::SetCursorPosY(prevPosY);
                  ImGui::Spacing();
                }

                ImGui::Unindent(spacing);
              }
            }

            ImGui::Unindent(textAlignPosX - startingPos);
          }
          else // No projects
          {
            ImGui::MenuItem(vcString::Get("menuProjectNone"), nullptr, nullptr, false);
          }
          udReadUnlockRWLock(pProgramState->pSessionLock);
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
        case udR_ObjectNotFound:
          pMessage = vcString::Get("sceneExplorerProjectChangeNotFoundOrDenied");
          break;
        case udR_ExceededAllowedLimit:
          pMessage = vcString::Get("errorExceedsProjectLimit");
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

void vcModals_DrawInputHelper(vcState* pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ShowInputInfo))
    ImGui::OpenPopup("###sceneInputHelper");

  ImGui::SetNextWindowSize(ImVec2(850.f, 400.f), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal("###sceneInputHelper", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
  {
    if (pProgramState->closeModals & (1 << vcMT_ShowInputInfo))
      ImGui::CloseCurrentPopup();
    else
      pProgramState->modalOpen = true;

    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 itemSize = {};

    ImGui::Image(pProgramState->pInputsTexture, ImVec2(850.f, 330.f));

    const float ComboY = 50.f;
    const float ComboWidth = 150.f;
    const float DismissWidth = 100.f;

    const char *mouseModes[] = { vcString::Get("settingsControlsTumble"), vcString::Get("settingsControlsOrbit"), vcString::Get("settingsControlsPan"), vcString::Get("settingsControlsForward") };
    const char *scrollwheelModes[] = { vcString::Get("settingsControlsDolly"), vcString::Get("settingsControlsChangeMoveSpeed") };

    // Checks so the casts below are safe
    UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.cameraMouseBindings[0]) == sizeof(int), "Bindings is no longer sizeof(int)");
    UDCOMPILEASSERT(sizeof(pProgramState->settings.camera.scrollWheelMode) == sizeof(int), "ScrollWheel is no longer sizeof(int)");

    ImGui::SetCursorPos(ImVec2(20.f, ComboY));
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::Combo(vcString::Get("settingsControlsLeft"), (int*)&pProgramState->settings.camera.cameraMouseBindings[0], mouseModes, (int)udLengthOf(mouseModes));

    itemSize = ImGui::CalcTextSize(vcString::Get("settingsControlsRight"));
    ImGui::SetCursorPos(ImVec2(udMin(834.f, size.x - itemSize.x - ComboWidth - 15.f), ComboY));
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::Combo(vcString::Get("settingsControlsRight"), (int*)&pProgramState->settings.camera.cameraMouseBindings[1], mouseModes, (int)udLengthOf(mouseModes));

    itemSize = ImGui::CalcTextSize(vcString::Get("settingsControlsMiddle"));
    ImGui::SetCursorPos(ImVec2((size.x - ComboWidth - itemSize.x) / 2.f, ComboY - 24.f));
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::Combo(vcString::Get("settingsControlsScrollWheel"), (int*)&pProgramState->settings.camera.scrollWheelMode, scrollwheelModes, (int)udLengthOf(scrollwheelModes));

    ImGui::SetCursorPos(ImVec2((size.x - ComboWidth - itemSize.x) / 2.f, ComboY));
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::Combo(vcString::Get("settingsControlsMiddle"), (int*)&pProgramState->settings.camera.cameraMouseBindings[2], mouseModes, (int)udLengthOf(mouseModes));

    ImGui::SetCursorPos(ImVec2((size.x - DismissWidth) / 2.f, size.y - 30));
    if (ImGui::Button(vcString::Get("popupDismiss"), ImVec2(DismissWidth, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
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
  int cancelModals = ~(pProgramState->openModals & pProgramState->closeModals);
  pProgramState->openModals = (pProgramState->openModals & cancelModals);
  pProgramState->closeModals = (pProgramState->closeModals & cancelModals);

  pProgramState->modalOpen = false;

  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawAddSceneItem(pProgramState);
  vcModals_DrawWelcome(pProgramState);
  vcModals_DrawExportProject(pProgramState);
  vcModals_DrawCreateProject(pProgramState);
  vcModals_DrawLoadProject(pProgramState);
  vcModals_DrawProjectSettings(pProgramState);
  vcModals_DrawProjectChangeResult(pProgramState);
  vcModals_DrawProjectReadOnly(pProgramState);
  vcModals_DrawProjectInfo(pProgramState);
  vcModals_DrawImageViewer(pProgramState);
  vcModals_DrawUnsupportedFiles(pProgramState);
  vcModals_DrawProfile(pProgramState);
  vcModals_DrawChangePassword(pProgramState);
  vcModals_DrawConvert(pProgramState);

  if (gShowInputControlsNextHack && !pProgramState->modalOpen)
  {
    gShowInputControlsNextHack = false;
    vcModals_OpenModal(pProgramState, vcMT_ShowInputInfo);
  }
  vcModals_DrawInputHelper(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_LoggedOut));
  pProgramState->closeModals = 0;
}
