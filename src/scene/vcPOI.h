#ifndef vcPOI_h__
#define vcPOI_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "vcLineRenderer.h"
#include "gl/vcGLState.h"

class  vcPOIState;
struct vcRenderPolyInstance;
struct udWorkerPool;
struct vdkPointCloud;
struct vcTexture;
struct vcState;
struct vcFenceRenderer;
struct vcPolygonModel;

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
  friend class vcPOIState;
  friend class vcPOIState_General;
  friend class vcPOIState_Append;
private:
  vcLineInfo m_line; // TODO: 1452
  uint32_t m_nameColour;
  uint32_t m_backColour;
  vcLabelFontSize m_namePt;

  bool m_showArea;
  double m_calculatedArea;

  bool m_showLength;
  double m_calculatedLength;

  bool m_showAllLengths;
  udChunkedArray<vcLabelInfo> m_lengthLabels;

  vcFenceRenderer *m_pFence;
  vcLabelInfo *m_pLabelInfo;
  const char *m_pLabelText;

  vcLineInstance *m_pLine; // TODO: 1452

  bool m_hasPreviewPoint;

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

  vcPOIState *m_pState;

  void HandleBasicUI(vcState *pProgramState, size_t itemID);

public:
  vcPOI(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcPOI();

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void HandleContextMenu(vcState *pProgramState);
  void HandleAttachmentUI(vcState *pProgramState);
  void HandleToolUI(vcState *pProgramState);

  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  void AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview = false);
  void RemovePoint(vcState *pProgramState, int index);
  void UpdatePoints();

  void SetCameraPosition(vcState *pProgramState);
  udDouble4x4 GetWorldSpaceMatrix();

  void SelectSubitem(uint64_t internalId);
  bool IsSubitemSelected(uint64_t internalId);

private:
  void UpdateState(vcState *pProgramState);
  vcRenderPolyInstance *AddNodeToRenderData(vcState *pProgramState, vcRenderData *pRenderData, size_t i);
  bool IsVisible(vcState *pProgramState);
  void AddFenceToScene(vcRenderData *pRenderData);
  void AddLabelsToScene(vcRenderData *pRenderData);
  void AddAttachedModelsToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void DoFlythrough(vcState *pProgramState);
  bool LoadAttachedModel(const char *pNewPath);
  bool GetPointAtDistanceAlongLine(double distance, udDouble3 *pPoint, int *pSegmentIndex, double *pSegmentProgress);
};

#endif //vcPOI_h__
