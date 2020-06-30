#ifndef vcWater_h__
#define vcWater_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;

struct vcWaterRenderer;

class vcWater : public vcSceneItem
{
public:
  vcWater(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcWater() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();

private:
  udDouble3 m_pivot;
  vcWaterRenderer *m_pWaterRenderer;
  double m_altitude;
};

#endif //vcWater_h__
