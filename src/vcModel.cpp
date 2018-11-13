#include "vcModel.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udGeoZone.h"
#include "udPlatform/udJSON.h"

#include "gl/vcTexture.h"

#include "vdkModel.h"

struct vcModelLoadInfo
{
  bool jumpToLocation;
  vcState *pProgramState;
  vcModel *pModel;
};

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;

  int32_t status = udInterlockedCompareExchange(&pLoadInfo->pModel->loadStatus, vcMLS_Loading, vcMLS_Pending);

  if (status == vcMLS_Pending)
  {
    vdkError modelStatus = vdkModel_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->pVDKModel, pLoadInfo->pModel->path);
    if (modelStatus == vE_Success)
    {
      const char *pMetadata;
      pLoadInfo->pModel->pMetadata = udAllocType(udJSON, 1, udAF_Zero);

      if (vdkModel_GetMetadata(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->pVDKModel, &pMetadata) == vE_Success)
      {
        pLoadInfo->pModel->pMetadata->Parse(pMetadata);

        pLoadInfo->pModel->hasWatermark = pLoadInfo->pModel->pMetadata->Get("Watermark").IsString();

        vcSRID srid = 0;
        udJSON tempNode;

        const char *pSRID = pLoadInfo->pModel->pMetadata->Get("ProjectionID").AsString();
        if (pSRID != nullptr)
        {
          pSRID = udStrchr(pSRID, ":");
          if (pSRID != nullptr)
            srid = udStrAtou(&pSRID[1]);
        }

        const char *pWKT = pLoadInfo->pModel->pMetadata->Get("ProjectionWKT").AsString();
        if (srid == 0 && pWKT != nullptr && udParseWKT(&tempNode, pWKT) == udR_Success)
        {
          for (size_t i = 0; i < tempNode.Get("values").ArrayLength(); ++i)
          {
            if (udStrEquali(tempNode.Get("values[%llu].type", i).AsString(), "AUTHORITY"))
            {
              srid = tempNode.Get("values[%llu].values[0]", i).AsInt();
              break;
            }
          }

          pLoadInfo->pModel->pMetadata->Set(&tempNode, "ProjectionWKT");
        }

        if (srid != 0)
        {
          pLoadInfo->pModel->pZone = udAllocType(udGeoZone, 1, udAF_Zero);
          udGeoZone_SetFromSRID(pLoadInfo->pModel->pZone, srid);
        }
      }

      if (pLoadInfo->jumpToLocation)
        vcModel_MoveToModelProjection(pLoadInfo->pProgramState, pLoadInfo->pModel);
      else
        vcModel_UpdateMatrix(pLoadInfo->pProgramState, nullptr); // Set all model matrices
      pLoadInfo->pModel->loadStatus = vcMLS_Loaded;
    }
    else if (modelStatus == vE_OpenFailure)
    {
      pLoadInfo->pModel->loadStatus = vcMLS_OpenFailure;
    }
    else
    {
      pLoadInfo->pModel->loadStatus = vcMLS_Failed;
    }
  }
}

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath, bool jumpToModelOnLoad /*= true*/)
{
  if (pFilePath == nullptr)
    return;

  vcModel *pModel = nullptr;

  if (pProgramState->vcModelList.PushBack(&pModel) == udR_Success)
  {
    memset(pModel, 0, sizeof(vcModel));

    vcModelLoadInfo *pLoadInfo = udAllocType(vcModelLoadInfo, 1, udAF_Zero);
    if (pLoadInfo != nullptr)
    {
      // Prepare the model
      udStrcpy(pModel->path, sizeof(pModel->path), pFilePath);
      pModel->visible = true;

      // Prepare the load info
      pLoadInfo->pModel = pModel;
      pLoadInfo->pProgramState = pProgramState;
      pLoadInfo->jumpToLocation = jumpToModelOnLoad;

      // Queue for load
      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModel_LoadModel, pLoadInfo);
    }
    else
    {
      pModel->loadStatus = vcMLS_Failed;
    }
  }
}

void vcModel_RemoveFromList(vcState *pProgramState, size_t index)
{
  if (pProgramState->vcModelList[index].loadStatus == vcMLS_Pending)
    udInterlockedCompareExchange(&pProgramState->vcModelList[index].loadStatus, vcMLS_Unloaded, vcMLS_Pending);

  while (pProgramState->vcModelList[index].loadStatus == vcMLS_Loading)
    udYield(); // Spin until other thread stops processing

  if (pProgramState->vcModelList[index].loadStatus == vcMLS_Loaded)
  {
    vdkModel_Unload(pProgramState->pVDKContext, &pProgramState->vcModelList[index].pVDKModel);

    if (pProgramState->vcModelList[index].pWatermark != nullptr)
      vcTexture_Destroy(&pProgramState->vcModelList[index].pWatermark);

    pProgramState->vcModelList[index].pMetadata->Destroy();
    udFree(pProgramState->vcModelList[index].pMetadata);
    udFree(pProgramState->vcModelList[index].pZone);
  }

  pProgramState->vcModelList[index].loadStatus = vcMLS_Unloaded;
  pProgramState->vcModelList.RemoveAt(index);
}

void vcModel_UnloadList(vcState *pProgramState)
{
  while (pProgramState->vcModelList.length > 0)
    vcModel_RemoveFromList(pProgramState, 0);
}

void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel, udDouble4 offsetT)
{
  if (!pModel)
  {
    for (size_t i = 0; i < pProgramState->vcModelList.length; ++i)
      vcModel_UpdateMatrix(pProgramState, &pProgramState->vcModelList[i], offsetT);
  }
  else
  {
    udDouble4x4 matrix;
    vdkModel_GetLocalMatrix(pProgramState->pVDKContext, pModel->pVDKModel, matrix.a);

    if (pModel->flipYZ)
    {
      udDouble4 rowz = -matrix.axis.y;
      matrix.axis.y = matrix.axis.z;
      matrix.axis.z = rowz;
    }

    matrix = udDouble4x4::rotationZ(offsetT.w, offsetT.toVector3()) * matrix;

    // Handle transforming into the camera's GeoZone
    if (pProgramState->gis.isProjected && pModel->pZone != nullptr && pProgramState->gis.SRID != pModel->pZone->srid)
      matrix = udGeoZone_TransformMatrix(matrix, *pModel->pZone, pProgramState->gis.zone);

    vdkModel_SetWorldMatrix(pProgramState->pVDKContext, pModel->pVDKModel, matrix.a);
    pModel->worldMatrix = matrix;
  }
}

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  if ((pModel->pZone != nullptr && vcGIS_ChangeSpace(&pProgramState->gis, pModel->pZone->srid)) || (pModel->pZone == nullptr && vcGIS_ChangeSpace(&pProgramState->gis, 0)))
    vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone

  pProgramState->pCamera->position = vcModel_GetMidPointLocalSpace(pProgramState, pModel);

  return true;
}

udDouble3 vcModel_GetMidPointLocalSpace(vcState *pProgramState, vcModel *pModel)
{
  udDouble3 midPoint = udDouble3::zero();

  if (pModel != nullptr)
    vdkModel_GetModelCenter(pProgramState->pVDKContext, pModel->pVDKModel, &midPoint.x);

  return midPoint;
}
