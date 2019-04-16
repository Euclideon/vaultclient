#ifndef vcStrings_h__
#define vcStrings_h__

#include "vdkError.h"

struct vcTranslationInfo
{
  const char *pLocalName;
  const char *pEnglishName;
  const char *pTranslatorName;
  const char *pTranslatorContactEmail;
};

namespace vcString
{
  const char* Get(const char *pKey);

  vdkError LoadTable(const char *pFilename, vcTranslationInfo *pInfo);
  void FreeTable(vcTranslationInfo *pInfo);
}

#endif //vcStrings_h__
