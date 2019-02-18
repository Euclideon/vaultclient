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

  int32_t status = udInterlockedCompareExchange(&pLoadInfo->pModel->m_loadStatus, vcSLS_Loading, vcSLS_Pending);

  if (status == vcSLS_Pending)
  {
    vdkError modelStatus = vdkPointCloud_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_pPath);

    if (modelStatus == vE_Success)
    {
      const char *pMetadata;
      pLoadInfo->pModel->m_pMetadata = udAllocType(udJSON, 1, udAF_Zero);

      if (vdkPointCloud_GetMetadata(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->m_pPointCloud, &pMetadata) == vE_Success)
      {
        pLoadInfo->pModel->m_pMetadata->Parse(pMetadata);

        pLoadInfo->pModel->m_hasWatermark = pLoadInfo->pModel->m_pMetadata->Get("Watermark").IsString();

        pLoadInfo->pModel->m_meterScale = pLoadInfo->pModel->m_pMetadata->Get("info.meterScale").AsDouble(1.0);
        pLoadInfo->pModel->m_pivot = pLoadInfo->pModel->m_pMetadata->Get("info.pivot").AsDouble3();

        vcSRID srid = 0;
        udJSON tempNode;

        const char *pSRID = pLoadInfo->pModel->m_pMetadata->Get("ProjectionID").AsString();
        if (pSRID != nullptr)
        {
          pSRID = udStrchr(pSRID, ":");
          if (pSRID != nullptr)
            srid = udStrAtou(&pSRID[1]);
        }

        const char *pWKT = pLoadInfo->pModel->m_pMetadata->Get("ProjectionWKT").AsString();
        if (pWKT != nullptr)
        {
          if (udParseWKT(&tempNode, pWKT) == udR_Success)
          {
            for (size_t i = 0; i < tempNode.Get("values").ArrayLength() && srid == 0; ++i)
            {
              if (udStrEquali(tempNode.Get("values[%zu].m_type", i).AsString(), "AUTHORITY"))
              {
                srid = tempNode.Get("values[%zu].values[0]", i).AsInt();
                break;
              }
            }

            pLoadInfo->pModel->m_pMetadata->Set(&tempNode, "ProjectionWKT");
          }
        }

        if (srid != 0)
        {
          pLoadInfo->pModel->m_pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
          pLoadInfo->pModel->m_pZone = udAllocType(udGeoZone, 1, udAF_Zero);
          udGeoZone_SetFromSRID(pLoadInfo->pModel->m_pOriginalZone, srid);
          memcpy(pLoadInfo->pModel->m_pZone, pLoadInfo->pModel->m_pOriginalZone, sizeof(*pLoadInfo->pModel->m_pZone));
        }
      }

      vdkPointCloud_GetStoredMatrix(pLoadInfo->pProgramState->pVDKContext, pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_sceneMatrix.a);
      pLoadInfo->pModel->m_defaultMatrix = pLoadInfo->pModel->m_sceneMatrix;

      udDouble3 scaleFactor = udDouble3::create(udMag3(pLoadInfo->pModel->m_sceneMatrix.axis.x), udMag3(pLoadInfo->pModel->m_sceneMatrix.axis.y), udMag3(pLoadInfo->pModel->m_sceneMatrix.axis.z)) * pLoadInfo->scale;
      udDouble3 translate = pLoadInfo->pModel->m_sceneMatrix.axis.t.toVector3();
      udDouble3 ypr = udDouble3::zero();

      if (pLoadInfo->useRotation)
        ypr = pLoadInfo->rotation;

      if (pLoadInfo->usePosition)
        translate = pLoadInfo->position;

      if (pLoadInfo->useRotation || pLoadInfo->usePosition || pLoadInfo->scale != 1.0)
        pLoadInfo->pModel->m_sceneMatrix = udDouble4x4::translation(translate) * udDouble4x4::translation(pLoadInfo->pModel->m_pivot) * udDouble4x4::rotationYPR(ypr) * udDouble4x4::scaleNonUniform(scaleFactor) * udDouble4x4::translation(-pLoadInfo->pModel->m_pivot);

      if (pLoadInfo->jumpToLocation)
        vcScene_UseProjectFromItem(pLoadInfo->pProgramState, pLoadInfo->pModel);
      else
        pLoadInfo->pModel->ChangeProjection(pLoadInfo->pProgramState, pLoadInfo->pProgramState->gis.zone);

      pLoadInfo->pModel->m_loadStatus = vcSLS_Loaded;
    }
    else if (modelStatus == vE_OpenFailure)
    {
      pLoadInfo->pModel->m_loadStatus = vcSLS_OpenFailure;
    }
    else
    {
      pLoadInfo->pModel->m_loadStatus = vcSLS_Failed;
    }
  }
}

void vcModel::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  pRenderData->models.PushBack(this);
}

void vcModel::ChangeProjection(vcState * /*pProgramState*/, const udGeoZone &newZone)
{
  // This is not ideal as it will gather drift
  if (this->m_pZone != nullptr)
    this->m_sceneMatrix = udGeoZone_TransformMatrix(this->m_sceneMatrix, *this->m_pZone, newZone);

  // Call the parent version
  vcSceneItem::ChangeProjection(nullptr, newZone);
}

void vcModel::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;
}

void vcModel::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pPath);

  if (m_pMetadata != nullptr)
    vcImGuiValueTreeObject(m_pMetadata);
}

void vcModel::Cleanup(vcState *pProgramState)
{
  vdkPointCloud_Unload(pProgramState->pVDKContext, &m_pPointCloud);

  udFree(m_pName);
  udFree(m_pPath);

  if (m_pWatermark != nullptr)
  {
    if (pProgramState->pSceneWatermark == m_pWatermark)
      pProgramState->pSceneWatermark = nullptr;

    vcTexture_Destroy(&m_pWatermark);
  }

  this->vcModel::~vcModel();
}

udDouble3 vcModel::GetLocalSpacePivot()
{
  return m_pivot;
}

udDouble4x4 vcModel::GetWorldSpaceMatrix()
{
  return m_sceneMatrix;
}

void vcModel_AddToList(vcState *pProgramState, const char *pName, const char *pFilePath, bool jumpToModelOnLoad /*= true*/, udDouble3 *pOverridePosition /*= nullptr*/, udDouble3 *pOverrideYPR /*= nullptr*/, double scale /*= 1.0*/)
{
  if (pFilePath == nullptr)
    return;

  vcModel *pModel = udAllocType(vcModel, 1, udAF_Zero);
  pModel = new (pModel) vcModel();

  // Prepare the model
  pModel->m_pPath = udStrdup(pFilePath);

  if (pName == nullptr)
  {
    udFilename udfilename(pFilePath);
    pModel->m_pName = udStrdup(udfilename.GetFilenameWithExt());
  }
  else
  {
    pModel->m_pName = udStrdup(pName);
  }

  pModel->m_visible = true;
  pModel->m_type = vcSOT_PointCloud;

  udStrcpy(pModel->m_typeStr, sizeof(pModel->m_typeStr), "UDS");

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
    pModel->m_loadStatus = vcSLS_Failed;
  }
}
