#include "vcModel.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcBPA.h"
#include "vcStringFormat.h"
#include "vcModals.h"
#include "vcQueryNode.h"
#include "vcCrossSectionNode.h"
#include "vcSettingsUI.h"
#include "vcClassificationColours.h"

#include "vcTexture.h"

#include "imgui.h"
#include "imgui_internal.h" // Required for ButtonEx

#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "udPointCloud.h"

#include "udStringUtil.h"
#include "udMath.h"
#include "vcError.h"

struct vcModelLoadInfo
{
  vcState *pProgramState;
  vcModel *pModel;
  udDouble3 possibleLocation;
};

void vcModel_LoadMetadata(vcState *pProgramState, vcModel *pModel, udDouble3 possibleLocation)
{
  const char *pMetadata;

  if (udPointCloud_GetMetadata(pModel->m_pPointCloud, &pMetadata) == udE_Success)
  {
    pModel->m_metadata.Parse(pMetadata);
    pModel->m_hasWatermark = pModel->m_metadata.Get("Watermark").IsString();

    pModel->m_meterScale = pModel->m_pointCloudHeader.unitMeterScale;
    pModel->m_pivot = udDouble3::create(pModel->m_pointCloudHeader.pivot[0], pModel->m_pointCloudHeader.pivot[1], pModel->m_pointCloudHeader.pivot[2]);

    int32_t srid = 0;
    const char *pSRID = pModel->m_metadata.Get("ProjectionID").AsString();
    const char *pWKT = pModel->m_metadata.Get("ProjectionWKT").AsString();

    if (pSRID != nullptr)
    {
      pSRID = udStrchr(pSRID, ":");
      if (pSRID != nullptr)
        srid = udStrAtou(&pSRID[1]);

      pModel->m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
      if (udGeoZone_SetFromSRID(pModel->m_pPreferredProjection, srid) != udR_Success)
        udFree(pModel->m_pPreferredProjection);
    }

    if (pModel->m_pPreferredProjection == nullptr && pWKT != nullptr)
    {
      pModel->m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);

      if (udGeoZone_SetFromWKT(pModel->m_pPreferredProjection, pWKT) != udR_Success)
        udFree(pModel->m_pPreferredProjection);
    }

  }

  pModel->m_sceneMatrix = udDouble4x4::create(pModel->m_pointCloudHeader.storedMatrix);
  pModel->m_defaultMatrix = pModel->m_sceneMatrix;
  pModel->m_baseMatrix = pModel->m_defaultMatrix;

  if (pModel->m_pPreferredProjection == nullptr && possibleLocation != udDouble3::zero())
    vcProject_UpdateNodeGeometryFromCartesian(pModel->m_pProject, pModel->m_pNode, pProgramState->geozone, udPGT_Point, &possibleLocation, 1);

  pModel->OnNodeUpdate(pProgramState);

  if (pModel->m_pPreferredProjection)
    pModel->m_changeZones = true;
}

void vcModel_LoadModel(void *pLoadInfoPtr)
{
  vcModelLoadInfo *pLoadInfo = (vcModelLoadInfo*)pLoadInfoPtr;
  if (pLoadInfo->pProgramState->programComplete)
    return;

  int32_t status = udInterlockedCompareExchange(&pLoadInfo->pModel->m_loadStatus, vcSLS_Loading, vcSLS_Pending);

  if (status == vcSLS_Pending)
  {
    udError modelStatus = udPointCloud_Load(pLoadInfo->pProgramState->pUDSDKContext, &pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_pNode->pURI, &pLoadInfo->pModel->m_pointCloudHeader);

    if (modelStatus == udE_OpenFailure)
      modelStatus = udPointCloud_Load(pLoadInfo->pProgramState->pUDSDKContext, &pLoadInfo->pModel->m_pPointCloud, udTempStr("%s%s", pLoadInfo->pProgramState->activeProject.pRelativeBase, pLoadInfo->pModel->m_pNode->pURI), &pLoadInfo->pModel->m_pointCloudHeader);

    if (modelStatus != udE_Success)
    {
      const char dropboxPrefix[] = "https://www.dropbox.com/";
      if (udStrBeginsWithi(pLoadInfo->pModel->m_pNode->pURI, dropboxPrefix))
      {
        const char *pNewURL = nullptr;
        udSprintf(&pNewURL, "https://dl.dropboxusercontent.com/%s", pLoadInfo->pModel->m_pNode->pURI + udLengthOf(dropboxPrefix) - 1);
        if (vcModals_DropboxHelp(pLoadInfo->pProgramState, pLoadInfo->pModel->m_pNode->pURI, pNewURL))
        {
          udProjectNode_SetURI(pLoadInfo->pModel->m_pProject->pProject, pLoadInfo->pModel->m_pNode, pNewURL);
          pNewURL = nullptr;
          modelStatus = udPointCloud_Load(pLoadInfo->pProgramState->pUDSDKContext, &pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_pNode->pURI, &pLoadInfo->pModel->m_pointCloudHeader);
        }
        udFree(pNewURL);
      }
    }

    if (modelStatus == udE_Success)
    {
      vcModel_LoadMetadata(pLoadInfo->pProgramState, pLoadInfo->pModel, pLoadInfo->possibleLocation);
      pLoadInfo->pModel->m_loadStatus = vcSLS_Loaded;
    }
    else if (modelStatus == udE_OpenFailure)
    {
      pLoadInfo->pModel->m_loadStatus = vcSLS_OpenFailure;
    }
    else
    {
      pLoadInfo->pModel->m_loadStatus = vcSLS_Failed;
    }
  }
}

