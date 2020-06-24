#ifndef vcImageRenderer_h__
#define vcImageRenderer_h__

#include "udMath.h"
#include "vcTexture.h"

enum vcImageThumbnailSize
{
  vcIS_Native, // Renders at the native size of the image

  vcIS_Small, // Renders a small thumbnail
  vcIS_Large, // Renders a large thumbnail

  vcIS_Count,
};

enum vcImageType
{
  // billboard
  vcIT_StandardPhoto,

  // world geometry
  vcIT_OrientedPhoto,
  vcIT_Panorama,
  vcIT_PhotoSphere,

  // screen
  vcIT_ScreenPhoto,

  vcIT_Count
};

static const float vcISToWorldSize[] = { -1.f, 3.f, 10.f };
UDCOMPILEASSERT(udLengthOf(vcISToWorldSize) == vcIS_Count, "ImageWorldSize not equal size");

struct vcImageRenderInfo
{
  udDouble3 position;
  udDouble3 ypr;
  double scale;

  vcTexture *pTexture;
  vcImageThumbnailSize size;
  vcImageType type;
  udFloat4 colour;
};

struct udWorkerPool;

udResult vcImageRenderer_Init(udWorkerPool *pWorkerPool);
udResult vcImageRenderer_Destroy();

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, double zScale);

#endif//vcImageRenderer_h__
