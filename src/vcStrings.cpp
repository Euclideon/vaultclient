#include "vcStrings.h"
#include "vCore/vStringFormat.h"
#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

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
      gStringTable[pKey] = vStringFormat("{{0}}", pKey);
      return gStringTable[pKey];
    }
    else
    {
      return pValue;
    }
  }

  udResult LoadTable(const char *pFilename, vcTranslationInfo *pInfo)
  {
    udResult result = udR_Failure_;

    udJSON strings;
    char *pPos = nullptr;
    size_t count = 0;

    FreeTable(pInfo); // Empty the table

    UD_ERROR_NULL(pFilename, udR_InvalidParameter_);
    UD_ERROR_CHECK(udFile_Load(pFilename, (void **)&pPos));
    UD_ERROR_CHECK(strings.Parse(pPos));

    count = strings.MemberCount();

    for (size_t i = 0; i < count; ++i)
    {
      const char *pKey = strings.GetMemberName(i);
      const char *pValue = strings.GetMember(i)->AsString();

      if (pValue != nullptr)
        Set(pKey, udStrdup(pValue));
    }

    pInfo->pLocalName = udStrdup(strings.Get("_LanguageInfo.localname").AsString("?"));
    pInfo->pEnglishName = udStrdup(strings.Get("_LanguageInfo.englishname").AsString("?"));
    pInfo->pTranslatorName = udStrdup(strings.Get("_LanguageInfo.translationby").AsString("?"));
    pInfo->pTranslatorContactEmail = udStrdup(strings.Get("_LanguageInfo.contactemail").AsString("?"));
    pInfo->pTargetVersion = udStrdup(strings.Get("_LanguageInfo.targetVersion").AsString());

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

  epilogue:
    udFree(pPos);

    return result;
  }

  void FreeTable(vcTranslationInfo *pInfo)
  {
    if (pInfo != nullptr)
    {
      udFree(pInfo->pLocalName);
      udFree(pInfo->pEnglishName);
      udFree(pInfo->pTranslatorName);
      udFree(pInfo->pTranslatorContactEmail);
      udFree(pInfo->pTargetVersion);
    }

    for (std::pair<std::string, const char*> kvp : gStringTable)
      udFree(kvp.second);

    gStringTable.clear();
  }
}
