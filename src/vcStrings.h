#ifndef vcStrings_h__
#define vcStrings_h__

#include "udResult.h"
struct vcState;

struct vcTranslationInfo
{
  const char *pLocalName;
  const char *pEnglishName;
  const char *pTranslatorName;
  const char *pTranslatorContactEmail;
  const char *pTargetVersion;
};

namespace vcString
{
  const char* Get(const char *pKey);

  udResult LoadTableFromMemory(const char *pFilename, vcTranslationInfo *pInfo);
  udResult LoadTableFromFile(const char *pFilename, vcTranslationInfo *pInfo);
  void FreeTable(vcTranslationInfo *pInfo);

  void ShowMissingStringUI();
  void ShowTranslationHelperUI(vcState *pProgramState);

  bool GetTableLoaded();
}

#endif //vcStrings_h__
