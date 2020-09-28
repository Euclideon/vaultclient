#ifndef vcFlythrough_h__
#define vcFlythrough_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"
#include "vcSettings.h"

struct udPointCloud;
struct vcState;
struct vcLineInstance;

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
  char m_exportPath[vcMaxPathLength];
  int m_selectedResolutionIndex;
  int m_selectedExportFPSIndex;
  int m_selectedExportFormatIndex;
  vcLineInstance *m_pLine;
  int m_selectedFlightPoint;

  udDouble3 m_centerPoint;

  struct
  {
    int currentFrame;
    double frameDelta;
  } m_exportInfo;

  vcState *m_pProgramState;

  void UpdateCameraPosition(vcState *pProgramState);
  void UpdateLinePoints();
  void UpdateCenterPoint();
  void LoadFlightPoints(vcState *pProgramState, const udGeoZone &zone);
  void SaveFlightPoints(vcState *pProgramState);
  void SmoothFlightPoints();
  void LerpFlightPoints(double timePosition, const vcFlightPoint &flightPoint1, const vcFlightPoint &flightPoint2, udDouble3 *pLerpedPosition, udDouble2 *pLerpedHeadingPitch);
  const char* GenerateFrameExportPath(int frameIndex);

public:
  vcFlythrough(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcFlythrough() {};

  void CancelExport(vcState *pProgramState);

  void OnNodeUpdate(vcState *pProgramState) override;
  void SelectSubitem(uint64_t internalId) override;
  bool IsSubitemSelected(uint64_t internalId) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  udDouble4x4 GetWorldSpaceMatrix() override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  void SetCameraPosition(vcState *pProgramState) override;
};

#endif //vcFlythrough_h__
