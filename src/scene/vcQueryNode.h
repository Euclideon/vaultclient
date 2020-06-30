#ifndef vcQueryNode_h__
#define vcQueryNode_h__

#include "vcSceneItem.h"

struct vcState;
struct vdkQueryFilter;

enum vcQueryNodeFilterShape
{
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
  vcQueryNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcQueryNode();

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleSceneEmbeddedUI(vcState *pProgramState);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble4x4 GetWorldSpaceMatrix();
  vcGizmoAllowedControls GetAllowedControls();

  vdkQueryFilter *m_pFilter;
};

#endif //vcViewpoint_h__