vcModel::vcModel(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_defaultMatrix(udDouble4x4::identity()),
  m_pCurrentZone(nullptr),
  m_sceneMatrix(udDouble4x4::identity()),
  m_baseMatrix(udDouble4x4::identity()),
  m_changeZones(false),
  m_meterScale(0.0),
  m_hasWatermark(false),
  m_pWatermark(nullptr),
  m_visualization()
{
  Init(pProgramState);

  vcModelLoadInfo *pLoadInfo = udAllocType(vcModelLoadInfo, 1, udAF_Zero);
  if (pLoadInfo != nullptr)
  {
    // Prepare the load info
    pLoadInfo->pModel = this;
    pLoadInfo->pProgramState = pProgramState;

    if (!pNode->hasBoundingBox)
      pLoadInfo->possibleLocation = udDouble3::create(pNode->boundingBox[0], pNode->boundingBox[1], pNode->boundingBox[2]);
    else
      pLoadInfo->possibleLocation = udDouble3::zero();

    // Queue for load
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcModel_LoadModel, pLoadInfo, true);
  }
  else
  {
    m_loadStatus = vcSLS_Failed;
  }
}

vcModel::vcModel(vcState *pProgramState, const char *pName, udPointCloud *pCloud) :
  vcSceneItem(pProgramState, "UDS", pName),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_defaultMatrix(udDouble4x4::identity()),
  m_pCurrentZone(nullptr),
  m_sceneMatrix(udDouble4x4::identity()),
  m_baseMatrix(udDouble4x4::identity()),
  m_changeZones(false),
  m_meterScale(0.0),
  m_hasWatermark(false),
  m_pWatermark(nullptr),
  m_visualization()
{
  Init(pProgramState);

  m_pPointCloud = pCloud;
  m_loadStatus = vcSLS_Loaded;

  vcModel_LoadMetadata(pProgramState, this, udDouble3::zero());

  m_pNode->pUserData = this;
 }

void vcModel::Init(vcState *pProgramState)
{
  //Visualisations...
  memcpy(m_visualization.customClassificationColors, GeoverseClassificationColours, sizeof(m_visualization.customClassificationColors));

  //TODO These should live in m_pNode, but I don't think the api exists yet to manipulate arrays.
  m_visualization.pointSourceID.colourMap.Init(32);
  for (size_t i = 0; i < pProgramState->settings.visualization.pointSourceID.colourMap.length; i++)
    m_visualization.pointSourceID.colourMap.PushBack(pProgramState->settings.visualization.pointSourceID.colourMap[i]);
}

void vcModel::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (m_pPointCloud == nullptr)
    return; // Nothing else we can do yet

  udError status = udPointCloud_GetStreamingStatus(m_pPointCloud);
  if (status != udE_Success)
  {
    if (status == udE_ParseError)
      m_pActiveWarningStatus = "sceneExplorerErrorCorrupt";
    else if (status == udE_OpenFailure)
      m_pActiveWarningStatus = "sceneExplorerErrorOpen";
    else
      m_pActiveWarningStatus = "sceneExplorerErrorLoad";
  }

  if (m_changeZones)
    ChangeProjection(pProgramState->geozone);

  pRenderData->models.PushBack(this);
}

