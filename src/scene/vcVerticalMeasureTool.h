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

  void Preview(const udDouble3 &position);
  void EndMeasure(vcState *pProgramState, const udDouble3 &position);

  virtual void OnNodeUpdate(vcState *pProgramState);
  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  virtual void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  virtual void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  virtual void HandleSceneEmbeddedUI(vcState *pProgramState);
  virtual void Cleanup(vcState *pProgramState);
  virtual void ChangeProjection(const udGeoZone &newZone);
  virtual udDouble3 GetLocalSpacePivot();
  virtual bool IsValid() const { return !m_markDelete; }
  virtual void HandleToolUI(vcState *pProgramState);

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
};

#endif

