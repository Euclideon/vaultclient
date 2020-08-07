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
  vcQNFS_CrossSection,

  vcQNFS_Count
};

class vcQueryNode : public vcSceneItem
{
//private:
public:
  vcQueryNodeFilterShape m_shape;

  bool m_inverted;

  udDoubleQuat m_currentProjection;
  udDoubleQuat m_currentHPRQuaternion;

  udDouble3 m_center;
  udDouble3 m_extents;
  udDouble2 m_headingPitch;

public:
  vcQueryNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
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

  udQueryFilter *m_pFilter;
};

struct udProjectNode;
struct vcQueryNodeFilterInput
{
  vcQueryNodeFilterShape shape;
  udDouble3 pickPoint;
  udDouble3 size;
  udDouble3 endPoint;
  udProjectNode *pNode;
  uint32_t holdCount;
};

void vcQueryNodeFilter_InitFilter(vcQueryNodeFilterInput *pFilter, vcQueryNodeFilterShape shape);
void vcQueryNodeFilter_Clear(vcQueryNodeFilterInput *pFilter);

void vcQueryNodeFilter_HandleSceneInput(vcState *pProgramState, bool isBtnHeld, bool isBtnReleased);
bool vcQueryNodeFilter_IsDragActive(vcState *pProgramState);

#endif //vcViewpoint_h__