void vcModel::OnNodeUpdate(vcState *pProgramState)
{
  udDouble3 *pPosition = nullptr;
  double scale;
  udDouble3 euler;

  if (m_pNode->geomCount != 0)
  {
    if (m_pCurrentZone != nullptr)
    {
      vcProject_FetchNodeGeometryAsCartesian(&pProgramState->activeProject, m_pNode, *m_pCurrentZone, &pPosition, nullptr);
    }
    else if (m_pPreferredProjection != nullptr)
    {
      vcProject_FetchNodeGeometryAsCartesian(&pProgramState->activeProject, m_pNode, *m_pPreferredProjection, &pPosition, nullptr);
      m_pCurrentZone = (udGeoZone*)udMemDup(m_pPreferredProjection, sizeof(udGeoZone), 0, udAF_None);
    }
    else
    {
      vcProject_FetchNodeGeometryAsCartesian(&pProgramState->activeProject, m_pNode, pProgramState->geozone, &pPosition, nullptr);
      m_pCurrentZone = (udGeoZone*)udMemDup(&pProgramState->geozone, sizeof(udGeoZone), 0, udAF_None);
    }

    udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &euler.x, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &euler.y, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.r", &euler.z, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, "transform.scale", &scale, udMag3(m_baseMatrix.axis.x));

    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationYPR(UD_DEG2RAD(euler), *pPosition) * udDouble4x4::scaleUniform(scale) * udDouble4x4::translation(-m_pivot);
  }

  udFree(pPosition);

  int mode = 0;
  udProjectNode_GetMetadataInt(m_pNode, "visualization.mode", &mode, 0);
  m_visualization.mode = (vcVisualizatationMode)mode;

  udProjectNode_GetMetadataInt(m_pNode, "visualization.minIntensity", &m_visualization.minIntensity, -1);
  udProjectNode_GetMetadataInt(m_pNode, "visualization.maxIntensity", &m_visualization.maxIntensity, -1);
  if (m_visualization.minIntensity == -1 && m_visualization.maxIntensity == -1)
  {
    const char *pIntensity = m_metadata.Get("AttrMinMax_udIntensity").AsString();
    if (pIntensity != nullptr && udStrcmp(pIntensity, "") != 0)
    {
      char pStart[128] = "";
      udStrcpy(pStart, pIntensity);
      char *pIntensityArray[2];
      udStrTokenSplit(pStart, ",", pIntensityArray, 2);

      if (pIntensityArray[0] != nullptr && udStrcmp(pIntensityArray[0], "") != 0)
        m_visualization.minIntensity = udStrAtoi(pIntensityArray[0]);

      if (pIntensityArray[1] != nullptr && udStrcmp(pIntensityArray[1], "") != 0)
        m_visualization.maxIntensity = udStrAtoi(pIntensityArray[1]);
    }
    else
    {
      m_visualization.minIntensity = 0;
      m_visualization.maxIntensity = 0;
    }
  }

  udDouble2 displacement;
  udProjectNode_GetMetadataDouble(m_pNode, "visualization.displacement.x", &displacement.x, 0.0);
  udProjectNode_GetMetadataDouble(m_pNode, "visualization.displacement.y", &displacement.y, 0.0);
  if (displacement.x == 0.0 && displacement.y == 0.0)
  {
    const char *pDisplacement = m_metadata.Get("AttrMinMax_udDisplacement").AsString();
    if (pDisplacement != nullptr && udStrcmp(pDisplacement, "") != 0)
    {
      char pStart[128] = "";
      udStrcpy(pStart, pDisplacement);
      char *pDisplacementArray[2];
      udStrTokenSplit(pStart, ",", pDisplacementArray, 2);

      if (pDisplacementArray[0] != nullptr && udStrcmp(pDisplacementArray[0], "") != 0)
        displacement.x = udStrAtof64(pDisplacementArray[0]);

      if (pDisplacementArray[1] != nullptr && udStrcmp(pDisplacementArray[1], "") != 0)
        displacement.y = udStrAtof64(pDisplacementArray[1]);
    }
    else
    {
      displacement.x = 0.0;
      displacement.y = 1.0;
    }
  }

  if (udProjectNode_GetMetadataDouble(m_pNode, "visualization.GPSTime.minTime", &m_visualization.GPSTime.minTime, pProgramState->settings.visualization.GPSTime.minTime) != udE_Success && udProjectNode_GetMetadataDouble(m_pNode, "visualization.GPSTime.maxTime", &m_visualization.GPSTime.maxTime, pProgramState->settings.visualization.GPSTime.maxTime) != udE_Success)
  {
    const char *pGPSTime = m_metadata.Get("AttrMinMax_udGPSTime").AsString();
    if (pGPSTime != nullptr && udStrcmp(pGPSTime, "") != 0)
    {
      char pStart[128] = "";
      udStrcpy(pStart, pGPSTime);
      char *pGPSTimeArray[2];
      udStrTokenSplit(pStart, ",", pGPSTimeArray, 2);

      if (pGPSTimeArray[0] != nullptr && udStrcmp(pGPSTimeArray[0], "") != 0)
        m_visualization.GPSTime.minTime = udStrAtoi(pGPSTimeArray[0]);

      if (pGPSTimeArray[1] != nullptr && udStrcmp(pGPSTimeArray[1], "") != 0)
        m_visualization.GPSTime.maxTime = udStrAtoi(pGPSTimeArray[1]);
    }
  }

  if (udProjectNode_GetMetadataDouble(m_pNode, "visualization.scanAngle.minAngle", &m_visualization.scanAngle.minAngle, pProgramState->settings.visualization.scanAngle.minAngle) != udE_Success && udProjectNode_GetMetadataDouble(m_pNode, "visualization.scanAngle.maxAngle", &m_visualization.scanAngle.maxAngle, pProgramState->settings.visualization.scanAngle.maxAngle) != udE_Success)
  {
    const char *pScanAngle = m_metadata.Get("AttrMinMax_udScanAngle").AsString();
    if (pScanAngle != nullptr && udStrcmp(pScanAngle, "") != 0)
    {
      char pStart[128] = "";
      udStrcpy(pStart, pScanAngle);
      char *pScanAngleArray[2];
      udStrTokenSplit(pStart, ",", pScanAngleArray, 2);

      if (pScanAngleArray[0] != nullptr && udStrcmp(pScanAngleArray[0], "") != 0)
        m_visualization.scanAngle.minAngle = udStrAtoi(pScanAngleArray[0]);

      if (pScanAngleArray[1] != nullptr && udStrcmp(pScanAngleArray[1], "") != 0)
        m_visualization.scanAngle.maxAngle = udStrAtoi(pScanAngleArray[1]);
    }
  }

  udProjectNode_GetMetadataUint(m_pNode, "visualization.pointSourceID.defaultColour", &m_visualization.pointSourceID.defaultColour, pProgramState->settings.visualization.pointSourceID.defaultColour);

  for (uint32_t i = 0; i < vcVisualizationSettings::s_maxReturnNumbers; ++i)
  {
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualization.returnNumberColours[%u]", i), &m_visualization.returnNumberColours[i], pProgramState->settings.visualization.returnNumberColours[i]);
    udProjectNode_GetMetadataUint(m_pNode, udTempStr("visualization.numberOfReturnsColours[%u]", i), &m_visualization.numberOfReturnsColours[i], pProgramState->settings.visualization.numberOfReturnsColours[i]);
  }

  m_visualization.displacement.bounds = udFloat2::create((float)displacement.x, (float)displacement.y);

  udProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.minColour", &m_visualization.displacement.min, pProgramState->settings.visualization.displacement.min);
  udProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.maxColour", &m_visualization.displacement.max, pProgramState->settings.visualization.displacement.max);
  udProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.errorColour", &m_visualization.displacement.error, pProgramState->settings.visualization.displacement.error);
  udProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.midColour", &m_visualization.displacement.mid, pProgramState->settings.visualization.displacement.mid);

  // TODO: Handle this better (EVC-535)
  pProgramState->settings.visualization.minIntensity = udMax(pProgramState->settings.visualization.minIntensity, m_visualization.minIntensity);
  pProgramState->settings.visualization.maxIntensity = udMin(pProgramState->settings.visualization.maxIntensity, m_visualization.maxIntensity);

  m_changeZones = true;
}

