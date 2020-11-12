#ifndef vcColourScales_h__
#define vcColourScales_h__
#include "udMath.h"

enum vcCS_Scale {
  vcCSS_Acton,
  vcCSS_Bamako,
  vcCSS_Batlow,
  vcCSS_Berlin,
  vcCSS_Bilbao,
  vcCSS_Broc,
  vcCSS_BrocO,
  vcCSS_Buda,
  vcCSS_Cork,
  vcCSS_CorkO,
  vcCSS_Davos,
  vcCSS_Devon,
  vcCSS_GrayC,
  vcCSS_Hawaii,
  vcCSS_Imola,
  vcCSS_Lajolla,
  vcCSS_Lapaz,
  vcCSS_Lisbon,
  vcCSS_Nuuk,
  vcCSS_Oleron,
  vcCSS_Oslo,
  vcCSS_Roma,
  vcCSS_RomaO,
  vcCSS_Tofino,
  vcCSS_Tokyo,
  vcCSS_Turku,
  vcCSS_Vik,
  vcCSS_VikO,

  vcCSS_Count
};

const char* vcCS_ScaleNames[];
const uint8_t* vcCS_Scales[];

#endif
