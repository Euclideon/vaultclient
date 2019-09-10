#ifndef vcInternalModels_h__
#define vcInternalModels_h__

#include "gl/vcMesh.h"

enum vcInternalMeshType
{
  vcIMT_ScreenQuad,
  vcIMT_FlippedScreenQuad,

  vcIMT_Billboard,
  vcIMT_Sphere,
  vcIMT_Tube,

  vcIMT_Compass,
  vcIMT_Orbit,

  vcIMT_Count
};
extern vcMesh *gInternalModels[vcIMT_Count];

udResult vcInternalModels_Init();
udResult vcInternalModels_Deinit();

#endif //vcInternalModels_h__
