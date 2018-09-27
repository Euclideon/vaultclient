#ifndef vcModel_h__
#define vcModel_h__

#include "vcGIS.h"

struct vdkModel;
class udJSON;
struct vcTexture;

struct vcModel
{
  char modelPath[1024];
  bool modelLoaded;
  bool modelVisible;
  bool modelSelected;
  bool flipYZ;
  udDouble4x4 worldMatrix;
  vdkModel *pVaultModel;
  udJSON *pMetadata;
  vcTexture *pWatermark;
};

#include "vcState.h"

bool vcModel_AddToList(vcState *pProgramState, const char *pFilePath);
bool vcModel_RemoveFromList(vcState *pProgramState, size_t index);
bool vcModel_UnloadList(vcState *pProgramState);
void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel);

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);

udDouble3 vcModel_GetMidPointLocalSpace(vcState *pProgramState, vcModel *pModel);
vcSRID vcModel_GetSRID(vcState *pProgramState, vcModel *pModel);

#endif //vcModel_h__
