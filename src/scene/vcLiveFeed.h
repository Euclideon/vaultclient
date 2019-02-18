#ifndef vcLiveFeed_h__
#define vcLiveFeed_h__

#include "vcScene.h"

#include "vdkRenderContext.h"
#include "vdkError.h"

#include <vector>
#include "udPlatform/udThread.h"

struct vcState;
struct vcLiveFeedItem;

class vcLiveFeed : public vcSceneItem
{
public:
  double m_lastUpdateTime;

  std::vector<vcLiveFeedItem*> m_feedItems;
  size_t m_visibleItems;

  double m_updateFrequency; // Delay in seconds between updates
  double m_decayFrequency; // Remove items if they haven't updated more recently than this

  double m_detailDistance; // Distance to use 'simple' system
  double m_falloffDistance; // Distance to stop displaying entirely

  udMutex *m_pMutex;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

void vcLiveFeed_AddToList(vcState *pProgramState);

#endif //vcLiveFeed_h__
