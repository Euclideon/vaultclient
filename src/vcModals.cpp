#include "vcModals.h"

#include "vcState.h"
#include "vcPOI.h"
#include "gl/vcTexture.h"
#include "vcRender.h"
#include "vcStrings.h"
#include "vcConvert.h"
#include "vcProxyHelper.h"
#include "vcStringFormat.h"
#include "vcHotkey.h"
#include "vcWebFile.h"
#include "vcSession.h"

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
bool vcModals_OverwriteExistingFile(const char *pFilename)
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

// Presents user with a message if the specified file exists, then returns false if user declines to overwrite the file
bool vcModals_AllowDestructiveAction(const char *pTitle, const char *pMessage)
{
#if UDPLATFORM_EMSCRIPTEN
  udUnused(pTitle);
  udUnused(pMessage);
  return true;
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
    NULL,
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

  bool retVal = vcModals_AllowDestructiveAction(isQuit ? vcString::Get("endSessionExitTitle") : vcString::Get("endSessionLogoutTitle"), pMessage);

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

void vcModals_DrawNewProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_NewProject))
    ImGui::OpenPopup(vcString::Get("modalProjectNewTitle"));

  ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("modalProjectNewTitle")))
  {
    pProgramState->modalOpen = true;

    static int zoneType = 0;
    static int zoneCustomSRID = 84;

    vcIGSW_InputText(vcString::Get("modalProjectNewName"), pProgramState->modelPath, udLengthOf(pProgramState->modelPath));

    if (ImGui::RadioButton(vcString::Get("modalProjectNewGeolocated"), &zoneType, 0))
      zoneCustomSRID = 84;
    if (ImGui::RadioButton(vcString::Get("modalProjectNewNonGeolocated"), &zoneType, 1))
      zoneCustomSRID = 0;

    ImGui::RadioButton(vcString::Get("modalProjectNewGeolocatedSpecificZone"), &zoneType, 2);
    if (zoneType == 2)
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

    if (ImGui::Button(vcString::Get("modalProjectNewCreate"), ImVec2(100.f, 0)) && vcProject_AbleToChange(pProgramState))
    {
      vcProject_InitBlankScene(pProgramState, pProgramState->modelPath, zoneCustomSRID);
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

    //TODO: Additional export settings

    ImGui::EndPopup();
  }
}

void vcModals_DrawExportProject(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ExportProject))
    ImGui::OpenPopup(vcString::Get("menuProjectExportTitle"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("menuProjectExportTitle")))
  {
    pProgramState->modalOpen = true;
    
    vcIGSW_FilePicker(pProgramState, vcString::Get("menuFileName"), pProgramState->modelPath, SupportedFileTypes_ProjectsExport, vcFDT_SaveFile, nullptr);

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("sceneExplorerExportButton"), ImVec2(100.f, 0)))
    {
      vcProject_Save(pProgramState, pProgramState->modelPath, false);
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

    //TODO: Additional export settings

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
    pProgramState->modalOpen = true;

    if (pProgramState->sessionInfo.isOffline)
    {
      ImGui::TextUnformatted(vcString::Get("modalProfileOffline"));

      ImGui::InputText(vcString::Get("modalProfileDongle"), pProgramState->sessionInfo.displayName, udLengthOf(pProgramState->sessionInfo.displayName), ImGuiInputTextFlags_ReadOnly);
    }
    else
    {
      const char *pUsername = pProgramState->profileInfo.Get("user.username").AsString("");
      ImGui::InputText(vcString::Get("modalProfileUsername"), (char*)pUsername, udStrlen(pUsername), ImGuiInputTextFlags_ReadOnly);

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
    pProgramState->modalOpen = true;

    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 125.f);

    ImGui::TextUnformatted(vcString::Get("convertTitle"));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("popupClose"), ImVec2(-1, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      ImGui::CloseCurrentPopup();

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

void vcModals_DrawModals(vcState *pProgramState)
{
  pProgramState->modalOpen = false;
  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawAddSceneItem(pProgramState);
  vcModals_DrawNewProject(pProgramState);
  vcModals_DrawExportProject(pProgramState);
  vcModals_DrawImportProject(pProgramState);
  vcModals_DrawProjectChangeResult(pProgramState);
  vcModals_DrawProjectReadOnly(pProgramState);
  vcModals_DrawImageViewer(pProgramState);
  vcModals_DrawUnsupportedFiles(pProgramState);
  vcModals_DrawProfile(pProgramState);
  vcModals_DrawConvert(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_LoggedOut));
}
