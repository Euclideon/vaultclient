#ifndef vcTerrain_h__
#define vcTerrain_h__

#include "udPlatform/udMath.h"

struct vcTerrain;
struct vcSettings;
struct vcGISSpace;

udResult vcTerrain_Init(vcTerrain **ppTerrain, vcSettings *pSettings);
udResult vcTerrain_Destroy(vcTerrain **ppTerrain);

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, vcGISSpace *pSpace, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos, const udDouble4x4 &viewProjectionMatrix);
void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &view, const udDouble4x4 &proj);

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled);
void vcTerrain_ClearTiles(vcTerrain *pTerrain);

#endif//vcTerrain_h__
