#ifndef vcMedia_h__
#define vcMedia_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

class vcMedia : public vcSceneItem
{
public:
  vcMedia(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState);
  ~vcMedia();

  void OnNodeUpdate(vcState *pProgramState);

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID);
  void HandleSceneEmbeddedUI(vcState *pProgramState);
  void HandleContextMenu(vcState *pProgramState);

  void Cleanup(vcState *pProgramState);
  void ChangeProjection(const udGeoZone &newZone);

  void SetCameraPosition(vcState *pProgramState);
  udDouble4x4 GetWorldSpaceMatrix();

  void SetImageData(void **ppImageData, int64_t imageSize); // This takes ownership if it can

  vcGizmoAllowedControls GetAllowedControls();

public:
  const char *m_pLoadedURI; // The URI of the media we have (or are loading) currently

private:
  vcImageRenderInfo m_image;

  double m_loadLoadTimeSec;
  double m_reloadTimeSecs;

  void *m_pImageData;
  uint64_t m_imageDataSize;
};

#endif //vcPOI_h__
