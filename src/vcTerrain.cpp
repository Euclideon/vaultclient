
#include "vcTerrain.h"
#include "vcQuadTree.h"
#include "vcTerrainRenderer.h"


struct vcTerrain
{
  bool enabled;
  vcTerrainRenderer *pTerrainRenderer;
};

udResult vcTerrain_Init(vcTerrain **ppTerrain, vcSettings *pSettings)
{
  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  UD_ERROR_NULL(ppTerrain, udR_InvalidParameter_);

  pTerrain = udAllocType(vcTerrain, 1, udAF_Zero);
  UD_ERROR_IF(pTerrain == nullptr, udR_MemoryAllocationFailure);

  pTerrain->enabled = true;
  vcTerrainRenderer_Init(&pTerrain->pTerrainRenderer, pSettings);

  *ppTerrain = pTerrain;
epilogue:
  return result;
}

udResult vcTerrain_Destroy(vcTerrain **ppTerrain)
{
  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  UD_ERROR_NULL(ppTerrain, udR_InvalidParameter_);
  pTerrain = *ppTerrain;
  *ppTerrain = nullptr;

  vcTerrainRenderer_Destroy(&pTerrain->pTerrainRenderer);

epilogue:
  udFree(pTerrain);
  return result;
}

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, int16_t srid, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &localViewPos, const double localViewSize)
{
  if (!pTerrain->enabled)
    return;

  vcQuadTreeMetaData treeData;
  vcQuadTreeNode *pNodeList = nullptr;
  int nodeCount = 0;

  // calculate view in quad tree space [0, 1]
  udDouble2 worldScale = udDouble2::create(worldCorners[3].x - worldCorners[0].x, worldCorners[0].y - worldCorners[3].y);
  udFloat2 viewPosMS = udFloat2::create((localViewPos.toVector2() - udDouble2::create(worldCorners[0].x, worldCorners[2].y)) / worldScale);
  udFloat2 viewSizeMS = udFloat2::create((float)localViewSize);

  vcQuadTree_GenerateNodeList(&pNodeList, &nodeCount, slippyCoords.z, viewPosMS, viewSizeMS, &treeData);
  vcTerrainRenderer_BuildTiles(pTerrain->pTerrainRenderer, srid, slippyCoords, localViewPos, pNodeList, nodeCount, treeData.visibleNodeCount);
}

void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &viewProj)
{
  if (!pTerrain->enabled)
    return;

  vcTerrainRenderer_Render(pTerrain->pTerrainRenderer, viewProj);
}

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled)
{
  pTerrain->enabled = enabled;
}

bool vcTerrain_ClearCache(vcTerrain *pTerrain)
{
  vcTerrainRenderer_ClearCache(pTerrain->pTerrainRenderer);
  return true;
}
