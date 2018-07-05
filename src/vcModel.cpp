#include "vcModel.h"

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

  if (pSRID != nullptr)
  {
    pSRID = udStrchr(pSRID, ":");
    if (pSRID != nullptr)
      pProgramState->currentSRID = (uint16_t)udStrAtou(&pSRID[1]);
    else
      pProgramState->currentSRID = 0;
  }
  else if (pWKT != nullptr)
  {
    // Not sure?
  }
  else //No SRID available so set back to no projection
  {
    pProgramState->currentSRID = 0;
  }

  return true;
}
