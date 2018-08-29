#ifndef vcTerrainRenderer_h__
#define vcTerrainRenderer_h__

#include "udPlatform/udMath.h"

struct vcQuadTreeNode;
struct vcTerrainRenderer;
struct vcSettings;

void vcTerrainRenderer_Init(vcTerrainRenderer **ppTerrainRenderer, vcSettings *pSettings);
void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer);

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, int16_t srid, const udInt3 &slippyCoords, const udDouble3 &cameraLocalPosition, const vcQuadTreeNode *pNodeList, int nodeCount);
void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &view, const udDouble4x4 &proj);

void vcTerrainRenderer_ClearCache(vcTerrainRenderer *pTerrainRenderer);

#endif//vcTerrainRenderer_h__