void vcModel::ChangeProjection(const udGeoZone &newZone)
{
  if (m_loadStatus != vcSLS_Loaded)
  {
    m_changeZones = true;
    return;
  }

  m_changeZones = false;

  if (newZone.srid == 0)
    return;

  if (m_pCurrentZone != nullptr)
    m_sceneMatrix = udGeoZone_TransformMatrix(m_sceneMatrix, *m_pCurrentZone, newZone);
  else if (m_pPreferredProjection != nullptr)
    m_sceneMatrix = udGeoZone_TransformMatrix(m_baseMatrix, *m_pPreferredProjection, newZone);

  if (m_pCurrentZone == nullptr)
    m_pCurrentZone = udAllocType(udGeoZone, 1, udAF_None);

  if (m_pCurrentZone != nullptr) // In case it didn't allocate correctly
    memcpy(m_pCurrentZone, &newZone, sizeof(newZone));
}

void vcModel::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  m_sceneMatrix = delta * m_sceneMatrix;

  // Save it to the node...
  if (m_pCurrentZone != nullptr || m_pProject->baseZone.srid == 0)
  {
    udDouble3 position;
    udDouble3 scale;
    udDoubleQuat orientation;

    (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(position, scale, orientation);

    udDouble3 eulerRotation = UD_RAD2DEG(orientation.eulerAngles());

    if (m_pCurrentZone != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pCurrentZone, udPGT_Point, &position, 1);
    else if (m_pPreferredProjection != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pPreferredProjection, udPGT_Point, &position, 1);
    else
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, pProgramState->geozone, udPGT_Point, &position, 1);

    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.scale", scale.x);
  }
}

