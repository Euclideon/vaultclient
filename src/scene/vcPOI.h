#ifndef vcPOI_h__
#define vcPOI_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "gl/vcGLState.h"

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
private:
  vcLineInfo m_line;
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
    udDouble3 segmentStartPos;
    udDouble3 segmentEndPos;
    udDouble3 eulerAngles;
  } m_attachment;

public:
  vcPOI(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcPOI() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void HandleContextMenu(vcState *pProgramState);
  void HandleAttachmentUI(vcState *pProgramState);

  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  void AddPoint(vcState *pProgramState, const udDouble3 &position);
  void RemovePoint(vcState *pProgramState, int index);
  void UpdatePoints();

  void SetCameraPosition(vcState *pProgramState);
  udDouble4x4 GetWorldSpaceMatrix();

  void SelectSubitem(uint64_t internalId);
  bool IsSubitemSelected(uint64_t internalId);

private:
  bool LoadAttachedModel(const char *pNewPath);
};

#endif //vcPOI_h__
