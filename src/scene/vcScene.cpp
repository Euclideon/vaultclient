#include "vcScene.h"

#include "vcState.h"

void vcScene_RemoveItem(vcState *pProgramState, size_t index)
{
  if (pProgramState->sceneList[index]->loadStatus == vcSLS_Pending)
    udInterlockedCompareExchange(&pProgramState->sceneList[index]->loadStatus, vcSLS_Unloaded, vcSLS_Pending);

  while (pProgramState->sceneList[index]->loadStatus == vcSLS_Loading)
    udYield(); // Spin until other thread stops processing

  if (pProgramState->sceneList[index]->loadStatus == vcSLS_Loaded || pProgramState->sceneList[index]->loadStatus == vcSLS_OpenFailure || pProgramState->sceneList[index]->loadStatus == vcSLS_Failed)
  {
    if (pProgramState->sceneList[index]->pCleanupFunc)
      pProgramState->sceneList[index]->pCleanupFunc(pProgramState, pProgramState->sceneList[index]);

    if (pProgramState->sceneList[index]->pMetadata)
      pProgramState->sceneList[index]->pMetadata->Destroy();

    udFree(pProgramState->sceneList[index]->pMetadata);
    udFree(pProgramState->sceneList[index]->pZone);
  }

  pProgramState->sceneList[index]->loadStatus = vcSLS_Unloaded;

  udFree(pProgramState->sceneList.at(index));
  pProgramState->sceneList.erase(pProgramState->sceneList.begin() + index);
}

void vcScene_RemoveAll(vcState *pProgramState)
{
  while (pProgramState->sceneList.size() > 0)
    vcScene_RemoveItem(pProgramState, 0);
  pProgramState->numSelectedModels = 0;
}

void vcScene_UpdateItemToCurrentProjection(vcState *pProgramState, vcSceneItem *pModel)
{
  if (!pModel)
  {
    for (size_t i = 0; i < pProgramState->sceneList.size(); ++i)
      vcScene_UpdateItemToCurrentProjection(pProgramState, pProgramState->sceneList[i]);
  }
  else
  {
    // Handle transforming into the camera's GeoZone
    if (pProgramState->gis.isProjected && pModel->pZone != nullptr && pProgramState->gis.SRID != pModel->pZone->srid)
    {
      pModel->sceneMatrix = udGeoZone_TransformMatrix(pModel->sceneMatrix, *pModel->pZone, pProgramState->gis.zone);

      // This is not ideal as it will gather drift
      memcpy(pModel->pZone, &pProgramState->gis.zone, sizeof(pProgramState->gis.zone));
    }
  }
}

bool vcScene_UseProjectFromItem(vcState *pProgramState, vcSceneItem *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  if ((pModel->pZone != nullptr && vcGIS_ChangeSpace(&pProgramState->gis, pModel->pZone->srid)) || (pModel->pZone == nullptr && vcGIS_ChangeSpace(&pProgramState->gis, 0)))
    vcScene_UpdateItemToCurrentProjection(pProgramState, nullptr); // Update all models to new zone

  pProgramState->pCamera->position = vcScene_GetItemWorldSpacePivotPoint(pModel);

  return true;
}

udDouble3 vcScene_GetItemWorldSpacePivotPoint(vcSceneItem *pModel)
{
  udDouble3 midPoint = udDouble3::zero();

  if (pModel != nullptr)
    midPoint = (pModel->sceneMatrix * udDouble4::create(pModel->pivot, 1.0)).toVector3();

  return midPoint;
}