void vcModel::HandleSceneExplorerUI(vcState *pProgramState, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);

  if (m_pPointCloud == nullptr)
    return; // Nothing else we can do yet

  udDouble3 position;
  udDouble3 scale;
  udDoubleQuat orientation;
  (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(position, scale, orientation);

  bool repackMatrix = false;
  ImGui::InputScalarN(vcString::Get("sceneModelPosition"), ImGuiDataType_Double, &position.x, 3);

  if (ImGui::IsItemDeactivatedAfterEdit())
  {
    repackMatrix = true;
    static udDouble3 minDouble = udDouble3::create(-1e7, -1e7, -1e7);
    static udDouble3 maxDouble = udDouble3::create(1e7, 1e7, 1e7);
    position = udClamp(position, minDouble, maxDouble);
  }    

  udDouble3 eulerRotation = UD_RAD2DEG(orientation.eulerAngles());
  ImGui::InputScalarN(vcString::Get("sceneModelRotation"), ImGuiDataType_Double, &eulerRotation.x, 3);

  if (ImGui::IsItemDeactivatedAfterEdit())
  {
    repackMatrix = true;
    orientation = udDoubleQuat::create(UD_DEG2RAD(eulerRotation));
  }

  ImGui::InputScalarN(vcString::Get("sceneModelScale"), ImGuiDataType_Double, &scale.x, 1);

  if (ImGui::IsItemDeactivatedAfterEdit())
  {
    repackMatrix = true;
    scale.x = udMax(scale.x, 1e-7);
  }

  if (repackMatrix)
  {
    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationQuat(orientation, position) * udDouble4x4::scaleUniform(scale.x) * udDouble4x4::translation(-m_pivot);

    if (m_pCurrentZone != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pCurrentZone, udPGT_Point, &position, 1);
    else if (m_pPreferredProjection != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pPreferredProjection, udPGT_Point, &position, 1);
    else
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, pProgramState->geozone, udPGT_Point, &position, 1);

    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
    udProjectNode_SetMetadataDouble(m_pNode, "transform.scale", scale.x);
  }

  // Show visualization info
  if (vcSettingsUI_VisualizationSettings(&m_visualization, false, &m_pointCloudHeader.attributes))
  {
    udProjectNode_SetMetadataInt(m_pNode, "visualization.mode", m_visualization.mode);
    //Set all here
    udProjectNode_SetMetadataInt(m_pNode, "visualization.minIntensity", m_visualization.minIntensity);
    udProjectNode_SetMetadataInt(m_pNode, "visualization.maxIntensity", m_visualization.maxIntensity);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.x", m_visualization.displacement.bounds.x);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.y", m_visualization.displacement.bounds.y);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.minColour", m_visualization.displacement.min);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.maxColour", m_visualization.displacement.max);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.errorColour", m_visualization.displacement.error);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.midColour", m_visualization.displacement.mid);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.GPSTime.minTime", m_visualization.GPSTime.minTime);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.GPSTime.maxTime", m_visualization.GPSTime.maxTime);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.scanAngle.minAngle", m_visualization.scanAngle.minAngle);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.scanAngle.maxAngle", m_visualization.scanAngle.maxAngle);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.pointSourceID.defaultColour", m_visualization.pointSourceID.defaultColour);

    for (uint32_t i = 0; i < vcVisualizationSettings::s_maxReturnNumbers; ++i)
    {
      udProjectNode_SetMetadataUint(m_pNode, udTempStr("visualization.returnNumberColours[%u]", i), m_visualization.returnNumberColours[i]);
      udProjectNode_SetMetadataUint(m_pNode, udTempStr("visualization.numberOfReturnsColours[%u]", i), m_visualization.numberOfReturnsColours[i]);
    }
  }

  // Show MetaData Info
  {
    char buffer[128];

    ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("sceneModelMetreScale"), udTempStr("%.4f", m_pointCloudHeader.unitMeterScale)));
    ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("sceneModelLODLayers"), udTempStr("%u", m_pointCloudHeader.totalLODLayers)));
    ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("sceneModelResolution"), udTempStr("%.4f", m_pointCloudHeader.convertedResolution)));

    ImGui::TextUnformatted(vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("sceneModelAttributes"), udTempStr("%u", m_pointCloudHeader.attributes.count)));
    ImGui::Indent();
    for (uint32_t i = 0; i < m_pointCloudHeader.attributes.count; ++i)
    {
      ImGui::BulletText("%s", m_pointCloudHeader.attributes.pDescriptors[i].name);

      if (pProgramState->settings.presentation.showDiagnosticInfo)
      {
        ImGui::Indent();
        switch (m_pointCloudHeader.attributes.pDescriptors[i].typeInfo)
        {
        case udATI_uint8:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint8"));
          break;
        case udATI_uint16:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint16"));
          break;
        case udATI_uint32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint32"));
          break;
        case udATI_uint64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint64"));
          break;
        case udATI_int8:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt8"));
          break;
        case udATI_int16:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt16"));
          break;
        case udATI_int32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt32"));
          break;
        case udATI_int64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt64"));
          break;
        case udATI_float32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeFloat32"));
          break;
        case udATI_float64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeFloat64"));
          break;
        case udATI_color32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeColour"));
          break;
        case udATI_normal32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeNormal"));
          break;
        case udATI_vec3f32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeVec3F32"));
          break;
        default:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUnknown"));
          break;
        }

        ImGui::BulletText("%s", buffer);

        switch (m_pointCloudHeader.attributes.pDescriptors[i].blendType)
        {
        case udABT_Mean:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeBlending"), vcString::Get("attributeBlendingMean"));
          break;
        case udABT_FirstChild:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeBlending"), vcString::Get("attributeBlendingSingle"));
          break;
        default:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeBlending"), vcString::Get("attributeBlendingUnknown"));
          break;
        }

        ImGui::BulletText("%s", buffer);

        ImGui::Unindent();
      }
    }
    ImGui::Unindent();
  }

  vcImGuiValueTreeObject(&m_metadata);
}

