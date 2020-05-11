#include "vcLayout.h"

uint32_t vcLayout_GetSize(const vcVertexLayoutTypes layoutType)
{
  switch (layoutType)
  {
  case vcVLT_Position2:
    return 8;
  case vcVLT_Position3:
    return 12;
  case vcVLT_Position4:
    return 16;
  case vcVLT_TextureCoords2:
    return 8;
  case vcVLT_ColourBGRA:
    return 4;
  case vcVLT_Normal3:
    return 12;
  case vcVLT_QuadCorner:
    return 8;
  case vcVLT_Color0:
    return 16;
  case vcVLT_Color1:
    return 16;
  case vcVLT_Unsupported:
    return 0;
  case vcVLT_TotalTypes: // fall through
  default:
    return 0;
  }
}

uint32_t vcLayout_GetSize(const vcVertexLayoutTypes *pLayout, int numTypes)
{
  uint32_t accumlatedOffset = 0;

  for (int i = 0; i < numTypes; ++i)
    accumlatedOffset += vcLayout_GetSize(pLayout[i]);

  return accumlatedOffset;
}
