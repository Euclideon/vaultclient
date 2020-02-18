#ifndef vcWaterRenderer_h__
#define vcWaterRenderer_h__

#include "udMath.h"
#include <vector>

struct vcWaterRenderer;
struct vcCamera;
struct vcTexture;

udResult vcWaterRenderer_Create(vcWaterRenderer **ppWaterRenderer);
udResult vcWaterRenderer_Destroy(vcWaterRenderer **ppWaterRenderer);

udResult vcWaterRenderer_AddVolume(vcWaterRenderer *pWaterRenderer, udDouble2 *pPoints, size_t pointCount
  , const std::vector< std::pair<const udDouble2 *, size_t> > &islandPoints);
void vcWaterRenderer_ClearAllVolumes(vcWaterRenderer *pWaterRenderer);

bool vcWaterRenderer_Render(vcWaterRenderer *pWaterRenderer, const udDouble4x4 &view, const udDouble4x4 &viewProjection, vcTexture *pSkyboxTexture, double deltaTime);

#endif//vcWaterRenderer_h__
