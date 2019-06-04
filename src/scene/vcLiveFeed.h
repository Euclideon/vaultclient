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

enum vcLiveFeedMode
{
  vcLFM_Group, // Updates all feeds in a group
  vcLFM_Position, // Updates all feeds around a fixed point
  vcLFM_Camera, // Updates all feeds around the camera (expensive!)

  vcLFM_Count
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

  vcLiveFeedMode m_updateMode;

  udDouble3 m_storedCameraPosition;  // camera position at time of last update
  udDouble3 m_position; // required for position mode
  vUUID m_groupID; // Required for updating group mode

  udMutex *m_pMutex;

  vcLiveFeed(vdkProjectNode *pProjectNode, vcState *pProgramState);
  ~vcLiveFeed() {};

  void OnNodeUpdate();

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble3 GetLocalSpacePivot();
};

#endif //vcLiveFeed_h__
