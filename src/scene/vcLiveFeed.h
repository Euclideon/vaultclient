#ifndef vcLiveFeed_h__
#define vcLiveFeed_h__

#include "vcScene.h"

#include "vdkRenderContext.h"
#include "vdkError.h"

#include <vector>
#include "udPlatform/udThread.h"

struct vcState;
struct vcLiveFeedItem;

struct vcLiveFeed : public vcSceneItem
{
  double lastUpdateTime;

  std::vector<vcLiveFeedItem*> feedItems;
  size_t visibleItems;

  double updateFrequency; // Delay in seconds between updates
  double decayFrequency; // Remove items if they haven't updated more recently than this

  double detailDistance; // Distance to use 'simple' system
  double falloffDistance; // Distance to stop displaying entirely

  udMutex *pMutex;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

void vcLiveFeed_AddToList(vcState *pProgramState);

#endif //vcLiveFeed_h__
