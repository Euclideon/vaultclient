#ifndef vcModel_h__
#define vcModel_h__

#include "vcScene.h"
#include "vdkRenderContext.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

struct vcModel : public vcSceneItem
{
  vdkPointCloud *pPointCloud;

  udDouble3 pivot;
  udDouble4x4 defaultMatrix; // This is the matrix that was originally loaded
  udDouble4x4 sceneMatrix; // This is the matrix used to render into the current projection

  double meterScale;

  bool hasWatermark; // True if the model has a watermark (might not be loaded)
  vcTexture *pWatermark; // If the watermark is loaded, it will be here

  void ChangeProjection(vcState *pProgramState, const udGeoZone &newZone);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);

  udDouble3 GetLocalSpacePivot();
  udDouble4x4 GetWorldSpaceMatrix();
};

void vcModel_AddToList(vcState *pProgramState, const char *pName, const char *pFilePath, bool jumpToModelOnLoad = true, udDouble3 *pOverridePosition = nullptr, udDouble3 *pOverrideYPR = nullptr, double scale = 1.0);

#endif //vcModel_h__
