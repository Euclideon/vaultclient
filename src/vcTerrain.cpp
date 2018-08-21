
#include "vcTerrain.h"
#include "vcQuadTree.h"
#include "vcTerrainRenderer.h"
#include "vcSettings.h"

struct vcTerrain
{
  bool enabled;
  vcSettings *pSettings;
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
  pTerrain->pSettings = pSettings;
  vcTerrainRenderer_Init(&pTerrain->pTerrainRenderer, pSettings);

  *ppTerrain = pTerrain;
epilogue:
  return result;
}

udResult vcTerrain_Destroy(vcTerrain **ppTerrain)
{
  if (ppTerrain == nullptr || *ppTerrain == nullptr)
    return udR_Success;

  udResult result = udR_Success;
  vcTerrain *pTerrain = nullptr;

  UD_ERROR_NULL(ppTerrain, udR_InvalidParameter_);
  pTerrain = *ppTerrain;
  *ppTerrain = nullptr;

  if (pTerrain->pTerrainRenderer != nullptr)
    vcTerrainRenderer_Destroy(&pTerrain->pTerrainRenderer);

epilogue:
  udFree(pTerrain);
  return result;
}

void vcTerrain_BuildTerrain(vcTerrain *pTerrain, int16_t srid, const udDouble3 worldCorners[4], const udInt3 &slippyCoords, const udDouble3 &cameraWorldPos)
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
    pTerrain->pSettings->maptiles.mapHeight
  };
  
  vcQuadTree_GenerateNodeList(&pNodeList, &nodeCount, createInfo, &treeData);
  vcTerrainRenderer_BuildTiles(pTerrain->pTerrainRenderer, srid, slippyCoords, cameraWorldPos, pNodeList, nodeCount, treeData.leafNodeCount);
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
