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

  udDouble3 *pPosition;
  udDouble3 *pRotation;
  double scale;
};

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

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

        pLoadInfo->pModel->meterScale = pLoadInfo->pModel->pMetadata->Get("info.meterScale").AsDouble(1.0);
        pLoadInfo->pModel->pivot = pLoadInfo->pModel->pMetadata->Get("info.pivot").AsDouble3();
        pLoadInfo->pModel->boundsMin = pLoadInfo->pModel->pMetadata->Get("info.bounds.min").AsDouble3();
        pLoadInfo->pModel->boundsMax = pLoadInfo->pModel->pMetadata->Get("info.bounds.max").AsDouble3();

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
        if (pWKT != nullptr)
        {
          if (udParseWKT(&tempNode, pWKT) == udR_Success)
          {
            for (size_t i = 0; i < tempNode.Get("values").ArrayLength() && srid == 0; ++i)
            {
              if (udStrEquali(tempNode.Get("values[%llu].type", i).AsString(), "AUTHORITY"))
              {
                srid = tempNode.Get("values[%llu].values[0]", i).AsInt();
                break;
              }
            }

            pLoadInfo->pModel->pMetadata->Set(&tempNode, "ProjectionWKT");
          }
        }

        if (srid != 0)
        {
          pLoadInfo->pModel->pZone = udAllocType(udGeoZone, 1, udAF_Zero);
          udGeoZone_SetFromSRID(pLoadInfo->pModel->pZone, srid);
        }
      }

      vdkModel_GetLocalMatrix(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->pVDKModel, pLoadInfo->pModel->baseMatrix.a);

      if (pLoadInfo->scale != 1.0)
        __debugbreak();

      udDouble3 scaleFactor = udDouble3::create(udMag3(pLoadInfo->pModel->baseMatrix.axis.x), udMag3(pLoadInfo->pModel->baseMatrix.axis.y), udMag3(pLoadInfo->pModel->baseMatrix.axis.z)) * pLoadInfo->scale;
      udDouble3 translate = pLoadInfo->pModel->baseMatrix.axis.t.toVector3();
      udDouble3 ypr = udDouble3::zero();

      if (pLoadInfo->pRotation)
        ypr = *pLoadInfo->pRotation;

      if (pLoadInfo->pPosition)
        translate = *pLoadInfo->pPosition;

      if (ypr != udDouble3::zero() || pLoadInfo->scale != 1.0 || pLoadInfo->pPosition != nullptr)
        pLoadInfo->pModel->baseMatrix = udDouble4x4::translation(translate) * udDouble4x4::translation(pLoadInfo->pModel->pivot) * udDouble4x4::rotationYPR(ypr) * udDouble4x4::scaleNonUniform(scaleFactor) * udDouble4x4::translation(-pLoadInfo->pModel->pivot);

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

void vcModel_AddToList(vcState *pProgramState, const char *pFilePath, bool jumpToModelOnLoad /*= true*/, udDouble3 *pOverridePosition /*= nullptr*/, udDouble3 *pOverrideYPR /*= nullptr*/, double scale /*= 1.0*/)
{
  if (pFilePath == nullptr)
    return;

  vcModel *pModel = udAllocType(vcModel, 1, udAF_Zero);

  pProgramState->vcModelList.push_back(pModel); // TODO: Proper Exception Handling
  {
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

      pLoadInfo->pPosition = pOverridePosition;
      pLoadInfo->pRotation = pOverrideYPR;
      pLoadInfo->scale = scale;

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
  if (pProgramState->vcModelList[index]->loadStatus == vcMLS_Pending)
    udInterlockedCompareExchange(&pProgramState->vcModelList[index]->loadStatus, vcMLS_Unloaded, vcMLS_Pending);

  while (pProgramState->vcModelList[index]->loadStatus == vcMLS_Loading)
    udYield(); // Spin until other thread stops processing

  if (pProgramState->vcModelList[index]->loadStatus == vcMLS_Loaded)
  {
    vdkModel_Unload(pProgramState->pVDKContext, &pProgramState->vcModelList[index]->pVDKModel);

    if (pProgramState->vcModelList[index]->pWatermark != nullptr)
      vcTexture_Destroy(&pProgramState->vcModelList[index]->pWatermark);

    pProgramState->vcModelList[index]->pMetadata->Destroy();
    udFree(pProgramState->vcModelList[index]->pMetadata);
    udFree(pProgramState->vcModelList[index]->pZone);
  }

  pProgramState->vcModelList[index]->loadStatus = vcMLS_Unloaded;

  udFree(pProgramState->vcModelList.at(index));
  pProgramState->vcModelList.erase(pProgramState->vcModelList.begin() + index);
}

void vcModel_UnloadList(vcState *pProgramState)
{
  while (pProgramState->vcModelList.size() > 0)
    vcModel_RemoveFromList(pProgramState, 0);
}

void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel)
{
  if (!pModel)
  {
    for (size_t i = 0; i < pProgramState->vcModelList.size(); ++i)
      vcModel_UpdateMatrix(pProgramState, pProgramState->vcModelList[i]);
  }
  else
  {
    udDouble4x4 matrix = pModel->baseMatrix;

    if (pModel->flipYZ)
    {
      udDouble4 rowz = -matrix.axis.y;
      matrix.axis.y = matrix.axis.z;
      matrix.axis.z = rowz;
    }

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

  pProgramState->pCamera->position = vcModel_GetPivotPointWorldSpace(pModel);

  return true;
}

udDouble3 vcModel_GetPivotPointWorldSpace(vcModel *pModel)
{
  udDouble3 midPoint = udDouble3::zero();

  if (pModel != nullptr)
    midPoint = (pModel->worldMatrix * udDouble4::create(pModel->pivot, 1.0)).toVector3();

  return midPoint;
}
