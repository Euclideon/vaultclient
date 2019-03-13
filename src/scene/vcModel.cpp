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

void vcModel_PostLoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

  if (pLoadInfo->pModel->m_loadStatus != vcSLS_Loaded)
    return;

  if (pLoadInfo->jumpToLocation)
    vcScene_UseProjectFromItem(pLoadInfo->pProgramState, pLoadInfo->pModel);
  else if (pLoadInfo->pProgramState->gis.isProjected)
    pLoadInfo->pModel->ChangeProjection(pLoadInfo->pProgramState, pLoadInfo->pProgramState->gis.zone);
}

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  udGeoZone *pMemberZone = nullptr;
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
        const char *pWKT = pLoadInfo->pModel->m_pMetadata->Get("ProjectionWKT").AsString();

        if (pSRID != nullptr)
        {
          pSRID = udStrchr(pSRID, ":");
          if (pSRID != nullptr)
            srid = udStrAtou(&pSRID[1]);

          pLoadInfo->pModel->m_pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
          pMemberZone = udAllocType(udGeoZone, 1, udAF_Zero);
          if (udGeoZone_SetFromSRID(pLoadInfo->pModel->m_pOriginalZone, srid) == udR_Success)
          {
            memcpy(pMemberZone, pLoadInfo->pModel->m_pOriginalZone, sizeof(*pMemberZone));
          }
          else
          {
            udFree(pLoadInfo->pModel->m_pOriginalZone);
            udFree(pMemberZone);
          }
        }

        if (pLoadInfo->pModel->m_pOriginalZone == nullptr && pMemberZone == nullptr && pWKT != nullptr)
        {
          pLoadInfo->pModel->m_pOriginalZone = udAllocType(udGeoZone, 1, udAF_Zero);
          pMemberZone = udAllocType(udGeoZone, 1, udAF_Zero);

          udGeoZone_SetFromWKT(pLoadInfo->pModel->m_pOriginalZone, pWKT);

          memcpy(pMemberZone, pLoadInfo->pModel->m_pOriginalZone, sizeof(*pMemberZone));
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

      if (pMemberZone)
      {
        pLoadInfo->pModel->m_pZone = pMemberZone;
        pMemberZone = nullptr;
      }

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
  udFree(pMemberZone);
}

vcModel::vcModel(vcState *pProgramState, const char *pName, const char *pFilePath, bool jumpToModelOnLoad /*= true*/, udDouble3 *pOverridePosition /*= nullptr*/, udDouble3 *pOverrideYPR /*= nullptr*/, double scale /*= 1.0*/) :
  m_pPointCloud(nullptr), m_pivot(udDouble3::zero()), m_defaultMatrix(udDouble4x4::identity()), m_sceneMatrix(udDouble4x4::identity()),
  m_meterScale(0.0), m_hasWatermark(false), m_pWatermark(nullptr)
{
  if (pFilePath == nullptr)
    return;

  // Prepare the model
  m_pPath = udStrdup(pFilePath);

  if (pName == nullptr)
  {
    udFilename udfilename(pFilePath);
    m_pName = udStrdup(udfilename.GetFilenameWithExt());
  }
  else
  {
    m_pName = udStrdup(pName);
  }

  m_visible = true;
  m_type = vcSOT_PointCloud;

  udStrcpy(m_typeStr, sizeof(m_typeStr), "UDS");

  vcModelLoadInfo *pLoadInfo = udAllocType(vcModelLoadInfo, 1, udAF_Zero);
  if (pLoadInfo != nullptr)
  {
    // Prepare the load info
    pLoadInfo->pModel = this;
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
    vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModel_LoadModel, pLoadInfo, true, vcModel_PostLoadModel);
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }
}

void vcModel::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  pRenderData->models.PushBack(this);
}

void vcModel::ChangeProjection(vcState * /*pProgramState*/, const udGeoZone &newZone)
{
  // This is not ideal as it will gather drift
  if (m_pZone != nullptr)
    m_sceneMatrix = udGeoZone_TransformMatrix(this->m_sceneMatrix, *m_pZone, newZone);

  // Call the parent version
  vcSceneItem::ChangeProjection(nullptr, newZone);
}

void vcModel::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;
  m_moved = true;
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
}

udDouble3 vcModel::GetLocalSpacePivot()
{
  return m_pivot;
}

udDouble4x4 vcModel::GetWorldSpaceMatrix()
{
  return m_sceneMatrix;
}
