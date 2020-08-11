#include "vcStrings.h"

#include "vcStringFormat.h"
#include "vcModals.h"
#include "vcState.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

#include "imgui.h"

#include <string>
#include <unordered_map>

namespace vcString
{
  static std::unordered_map<std::string, const char*> *g_pStringTable = nullptr;
  static udChunkedArray<const char *> g_missingStringTable = {};

  static udJSON g_enAUCurrent = {};
  static udJSON g_enAUPrevious = {};
  static udJSON g_targetVersion = {};

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
    const char *pValue = (*g_pStringTable)[pKey];

    if (pValue == nullptr)
    {
      g_missingStringTable.PushBack(udStrdup(pKey));
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
    g_missingStringTable.Init(32);

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

    if (g_pStringTable != nullptr)
    {
      for (std::pair<std::string, const char*> kvp : (*g_pStringTable))
        udFree(kvp.second);

      g_pStringTable->clear();
      delete g_pStringTable;
      g_pStringTable = nullptr;
    }

    for (size_t i = 0; i < g_missingStringTable.length; ++i)
      udFree(g_missingStringTable[i]);

    g_missingStringTable.Deinit();

    g_enAUCurrent.SetVoid();
    g_enAUPrevious.SetVoid();
    g_targetVersion.SetVoid();
  }

  void ShowMissingStringUI()
  {
    if (g_missingStringTable.length > 0)
    {
      vcIGSW_ShowLoadStatusIndicator(vcSLS_Failed);
      ImGui::TextWrapped("%s", vcString::Get("translationStringsMissing"));
      ImGui::Separator();

      for (size_t i = 0; i < g_missingStringTable.length; ++i)
        ImGui::TextUnformatted(g_missingStringTable[i]);

      ImGui::Separator();
    }
    else
    {
      vcIGSW_ShowLoadStatusIndicator(vcSLS_Success);
      ImGui::TextUnformatted(vcString::Get("translationStringsNoneMissing"));
    }
  }

