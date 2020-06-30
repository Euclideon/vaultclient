#ifndef vcLiveFeed_h__
#define vcLiveFeed_h__

#include "vcSceneItem.h"

#include "vdkRenderContext.h"
#include "vdkError.h"

#include "udUUID.h"
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
  double m_lastFeedSync; // When the previous feed sync occurred
  bool m_fetchNow; // Should we fetch immediately

  double m_newestFeedUpdate; // The time of the item that was most recently updated
  double m_oldestFeedUpdate; // The time of the item that was first updated

  udChunkedArray<vcLiveFeedItem*> m_feedItems;
  udChunkedArray<vcLiveFeedPolyCache> m_polygonModels;

  uint64_t m_selectedItem;
  size_t m_visibleItems;

  bool m_tweenPositionAndOrientation; // Should this feed make up data to smooth out updates
  double m_updateFrequency; // Delay in seconds between updates
  double m_decayFrequency; // Remove items if they haven't updated more recently than this
  double m_maxDisplayDistance; // Distance to stop displaying entirely
  double m_labelLODModifier; // Distance modifier for label LOD
  bool m_snapToMap; // snaps items to the map

  udUUID m_groupID; // Required for updating group mode

  udMutex *m_pMutex;

  vcLiveFeed(vcProject *pProject, vdkProjectNode *pProjectNode, vcState *pProgramState);
  ~vcLiveFeed() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();

  void SelectSubitem(uint64_t internalId);
  bool IsSubitemSelected(uint64_t internalId);
};

#endif //vcLiveFeed_h__
