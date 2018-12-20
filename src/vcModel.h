#ifndef vcModel_h__
#define vcModel_h__

#include "vcScene.h"
#include "vdkRenderContext.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

struct vcModel : public vcSceneItem
{
  char path[1024];

  vdkRenderInstance renderInstance;

  double meterScale;
  udDouble3 boundsMin;
  udDouble3 boundsMax;

  bool hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *pWatermark; // If the watermark is loaded, it will be here
};

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath, bool jumpToModelOnLoad = true, udDouble3 *pOverridePosition = nullptr, udDouble3 *pOverrideYPR = nullptr, double scale = 1.0);

#endif //vcModel_h__
