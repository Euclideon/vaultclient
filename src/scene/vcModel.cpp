#include "vcModel.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcBPA.h"
#include "vcStringFormat.h"
#include "vcModals.h"
#include "vcQueryNode.h"
#include "vcSettingsUI.h"
#include "vcClassificationColours.h"

#include "vcTexture.h"

#include "imgui.h"
#include "imgui_internal.h" // Required for ButtonEx

#include "imgui_ex/imgui_udValue.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "vdkPointCloud.h"

#include "udStringUtil.h"
#include "udMath.h"

struct vcModelLoadInfo
{
  vcState *pProgramState;
  vcModel *pModel;
  udDouble3 possibleLocation;
};

void vcModel_LoadMetadata(vcState *pProgramState, vcModel *pModel, udDouble3 possibleLocation)
{
  const char *pMetadata;

  if (vdkPointCloud_GetMetadata(pModel->m_pPointCloud, &pMetadata) == vE_Success)
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
    vcProject_UpdateNodeGeometryFromCartesian(pModel->m_pProject, pModel->m_pNode, pProgramState->geozone, vdkPGT_Point, &possibleLocation, 1);

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
    vdkError modelStatus = vdkPointCloud_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->m_pPointCloud, pLoadInfo->pModel->m_pNode->pURI, &pLoadInfo->pModel->m_pointCloudHeader);

    if (modelStatus == vE_OpenFailure)
      modelStatus = vdkPointCloud_Load(pLoadInfo->pProgramState->pVDKContext, &pLoadInfo->pModel->m_pPointCloud, udTempStr("%s%s", pLoadInfo->pProgramState->activeProject.pRelativeBase, pLoadInfo->pModel->m_pNode->pURI), &pLoadInfo->pModel->m_pointCloudHeader);

    if (modelStatus == vE_Success)
    {
      vcModel_LoadMetadata(pLoadInfo->pProgramState, pLoadInfo->pModel, pLoadInfo->possibleLocation);
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

vcModel::vcModel(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_positionRequest(udDouble3::create(nan(""), nan(""), nan(""))),
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

  memcpy(m_visualization.customClassificationColors, GeoverseClassificationColours, sizeof(m_visualization.customClassificationColors));
}

vcModel::vcModel(vcState *pProgramState, const char *pName, vdkPointCloud *pCloud) :
  vcSceneItem(pProgramState, "UDS", pName),
  m_pPointCloud(nullptr),
  m_pivot(udDouble3::zero()),
  m_positionRequest(udDouble3::create(nan(""), nan(""), nan(""))),
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
  m_pPointCloud = pCloud;
  m_loadStatus = vcSLS_Loaded;

  vcModel_LoadMetadata(pProgramState, this, udDouble3::zero());

  m_pNode->pUserData = this;

  memcpy(m_visualization.customClassificationColors, GeoverseClassificationColours, sizeof(m_visualization.customClassificationColors));
}

void vcModel::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (m_pPointCloud == nullptr)
    return; // Nothing else we can do yet

  vdkError status = vdkPointCloud_GetStreamingStatus(m_pPointCloud);
  if (status != vE_Success)
  {
    if (status == vE_ParseError)
      m_pActiveWarningStatus = "sceneExplorerErrorCorrupt";
    else if (status == vE_OpenFailure)
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

    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &euler.x, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &euler.y, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.r", &euler.z, 0.0);
    vdkProjectNode_GetMetadataDouble(m_pNode, "transform.scale", &scale, udMag3(m_baseMatrix.axis.x));

    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationYPR(UD_DEG2RAD(euler), *pPosition) * udDouble4x4::scaleUniform(scale) * udDouble4x4::translation(-m_pivot);
  }

  udFree(pPosition);

  int mode = 0;
  vdkProjectNode_GetMetadataInt(m_pNode, "visualization.mode", &mode, 0);
  m_visualization.mode = (vcVisualizatationMode)mode;

  vdkProjectNode_GetMetadataInt(m_pNode, "visualization.minIntensity", &m_visualization.minIntensity, -1);
  vdkProjectNode_GetMetadataInt(m_pNode, "visualization.maxIntensity", &m_visualization.maxIntensity, -1);
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

  vdkProjectNode_GetMetadataBool(m_pNode, "visualization.showColourTable", &m_visualization.useCustomClassificationColours, false);
  udDouble2 displacement;
  vdkProjectNode_GetMetadataDouble(m_pNode, "visualization.displacement.x", &displacement.x, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "visualization.displacement.y", &displacement.y, 0.0);
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

  m_visualization.displacement.bounds = udFloat2::create((float)displacement.x, (float)displacement.y);

  vdkProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.minColour", &m_visualization.displacement.min, pProgramState->settings.visualization.displacement.min);
  vdkProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.maxColour", &m_visualization.displacement.max, pProgramState->settings.visualization.displacement.max);
  vdkProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.errorColour", &m_visualization.displacement.error, pProgramState->settings.visualization.displacement.error);
  vdkProjectNode_GetMetadataUint(m_pNode, "visualization.displacement.midColour", &m_visualization.displacement.mid, pProgramState->settings.visualization.displacement.mid);

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

    vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pCurrentZone, vdkPGT_Point, &position, 1);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale", scale.x);
  }
}

