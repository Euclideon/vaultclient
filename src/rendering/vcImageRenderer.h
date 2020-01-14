#ifndef vcImageRenderer_h__
#define vcImageRenderer_h__

#include "udMath.h"
#include "gl/vcTexture.h"

enum vcImageThumbnailSize
{
  vcIS_Native, // Renders at the native size of the image

  vcIS_Small, // Renders a small thumbnail
  vcIS_Large, // Renders a large thumbnail

  vcIS_Count,
};

enum vcImageType
{
  vcIT_StandardPhoto,
  vcIT_Panorama,
  vcIT_PhotoSphere,

  vcIT_Count
};

struct vcImageRenderInfo
{
  udDouble3 position;
  udDouble3 ypr;
  udDouble3 scale;

  vcTexture *pTexture;
  vcImageThumbnailSize size;
  vcImageType type;
  udFloat4 colour;
};

void vcImageRenderer_Init();
void vcImageRenderer_Destroy();

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, double zScale);

#endif//vcImageRenderer_h__
