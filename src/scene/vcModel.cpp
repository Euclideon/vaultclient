#include "vcModel.h"

#include "vcScene.h"
#include "vcState.h"
#include "vcRender.h"

#include "gl/vcTexture.h"

#include "imgui.h"
#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "vdkPointCloud.h"

struct vcModelLoadInfo
{
  bool jumpToLocation;
  vcState *pProgramState;
  vcModel *pModel;

  bool usePosition;
  udDouble3 position;

  bool useRotation;
  udDouble3 rotation;

  double scale; // Always uses this one
};

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

  int32_t status = udInterlockedCompareExchange(&pLoadInfo->pModel->loadStatus, vcSLS_Loading, vcSLS_Pending);

  if (status == vcSLS_Pending)
  {
    vdkError modelStatus = vdkPointCloud_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->pPointCloud, pLoadInfo->pModel->pPath);

    if (modelStatus == vE_Success)
    {
      const char *pMetadata;
      pLoadInfo->pModel->pMetadata = udAllocType(udJSON, 1, udAF_Zero);

      if (vdkPointCloud_GetMetadata(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->pPointCloud, &pMetadata) == vE_Success)
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
              if (udStrEquali(tempNode.Get("values[%zu].type", i).AsString(), "AUTHORITY"))
              {
                srid = tempNode.Get("values[%zu].values[0]", i).AsInt();
                break;
              }
            }

            pLoadInfo->pModel->pMetadata->Set(&tempNode, "ProjectionWKT");
          }
        }

        if (srid != 0)
        {
          pLoadInfo->pModel->pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
          pLoadInfo->pModel->pZone = udAllocType(udGeoZone, 1, udAF_Zero);
          udGeoZone_SetFromSRID(pLoadInfo->pModel->pOriginalZone, srid);
          memcpy(pLoadInfo->pModel->pZone, pLoadInfo->pModel->pOriginalZone, sizeof(*pLoadInfo->pModel->pZone));
        }
      }

      vdkPointCloud_GetStoredMatrix(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->pPointCloud, pLoadInfo->pModel->sceneMatrix.a);
      pLoadInfo->pModel->defaultMatrix = pLoadInfo->pModel->sceneMatrix;

      udDouble3 scaleFactor = udDouble3::create(udMag3(pLoadInfo->pModel->sceneMatrix.axis.x), udMag3(pLoadInfo->pModel->sceneMatrix.axis.y), udMag3(pLoadInfo->pModel->sceneMatrix.axis.z)) * pLoadInfo->scale;
      udDouble3 translate = pLoadInfo->pModel->sceneMatrix.axis.t.toVector3();
      udDouble3 ypr = udDouble3::zero();

      if (pLoadInfo->useRotation)
        ypr = pLoadInfo->rotation;

      if (pLoadInfo->usePosition)
        translate = pLoadInfo->position;

      if (pLoadInfo->useRotation || pLoadInfo->usePosition || pLoadInfo->scale != 1.0)
        pLoadInfo->pModel->sceneMatrix = udDouble4x4::translation(translate) * udDouble4x4::translation(pLoadInfo->pModel->pivot) * udDouble4x4::rotationYPR(ypr) * udDouble4x4::scaleNonUniform(scaleFactor) * udDouble4x4::translation(-pLoadInfo->pModel->pivot);

      if (pLoadInfo->jumpToLocation)
        vcScene_UseProjectFromItem(pLoadInfo->pProgramState, pLoadInfo->pModel);
      else
        vcScene_UpdateItemToCurrentProjection(pLoadInfo->pProgramState, nullptr); // Set all model matrices

      pLoadInfo->pModel->loadStatus = vcSLS_Loaded;
    }
    else if (modelStatus == vE_OpenFailure)
    {
      pLoadInfo->pModel->loadStatus = vcSLS_OpenFailure;
    }
    else
    {
      pLoadInfo->pModel->loadStatus = vcSLS_Failed;
    }
  }
}

void vcModel::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  pRenderData->models.PushBack(this);
}

void vcModel::ApplyDelta(vcState * /*pProgramState*/)
{

}

void vcModel::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", pPath);

  if (pMetadata != nullptr)
    vcImGuiValueTreeObject(pMetadata);
}

void vcModel::Cleanup(vcState *pProgramState)
{
  vdkPointCloud_Unload(pProgramState->pVDKContext, &pPointCloud);

  udFree(pName);
  udFree(pPath);

  if (pWatermark != nullptr)
    vcTexture_Destroy(&pWatermark);

  this->vcModel::~vcModel();
}

void vcModel_AddToList(vcState *pProgramState, const char *pName, const char *pFilePath, bool jumpToModelOnLoad /*= true*/, udDouble3 *pOverridePosition /*= nullptr*/, udDouble3 *pOverrideYPR /*= nullptr*/, double scale /*= 1.0*/)
{
  if (pFilePath == nullptr)
    return;

  vcModel *pModel = udAllocType(vcModel, 1, udAF_Zero);
  pModel = new (pModel) vcModel();

  // Prepare the model
  pModel->pPath = udStrdup(pFilePath);

  if (pName == nullptr)
  {
    udFilename udfilename(pFilePath);
    pModel->pName = udStrdup(udfilename.GetFilenameWithExt());
  }
  else
  {
    pModel->pName = udStrdup(pName);
  }

  pModel->visible = true;
  pModel->type = vcSOT_PointCloud;

  udStrcpy(pModel->typeStr, sizeof(pModel->typeStr), "UDS");

  // Add it to the load queue
  pModel->AddItem(pProgramState);

  vcModelLoadInfo *pLoadInfo = udAllocType(vcModelLoadInfo, 1, udAF_Zero);
  if (pLoadInfo != nullptr)
  {
    // Prepare the load info
    pLoadInfo->pModel = pModel;
    pLoadInfo->pProgramState = pProgramState;
    pLoadInfo->jumpToLocation = jumpToModelOnLoad;

    if (pOverridePosition)
    {
      pLoadInfo->usePosition = true;
      pLoadInfo->position = *pOverridePosition;
    }

    if (pOverrideYPR)
    {
      pLoadInfo->useRotation = true;
      pLoadInfo->rotation = *pOverrideYPR;
    }

    pLoadInfo->scale = scale;

    // Queue for load
    vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModel_LoadModel, pLoadInfo);
  }
  else
  {
    pModel->loadStatus = vcSLS_Failed;
  }
}
