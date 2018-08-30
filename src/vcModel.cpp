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

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  double midPoint[3];
  vdkModel_GetModelCenter(pProgramState->pVDKContext, pModel->pVaultModel, midPoint);
  pProgramState->pCamera->position = udDouble3::create(midPoint[0], midPoint[1], midPoint[2]);

  const char *pSRID = pModel->pMetadata->Get("ProjectionID").AsString();
  const char *pWKT = pModel->pMetadata->Get("ProjectionWKT").AsString();

  vcSRID newSRID = 0;

  if (pSRID != nullptr)
  {
    pSRID = udStrchr(pSRID, ":");
    if (pSRID != nullptr)
      newSRID = (uint16_t)udStrAtou(&pSRID[1]);
  }
  else if (pWKT != nullptr)
  {
    // TODO: Implement this properly
  }

  return vcGIS_ChangeSpace(&pProgramState->gis, newSRID);
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
    // TODO: Store the srid and zone at load time
    const char *pSRID = udStrchr(pModel->pMetadata->Get("ProjectionID").AsString(), ":");
    udGeoZone fromZone, newZone;
    if (pSRID && udGeoZone_SetFromSRID(&fromZone, udStrAtoi(pSRID + 1)) == udR_Success)
    {
      udGeoZone_SetFromSRID(&newZone, pProgramState->currentSRID);
      matrix = udGeoZone_TransformMatrix(matrix, fromZone, newZone);
    }

    vdkModel_SetWorldMatrix(pProgramState->pVDKContext, pModel->pVaultModel, matrix.a);
  }
}
