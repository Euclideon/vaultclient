#ifndef vcPlaceLayer_h__
#define vcPlaceLayer_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"
#include "udError.h"
#include "vcLineRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "vcGLState.h"

struct udWorkerPool;
struct udPointCloud;
struct vcTexture;
struct vcState;
struct vcFenceRenderer;
struct vcPolygonModel;

class vcPlaceLayer : public vcSceneItem
{
private:
  struct Place
  {
    const char *pName;
    udDouble3 latLongAlt;
    udDouble3 localSpace;
    int count; // Some places are already bunched
  };

  const char *m_pPinIcon;
  udChunkedArray<Place> m_places;

  // Close labels and close lines are kept in step (size should be the same)
  udChunkedArray<vcLabelInfo> m_closeLabels;
  udChunkedArray<vcLineInstance*> m_closeLines;

  double m_pinDistance;
  double m_labelDistance;

public:
  vcPlaceLayer(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcPlaceLayer() {};

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;

  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;
};

#endif //vcPlaceLayer_h__
