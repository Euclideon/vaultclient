#ifndef vcModel_h__
#define vcModel_h__

#include "udPlatform/udValue.h"
#include "vdkModel.h"

#include "vcGIS.h"

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

bool vcModel_AddToList(vcState *pProgramState, const char *pFilePath);
bool vcModel_UnloadList(vcState *pProgramState);
void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel);

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);

udDouble3 vcModel_GetMidPointLocalSpace(vcState *pProgramState, vcModel *pModel);
vcSRID vcModel_GetSRID(vcState *pProgramState, vcModel *pModel);

#endif //vcModel_h__
