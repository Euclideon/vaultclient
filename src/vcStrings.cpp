#include "vcStrings.h"
#include "vcStringFormat.h"

#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

#include <string>
#include <unordered_map>

namespace vcString
{
  static std::unordered_map<std::string, const char*> *g_pStringTable;

  // pValue needs to be already duplicated before calling this function
  void Set(const char *pKey, const char *pValue)
  {
    const char *pExistingValue = (*g_pStringTable)[pKey];

    if (pExistingValue != nullptr)
      udFree(pExistingValue);

    (*g_pStringTable)[pKey] = pValue;
  }

  const char* Get(const char *pKey)
  {
    if (g_pStringTable == nullptr)
      g_pStringTable = new std::unordered_map<std::string, const char*>();

    const char *pValue = (*g_pStringTable)[pKey];

    if (pValue == nullptr)
    {
      // Caches the mising string-
      (*g_pStringTable)[pKey] = vcStringFormat("{{0}}", pKey);
      return (*g_pStringTable)[pKey];
    }
    else
    {
      return pValue;
    }
  }

  udResult LoadTableFromMemory(const char *pJSON, vcTranslationInfo *pInfo)
  {
    udResult result = udR_Failure_;
    udJSON strings;
    size_t count = 0;

    FreeTable(pInfo); // Empty the table
    g_pStringTable = new std::unordered_map<std::string, const char*>();

    UD_ERROR_CHECK(strings.Parse(pJSON));

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

  epilogue:
    return result;
  }

  udResult LoadTableFromFile(const char *pFilename, vcTranslationInfo *pInfo)
  {
    udResult result = udR_Failure_;
    char *pPos = nullptr;

    UD_ERROR_NULL(pFilename, udR_InvalidParameter_);
    UD_ERROR_CHECK(udFile_Load(pFilename, (void **)&pPos));

    UD_ERROR_CHECK(LoadTableFromMemory(pPos, pInfo));

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

    if (g_pStringTable == nullptr)
      return;

    for (std::pair<std::string, const char*> kvp : (*g_pStringTable))
      udFree(kvp.second);

    g_pStringTable->clear();
    delete g_pStringTable;
    g_pStringTable = nullptr;
  }
}