void vcModel::HandleImGui(vcState *pProgramState, size_t * /*pItemID*/)
{
  ImGui::TextWrapped("Path: %s", m_pNode->pURI);

  if (m_pPointCloud == nullptr)
    return; // Nothing else we can do yet

  udDouble3 position;
  udDouble3 scale;
  udDoubleQuat orientation;
  (udDouble4x4::translation(-m_pivot) * m_sceneMatrix * udDouble4x4::translation(m_pivot)).extractTransforms(position, scale, orientation);

  if (isnan(m_positionRequest[0]))
    m_positionRequest = position;

  bool repackMatrix = false;
  if (ImGui::InputScalarN(vcString::Get("sceneModelPosition"), ImGuiDataType_Double, &m_positionRequest.x, 3))
    repackMatrix = true;

  udDouble3 eulerRotation = UD_RAD2DEG(orientation.eulerAngles());
  if (ImGui::InputScalarN(vcString::Get("sceneModelRotation"), ImGuiDataType_Double, &eulerRotation.x, 3))
  {
    repackMatrix = true;
    orientation = udDoubleQuat::create(UD_DEG2RAD(eulerRotation));
  }

  if (ImGui::InputScalarN(vcString::Get("sceneModelScale"), ImGuiDataType_Double, &scale.x, 1))
    repackMatrix = true;

  if (repackMatrix)
  {
    m_sceneMatrix = udDouble4x4::translation(m_pivot) * udDouble4x4::rotationQuat(orientation, m_positionRequest) * udDouble4x4::scaleUniform(scale.x) * udDouble4x4::translation(-m_pivot);

    if (m_pCurrentZone != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pCurrentZone, vdkPGT_Point, &m_positionRequest, 1);
    else if (m_pPreferredProjection != nullptr)
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, *m_pPreferredProjection, vdkPGT_Point, &m_positionRequest, 1);
    else
      vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_positionRequest, 1);

    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", eulerRotation.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", eulerRotation.y);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", eulerRotation.z);
    vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale", scale.x);
  }

  // Show visualization info
  if (vcSettingsUI_VisualizationSettings(&m_visualization))
  {
    vdkProjectNode_SetMetadataInt(m_pNode, "visualization.mode", m_visualization.mode);
    //Set all here
    vdkProjectNode_SetMetadataInt(m_pNode, "visualization.minIntensity", m_visualization.minIntensity);
    vdkProjectNode_SetMetadataInt(m_pNode, "visualization.maxIntensity", m_visualization.maxIntensity);
    vdkProjectNode_SetMetadataBool(m_pNode, "visualization.showColourTable", m_visualization.useCustomClassificationColours);
    vdkProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.x", m_visualization.displacement.bounds.x);
    vdkProjectNode_SetMetadataDouble(m_pNode, "visualization.displacement.y", m_visualization.displacement.bounds.y);
    vdkProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.minColour", m_visualization.displacement.min);
    vdkProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.maxColour", m_visualization.displacement.max);
    vdkProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.errorColour", m_visualization.displacement.error);
    vdkProjectNode_SetMetadataUint(m_pNode, "visualization.displacement.midColour", m_visualization.displacement.mid);
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
        case vdkAttributeTypeInfo_uint8:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint8"));
          break;
        case vdkAttributeTypeInfo_uint16:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint16"));
          break;
        case vdkAttributeTypeInfo_uint32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint32"));
          break;
        case vdkAttributeTypeInfo_uint64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUint64"));
          break;
        case vdkAttributeTypeInfo_int8:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt8"));
          break;
        case vdkAttributeTypeInfo_int16:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt16"));
          break;
        case vdkAttributeTypeInfo_int32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt32"));
          break;
        case vdkAttributeTypeInfo_int64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeInt64"));
          break;
        case vdkAttributeTypeInfo_float32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeFloat32"));
          break;
        case vdkAttributeTypeInfo_float64:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeFloat64"));
          break;
        case vdkAttributeTypeInfo_color32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeColour"));
          break;
        case vdkAttributeTypeInfo_normal32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeNormal"));
          break;
        case vdkAttributeTypeInfo_vec3f32:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeVec3F32"));
          break;
        default:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeType"), vcString::Get("attributeTypeUnknown"));
          break;
        }

        ImGui::BulletText("%s", buffer);

        switch (m_pointCloudHeader.attributes.pDescriptors[i].blendMode)
        {
        case vdkABM_Mean:
          vcStringFormat(buffer, udLengthOf(buffer), vcString::Get("attributeBlending"), vcString::Get("attributeBlendingMean"));
          break;
        case vdkABM_SingleValue:
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

void vcModel::ContextMenuListModels(vcState *pProgramState, vdkProjectNode *pParentNode, vcSceneItem **ppCurrentSelectedModel, const char *pProjectNodeType, bool allowEmpty)
{
  vdkProjectNode *pChildNode = pParentNode->pFirstChild;

  while (pChildNode != nullptr)
  {
    if (pChildNode->itemtype == vdkPNT_Folder)
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
  if ((m_pPreferredProjection == nullptr && pProgramState->geozone.srid == 0) || (m_pPreferredProjection != nullptr && m_pPreferredProjection->srid == pProgramState->geozone.srid))
  {
    bool matrixEqual = udMatrixEqualApprox(m_defaultMatrix, m_sceneMatrix);
    if (matrixEqual && ImGui::BeginMenu(vcString::Get("sceneExplorerExportPointCloud")))
    {
      static vcQueryNode *s_pQuery = nullptr;

      if (ImGui::IsWindowAppearing())
      {
        s_pQuery = nullptr;

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

      if (ImGui::BeginCombo(vcString::Get("sceneExplorerExportQueryFilter"), (s_pQuery == nullptr ? vcString::Get("sceneExplorerExportEntireModel") : s_pQuery->m_pNode->pName)))
      {
        if (ImGui::MenuItem(vcString::Get("sceneExplorerExportEntireModel"), nullptr, (s_pQuery == nullptr)))
          s_pQuery = nullptr;

        ContextMenuListModels(pProgramState, pProgramState->activeProject.pFolder->m_pNode, (vcSceneItem**)&s_pQuery, "QFilter", true);
        ImGui::EndCombo();
      }

      vcIGSW_FilePicker(pProgramState, vcString::Get("sceneExplorerExportFilename"), pProgramState->modelPath, SupportedTileTypes_QueryExport, vcFDT_SaveFile, nullptr);

      if (ImGui::Button(vcString::Get("sceneExplorerExportBegin")))
      {
        if (udFileExists(pProgramState->modelPath) != udR_Success || vcModals_OverwriteExistingFile(pProgramState, pProgramState->modelPath))
        {
          if (!udStrchr(pProgramState->modelPath, "."))
            udStrcat(pProgramState->modelPath, SupportedTileTypes_QueryExport[0]);

          vdkQueryFilter *pFilter = ((s_pQuery == nullptr) ? nullptr : s_pQuery->m_pFilter);
          vdkPointCloud *pCloud = m_pPointCloud;

          ++pProgramState->backgroundWork.exportsRunning;

          udWorkerPoolCallback callback = [pProgramState, pCloud, pFilter](void*)
          {
            vdkError result = vdkPointCloud_Export(pCloud, pProgramState->modelPath, pFilter);
            if (result != vE_Success)
            {
              vcState::ErrorItem status;
              status.source = vcES_File;
              status.pData = udStrdup(pProgramState->modelPath);
              switch (result)
              {
              case vE_OpenFailure:
                status.resultCode = udR_OpenFailure;
                break;
              case vE_ReadFailure:
                status.resultCode = udR_ReadFailure;
                break;
              case vE_WriteFailure:
                status.resultCode = udR_WriteFailure;
                break;
              case vE_OutOfSync:
                status.resultCode = udR_OutOfSync;
                break;
              case vE_ParseError:
                status.resultCode = udR_ParseError;
                break;
              case vE_ImageParseError:
                status.resultCode = udR_ImageLoadFailure;
                break;
              default:
                status.resultCode = udR_Failure_;
                break;
              }

              pProgramState->errorItems.PushBack(status);
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
  if (ImGui::BeginMenu(vcString::Get("sceneExplorerCompareModels")))
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

    if (ImGui::ButtonEx(vcString::Get("sceneExplorerCompareModels"), ImVec2(0, 0), (s_pOldModel == nullptr ? ImGuiButtonFlags_Disabled : ImGuiButtonFlags_None)))
    {
      char newName[vcMaxPathLength] = {};
      char oldName[vcMaxPathLength] = {};
      udFilename(this->m_pNode->pName).ExtractFilenameOnly(newName, sizeof(newName));
      udFilename(s_pOldModel->m_pNode->pName).ExtractFilenameOnly(oldName, sizeof(oldName));

      const char *pNameBuffer = nullptr;
      udSprintf(&pNameBuffer, "Displacement_%s_%s", oldName, newName);
      udFilename temp;
      temp.SetFromFullPath(this->m_pNode->pURI);
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

void vcModel::Cleanup(vcState *pProgramState)
{
  vdkPointCloud_Unload(&m_pPointCloud);
  udFree(m_pCurrentZone);

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

vcGizmoAllowedControls vcModel::GetAllowedControls()
{
  return vcGAC_AllUniform;
}