void vcModel::HandleSceneEmbeddedUI(vcState * /*pProgramState*/)
{
  if (m_pPointCloud == nullptr)
    return; // Nothing else we can do yet

  if (vcSettingsUI_VisualizationSettings(&m_visualization, false, &m_pointCloudHeader.attributes))
  {
    udProjectNode_SetMetadataInt(m_pNode, "visualization.mode", m_visualization.mode);
    //Set all here
    udProjectNode_SetMetadataInt(m_pNode, "visualization.minIntensity", m_visualization.minIntensity);
    udProjectNode_SetMetadataInt(m_pNode, "visualization.maxIntensity", m_visualization.maxIntensity);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.x", m_visualization.displacement.bounds.x);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.y", m_visualization.displacement.bounds.y);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.minColour", m_visualization.displacement.min);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.maxColour", m_visualization.displacement.max);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.errorColour", m_visualization.displacement.error);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.midColour", m_visualization.displacement.mid);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.GPSTime.minTime", m_visualization.GPSTime.minTime);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.GPSTime.maxTime", m_visualization.GPSTime.maxTime);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.scanAngle.minAngle", m_visualization.scanAngle.minAngle);
    udProjectNode_SetMetadataDouble(m_pNode, "visualization.scanAngle.maxAngle", m_visualization.scanAngle.maxAngle);
    udProjectNode_SetMetadataUint(m_pNode, "visualization.pointSourceID.defaultColour", m_visualization.pointSourceID.defaultColour);

    for (uint32_t i = 0; i < vcVisualizationSettings::s_maxReturnNumbers; ++i)
    {
      udProjectNode_SetMetadataUint(m_pNode, udTempStr("visualization.returnNumberColours[%u]", i), m_visualization.returnNumberColours[i]);
      udProjectNode_SetMetadataUint(m_pNode, udTempStr("visualization.numberOfReturnsColours[%u]", i), m_visualization.numberOfReturnsColours[i]);
    }
  }
}

void vcModel::ContextMenuListModels(vcState *pProgramState, udProjectNode *pParentNode, vcSceneItem **ppCurrentSelectedModel, const char *pProjectNodeType, bool allowEmpty)
{
  udProjectNode *pChildNode = pParentNode->pFirstChild;

  while (pChildNode != nullptr)
  {
    if (pChildNode->itemtype == udPNT_Folder)
    {
      ContextMenuListModels(pProgramState, pChildNode, ppCurrentSelectedModel, pProjectNodeType, allowEmpty);
    }
    else if (udStrEqual(pChildNode->itemtypeStr, pProjectNodeType) && pChildNode->pUserData != this)
    {
      if (!allowEmpty && *ppCurrentSelectedModel == nullptr) // If nothing is selected, it will pick the first thing it can
        *ppCurrentSelectedModel = (vcModel *)pChildNode->pUserData;

      if (ImGui::MenuItem(pChildNode->pName, nullptr, (pChildNode->pUserData == *ppCurrentSelectedModel)))
        *ppCurrentSelectedModel = (vcModel *)pChildNode->pUserData;
    }

    pChildNode = pChildNode->pNextSibling;
  }
}

