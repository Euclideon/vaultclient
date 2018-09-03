
#include "vcTerrain.h"
#include "vcQuadTree.h"
#include "vcTerrainRenderer.h"
#include "vcSettings.h"

struct vcTerrain
{
  bool enabled;
  vcSettings *pSettings;
  vcQuadTree *pQuadTree;
  vcTerrainRenderer *pTerrainRenderer;
};

udResult vcTerrain_Init(vcTerrain **ppTerrain, vcSettings *pSettings)
{
  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  UD_ERROR_NULL(ppTerrain, udR_InvalidParameter_);

  pTerrain = udAllocType(vcTerrain, 1, udAF_Zero);
  UD_ERROR_IF(!pTerrain, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcQuadTree_Init(&pTerrain->pQuadTree));

  pTerrain->enabled = true;
  pTerrain->pSettings = pSettings;
  vcTerrainRenderer_Init(&pTerrain->pTerrainRenderer, pSettings);

  *ppTerrain = pTerrain;
  pTerrain = nullptr;

epilogue:
  if (pTerrain)
    vcTerrain_Destroy(&pTerrain);

  return result;
}

udResult vcTerrain_Destroy(vcTerrain **ppTerrain)
{
  if (ppTerrain == nullptr || *ppTerrain == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  pTerrain = *ppTerrain;
  *ppTerrain = nullptr;

  UD_ERROR_CHECK(vcQuadTree_Destroy(&pTerrain->pQuadTree));

  if (pTerrain->pTerrainRenderer != nullptr)
    vcTerrainRenderer_Destroy(&pTerrain->pTerrainRenderer);

epilogue:
  udFree(pTerrain);
  return result;
}

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, int16_t srid, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos, const udDouble4x4 &viewProjectionMatrix)
{
  if (!pTerrain->enabled)
    return;

  vcQuadTreeMetaData treeData;
  vcQuadTreeNode *pNodeList = nullptr;
  int nodeCount = 0;

  double slippyCornersViewSize = udMag3(worldCorners[1] - worldCorners[2]) * 0.5;
  vcQuadTreeCreateInfo createInfo =
  {
    srid,
    slippyCoords,
    cameraWorldPos,
    slippyCornersViewSize,
    (double)pTerrain->pSettings->camera.farPlane,
    pTerrain->pSettings->maptiles.mapHeight,
    viewProjectionMatrix
  };

  vcQuadTree_GenerateNodeList(pTerrain->pQuadTree, createInfo, &treeData);
  vcQuadTree_GetNodeList(pTerrain->pQuadTree, &pNodeList, &nodeCount);

  vcTerrainRenderer_BuildTiles(pTerrain->pTerrainRenderer, srid, slippyCoords, cameraWorldPos, pNodeList, nodeCount);
}

void vcTerrain_Render(vcTerrain *pTerrain, const udDouble4x4 &view, const udDouble4x4 &proj)
{
  if (!pTerrain->enabled)
    return;

  vcTerrainRenderer_Render(pTerrain->pTerrainRenderer, view, proj);
}

void vcTerrain_SetEnabled(vcTerrain *pTerrain, bool enabled)
{
  pTerrain->enabled = enabled;
}
