#ifndef vcQueryNode_h__
#define vcQueryNode_h__

#include "vcSceneItem.h"

struct vcState;
struct udQueryFilter;

enum vcQueryNodeFilterShape
{
  vcQNFS_None = -1,
  vcQNFS_Box,
  vcQNFS_Cylinder,
  vcQNFS_Sphere,

  vcQNFS_Count
};

class vcQueryNode : public vcSceneItem
{
private:
  vcQueryNodeFilterShape m_shape;

  bool m_inverted;

  udDoubleQuat m_currentProjection;

  udDouble3 m_center;
  udDouble3 m_extents;
  udDouble3 m_ypr;

public:
  vcQueryNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcQueryNode();

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

  void EndQuery(vcState *pProgramState, const udDouble3 &position, bool isPreview = false);

  udQueryFilter *m_pFilter;
};

#endif //vcQueryNode_h__
