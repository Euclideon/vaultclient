#ifndef vcPOI_h__
#define vcPOI_h__

#include "vcScene.h"
#include "vdkRenderContext.h"
#include "vdkError.h"
#include "gl/vcFenceRenderer.h"
#include "gl/vcLabelRenderer.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;
struct vcFenceRenderer;

struct vcLineInfo
{
  udDouble3 *pPoints;
  size_t numPoints;
  uint32_t lineColour;
  uint32_t lineWidth;
  size_t selectedPoint;
  bool closed;
  vcFenceRendererImageMode lineStyle;
};

class vcPOI : public vcSceneItem
{
public:
  vcLineInfo line;
  uint32_t nameColour;
  uint32_t backColour;
  double namePt;

  vcFenceRenderer *pFence;
  vcLabelInfo *pLabelInfo;

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);

  udDouble4x4 GetWorldSpaceMatrix();
};

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes = "");
void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, udDouble3 position, int32_t srid, const char *pNotes = "");

#endif //vcPOI_h__
