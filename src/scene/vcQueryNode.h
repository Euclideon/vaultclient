#ifndef vcQueryNode_h__
#define vcQueryNode_h__

#include "vcSceneItem.h"

struct vcState;
struct vdkQueryFilter;

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

struct vdkProjectNode;
struct vcQueryNodeFilterInput
{
  vcQueryNodeFilterShape shape;
  udDouble3 pickPoint;
  udDouble3 size;
  udDouble3 endPoint;
  vdkProjectNode *pNode;
  uint32_t holdCount;
};

void vcQueryNodeFilter_InitFilter(vcQueryNodeFilterInput *pFilter, vcQueryNodeFilterShape shape);
void vcQueryNodeFilter_Clear(vcQueryNodeFilterInput *pFilter);

void vcQueryNodeFilter_HandleSceneInput(vcState *pProgramState, bool isBtnHeld, bool isBtnReleased);
bool vcQueryNodeFilter_IsDragActive(vcState *pProgramState);

#endif //vcViewpoint_h__