void vcModel::HandleContextMenu(vcState *pProgramState)
{
  ImGui::Separator();

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetAll"), false))
  {
    if (m_pPreferredProjection)
      ChangeProjection(*m_pPreferredProjection);

    m_sceneMatrix = m_defaultMatrix;

    ChangeProjection(pProgramState->geozone);
    ApplyDelta(pProgramState, udDouble4x4::identity());
  }

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetPosition"), false))
  {
    if (m_pPreferredProjection)
      ChangeProjection(*m_pPreferredProjection);

    udDouble3 positionCurrent, positionDefault;
    udDouble3 scaleCurrent, scaleDefault;
    udDoubleQuat orientationCurrent, orientationDefault;

    (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionCurrent, scaleCurrent, orientationCurrent);
    (udDouble4x4::translation(-m_pivot) * m_defaultMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionDefault, scaleDefault, orientationDefault);

    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationQuat(orientationCurrent, positionDefault) * udDouble4x4::scaleNonUniform(scaleCurrent) * udDouble4x4::translation(-m_pivot);;

    ChangeProjection(pProgramState->geozone);
    ApplyDelta(pProgramState, udDouble4x4::identity());
  }

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetOrientation"), false))
  {
    if (m_pPreferredProjection)
      ChangeProjection(*m_pPreferredProjection);

    udDouble3 positionCurrent, positionDefault;
    udDouble3 scaleCurrent, scaleDefault;
    udDoubleQuat orientationCurrent, orientationDefault;

    (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionCurrent, scaleCurrent, orientationCurrent);
    (udDouble4x4::translation(-m_pivot) * m_defaultMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionDefault, scaleDefault, orientationDefault);

    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationQuat(orientationDefault, positionCurrent) * udDouble4x4::scaleNonUniform(scaleCurrent) * udDouble4x4::translation(-m_pivot);;

    ChangeProjection(pProgramState->geozone);
    ApplyDelta(pProgramState, udDouble4x4::identity());
  }

  if (ImGui::Selectable(vcString::Get("sceneExplorerResetScale"), false))
  {
    if (m_pPreferredProjection)
      ChangeProjection(*m_pPreferredProjection);

    udDouble3 positionCurrent, positionDefault;
    udDouble3 scaleCurrent, scaleDefault;
    udDoubleQuat orientationCurrent, orientationDefault;

    (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionCurrent, scaleCurrent, orientationCurrent);
    (udDouble4x4::translation(-m_pivot) * m_defaultMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(positionDefault, scaleDefault, orientationDefault);

    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationQuat(orientationCurrent, positionCurrent) * udDouble4x4::scaleNonUniform(scaleDefault) * udDouble4x4::translation(-m_pivot);;

    ChangeProjection(pProgramState->geozone);
    ApplyDelta(pProgramState, udDouble4x4::identity());
  }

  ImGui::Separator();

#if VC_HASCONVERT
  if (pProgramState->branding.exportEnabled && ((m_pPreferredProjection == nullptr && pProgramState->geozone.srid == 0) || (m_pPreferredProjection != nullptr && m_pPreferredProjection->srid == pProgramState->geozone.srid)))
  {
    bool matrixEqual = udMatrixEqualApprox(m_defaultMatrix, m_sceneMatrix);
    if (matrixEqual && ImGui::BeginMenu(vcString::Get("sceneExplorerExportPointCloud")))
    {
      static vcQueryNode* s_pQuery = nullptr;
      static vcCrossSectionNode *s_pCrossSection = nullptr;

      if (ImGui::IsWindowAppearing())
      {
        s_pQuery = nullptr;
        s_pCrossSection = nullptr;

        // Set a default file name to be exported if there isn't.
        if (udStrlen(pProgramState->modelPath) == 0)
        {
          size_t fileNameLength = 0;
          if (udStrchr(pProgramState->sceneExplorer.clickedItem.pItem->pName, ".", &fileNameLength))
          {
            memset(pProgramState->modelPath, 0, sizeof(pProgramState->modelPath));
            udStrncpy(pProgramState->modelPath, pProgramState->sceneExplorer.clickedItem.pItem->pName, fileNameLength);
            udStrcat(pProgramState->modelPath, SupportedTileTypes_QueryExport[0]);
          }
        }
      }

      if (ImGui::BeginCombo(vcString::Get("sceneExplorerExportQueryFilter"), (s_pQuery == nullptr ? (s_pCrossSection == nullptr ? vcString::Get("sceneExplorerExportEntireModel") : s_pCrossSection->m_pNode->pName) : s_pQuery->m_pNode->pName)))
      {
        if (ImGui::MenuItem(vcString::Get("sceneExplorerExportEntireModel"), nullptr, (s_pQuery == nullptr && s_pCrossSection == nullptr)))
        {
          s_pQuery = nullptr;
          s_pCrossSection = nullptr;
        }

        ContextMenuListModels(pProgramState, pProgramState->activeProject.pFolder->m_pNode, (vcSceneItem**)&s_pQuery, "QFilter", true);
        ContextMenuListModels(pProgramState, pProgramState->activeProject.pFolder->m_pNode, (vcSceneItem**)&s_pCrossSection, "XSlice", true);
        ImGui::EndCombo();
      }

      vcIGSW_FilePicker(pProgramState, vcString::Get("sceneExplorerExportFilename"), pProgramState->modelPath, SupportedTileTypes_QueryExport, vcFDT_SaveFile, nullptr);

      if (ImGui::Button(vcString::Get("sceneExplorerExportBegin")))
      {
        if (udFileExists(pProgramState->modelPath) != udR_Success || vcModals_OverwriteExistingFile(pProgramState, pProgramState->modelPath))
        {
          if (!udStrchr(pProgramState->modelPath, "."))
            udStrcat(pProgramState->modelPath, SupportedTileTypes_QueryExport[0]);

          udQueryFilter *pFilter = ((s_pQuery == nullptr) ? ((s_pCrossSection == nullptr) ? nullptr : s_pCrossSection->m_pFilter) : s_pQuery->m_pFilter);
          udPointCloud *pCloud = m_pPointCloud;

          ++pProgramState->backgroundWork.exportsRunning;

          udWorkerPoolCallback callback = [pProgramState, pCloud, pFilter](void*)
          {
            udError result = udPointCloud_Export(pCloud, pProgramState->modelPath, pFilter);
            if (result != udE_Success)
            {
              ErrorItem status;
              status.source = vcES_File;
              status.pData = udStrdup(pProgramState->modelPath);
              switch (result)
              {
              case udE_OpenFailure:
                status.resultCode = udR_OpenFailure;
                break;
              case udE_ReadFailure:
                status.resultCode = udR_ReadFailure;
                break;
              case udE_WriteFailure:
                status.resultCode = udR_WriteFailure;
                break;
              case udE_OutOfSync:
                status.resultCode = udR_OutOfSync;
                break;
              case udE_ParseError:
                status.resultCode = udR_ParseError;
                break;
              case udE_ImageParseError:
                status.resultCode = udR_ImageLoadFailure;
                break;
              default:
                status.resultCode = udR_Failure_;
                break;
              }

              vcError_AddError(pProgramState, status);
              udFileDelete(pProgramState->modelPath);
            }
            --pProgramState->backgroundWork.exportsRunning;
          };

          // Add post callback
          udWorkerPool_AddTask(pProgramState->pWorkerPool, callback, nullptr, false);
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::EndMenu();
    }
  }

  // Compare models
  if (pProgramState->branding.convertEnabled && ImGui::BeginMenu(vcString::Get("sceneExplorerCompareModels")))
  {
    static vcModel *s_pOldModel = nullptr;
    static double s_ballRadius = 0.15;
    static double s_gridSize = 1.0;

    if (ImGui::IsWindowAppearing())
      s_pOldModel = nullptr;

    if (ImGui::BeginCombo(vcString::Get("sceneExplorerDisplacementModel"), (s_pOldModel == nullptr ? "..." : s_pOldModel->m_pNode->pName)))
    {
      ContextMenuListModels(pProgramState, pProgramState->activeProject.pFolder->m_pNode, (vcSceneItem**)&s_pOldModel, "UDS", false);
      ImGui::EndCombo();
    }

    if (ImGui::InputDouble(vcString::Get("sceneExplorerCompareModelsBallRadius"), &s_ballRadius))
      s_ballRadius = udClamp(s_ballRadius, DBL_EPSILON, 100.0);

    if (ImGui::InputDouble(vcString::Get("sceneExplorerCompareModelsGridSize"), &s_gridSize))
      s_gridSize = udClamp(s_gridSize, 0.25, 100.0);

    if (ImGui::ButtonEx(vcString::Get("sceneExplorerCompareModels"), ImVec2(0, 0), (s_pOldModel == nullptr ? (ImGuiButtonFlags_)ImGuiButtonFlags_Disabled : ImGuiButtonFlags_None)))
    {
      char newName[vcMaxPathLength] = {};
      char oldName[vcMaxPathLength] = {};
      udFilename(this->m_pNode->pName).ExtractFilenameOnly(newName, sizeof(newName));
      udFilename(s_pOldModel->m_pNode->pName).ExtractFilenameOnly(oldName, sizeof(oldName));

      const char *pNameBuffer = nullptr;
      udSprintf(&pNameBuffer, "Displacement_%s_%s", oldName, newName);
      udFilename temp;
      temp.SetFromFullPath("%s", this->m_pNode->pURI);
      temp.SetFilenameNoExt(pNameBuffer);
      const char *pName = udStrdup(temp.GetPath());

      vcModel *pOldModel = s_pOldModel;
      double ballRadius = s_ballRadius;
      double gridSize = s_gridSize;

      udWorkerPoolCallback callback = [this, pProgramState, pOldModel, ballRadius, gridSize, pName](void*)
      {
        vcBPA_CompareExport(pProgramState, pOldModel->m_pNode->pURI, this->m_pNode->pURI, ballRadius, gridSize, pName);
      };
      udWorkerPool_AddTask(pProgramState->pWorkerPool, callback, nullptr, false);
      vcModals_OpenModal(pProgramState, vcMT_Convert);
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndMenu();
  }
#endif //VC_HASCONVERT
}

vcMenuBarButtonIcon vcModel::GetSceneExplorerIcon()
{
  return vcMBBI_UDS;
}

void vcModel::Cleanup(vcState * /*pProgramState*/)
{
  udPointCloud_Unload(&m_pPointCloud);
  udFree(m_pCurrentZone);

  m_visualization.pointSourceID.colourMap.Deinit();
}

udDouble3 vcModel::GetLocalSpacePivot()
{
  return m_pivot;
}

udDouble4x4 vcModel::GetWorldSpaceMatrix()
{
  return m_sceneMatrix;
}

vcGizmoAllowedControls vcModel::GetAllowedControls()
{
  return vcGAC_AllUniform;
}
