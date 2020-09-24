#ifndef vcCrossSectionNode_h__
#define vcCrossSectionNode_h__

#include "vcSceneItem.h"

#include "udGeometry.h"

struct vcState;
struct udQueryFilter;

class vcCrossSectionNode : public vcSceneItem
{
private:
  bool m_inverted;

  udDoubleQuat m_currentProjectionHeadingPitch;

  udDouble3 m_position;
  udDouble2 m_headingPitch;
  double m_distance;

  udGeometry m_geometry[3]; // CSG, Left Plane, Right Plane

public:
  vcCrossSectionNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcCrossSectionNode();

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  udDouble4x4 GetWorldSpaceMatrix() override;
  vcGizmoAllowedControls GetAllowedControls() override;

  void UpdateFilters();

  void EndQuery(vcState *pProgramState, const udDouble3 &position, bool isPreview = false);

  udQueryFilter *m_pFilter;
};

#endif //vcCrossSectionNode_h__
