#ifndef vcPlaceLayer_h__
#define vcPlaceLayer_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"
#include "gl/vcGLState.h"

struct udWorkerPool;
struct vdkPointCloud;
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
  udChunkedArray<vcLabelInfo> m_closeLabels;

  double m_pinDistance;
  double m_labelDistance;

  vcFenceRenderer *m_pFences;

public:
  vcPlaceLayer(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcPlaceLayer() {};

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleImGui(vcState *pProgramState, size_t *pItemID);

  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);
};

#endif //vcPlaceLayer_h__
