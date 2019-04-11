#ifndef vcImageRenderer_h__
#define vcImageRenderer_h__

#include "udPlatform/udMath.h"
#include "gl/vcTexture.h"

enum vcImageSize
{
  vcIS_Small,
  vcIS_Large,

  vcIS_Count,
};

// TODO: Probably move to `vcMediaPoint` once pauls changes go through
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
  vcImageSize size;
  vcImageType type;
  udFloat4 colour;
};

void vcImageRenderer_Init();
void vcImageRenderer_Destroy();

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize);

#endif//vcImageRenderer_h__
