
#include "vcTerrain.h"
#include "vcQuadTree.h"
#include "vcTerrainRenderer.h"

float gWorldScale = 1000.0f; // temp world scale until i sort out lat/long coordinates

struct vcTerrain
{
  bool enabled;
  vcQuadTree *pQuadTree;
  vcTerrainRenderer *pTerrainRenderer;
};

udResult vcTerrain_Init(vcTerrain **ppTerrain)
{
  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  UD_ERROR_NULL(ppTerrain, udR_InvalidParameter_);

  pTerrain = udAllocType(vcTerrain, 1, udAF_Zero);
  UD_ERROR_IF(pTerrain == nullptr, udR_MemoryAllocationFailure);

  pTerrain->enabled = true;
  vcTerrainRenderer_Init(&pTerrain->pTerrainRenderer);

  if (vcQuadTree_Init(&pTerrain->pQuadTree) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

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
  if (vcQuadTree_Destroy(&pTerrain->pQuadTree) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  udFree(pTerrain);
  return result;
}

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, const udFloat2 &viewLatLong, const float viewSize)
{
  if (!pTerrain->enabled)
    return;

  vcQuadTreeMetaData treeData;
  vcQuadTreeNode *pNodeList = nullptr;
  int nodeCount = 0;

  // todo: convert from lat, long to local tree space
  udFloat2 viewLatLongMS = udFloat2::create(viewLatLong);
  udFloat2 viewSizeMS = udFloat2::create(viewSize);
  
  vcQuadTree_GenerateNodeList(pTerrain->pQuadTree, &pNodeList, &nodeCount, viewLatLongMS, viewSizeMS, &treeData);
  vcTerrainRenderer_BuildTiles(pTerrain->pTerrainRenderer, pNodeList, nodeCount, treeData.leafNodeCount);
}

void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &viewProjection)
{
  if (!pTerrain->enabled)
    return;

  vcTerrainRenderer_Render(pTerrain->pTerrainRenderer, viewProjection);
}

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled)
{
  pTerrain->enabled = enabled;
}