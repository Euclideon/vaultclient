#ifndef vcTileRenderer_h__
#define vcTileRenderer_h__

#include "udMath.h"

struct vcSettings;
struct udGeoZone;
struct udWorkerPool;
struct vcCamera;

struct vcTileRenderer;

udResult vcTileRenderer_Create(vcTileRenderer **ppTileRenderer, udWorkerPool *pWorkerPool, vcSettings *pSettings);
udResult vcTileRenderer_Destroy(vcTileRenderer **ppTileRenderer);

udResult vcTileRenderer_ReloadShaders(vcTileRenderer *pTileRenderer, udWorkerPool *pWorkerPool);

void vcTileRenderer_Update(vcTileRenderer *pTileRenderer, const double deltaTime, udGeoZone *pGeozone, const udInt3 &slippyCoords, const vcCamera *pCamera, const udDouble4x4 &viewProjectionMatrix, bool *pIsLoading);
void vcTileRenderer_Render(vcTileRenderer *pTileRenderer, const udDouble4x4 &view, const udDouble4x4 &proj, const bool cameraInsideGround, const float encodedObjectId);

void vcTileRenderer_ClearTiles(vcTileRenderer *pTileRenderer);

udDouble3 vcTileRenderer_QueryMapAtCartesian(vcTileRenderer *pTileRenderer, const udDouble3 &point, const udDouble3 *pWorldUp, udDouble3 *pNormal);

#endif//vcTileRenderer_h__
