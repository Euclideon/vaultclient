#ifndef vcVerticalMeasureTool_h__
#define vcVerticalMeasureTool_h__

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

class vcVerticalMeasureTool : public vcSceneItem
{
public:
  vcVerticalMeasureTool(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  virtual ~vcVerticalMeasureTool();

  void Preview(vcState *pProgramState, const udDouble3 &position);
  void EndMeasure(vcState *pProgramState, const udDouble3 &position);

  void OnNodeUpdate(vcState *pProgramState) override;
  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;
  udDouble3 GetLocalSpacePivot() override;
  bool IsValid() const  override { return !m_markDelete; }
  void HandleToolUI(vcState *pProgramState) override;

private:
  void UpdateIntersectionPosition(vcState *pProgramState);
  bool HasLine();
  void ClearPoints();
  void UpdateSetting(vcState *pProgramState);

private:
  bool m_done;
  int32_t m_selectedPoint;
  bool m_markDelete;
  bool m_showAllDistances;
  udDouble3 m_points[3];

  vcLabelInfo m_labelList[2];
  vcLineInstance *m_pLineInstance;

  uint32_t m_textColourBGRA;
  uint32_t m_textBackgroundBGRA;
  uint32_t m_lineColour;
  float m_lineWidth;

  double m_distStraight;
  double m_distHoriz;
  double m_distVert;

  char m_description[vcMaxPathLength];
  bool m_flipOrder;
};

#endif

