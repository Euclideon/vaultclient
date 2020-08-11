#ifndef vcFlythrough_h__
#define vcFlythrough_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"

struct udPointCloud;
struct vcState;

struct vcFlightPoint
{
  double time;
  udDouble3 m_CameraPosition;
  udDouble2 m_CameraHeadingPitch;
};

enum vcFlythroughState
{
  vcFTS_None,
  vcFTS_Playing,
  vcFTS_Recording,
  vcFTS_Exporting
};

class vcFlythrough : public vcSceneItem
{
private:
  udChunkedArray<vcFlightPoint> m_flightPoints;
  vcFlythroughState m_state;
  double m_timePosition;
  double m_timeLength;

  struct
  {
    int currentFrame;
    double frameDelta;
  } m_exportInfo;

  void UpdateCameraPosition(vcState *pProgramState);

public:
  vcFlythrough(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcFlythrough() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleSceneEmbeddedUI(vcState *pProgramState);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  void SetCameraPosition(vcState *pProgramState);
};

#endif //vcFlythrough_h__
