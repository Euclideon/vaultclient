#include "vcFileDialog.h"

#include "udStringUtil.h"
#include "vcState.h"
#include "imgui.h"

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

bool vcFileDialog_ListFolder(const char *pFolderPath, char *pLoadPath, size_t loadPathLen, const char **ppExtensions, size_t extensionCount)
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

      if (opened)
      {
        udSprintf(fullPath, "%s%s/", pFolderPath, pDir->pFilename);
        clicked |= vcFileDialog_ListFolder(fullPath, pLoadPath, loadPathLen, ppExtensions, extensionCount);
        ImGui::TreePop();
      }
    }
    else
    {
      bool show = (ppExtensions == nullptr || extensionCount == 0);

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

bool vcFileDialog_DrawImGui(char *pPath, size_t pathLength, bool loadOnly /*= true*/, const char **ppExtensions /*= nullptr*/, size_t extensionCount /*= 0*/)
{
  udUnused(loadOnly);

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
      clickedFile |= vcFileDialog_ListFolder(ppDrives[i], pPath, pathLength, ppExtensions, extensionCount);
      ImGui::TreePop();
    }
  }

  ImGui::EndChild();
  vcFileDialog_FreeDrives(&ppDrives, &length);

  return clickedFile; // return true if a file was selected
}
