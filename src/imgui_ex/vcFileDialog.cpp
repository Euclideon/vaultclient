#include "vcFileDialog.h"

#include "udStringUtil.h"
#include "vcState.h"
#include "vcHotkey.h"
#include "imgui.h"
#include "vcImGuiSimpleWidgets.h"

#if UDPLATFORM_WINDOWS
# include <shobjidl.h>
# include <SDL2/SDL_syswm.h>
#endif

bool vcFileDialog_DrawImGui(char *pPath, size_t pathLength, vcFileDialogType dialogType = vcFDT_OpenFile, const char **ppExtensions = nullptr, size_t extensionCount = 0);

void vcFileDialog_Open(vcState *pProgramState, const char *pLabel, char *pPath, size_t pathLen, const char **ppExtensions, size_t numExtensions, vcFileDialogType dialogType, vcFileDialogCallback callback)
{
  memset(&pProgramState->fileDialog, 0, sizeof(pProgramState->fileDialog));

  pProgramState->fileDialog.pLabel = pLabel;
  pProgramState->fileDialog.dialogType = dialogType;

  pProgramState->fileDialog.pPath = pPath;
  pProgramState->fileDialog.pathLen = pathLen;

  pProgramState->fileDialog.ppExtensions = ppExtensions;
  pProgramState->fileDialog.numExtensions = numExtensions;

  pProgramState->fileDialog.onSelect = callback;

  ImGui::OpenPopup(udTempStr("%s##embeddedFileDialog", pLabel));
}

void vcFileDialog_ShowModal(vcState *pProgramState)
{
  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(udTempStr("%s##embeddedFileDialog", pProgramState->fileDialog.pLabel)))
  {
#if UDPLATFORM_WINDOWS
    if (pProgramState->settings.window.useNativeUI)
    {
      HRESULT hr = 0;
      IFileDialog *pFileOpen = nullptr;

      // Create the FileOpenDialog object.
      if (pProgramState->fileDialog.dialogType == vcFDT_OpenFile || pProgramState->fileDialog.dialogType == vcFDT_SelectDirectory)
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
      else // if (pProgramState->fileDialog.dialogType == vcFDT_SaveFile)
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

      if (SUCCEEDED(hr) && pProgramState->fileDialog.dialogType == vcFDT_SelectDirectory)
        hr = pFileOpen->SetOptions(FOS_PICKFOLDERS);

      if (SUCCEEDED(hr))
      {
        char extBuffer[1024] = "\0";
        COMDLG_FILTERSPEC spec = {};
        udOSString *pOSStr = nullptr;

        if (pProgramState->fileDialog.numExtensions > 0)
        {
          for (size_t i = 0; i < pProgramState->fileDialog.numExtensions; ++i)
          {
            if (i == 0)
              udStrcpy(extBuffer, "*");
            else
              udStrcat(extBuffer, ";*");

            udStrcat(extBuffer, pProgramState->fileDialog.ppExtensions[i]);
          }

          pOSStr = new udOSString(extBuffer);

          spec.pszName = L"Any Supported";
          spec.pszSpec = pOSStr->pWide;

          hr = pFileOpen->SetFileTypes(1U, &spec);
          hr = pFileOpen->SetDefaultExtension(spec.pszSpec);
        }

        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {
          // Show the Open dialog box.
          SDL_SysWMinfo info = {};
          SDL_GetWindowWMInfo(pProgramState->pWindow, &info);

          hr = pFileOpen->Show(info.info.win.window);

          if (SUCCEEDED(hr))
          {
            IShellItem *pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);

            if (SUCCEEDED(hr))
            {
              PWSTR pszFilePath = nullptr;
              hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

              // Display the file name to the user.
              if (SUCCEEDED(hr))
              {
                udStrcpy(pProgramState->fileDialog.pPath, pProgramState->fileDialog.pathLen, udOSString(pszFilePath).pUTF8);
                if (pProgramState->fileDialog.onSelect != nullptr)
                  pProgramState->fileDialog.onSelect();
                CoTaskMemFree(pszFilePath);
              }

              pItem->Release();
            }
          }
        }

        if (pOSStr != nullptr)
          delete pOSStr;
      }

      if (pFileOpen != nullptr)
        pFileOpen->Release();

      memset(&pProgramState->fileDialog, 0, sizeof(vcFileDialog));
      ImGui::CloseCurrentPopup();
    }
    else
