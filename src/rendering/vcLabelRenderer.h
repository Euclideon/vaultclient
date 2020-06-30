#ifndef vcLabelRenderer_h__
#define vcLabelRenderer_h__

#include "udMath.h"

enum vcLabelFontSize
{
  vcLFS_Medium, // Default size is medium

  vcLFS_Small,
  vcLFS_Large,

  vcLFS_Count,
};

class vcSceneItem;

struct vcLabelInfo
{
  udDouble3 worldPosition;

  const char *pText;
  uint32_t textColourRGBA;
  uint32_t backColourRGBA;

  vcSceneItem *pSceneItem;
  uint64_t sceneItemInternalId; // 0 is entire model; for most systems this will be +1 compared to internal arrays
};

struct ImDrawList;
struct vcTexture;
struct udWorkerPool;

udResult vcLabelRenderer_Init(udWorkerPool *pWorkerPool);
udResult vcLabelRenderer_Destroy();

bool vcLabelRenderer_Render(ImDrawList *drawList, vcLabelInfo *pLabel, float encodedObjectId, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize);
#endif//vcLabelRenderer_h__
