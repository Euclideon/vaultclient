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

  vcPolyModelNode(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcPolyModelNode() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;
  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  udDouble4x4 GetWorldSpaceMatrix() override;
  udDouble3 GetLocalSpacePivot() override;

private:
  void LoadTransforms(const udGeoZone &zone);

  udDouble4x4 m_matrix;

  vcGLStateCullMode m_cullFace;
  bool m_ignoreTint;
  bool m_flipAxis;
};

#endif //vcPolyModelNode_h__
