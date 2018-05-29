#include "vcSettings.h"
#include "imgui.h"
#include "udPlatform/udPlatformUtil.h"

bool vcSettings_Load(vcSettings *pSettings)
{
  ImGui::GetIO().IniFilename = NULL; // Disables auto save and load

  udUnused(pSettings);

  return true;
}

bool vcSettings_Save(vcSettings *pSettings)
{
  ImGui::GetIO().WantSaveIniSettings = false;

  udUnused(pSettings);

  return false;
}
