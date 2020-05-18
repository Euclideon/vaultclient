#ifndef vcTileRenderer_h__
#define vcTileRenderer_h__

#include "udMath.h"

// Cap depth at level 19 (system doesn't have access to these tiles yet)
enum { MaxVisibleTileLevel = 19 };

struct vcSettings;
struct udGeoZone;

struct vcTileRenderer;

udResult vcTileRenderer_Create(vcTileRenderer **ppTileRenderer, vcSettings *pSettings);
udResult vcTileRenderer_Destroy(vcTileRenderer **ppTileRenderer);

udResult vcTileRenderer_ReloadShaders(vcTileRenderer *pTileRenderer);

void vcTileRenderer_Update(vcTileRenderer *pTileRenderer, const double deltaTime, udGeoZone *pGeozone, const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos, const udDouble3 &cameraZeroAltitude, const udDouble4x4 &viewProjectionMatrix);
void vcTileRenderer_Render(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, const udDouble4x4 &proj, const bool cameraInsideGround, const float encodedObjectId);

void vcTileRenderer_ClearTiles(vcTileRenderer *pTileRenderer);

udDouble3 vcTileRenderer_QueryMapPositionAtCartesian(vcTileRenderer *pTileRenderer, const udDouble3 &point, udDouble3 *pNormal);

#endif//vcTileRenderer_h__
