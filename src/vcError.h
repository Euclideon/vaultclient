#ifndef vcError_h__
#define vcError_h__

#include "udResult.h"

enum vcErrorSource
{
  vcES_File,
  vcES_ProjectChange,
  vcES_WorldPosition,
};

struct ErrorItem
{
  vcErrorSource source;
  const char *pData;
  udResult resultCode;
};

struct vcState;

void vcError_AddError(vcState *pProgramState, const ErrorItem &error);

#endif //vcError_h__
