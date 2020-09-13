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
  char m_description[1024];

public:
  vcViewpoint(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcViewpoint() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  void SetCameraPosition(vcState *pProgramState) override;
  udDouble4x4 GetWorldSpaceMatrix() override;
  void ApplySettings(vcState *pProgramState);
  static void SaveSettings(vcState *pProgramState, udProjectNode *pNode);
};

#endif //vcViewpoint_h__
