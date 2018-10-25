#ifndef vcModel_h__
#define vcModel_h__

#include "vcGIS.h"

struct vdkModel;
class udJSON;
struct vcTexture;

enum vcModelLoadStatus
{
  vcMLS_Pending,
  vcMLS_Loading,
  vcMLS_Loaded,
  vcMLS_Failed,
  vcMLS_Unloaded,

  VCMLS_Count
};

struct vcModel
{
  char path[1024];
  volatile int32_t loadStatus;
  bool visible;
  bool selected;

  bool flipYZ;
  udDouble4x4 worldMatrix;
  vdkModel *pVDKModel;
  udJSON *pMetadata;

  bool hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *pWatermark; // If the watermark is loaded, it will be here

  udGeoZone *pZone; // nullptr if not geolocated
};

#include "vcState.h"

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath);
void vcModel_RemoveFromList(vcState *pProgramState, size_t index);
void vcModel_UnloadList(vcState *pProgramState);
void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel);

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);

udDouble3 vcModel_GetMidPointLocalSpace(vcState *pProgramState, vcModel *pModel);

#endif //vcModel_h__
