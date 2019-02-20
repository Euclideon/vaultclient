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
  int numPoints;
  uint32_t colourPrimary;
  uint32_t colourSecondary;
  uint32_t lineWidth;
  int selectedPoint;
  bool closed;
  vcFenceRendererImageMode lineStyle;
  vcFenceRendererVisualMode fenceMode;
};

class vcPOI : public vcSceneItem
{
public:
  vcLineInfo m_line;
  uint32_t m_nameColour;
  uint32_t m_backColour;
  vcLabelFontSize m_namePt;

  bool m_showArea;
  double m_calculatedArea;

  bool m_showLength;
  double m_calculatedLength;

  vcFenceRenderer *m_pFence;
  vcLabelInfo *m_pLabelInfo;
  const char *m_pLabelText;

  vcPOI(const char *pName, uint32_t nameColour, vcLabelFontSize namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes = "");
  vcPOI(const char *pName, uint32_t nameColour, vcLabelFontSize namePt, udDouble3 position, int32_t srid, const char *pNotes = "");

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData);
  void ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta);
  void HandleImGui(vcState *pProgramState, size_t *pItemID);
  void Cleanup(vcState *pProgramState);

  void OnNameChange();

  void AddPoint(const udDouble3 &position);
  void UpdatePoints();

  udDouble4x4 GetWorldSpaceMatrix();
protected:
  void Init(const char *pName, uint32_t nameColour, vcLabelFontSize namePt, vcLineInfo *pLine, int32_t srid, const char *pNotes = "");
};

#endif //vcPOI_h__
