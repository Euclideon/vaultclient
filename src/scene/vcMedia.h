#ifndef vcMedia_h__
#define vcMedia_h__

#include "vcSceneItem.h"
#include "vcCamera.h"
#include "udRenderContext.h"
#include "udError.h"
#include "vcFenceRenderer.h"
#include "vcLabelRenderer.h"
#include "vcImageRenderer.h"

struct udPointCloud;
struct vcTexture;
struct vcState;

class vcMedia : public vcSceneItem
{
public:
  vcMedia(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState);
  ~vcMedia();

  void OnNodeUpdate(vcState *pProgramState) override;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override;
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta) override;

  void HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID) override;
  void HandleSceneEmbeddedUI(vcState *pProgramState) override;
  void HandleContextMenu(vcState *pProgramState) override;
  vcMenuBarButtonIcon GetSceneExplorerIcon() override;

  void Cleanup(vcState *pProgramState) override;
  void ChangeProjection(const udGeoZone &newZone) override;

  void SetCameraPosition(vcState *pProgramState) override;
  udDouble4x4 GetWorldSpaceMatrix() override;

  void SetImageData(void **ppImageData, int64_t imageSize); // This takes ownership if it can

  vcGizmoAllowedControls GetAllowedControls() override;

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
