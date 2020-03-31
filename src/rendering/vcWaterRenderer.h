#ifndef vcWaterRenderer_h__
#define vcWaterRenderer_h__

#include "udMath.h"
#include "udGeoZone.h"

struct vcWaterRenderer;
struct vcTexture;

udResult vcWaterRenderer_Create(vcWaterRenderer **ppWaterRenderer);
udResult vcWaterRenderer_Destroy(vcWaterRenderer **ppWaterRenderer);

udResult vcWaterRenderer_AddVolume(vcWaterRenderer *pWaterRenderer, const udGeoZone &zone, double altitude, udDouble3 *pPoints, size_t pointCount);
void vcWaterRenderer_ClearAllVolumes(vcWaterRenderer *pWaterRenderer);

bool vcWaterRenderer_Render(vcWaterRenderer *pWaterRenderer, const udDouble4x4 &view, const udDouble4x4 &viewProjection, vcTexture *pSkyboxTexture, double deltaTime);

#endif//vcWaterRenderer_h__