  void ShowTranslationHelperUI(vcState *pProgramState)
  {
    static char loadedLanguage[8] = "zhCN";
    static bool hideUnchanged = true;
    static bool showStatusColumn = true;

    static bool modified = false;
    static const char *pModifyKey = nullptr;
    static char newValue[256];

    const char *pData = nullptr;

    ImGui::Checkbox(vcString::Get("translationHideUnchanged"), &hideUnchanged);
    ImGui::SameLine();
    ImGui::Checkbox(vcString::Get("translationShowStatus"), &showStatusColumn);
    ImGui::SameLine();

    ImGui::Button("...");
    if (ImGui::BeginPopupContextItem("langList", 0))
    {
      for (auto langItem : pProgramState->settings.languageOptions)
      {
        if (ImGui::MenuItem(langItem.languageName))
        {
          modified = false;
          pModifyKey = nullptr;
          memset(newValue, 0, sizeof(newValue));

          g_targetVersion.SetVoid();

          udStrcpy(loadedLanguage, langItem.filename);
        }
      }

      ImGui::EndPopup();
    }
    ImGui::SameLine();

    if (ImGui::InputText(vcString::Get("translationLoaded"), loadedLanguage, udLengthOf(loadedLanguage)))
    {
      modified = false;
      pModifyKey = nullptr;
      memset(newValue, 0, sizeof(newValue));

      g_targetVersion.SetVoid();
    }

    if (modified)
    {
      if (ImGui::Button(vcString::Get("translationSave")) && vcModals_AllowDestructiveAction(pProgramState, vcString::Get("translationOverride"), vcString::Get("translationOverride")))
      {
        g_targetVersion.Export(&pData, udJEO_FormatWhiteSpace);
        udFile_Save(udTempStr("asset://assets/lang/%s.json", loadedLanguage), pData, udStrlen(pData));
        udFree(pData);
        modified = false;
      }
    }

    ImGui::Separator();

    if (ImGui::BeginChild("translationTables"))
    {

      if (g_enAUCurrent.IsVoid())
      {
        if (udFile_Load("asset://assets/lang/enAU.json", &pData) == udR_Success)
        {
          g_enAUCurrent.Parse(pData);
          udFree(pData);
        }
      }

      if (g_enAUPrevious.IsVoid())
      {
        if (udFile_Load("asset://assets/lang/enAU_previous.json", &pData) == udR_Success)
        {
          g_enAUPrevious.Parse(pData);
          udFree(pData);
        }
      }

      if (g_targetVersion.IsVoid())
      {
        if (udFile_Load(udTempStr("asset://assets/lang/%s.json", loadedLanguage), &pData) == udR_Success)
        {
          g_targetVersion.Parse(pData);
          g_targetVersion.Set("_LanguageInfo.targetVersion = '%s'", g_enAUCurrent.Get("_LanguageInfo.targetVersion").AsString());
          udFree(pData);
        }
      }

      ImGui::Columns(3);

      udJSONObject *pCurrent = g_enAUCurrent.AsObject();

      if (pCurrent == nullptr)
        return;

      for (size_t i = 0; i < pCurrent->length; ++i)
      {
        const char *pKey = pCurrent->GetElement(i)->pKey;

        // Skip 'internal' data
        if (pKey[0] == '_')
          continue;

        const char *pCurrentVal = pCurrent->GetElement(i)->value.AsString();
        const char *pPreviousVal = g_enAUPrevious.Get("%s", pKey).AsString();
        const char *pTranslatedVal = g_targetVersion.Get("%s", pKey).AsString();

        if (hideUnchanged && udStrEqual(pCurrentVal, pPreviousVal) && pTranslatedVal != nullptr)
          continue;

        ImGui::TextUnformatted(pKey);

        if (showStatusColumn)
        {
          ImGui::SameLine();

          if (pPreviousVal == nullptr)
            ImGui::TextColored(ImVec4(1.0, 0.5, 0.5, 1.0), "[%s]", vcString::Get("translationStatusNew"));
          else if (udStrEqual(pCurrentVal, pPreviousVal))
            ImGui::TextColored(ImVec4(0.5, 1.0, 0.5, 1.0), "[%s]", vcString::Get("translationStatusSame"));
          else
            ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "[%s]", vcString::Get("translationStatusChanged"));
        }

        ImGui::NextColumn();

        ImGui::TextWrapped("%s", pCurrentVal);
        ImGui::NextColumn();

        if (pModifyKey == nullptr || !udStrEqual(pModifyKey, pKey))
        {
          if (pTranslatedVal != nullptr)
            ImGui::TextWrapped("%s", pTranslatedVal);
          else
            ImGui::TextColored(ImVec4(1.0, 0.5, 0.5, 1.0), "%s", vcString::Get("translationStatusNew"));

          if (ImGui::IsItemClicked())
          {
            pModifyKey = pKey;
            newValue[0] = '\0'; //Zero in case
            memcpy(newValue, pTranslatedVal, udStrlen(pTranslatedVal));
          }
        }
        else
        {
          bool saved = ImGui::InputText("##newTranslation", newValue, sizeof(newValue), ImGuiInputTextFlags_EnterReturnsTrue);
          ImGui::SetKeyboardFocusHere(-1);
          saved |= ImGui::IsItemDeactivatedAfterEdit();

          if (saved)
          {
            udJSON temp;
            temp.SetString(newValue);
            g_targetVersion.Set(&temp, "%s", pKey);
            pModifyKey = nullptr;
            modified = true;
          }

          if (ImGui::IsItemDeactivated())
            pModifyKey = nullptr;
        }

        ImGui::NextColumn();
      }

      ImGui::Columns(1);

      udJSONObject *pTargetObj = g_targetVersion.AsObject();

      if (pTargetObj != nullptr)
      {
        ImGui::Separator();

        bool removeUnused = ImGui::Button(vcString::Get("translationRemove"));
        ImGui::SameLine();
        ImGui::TextWrapped("%s", vcString::Get("translationInstructionRemoved"));

        ImGui::Columns(2);


        for (size_t i = 0; i < pTargetObj->length; ++i)
        {
          const char *pKey = pTargetObj->GetElement(i)->pKey;

          // Skip 'internal' data
          if (pKey[0] == '_')
            continue;

          if (g_enAUCurrent.Get("%s", pKey).IsString())
            continue;

          if (removeUnused)
          {
            pTargetObj->GetElement(i)->value.Clear();
            udFree(pKey);
            pTargetObj->RemoveAt(i);
            --i;

            modified = true;
          }
          else
          {
            ImGui::TextUnformatted(pKey);
            ImGui::NextColumn();
            ImGui::TextUnformatted(pTargetObj->GetElement(i)->value.AsString());
            ImGui::NextColumn();
          }
        }

        ImGui::Columns(1);
      }
    }

    ImGui::EndChild();
  }
}
