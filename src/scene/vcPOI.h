#ifndef vcPOI_h__
#define vcPOI_h__

#include "vcScene.h"
#include "vdkRenderContext.h"

struct vdkPointCloud;
struct vcTexture;
struct vcState;

struct vcLineInfo
{
  udDouble3 *pPoints;
  size_t numPoints;
  uint32_t lineColour;
  uint32_t lineWidth;
};

struct vcPOI : public vcSceneItem
{
  vcLineInfo line;
  uint32_t nameColour;
  double namePt;
};

void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, vcLineInfo *pLine, int32_t srid);
void vcPOI_AddToList(vcState *pProgramState, const char *pName, uint32_t nameColour, double namePt, udDouble3 position, int32_t srid);

#endif //vcPOI_h__
