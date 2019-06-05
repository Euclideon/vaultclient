#include "vcModel.h"

#include "vcState.h"
#include "vcRender.h"

#include "gl/vcTexture.h"

#include "imgui.h"
#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "vdkPointCloud.h"

#include "udStringUtil.h"

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
    vcProject_UseProjectionFromItem(pLoadInfo->pProgramState, pLoadInfo->pModel);
  else if (pLoadInfo->pProgramState->gis.isProjected)
    pLoadInfo->pModel->ChangeProjection(pLoadInfo->pProgramState->gis.zone);

  // TODO: (EVC-535) This works - but is sub-optimal
  // Not sure exactly how to handle auto-assignment...
  pLoadInfo->pProgramState->settings.visualization.minIntensity = udMax(pLoadInfo->pProgramState->settings.visualization.minIntensity, pLoadInfo->pModel->minMaxIntensity.x);
  pLoadInfo->pProgramState->settings.visualization.maxIntensity = udMin(pLoadInfo->pProgramState->settings.visualization.maxIntensity, pLoadInfo->pModel->minMaxIntensity.y);
}

void vcModel_LoadMetadata(vcState *pProgramState, vcModel *pModel, double scale, udDouble3 *pPosition = nullptr, udDouble3 *pRotation = nullptr)
{
  udGeoZone *pMemberZone = nullptr;
  const char *pMetadata;

  if (vdkPointCloud_GetMetadata(pProgramState->pVDKContext, pModel->m_pPointCloud, &pMetadata) == vE_Success)
  {
    pModel->m_metadata.Parse(pMetadata);
    pModel->m_hasWatermark = pModel->m_metadata.Get("Watermark").IsString();

    pModel->m_meterScale = pModel->m_metadata.Get("info.meterScale").AsDouble(1.0);
    pModel->m_pivot = pModel->m_metadata.Get("info.pivot").AsDouble3();

    vcSRID srid = 0;
    const char *pSRID = pModel->m_metadata.Get("ProjectionID").AsString();
    const char *pWKT = pModel->m_metadata.Get("ProjectionWKT").AsString();

    if (pSRID != nullptr)
    {
      pSRID = udStrchr(pSRID, ":");
      if (pSRID != nullptr)
        srid = udStrAtou(&pSRID[1]);

      pModel->m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
      if (udGeoZone_SetFromSRID(pModel->m_pPreferredProjection, srid) == udR_Success)
      {
        pMemberZone = udAllocType(udGeoZone, 1, udAF_Zero);
        memcpy(pMemberZone, pModel->m_pPreferredProjection, sizeof(*pMemberZone));
      }
      else
      {
        udFree(pModel->m_pPreferredProjection);
      }
    }

    if (pModel->m_pPreferredProjection == nullptr && pMemberZone == nullptr && pWKT != nullptr)
    {
      pModel->m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
      pMemberZone = udAllocType(udGeoZone, 1, udAF_Zero);

      udGeoZone_SetFromWKT(pModel->m_pPreferredProjection, pWKT);

      memcpy(pMemberZone, pModel->m_pPreferredProjection, sizeof(*pMemberZone));
    }

    // TODO: (EVC-535) This works, but this data is already extracted and stored in
    // `udOctreeUDS`... but we don't have access to that data?
    const char *pMinMaxIntensity = pModel->m_metadata.Get("AttrMinMax_udIntensity").AsString("0, 65535");
    int charCount = 0;
    pModel->minMaxIntensity.x = (uint16_t)udStrAtoi(pMinMaxIntensity, &charCount);
    pModel->minMaxIntensity.y = (uint16_t)(udStrAtoi(pMinMaxIntensity + charCount + 1));
  }

  vdkPointCloud_GetStoredMatrix(pProgramState->pVDKContext, pModel->m_pPointCloud, pModel->m_sceneMatrix.a);
  pModel->m_defaultMatrix = pModel->m_sceneMatrix;

  udDouble3 scaleFactor = udDouble3::create(udMag3(pModel->m_sceneMatrix.axis.x), udMag3(pModel->m_sceneMatrix.axis.y), udMag3(pModel->m_sceneMatrix.axis.z)) * scale;
  udDouble3 translate = pModel->m_sceneMatrix.axis.t.toVector3();
  udDouble3 ypr = udDouble3::zero();

  if (pRotation != nullptr)
    ypr = *pRotation;

  if (pPosition != nullptr)
    translate = *pPosition;

  if (pRotation != nullptr || pPosition != nullptr || scale != 1.0)
    pModel->m_sceneMatrix = udDouble4x4::translation(translate) * udDouble4x4::translation(pModel->m_pivot) * udDouble4x4::rotationYPR(ypr) * udDouble4x4::scaleNonUniform(scaleFactor) * udDouble4x4::translation(-pModel->m_pivot);

  if (pMemberZone)
  {
    vdkProjectNode_SetGeometry(pProgramState->sceneExplorer.pProject, pModel->m_pNode, vdkPGT_Point, 1, (double*)&translate);
    pModel->m_pCurrentProjection = pMemberZone;
    pMemberZone = nullptr;
  }

  udFree(pMemberZone);
}

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

  int32_t status = udInterlockedCompareExchange(&pLoadInfo->pModel->m_loadStatus, vcSLS_Loading, vcSLS_Pending);

  if (status == vcSLS_Pending)
  {
    vdkError modelStatus = vdkPointCloud_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_pNode->pURI);

    if (modelStatus == vE_Success)
    {
      vcModel_LoadMetadata(pLoadInfo->pProgramState, pLoadInfo->pModel, pLoadInfo->scale, pLoadInfo->usePosition ? &pLoadInfo->position : nullptr, pLoadInfo->useRotation ? &pLoadInfo->rotation : nullptr);

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

vcModel::vcModel(vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pNode, pProgramState),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_defaultMatrix(udDouble4x4::identity()),
  m_sceneMatrix(udDouble4x4::identity()),
  m_meterScale(0.0),
  m_hasWatermark(false),
  m_pWatermark(nullptr)
{
  const char *pFilePath = pNode->pURI;

  if (pFilePath == nullptr)
    return; // Can't load a model if we don't have the URL

  udFilename udfilename(pFilePath);

  vcModelLoadInfo *pLoadInfo = udAllocType(vcModelLoadInfo, 1, udAF_Zero);
  if (pLoadInfo != nullptr)
  {
    // Prepare the load info
    pLoadInfo->pModel = this;
    pLoadInfo->pProgramState = pProgramState;
    pLoadInfo->jumpToLocation = false;//jumpToModelOnLoad;

#if 0
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
#endif

    pLoadInfo->scale = 1.f;//scale;

                           // Queue for load
    vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModel_LoadModel, pLoadInfo, true, vcModel_PostLoadModel);
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }
}