#endif
    {
      pProgramState->modalOpen = true;

      ImGui::SetNextItemWidth(-270.f);
      bool loadFile = vcIGSW_InputText(vcString::Get("convertPathURL"), pProgramState->modelPath, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);

      ImGui::SameLine();

      const char *pLabel = "";
      if (pProgramState->fileDialog.dialogType == vcFDT_OpenFile || pProgramState->fileDialog.dialogType == vcFDT_SelectDirectory)
        pLabel = vcString::Get("popupLoad");
      else // if (pProgramState->fileDialog.dialogType == vcFDT_SaveFile)
        pLabel = vcString::Get("popupSave");

      if (ImGui::Button(pLabel, ImVec2(100.f, 0)))
        loadFile = true;
      ImGui::SameLine();

      if (ImGui::Button(vcString::Get("convertCancelButton"), ImVec2(100.f, 0)) || vcHotkey::IsPressed(vcB_Cancel))
      {
        memset(&pProgramState->fileDialog, 0, sizeof(vcFileDialog));
        ImGui::CloseCurrentPopup();
      }

      ImGui::Separator();

      if (vcFileDialog_DrawImGui(pProgramState->fileDialog.pPath, pProgramState->fileDialog.pathLen, pProgramState->fileDialog.dialogType, pProgramState->fileDialog.ppExtensions, pProgramState->fileDialog.numExtensions))
        loadFile = true;

      if (loadFile)
      {
        if (pProgramState->fileDialog.onSelect != nullptr)
          pProgramState->fileDialog.onSelect();
        memset(&pProgramState->fileDialog, 0, sizeof(vcFileDialog));
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
}

void vcFileDialog_FreeDrives(const char ***ppDrives, uint8_t *pDriveCount)
{
  if (ppDrives == nullptr || pDriveCount == nullptr)
    return;

  for (uint8_t i = 0; i < (*pDriveCount); i++)
  {
    udFree((*ppDrives)[i]);
  }
  udFree(*ppDrives);
  (*pDriveCount) = 0;
}

#if UDPLATFORM_WINDOWS
static bool vcFileDialog_IsDriveValid(const char *drive)
{
  uint32_t type = GetDriveTypeA(drive);
  return type == DRIVE_REMOVABLE || type == DRIVE_FIXED || type == DRIVE_REMOTE;
}
#endif

void vcFileDialog_GetDrives(const char ***ppDrives, uint8_t *pDriveCount)
{
  if (ppDrives == nullptr || pDriveCount == nullptr)
    return;

  vcFileDialog_FreeDrives(ppDrives, pDriveCount);

#if UDPLATFORM_WINDOWS
  DWORD driveMask = GetLogicalDrives();

  // Get number of drives, only include Removable, Fixed and Remote
  char buffer[] = "A:/";
  for (uint8_t i = 0; i < sizeof(driveMask) * 8; i++)
  {
    buffer[0] = ('A' + i);

    if ((driveMask >> i) & 1 && vcFileDialog_IsDriveValid(buffer))
      (*pDriveCount)++;
    else
      driveMask &= ~(1 << i); // Remove drive as it's invalid
  }

  // Create array and populate it
  (*ppDrives) = udAllocType(const char*, (*pDriveCount), udAF_None);
  uint8_t currentDrive = 0;
  for (uint8_t i = 0; i < sizeof(driveMask) * 8; i++)
  {
    buffer[0] = ('A' + i);

    if ((driveMask >> i) & 1)
    {
      (*ppDrives)[currentDrive] = udStrdup(buffer);
      currentDrive++;
    }
  }
#else
  // There are conflicting sources saying to use /proc/partitions and /proc/mounts
  // Use root directory for now
  (*pDriveCount) = 1;
  (*ppDrives) = udAllocType(const char*, 1, udAF_None);
  (*ppDrives)[0] = udStrdup("/");
#endif
}

bool vcFileDialog_ListFolder(const char *pFolderPath, char *pLoadPath, size_t loadPathLen, const char **ppExtensions, size_t extensionCount, bool showFiles)
{
  udFindDir *pDir;
  bool clicked = false;

  if (udOpenDir(&pDir, pFolderPath) != udR_Success)
    return false;

  do
  {
    char fullPath[vcMaxPathLength];

    if (pDir->pFilename[0] == '.' || pDir->pFilename[0] == '$') // Skip system '$*', this '.', parent '..' and misc 'hidden' folders '.*'
      continue;

    if (pDir->isDirectory)
    {
      bool opened = ImGui::TreeNode(pDir->pFilename);

      if (ImGui::IsItemClicked())
      {
        udSprintf(fullPath, "%s%s/", pFolderPath, pDir->pFilename);
        udStrcpy(pLoadPath, loadPathLen, fullPath);
      }

      if (!showFiles)
      {
        ImGui::SameLine();
        if (ImGui::SmallButton(udTempStr("%s##%s", vcString::Get("menuSelect"), pDir->pFilename)))
        {
          udSprintf(fullPath, "%s%s", pFolderPath, pDir->pFilename);
          udStrcpy(pLoadPath, loadPathLen, fullPath);
          clicked = true;
        }
      }

      if (opened)
      {
        udSprintf(fullPath, "%s%s/", pFolderPath, pDir->pFilename);
        clicked |= vcFileDialog_ListFolder(fullPath, pLoadPath, loadPathLen, ppExtensions, extensionCount, showFiles);
        ImGui::TreePop();
      }
    }
    else
    {
      bool show = (showFiles && (ppExtensions == nullptr || extensionCount == 0));

      for (size_t i = 0; i < extensionCount && !show; ++i)
        show = udStrEndsWithi(pDir->pFilename, ppExtensions[i]);

      if (show)
      {
        ImGui::Bullet();
        ImGui::SameLine();
        if (ImGui::Button(pDir->pFilename))
        {
          udSprintf(fullPath, "%s%s", pFolderPath, pDir->pFilename);
          udStrcpy(pLoadPath, loadPathLen, fullPath);
          clicked = true;
        }
      }
    }
  } while (udReadDir(pDir) == udR_Success);

  udCloseDir(&pDir); // Ignore failure here
  return clicked;
}

bool vcFileDialog_DrawImGui(char *pPath, size_t pathLength, vcFileDialogType dialogType /*= true*/, const char **ppExtensions /*= nullptr*/, size_t extensionCount /*= 0*/)
{
  const char **ppDrives = nullptr;
  uint8_t length = 0;
  bool clickedFile = false;

  vcFileDialog_GetDrives(&ppDrives, &length);
  ImGui::BeginChild("BrowseDialog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

  for (uint8_t i = 0; i < length; ++i)
  {
    bool opened = ImGui::TreeNode(ppDrives[i]);
    
    if (ImGui::IsItemClicked())
      udStrcpy(pPath, pathLength, ppDrives[i]);

    if (opened)
    {
      clickedFile |= vcFileDialog_ListFolder(ppDrives[i], pPath, pathLength, ppExtensions, extensionCount, (dialogType != vcFDT_SelectDirectory));
      ImGui::TreePop();
    }
  }

  ImGui::EndChild();
  vcFileDialog_FreeDrives(&ppDrives, &length);

  return clickedFile; // return true if a file was selected
}
