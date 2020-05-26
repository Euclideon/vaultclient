#ifndef vcVerticalMeasureTool_h__
#define vcVerticalMeasureTool_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "vcLineRenderer.h"
#include "vcGLState.h"
#include "vcSettings.h"

class vcVerticalMeasureTool : public vcSceneItem
{
public:
  vcVerticalMeasureTool(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  virtual ~vcVerticalMeasureTool();

  void Preview(vcState *pProgramState, const udDouble3 &position);
  void EndMeasure(vcState *pProgramState, const udDouble3 &position);

  virtual void OnNodeUpdate(vcState *pProgramState);
  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  virtual void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  virtual void HandleImGui(vcState *pProgramState, size_t *pItemID);
  virtual void Cleanup(vcState *pProgramState);
  virtual void ChangeProjection(const udGeoZone &newZone);
  virtual udDouble3 GetLocalSpacePivot();

private:
  void UpdateIntersectionPosition(vcState *pProgramState);
  bool HasLine();
  void RemoveMeasureInfo();

private:
  bool m_done;
  bool m_pickStart;
  bool m_pickEnd;
  udDouble3 m_points[3];

  vcLabelInfo m_labelInfo;
  vcLineInstance *m_pLineInstance;

};

#endif

