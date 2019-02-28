#include "vcStrings.h"
#include "vCore/vStringFormat.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udJSON.h"

#include <string>
#include <unordered_map>

namespace vcString
{
  static std::unordered_map<std::string, const char*> gStringTable;

  // pValue needs to be already duplicated before calling this function
  void Set(const char *pKey, const char *pValue)
  {
    const char *pExistingValue = gStringTable[pKey];

    if (pExistingValue != nullptr)
      udFree(pExistingValue);

    gStringTable[pKey] = pValue;
  }

  const char* Get(const char *pKey)
  {
    const char *pValue = gStringTable[pKey];

    if (pValue == nullptr)
    {
      // Caches the mising string-
      gStringTable[pKey] = vStringFormat("{MISSING/{0}}", pKey);
      return gStringTable[pKey];
    }
    else
    {
      return pValue;
    }
  }

  vdkError LoadTable(const char *pFilename)
  {
    char *pPos = nullptr;
    if (pFilename == nullptr || udFile_Load(pFilename, (void **)&pPos) != udR_Success)
      pPos = udStrdup(""); // So this can be free'd later

    udJSON strings;

    if (strings.Parse(pPos) != udR_Success)
      strings.SetObject();

    udFree(pPos);

    size_t count = strings.MemberCount();

    for (size_t i = 0; i < count; ++i)
    {
      const char *pKey = strings.GetMemberName(i);
      const char *pValue = strings.GetMember(i)->AsString();

      if (pValue != nullptr)
        Set(pKey, udStrdup(pValue));
    }

    // Concatenations
    Set("AppearanceID", vStringFormat("{0}##Settings", Get("settingsAppearance")));

    Set("InputControlsID", vStringFormat("{0}##Settings", Get("settingsControls")));
    Set("ViewportID", vStringFormat("{0}##Settings", Get("settingsViewport")));
    Set("DegreesFormat", vStringFormat("%.0f {0}", Get("settingsViewportDegrees")));
    Set("ElevationFormat", vStringFormat("{0}##Settings", Get("settingsMaps")));
    Set("VisualizationFormat", vStringFormat("{0}##Settings", Get("settingsVis")));
    Set("RestoreColoursID", vStringFormat("{0}##RestoreClassificationColors", Get("settingsVisClassRestoreDefaults")));

    Set("AppearanceRestore", vStringFormat("{0}##AppearanceRestore", Get("settingsAppearanceRestoreDefaults")));
    Set("InputRestore", vStringFormat("{0}##InputRestore", Get("settingsControlsRestoreDefaults")));
    Set("ViewportRestore", vStringFormat("{0}##ViewportRestore", Get("settingsViewportRestoreDefaults")));
    Set("MapsRestore", vStringFormat("{0}##MapsRestore", Get("settingsMapsRestoreDefaults")));
    Set("VisualizationRestore", vStringFormat("{0}##VisualizationRestore", Get("settingsVisRestoreDefaults")));

    return vE_Success;
  }

  void FreeTable()
  {
    for (std::pair<std::string, const char*> kvp : gStringTable)
      udFree(kvp.second);

    gStringTable.clear();
  }
}