vcModel::vcModel(vcState *pProgramState, const char *pName, vdkPointCloud *pCloud, bool jumpToModelOnLoad /*= false*/) :
  vcSceneItem(pProgramState, "UDS", pName),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_defaultMatrix(udDouble4x4::identity()),
  m_sceneMatrix(udDouble4x4::identity()),
  m_meterScale(0.0),
  m_hasWatermark(false),
  m_pWatermark(nullptr)
{
  m_pPointCloud = pCloud;

  //TODO: Update Name
  //if (pName == nullptr)
  //  m_pName = udStrdup("<?>");
  //else
  //  m_pName = udStrdup(pName);

  m_loadStatus = vcSLS_Loaded;

  vcModel_LoadMetadata(pProgramState, this, 1.0);

  if (jumpToModelOnLoad)
    vcProject_UseProjectionFromItem(pProgramState, this);
  else if (pProgramState->gis.isProjected)
    ChangeProjection(pProgramState->gis.zone);

  m_pNode->pUserData = this;
}

void vcModel::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  pRenderData->models.PushBack(this);
}

void vcModel::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pCurrentProjection != nullptr && newZone.srid == m_pCurrentProjection->srid)
    return;

  udDouble3 itemPos;

  if (m_pCurrentProjection == nullptr)
    itemPos = udGeoZone_ToLatLong(newZone, GetWorldSpacePivot());
  else
    itemPos = *(udDouble3*)m_pNode->pCoordinates;

  // If min == max then there are no bounds
  if (newZone.latLongBoundMin != newZone.latLongBoundMax)
  {
    if (itemPos.y > newZone.latLongBoundMax.x || itemPos.y < newZone.latLongBoundMin.x || itemPos.x > newZone.latLongBoundMax.y || itemPos.x < newZone.latLongBoundMin.y)
      return;
  }

  if (m_pCurrentProjection == nullptr)
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);

  memcpy(m_pCurrentProjection, &newZone, sizeof(udGeoZone));

  // This is not ideal as it will gather drift
  m_sceneMatrix = udGeoZone_TransformMatrix(m_sceneMatrix, *m_pCurrentProjection, newZone);
}

void vcModel::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;
  m_moved = true;
}

void vcModel::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);
  vcImGuiValueTreeObject(&m_metadata);
}

void vcModel::Cleanup(vcState *pProgramState)
{
  vdkPointCloud_Unload(pProgramState->pVDKContext, &m_pPointCloud);

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
