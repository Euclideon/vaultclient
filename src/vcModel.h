#ifndef vcModel_h__
#define vcModel_h__

#include "vcGIS.h"
#include "vdkRenderContext.h"

struct vdkPointCloud;
class udJSON;
struct vcTexture;

enum vcModelLoadStatus
{
  vcMLS_Pending,
  vcMLS_Loading,
  vcMLS_Loaded,
  vcMLS_Failed,
  vcMLS_OpenFailure,
  vcMLS_Unloaded,

  VCMLS_Count
};

struct vcModel
{
  char path[1024];
  volatile int32_t loadStatus;
  bool visible;
  bool selected;

  udDouble4x4 storedMatrix; // This is the 'local' matrix at load time
  udDouble4x4 *pWorldMatrix; // This points to the matrix in renderInstance to avoid having to copy it
  bool flipYZ;

  vdkRenderInstance renderInstance;
  udJSON *pMetadata;

  double meterScale;
  udDouble3 pivot;
  udDouble3 boundsMin;
  udDouble3 boundsMax;

  bool hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *pWatermark; // If the watermark is loaded, it will be here

  udGeoZone *pZone; // nullptr if not geolocated
};

#include "vcState.h"

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath, bool jumpToModelOnLoad = true, udDouble3 *pOverridePosition = nullptr, udDouble3 *pOverrideYPR = nullptr, double scale = 1.0);
void vcModel_RemoveFromList(vcState *pProgramState, size_t index);
void vcModel_UnloadList(vcState *pProgramState);

void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel);

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel);
udDouble3 vcModel_GetPivotPointWorldSpace(vcModel *pModel);

#endif //vcModel_h__
