#ifndef vcLayout_h__
#define vcLayout_h__

#include "udMath.h"

enum vcVertexLayoutTypes
{
  vcVLT_Position2, //Vec2
  vcVLT_Position3, //Vec3
  vcVLT_TextureCoords2, //Vec2
  vcVLT_ColourBGRA, //uint32_t
  vcVLT_Normal3, //Vec3
  vcVLT_RibbonInfo4, // Vec4

  vcVLT_Unsupported,

  vcVLT_TotalTypes
};

struct vcSimpleVertex
{
  udFloat3 position;
  udFloat2 uv;
};
const vcVertexLayoutTypes vcSimpleVertexLayout[] = { vcVLT_Position3, vcVLT_TextureCoords2 };

uint32_t vcLayout_GetSize(const vcVertexLayoutTypes layoutType);
uint32_t vcLayout_GetSize(const vcVertexLayoutTypes *pLayout, int numTypes);
uint32_t vcLayout_GetSize(const vcVertexLayoutTypes layoutType);

#endif//vcLayout_h__
