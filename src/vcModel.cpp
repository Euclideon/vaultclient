#include "vcModel.h"

#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udGeoZone.h"
#include "udPlatform/udJSON.h"

#include "gl/vcTexture.h"

#include "vdkModel.h"

#include "stb_image.h"

bool vcModel_AddToList(vcState *pProgramState, const char *pFilePath)
{
  if (pFilePath == nullptr)
    return false;

  vcModel model = {};
  model.modelLoaded = true;
  model.modelVisible = true;
  model.modelSelected = false;

  udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pFilePath);

  if (vdkModel_Load(pProgramState->pVDKContext, &model.pVaultModel, pFilePath) == vE_Success)
  {
    const char *pMetadata;
    model.pMetadata = udAllocType(udJSON, 1, udAF_Zero);
    if (vdkModel_GetMetadata(pProgramState->pVDKContext, model.pVaultModel, &pMetadata) == vE_Success)
    {
      model.pMetadata->Parse(pMetadata);

      const char *pWatermark = model.pMetadata->Get("Watermark").AsString();
      if (pWatermark)
      {
        uint8_t *pImage = nullptr;
        size_t imageLen = 0;
        if (udBase64Decode(&pImage, &imageLen, pWatermark) == udR_Success)
        {
          int imageWidth, imageHeight, imageChannels;
          unsigned char *pImageData = stbi_load_from_memory(pImage, (int)imageLen, &imageWidth, &imageHeight, &imageChannels, 4);
          vcTexture_Create(&model.pWatermark, imageWidth, imageHeight, pImageData, vcTextureFormat_RGBA8, vcTFM_Nearest, false);
          free(pImageData);
        }

        udFree(pImage);
      }


      vcSRID srid = 0;
      udJSON tempNode;

      const char *pSRID = model.pMetadata->Get("ProjectionID").AsString();
      if (pSRID != nullptr)
      {
        pSRID = udStrchr(pSRID, ":");
        if (pSRID != nullptr)
          srid = udStrAtou(&pSRID[1]);
      }

      const char *pWKT = model.pMetadata->Get("ProjectionWKT").AsString();
      if (pWKT != nullptr && udParseWKT(&tempNode, pWKT) == udR_Success)
      {
        for (size_t i = 0; i < tempNode.Get("values").ArrayLength(); ++i)
        {
          if (udStrEquali(tempNode.Get("values[%llu].type", i).AsString(), "AUTHORITY"))
          {
            srid = tempNode.Get("values[%llu].values[0]", i).AsInt();
            break;
          }
        }

        model.pMetadata->Set(&tempNode, "ProjectionWKT");
      }

      if (srid != 0)
      {
        model.pZone = udAllocType(udGeoZone, 1, udAF_Zero);
        udGeoZone_SetFromSRID(model.pZone, srid);
      }
    }

    vcModel_MoveToModelProjection(pProgramState, &model);
    pProgramState->camMatrix = vcCamera_GetMatrix(pProgramState->pCamera); // eh?

    pProgramState->vcModelList.PushBack(model);

    vcModel_UpdateMatrix(pProgramState, nullptr); // Set all model matrices

    return true;
  }

  return false;
}

bool vcModel_RemoveFromList(vcState *pProgramState, size_t index)
{
  vdkModel *pVaultModel = pProgramState->vcModelList[index].pVaultModel;
  vdkError err = vdkModel_Unload(pProgramState->pVDKContext, &pVaultModel);

  if (pProgramState->vcModelList[index].pWatermark != nullptr)
    vcTexture_Destroy(&pProgramState->vcModelList[index].pWatermark);

  pProgramState->vcModelList[index].pMetadata->Destroy();
  udFree(pProgramState->vcModelList[index].pMetadata);
  udFree(pProgramState->vcModelList[index].pZone);
  pProgramState->vcModelList[index].modelLoaded = false;
  pProgramState->vcModelList.RemoveAt(index);
  return err == vE_Success;
}

bool vcModel_UnloadList(vcState *pProgramState)
{
  bool success = true;
  while (pProgramState->vcModelList.length > 0)
    success = (vcModel_RemoveFromList(pProgramState, 0) && success);

  return success;
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
    if (pProgramState->gis.isProjected && pModel->pZone != nullptr && pProgramState->gis.SRID != pModel->pZone->srid)
      matrix = udGeoZone_TransformMatrix(matrix, *pModel->pZone, pProgramState->gis.zone);

    vdkModel_SetWorldMatrix(pProgramState->pVDKContext, pModel->pVaultModel, matrix.a);
    pModel->worldMatrix = matrix;
  }
}

bool vcModel_MoveToModelProjection(vcState *pProgramState, vcModel *pModel)
{
  if (pProgramState == nullptr || pModel == nullptr)
    return false;

  if (pModel->pZone != nullptr && vcGIS_ChangeSpace(&pProgramState->gis, pModel->pZone->srid))
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
