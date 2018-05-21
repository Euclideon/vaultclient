#ifndef vcTerrain_h__
#define vcTerrain_h__

#include "udPlatform/udMath.h"

struct vcTerrain;

udResult vcTerrain_Init(vcTerrain **ppTerrain);
udResult vcTerrain_Destroy(vcTerrain **ppTerrain);

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, const udFloat2 &viewLatLong, const float viewSize);
void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &viewProjection);

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled);

#endif//vcTerrain_h__