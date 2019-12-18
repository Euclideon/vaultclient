#ifndef vcTileRenderer_h__
#define vcTileRenderer_h__

#include "udMath.h"

// Cap depth at level 19 (system doesn't have access to these tiles yet)
enum { MaxVisibleTileLevel = 14 };

struct vcSettings;
struct vcGISSpace;

struct vcTileRenderer;

udResult vcTileRenderer_Create(vcTileRenderer **ppTileRenderer, vcSettings *pSettings);
udResult vcTileRenderer_Destroy(vcTileRenderer **ppTileRenderer);

void vcTileRenderer_Update(vcTileRenderer *pTileRenderer, const double deltaTime, vcGISSpace *pSpace, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos, const udDouble4x4 &viewProjectionMatrix);
void vcTileRenderer_Render(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, const udDouble4x4 &proj);

void vcTileRenderer_ClearTiles(vcTileRenderer *pTileRenderer);

#endif//vcTileRenderer_h__
