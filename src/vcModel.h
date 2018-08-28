#ifndef vcModel_h__
#define vcModel_h__

#include "udPlatform/udValue.h"
#include "vdkModel.h"

struct vcModel
{
  char modelPath[1024];
  bool modelLoaded;
  bool modelVisible;
  bool modelSelected;
  bool flipYZ;
  vdkModel *pVaultModel;
  udValue *pMetadata;
};

#include "vcState.h"

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath);
bool vcModel_UnloadList(vcState *pProgramState);
bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);

#endif //vcModel_h__
