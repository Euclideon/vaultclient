#ifndef vcPolyModelNode_h__
#define vcPolyModelNode_h__

#include "vcSceneItem.h"

struct vcState;
struct vcRenderData;
struct vcPolygonModel;

class vcPolyModelNode : public vcSceneItem
{
public:
  vcPolyModelNode(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcPolyModelNode() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  udDouble4x4 GetWorldSpaceMatrix();
  udDouble3 GetLocalSpacePivot();

private:
  vcPolygonModel *m_pModel;
  udDouble4x4 m_matrix;

  bool m_invert;
};

#endif //vcPolyModelNode_h__
