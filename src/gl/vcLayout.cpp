#include "vcLayout.h"


uint32_t vcLayout_GetSize(const vcVertexLayoutTypes *pLayout, int numTypes)
{
  uint32_t accumlatedOffset = 0;

  for (int i = 0; i < numTypes; ++i)
  {
    switch (pLayout[i])
    {
    case vcVLT_Position2:
      accumlatedOffset += 8;
      break;
    case vcVLT_Position3:
      accumlatedOffset += 12;
      break;
    case vcVLT_TextureCoords2:
      accumlatedOffset += 8;
      break;
    case vcVLT_RibbonInfo4:
      accumlatedOffset += 16;
      break;
    case vcVLT_ColourBGRA:
      accumlatedOffset += 4;
      break;
    case vcVLT_Normal3:
      accumlatedOffset += 12;
      break;
    case vcVLT_TotalTypes:
      break;
    }
  }
  return accumlatedOffset;
}
