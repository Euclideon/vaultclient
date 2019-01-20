#ifndef vcStrings_h__
#define vcStrings_h__

#include "vdkError.h"
#include "vcState.h"

namespace vcString
{
  const char* Get(const char *pKey);

  vdkError LoadTable(vcState *pProgramState, const char *pFilename = nullptr);
  void FreeTable();
}

#endif //vcStrings_h__
