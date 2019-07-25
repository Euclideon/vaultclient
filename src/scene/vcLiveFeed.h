#ifndef vcLiveFeed_h__
#define vcLiveFeed_h__

#include "vcSceneItem.h"

#include "vCore/vUUID.h"

#include "vdkRenderContext.h"
#include "vdkError.h"

#include "udThread.h"
#include "udChunkedArray.h"

struct vcState;
struct vcLiveFeedItem;
struct vcPolygonModel;

struct vcLiveFeedPolyCache
{
  const char *pModelURL;
  vcPolygonModel *pModel;

  enum LoadStatus
  {
    LS_InQueue,
    LS_Downloading,
    LS_Downloaded,
    LS_Loaded,

    LS_Failed
  } volatile loadStatus;

  void *pModelData;
  int64_t modelDataLength;
};

class vcLiveFeed : public vcSceneItem
{
public:
  double m_lastUpdateTime;

  udChunkedArray<vcLiveFeedItem*> m_feedItems;
  udChunkedArray<vcLiveFeedPolyCache> m_polygonModels;

  size_t m_visibleItems;

  bool m_tweenPositionAndOrientation; // Should this feed make up data to smooth out updates
  double m_updateFrequency; // Delay in seconds between updates
  double m_decayFrequency; // Remove items if they haven't updated more recently than this
  double m_maxDisplayDistance; // Distance to stop displaying entirely

  vUUID m_groupID; // Required for updating group mode

  udMutex *m_pMutex;

  vcLiveFeed(vdkProject *pProject, vdkProjectNode *pProjectNode, vcState *pProgramState);
  ~vcLiveFeed() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();

  void OnSceneSelect(uint64_t internalId);
  void OnSceneUnselect(uint64_t internalId);
  bool IsSceneSelected(uint64_t internalId);
};

#endif //vcLiveFeed_h__
