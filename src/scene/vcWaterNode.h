#ifndef vcWater_h__
#define vcWater_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

struct vcWaterRenderer;

class vcWater : public vcSceneItem
{
public:
  vcWater(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcWater() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  udDouble3 GetLocalSpacePivot() override;

private:
  udDouble3 m_pivot;
  vcWaterRenderer *m_pWaterRenderer;
  double m_altitude;
};

#endif //vcWater_h__
