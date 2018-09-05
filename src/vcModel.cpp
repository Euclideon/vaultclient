#include "vcModel.h"
#include "udPlatform/udGeoZone.h"

bool vcModel_AddToList(vcState *pProgramState, const char *pFilePath)
{
  if (pFilePath == nullptr)
    return false;

  vcModel model = {};
  model.modelLoaded = true;
  model.modelVisible = true;
  model.modelSelected = false;
  model.pMetadata = udAllocType(udValue, 1, udAF_Zero);

  udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pFilePath);

  if (vdkModel_Load(pProgramState->pVDKContext, &model.pVaultModel, pFilePath) == vE_Success)
  {
    const char *pMetadata;
    if (vdkModel_GetMetadata(pProgramState->pVDKContext, model.pVaultModel, &pMetadata) == vE_Success)
      model.pMetadata->Parse(pMetadata);

    vcModel_MoveToModelProjection(pProgramState, &model);
    pProgramState->camMatrix = vcCamera_GetMatrix(pProgramState->pCamera); // eh?

    pProgramState->vcModelList.PushBack(model);

    vcModel_UpdateMatrix(pProgramState, nullptr); // Set all model matrices

    return true;
  }

  return false;
}

bool vcModel_UnloadList(vcState *pProgramState)
{
  vdkError err;
  for (int i = 0; i < (int)pProgramState->vcModelList.length; i++)
  {
    vdkModel *pVaultModel;
    pVaultModel = pProgramState->vcModelList[i].pVaultModel;
    err = vdkModel_Unload(pProgramState->pVDKContext, &pVaultModel);
    pProgramState->vcModelList[i].pMetadata->Destroy();
    udFree(pProgramState->vcModelList[i].pMetadata);
    if (err != vE_Success)
      return false;
    pProgramState->vcModelList[i].modelLoaded = false;
  }

  while (pProgramState->vcModelList.length > 0)
    pProgramState->vcModelList.PopFront();

  return true;
}

void vcModel_UpdateMatrix(vcState *pProgramState, vcModel *pModel)
{
  if (!pModel)
  {
    for (size_t i = 0; i < pProgramState->vcModelList.length; ++i)
      vcModel_UpdateMatrix(pProgramState, &pProgramState->vcModelList[i]);
  }
  else
  {
    udDouble4x4 matrix;
    vdkModel_GetLocalMatrix(pProgramState->pVDKContext, pModel->pVaultModel, matrix.a);

    if (pModel->flipYZ)
    {
      udDouble4 rowz = -matrix.axis.y;
      matrix.axis.y = matrix.axis.z;
      matrix.axis.z = rowz;
    }

    // Handle transforming into the camera's GeoZone
    if (pProgramState->gis.isProjected)
    {
      uint16_t modelSRID = vcModel_GetSRID(pProgramState, pModel);
      udGeoZone fromZone;

      if (pProgramState->gis.SRID != modelSRID && modelSRID != 0 && udGeoZone_SetFromSRID(&fromZone, modelSRID) == udR_Success)
        matrix = udGeoZone_TransformMatrix(matrix, fromZone, pProgramState->gis.zone);
    }

    vdkModel_SetWorldMatrix(pProgramState->pVDKContext, pModel->pVaultModel, matrix.a);
  }
}

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  if (vcGIS_ChangeSpace(&pProgramState->gis, vcModel_GetSRID(pProgramState, pModel)))
    vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone

  pProgramState->pCamera->position = vcModel_GetMidPointLocalSpace(pProgramState, pModel);

  return true;
}

udDouble3 vcModel_GetMidPointLocalSpace(vcState *pProgramState, vcModel *pModel)
{
  udDouble3 midPoint = udDouble3::zero();

  if (pModel != nullptr)
    vdkModel_GetModelCenter(pProgramState->pVDKContext, pModel->pVaultModel, &midPoint.x);

  return midPoint;
}

uint16_t vcModel_GetSRID(vcState * /*pProgramState*/, vcModel *pModel)
{
  if (pModel == nullptr || pModel->pMetadata == nullptr)
    return 0;

  const char *pSRID = pModel->pMetadata->Get("ProjectionID").AsString();
  const char *pWKT = pModel->pMetadata->Get("ProjectionWKT").AsString();

  if (pSRID != nullptr)
  {
    pSRID = udStrchr(pSRID, ":");
    if (pSRID != nullptr)
      return (vcSRID)udStrAtou(&pSRID[1]);
  }
  else if (pWKT != nullptr)
  {
    // Not sure?
  }

  return 0;
}
