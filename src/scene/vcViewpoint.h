#ifndef vcViewpoint_h__
#define vcViewpoint_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"

struct udPointCloud;
struct vcState;

class vcViewpoint : public vcSceneItem
{
private:
  udDouble3 m_CameraPosition;
  udDouble2 m_CameraHeadingPitch;

public:
  vcViewpoint(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcViewpoint() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleSceneEmbeddedUI(vcState *pProgramState);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  void SetCameraPosition(vcState *pProgramState);
  udDouble4x4 GetWorldSpaceMatrix();
  void ApplySettings(vcState *pProgramState);
  static void SaveSettings(vcState *pProgramState, udProjectNode *pNode);
};

#endif //vcViewpoint_h__
