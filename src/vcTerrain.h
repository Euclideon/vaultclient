#ifndef vcTerrain_h__
#define vcTerrain_h__

#include "udPlatform/udMath.h"

struct vcTerrain;
struct vcSettings;

udResult vcTerrain_Init(vcTerrain **ppTerrain, vcSettings *pSettings);
udResult vcTerrain_Destroy(vcTerrain **ppTerrain);

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, int16_t srid, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &localViewPos, const double localViewSize);
void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &viewProj);

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled);

bool vcTerrain_ClearCache(vcTerrain *pTerrain);

#endif//vcTerrain_h__
