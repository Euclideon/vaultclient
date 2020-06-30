#ifndef vcPolyModelNode_h__
#define vcPolyModelNode_h__

#include "vcSceneItem.h"
#include "vcGLState.h"

struct vcState;
struct vcRenderData;
struct vcPolygonModel;

class vcPolyModelNode : public vcSceneItem
{
public:
  vcPolygonModel *m_pModel;

  vcPolyModelNode(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcPolyModelNode() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble4x4 GetWorldSpaceMatrix();
  udDouble3 GetLocalSpacePivot();

private:
  udDouble4x4 m_matrix;

  vcGLStateCullMode m_cullFace;
  bool m_ignoreTint;
};

#endif //vcPolyModelNode_h__
