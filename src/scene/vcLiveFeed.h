#ifndef vcLiveFeed_h__
#define vcLiveFeed_h__

#include "vcScene.h"

#include "vdkRenderContext.h"
#include "vdkError.h"

#include "udPlatform/udThread.h"
#include "udPlatform/udChunkedArray.h"

struct vcState;
struct vcLiveFeedItem;
struct vcPolygonModel;

struct vcLiveFeedPolyCache
{
  const char *pModelURL;
  vcPolygonModel *pModel;
};

class vcLiveFeed : public vcSceneItem
{
public:
  double m_lastUpdateTime;

  udChunkedArray<vcLiveFeedItem*> m_feedItems;
  udChunkedArray<vcLiveFeedPolyCache> m_polygonModels;

  size_t m_visibleItems;

  double m_updateFrequency; // Delay in seconds between updates
  double m_decayFrequency; // Remove items if they haven't updated more recently than this

  double m_falloffDistance; // Distance to stop displaying entirely

  udMutex *m_pMutex;

  vcLiveFeed();
  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
};

#endif //vcLiveFeed_h__
