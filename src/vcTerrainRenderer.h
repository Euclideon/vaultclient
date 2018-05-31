#ifndef vcTerrainRenderer_h__
#define vcTerrainRenderer_h__

#include "udPlatform/udMath.h"

struct vcQuadTreeNode;
struct vcTerrainRenderer;
struct vcSettings;

void vcTerrainRenderer_Init(vcTerrainRenderer **ppTerrainRenderer, vcSettings *pSettings);
void vcTerrainRenderer_Destroy(vcTerrainRenderer **ppTerrainRenderer);

void vcTerrainRenderer_BuildTiles(vcTerrainRenderer *pTerrainRenderer, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const vcQuadTreeNode *pNodeList, int nodeCount, int leafNodeCount);
void vcTerrainRenderer_Render(vcTerrainRenderer *pTerrainRenderer, const udDouble4x4 &viewProjection);

#endif//vcTerrainRenderer_h__
