#ifndef vcPOI_h__
#define vcPOI_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"
#include "udError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "vcLineRenderer.h"
#include "vcGLState.h"
#include "vcSettings.h"
#include "vcPolygonModel.h"

class vcPOIState_General;
struct vcRenderPolyInstance;
struct udWorkerPool;
struct udPointCloud;
struct vcTexture;
struct vcState;
struct vcFenceRenderer;
struct vcPolygonModel;
struct vcUnitConversionData;

struct vcLineInfo
{
  udDouble3 *pPoints;
  int numPoints;
  bool isDualColour;
  uint32_t colourPrimary;
  uint32_t colourSecondary;
  float lineWidth;
  int selectedPoint;
  bool closed;
  vcFenceRendererImageMode lineStyle;
  vcFenceRendererVisualMode fenceMode;
};

class vcPOI : public vcSceneItem
{
  friend class vcPOIState_General;
  friend class vcPOIState_Annotate;
  friend class vcPOIState_MeasureLine;
  friend class vcPOIState_MeasureArea;
private:
  vcLineInfo m_line; // TODO: 1452
  uint32_t m_measurementAreaFillColour;
  uint32_t m_nameColour;
  uint32_t m_backColour;
  char m_hyperlink[vcMaxPathLength];
  char m_description[vcMaxPathLength];

  bool m_showArea;
  bool m_showLength;
  bool m_showAllLengths;
  bool m_showAngles;
  bool m_showFill;

  double m_totalLength;
  double m_area;
  double m_meshArea;
  udDouble3 m_centroid;

  udDouble3 m_gizmoPivot;

  udChunkedArray<vcLabelInfo> m_labels;

  vcPolygonModel *m_pPolyModel;
  vcFenceRenderer *m_pFence;
  vcLabelInfo *m_pLabelInfo;
  const char *m_pLabelText;

  vcLineInstance *m_pLine; // TODO: 1452

  bool m_cameraFollowingAttachment; //True if following attachment, false if flying through points

  udWorkerPool *m_pWorkerPool;

  struct
  {
    vcPolygonModel *pModel;
    const char *pPathLoaded;

    double moveSpeed; // Speed in m/s
    vcGLStateCullMode cullMode;

    int segmentIndex;
    double segmentProgress;

    udDouble3 currentPos;
    udDouble3 eulerAngles;
  } m_attachment;

  struct
  {
    int segmentIndex;
    double segmentProgress;
  } m_flyThrough;

  vcPOIState_General *m_pState;

  void HandleBasicUI(vcState *pProgramState, size_t itemID);

  vcState *m_pProgramState;

public:
  vcPOI(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcPOI();

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  void HandleContextMenu(vcState *pProgramState) override;
  void HandleAttachmentUI(vcState *pProgramState) override;
  void HandleToolUI(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  void AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview = false);
  void RemovePoint(vcState *pProgramState, int index);
  void UpdatePoints(vcState *pProgramState);

  void SetCameraPosition(vcState *pProgramState) override;
  udDouble4x4 GetWorldSpaceMatrix() override;

  void SelectSubitem(uint64_t internalId) override;
  bool IsSubitemSelected(uint64_t internalId) override;

private:
  void RebuildSceneLabel(const vcUnitConversionData *pConversionData);
  void InsertPoint(const udDouble3 &position);
  void CalculateArea(const udDouble4 &projectionPlane);
  void CalculateTotalLength();
  void CalculateCentroid();
  void AddLengths(const vcUnitConversionData *pConversionData);
  void AddAngles(const vcUnitConversionData *pConversionData);
  void UpdateState(vcState *pProgramState);
  double DistanceToPoint(udDouble3 const &point);
  vcRenderPolyInstance *AddNodeToRenderData(vcState *pProgramState, vcRenderData *pRenderData, size_t i);
  bool IsVisible(vcState *pProgramState);
  void AddFenceToScene(vcRenderData *pRenderData);
  void AddLabelsToScene(vcRenderData *pRenderData, const vcUnitConversionData *pConversionData);
  void AddFillPolygonToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void AddAttachedModelsToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void DoFlythrough(vcState *pProgramState);
  bool LoadAttachedModel(const char *pNewPath);
  bool GetPointAtDistanceAlongLine(double distance, udDouble3 *pPoint, int *pSegmentIndex, double *pSegmentProgress);
  void GenerateLineFillPolygon(vcState *pProgramState);
};

#endif //vcPOI_h__
