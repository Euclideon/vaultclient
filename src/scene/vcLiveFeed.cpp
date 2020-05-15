#include "vcLiveFeed.h"

#include "vdkServerAPI.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "vcLabelRenderer.h"
#include "vcPolygonModel.h"
#include "vcStringFormat.h"

#include "udFile.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"
#include "udStringUtil.h"

struct vcLiveFeedItemLOD
{
  double distance; // Normalized Distance
  double sspixels; // Screenspace Pixels

  const char *pModelAddress;
  vcPolygonModel *pModel; // The LOD does not own this though

  const char *pPinIcon;
  vcLabelInfo *pLabelInfo;
  const char *pLabelText;
};

struct vcLiveFeedItem
{
  udUUID uuid;

  bool visible;
  bool selected;
  double lastUpdated;

  udDouble3 ypr; // Estimated for many IOTs

  udDouble3 previousPositionLatLong; // Previous known location
  udDouble3 livePositionLatLong; // Latest known location

  udDouble3 displayPosition; // Where we're going to display the item (geolocated space)

  double tweenAmount;

  double minBoundingRadius;
  udChunkedArray<vcLiveFeedItemLOD> lodLevels;
};

struct vcLiveFeedUpdateInfo
{
  vcState *pProgramState;
  vcLiveFeed *pFeed;

  bool newer; // Are we fetching newer or older data?
};

void vcLiveFeedItem_ClearLODs(vcLiveFeedItem *pFeedItem)
{
  for (size_t lodLevelIndex = 0; lodLevelIndex < pFeedItem->lodLevels.length; ++lodLevelIndex)
  {
    vcLiveFeedItemLOD &ref = pFeedItem->lodLevels[lodLevelIndex];

    udFree(ref.pPinIcon);
    udFree(ref.pLabelText);
    udFree(ref.pLabelInfo);
    udFree(ref.pModelAddress);
  }

  pFeedItem->lodLevels.Deinit();
}

void vcLiveFeed_LoadModel(void *pUserData)
{
  vcLiveFeed *pFeed = (vcLiveFeed*)pUserData;
  udLockMutex(pFeed->m_pMutex);

  vcLiveFeedPolyCache *pItem = nullptr;
  for (size_t pI = 0; pI < pFeed->m_polygonModels.length; ++pI)
  {
    if (pFeed->m_polygonModels[pI].loadStatus == vcLiveFeedPolyCache::LS_InQueue)
    {
      pItem = &pFeed->m_polygonModels[pI];
      break;
    }
  }

  if (!pItem)
    return;

  pItem->loadStatus = vcLiveFeedPolyCache::LS_Downloading;
  udReleaseMutex(pFeed->m_pMutex);

  if (udFile_Load(pItem->pModelURL, &pItem->pModelData, &pItem->modelDataLength) != udR_Success)
  {
    pItem->loadStatus = vcLiveFeedPolyCache::LS_Failed;
  }
  else
  {
    pItem->loadStatus = vcLiveFeedPolyCache::LS_Downloaded;
  }
}

void vcLiveFeed_UpdateFeed(void *pUserData)
{
  vcLiveFeedUpdateInfo *pInfo = (vcLiveFeedUpdateInfo*)pUserData;

  const char *pFeedsJSON = nullptr;

  const char *pServerAddr = "v1/feeds/fetch";
  const char *pMessage = udTempStr("{ \"groupid\": \"%s\", \"time\": %f, \"newer\": %s }", udUUID_GetAsString(&pInfo->pFeed->m_groupID), pInfo->newer ? pInfo->pFeed->m_newestFeedUpdate : pInfo->pFeed->m_oldestFeedUpdate, pInfo->newer ? "true" : "false");

  vdkError vError = vdkServerAPI_Query(pInfo->pProgramState->pVDKContext, pServerAddr, pMessage, &pFeedsJSON);

  double updatedTime = 0.0;

  pInfo->pFeed->m_lastFeedSync = udGetEpochSecsUTCf();

  if (vError == vE_Success)
  {
    udJSON data;
    if (data.Parse(pFeedsJSON) == udR_Success)
    {
      udJSONArray *pFeeds = data.Get("feeds").AsArray();

      pInfo->pFeed->m_fetchNow = pInfo->newer && data.Get("more").AsBool();
      updatedTime = data.Get("lastUpdate").AsDouble();

      if (updatedTime != 0.0)
      {
        if (pInfo->newer)
          pInfo->pFeed->m_newestFeedUpdate = updatedTime;
        else
          pInfo->pFeed->m_oldestFeedUpdate = updatedTime;
      }

      if (!pFeeds)
        goto epilogue;

      for (size_t i = 0; i < pFeeds->length; ++i)
      {
        udJSON *pNode = pFeeds->GetElement(i);

        udUUID uuid;
        if (udUUID_SetFromString(&uuid, pNode->Get("feedid").AsString()) != udR_Success)
          continue;

        vcLiveFeedItem *pFeedItem = nullptr;

        udLockMutex(pInfo->pFeed->m_pMutex);

        for (size_t j = 0; j < pInfo->pFeed->m_feedItems.length; ++j)
        {
          vcLiveFeedItem *pCachedFeedItem = pInfo->pFeed->m_feedItems[j];

          if (uuid == pCachedFeedItem->uuid)
          {
            pFeedItem = pCachedFeedItem;
            break;
          }
        }

        udDouble3 newPositionLatLong = pNode->Get("geometry.coordinates").AsDouble3();
        double updated = pNode->Get("updated").AsDouble();

        if (pFeedItem == nullptr)
        {
          pFeedItem = udAllocType(vcLiveFeedItem, 1, udAF_Zero);
          pFeedItem->lodLevels.Init(4);
          pFeedItem->uuid = uuid;
          udLockMutex(pInfo->pFeed->m_pMutex);
          pInfo->pFeed->m_feedItems.PushBack(pFeedItem);
          udReleaseMutex(pInfo->pFeed->m_pMutex);

          pFeedItem->previousPositionLatLong = newPositionLatLong;
          pFeedItem->tweenAmount = 1.0f;
        }

        udDouble3 dir = newPositionLatLong - pFeedItem->previousPositionLatLong;
        bool hasOrientation = !pNode->Get("data.orientation").IsVoid();

        if (dir.x != 0 || dir.y != 0 || dir.z != 0)
        {
          if (!hasOrientation)
          {
            //X and Y are latlong and Z is m so we need to fix it
            dir = udNormalize3(udGeoZone_LatLongToCartesian(pInfo->pProgramState->geozone, newPositionLatLong, true) - udGeoZone_LatLongToCartesian(pInfo->pProgramState->geozone, pFeedItem->previousPositionLatLong, true));

            pFeedItem->ypr = udDirectionToYPR(dir);
          }

          pFeedItem->tweenAmount = 0;
          pFeedItem->previousPositionLatLong = pFeedItem->livePositionLatLong;
        }

        if (hasOrientation)
        {
          pFeedItem->ypr.x = -UD_DEG2RAD(pNode->Get("data.orientation.heading").AsDouble());
          pFeedItem->ypr.y = UD_DEG2RAD(pNode->Get("data.orientation.pitch").AsDouble());
          pFeedItem->ypr.z = UD_DEG2RAD(pNode->Get("data.orientation.roll").AsDouble());
        }

        pFeedItem->lastUpdated = updated;
        pFeedItem->visible = true; // Just got updated

        pFeedItem->minBoundingRadius = pNode->Get("display.minBoundingRadius").AsDouble(1.0);

        udJSONArray *pLODS = pNode->Get("display.lods").AsArray();

        if (pLODS != nullptr)
        {
          if (pFeedItem->lodLevels.length > pLODS->length)
            vcLiveFeedItem_ClearLODs(pFeedItem);

          pFeedItem->lodLevels.GrowBack(pLODS->length - pFeedItem->lodLevels.length);

          // We just need to confirm the info is basically the same
          for (size_t lodLevelIndex = 0; lodLevelIndex < pLODS->length; ++lodLevelIndex)
          {
            udJSON *pLOD = pLODS->GetElement(lodLevelIndex);
            vcLiveFeedItemLOD &lodRef = pFeedItem->lodLevels[lodLevelIndex];

            lodRef.distance = pLOD->Get("distance").AsDouble();
            lodRef.sspixels = pLOD->Get("sspixels").AsDouble();

            const udJSON &modelObj = pLOD->Get("model");
            if (modelObj.IsObject())
            {
              if (udStrEquali(modelObj.Get("type").AsString(), "vsm") && !udStrEquali(lodRef.pModelAddress, modelObj.Get("url").AsString()))
              {
                udFree(lodRef.pModelAddress);
                lodRef.pModelAddress = udStrdup(modelObj.Get("url").AsString());
                lodRef.pModel = nullptr;
              }
            }

            const udJSON &labelObj = pLOD->Get("label");
            if (labelObj.IsObject())
            {
              // Was there a label before?
              if (lodRef.pLabelInfo == nullptr)
                lodRef.pLabelInfo = udAllocType(vcLabelInfo, 1, udAF_Zero);

              lodRef.pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(udStrAtou(labelObj.Get("bgcolor").AsString("7F000000"), nullptr, 16));
              lodRef.pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(udStrAtou(labelObj.Get("color").AsString("FFFFFFFF"), nullptr, 16));

              if ((lodRef.pLabelInfo->textColourRGBA & 0xFF000000) == 0)
                lodRef.pLabelInfo->textColourRGBA |= 0xFF000000; // If alpha is 0, set it to full

              if ((lodRef.pLabelInfo->backColourRGBA & 0xFF000000) == 0 && (lodRef.pLabelInfo->backColourRGBA & 0xFFFFFF) != 0)
                lodRef.pLabelInfo->backColourRGBA |= 0x7F000000; // If alpha is 0 and there is colour, set it to half alpha?

              udFree(lodRef.pLabelText);
              lodRef.pLabelText = udStrdup(labelObj.Get("text").AsString("[?]"));

              const char *pTextSize = labelObj.Get("size").AsString();
              if (udStrEquali(pTextSize, "x-small") || udStrEquali(pTextSize, "small"))
                lodRef.pLabelInfo->textSize = vcLFS_Small;
              else if (udStrEquali(pTextSize, "large") || udStrEquali(pTextSize, "x-large"))
                lodRef.pLabelInfo->textSize = vcLFS_Large;
              else
                lodRef.pLabelInfo->textSize = vcLFS_Medium;

              lodRef.pLabelInfo->pText = lodRef.pLabelText;
            }

            const udJSON &pinObj = pLOD->Get("pin");
            if (pinObj.IsObject())
            {
              udFree(lodRef.pPinIcon);
              lodRef.pPinIcon = udStrdup(pinObj.Get("image").AsString());
            }
          }
        }

        pFeedItem->livePositionLatLong = newPositionLatLong;

        udReleaseMutex(pInfo->pFeed->m_pMutex);
      }
    }
  }

epilogue:
  vdkServerAPI_ReleaseResult(&pFeedsJSON);

  pInfo->pFeed->m_loadStatus = vcSLS_Loaded;
}


vcLiveFeed::vcLiveFeed(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_selectedItem(0),
  m_visibleItems(0),
  m_tweenPositionAndOrientation(true),
  m_updateFrequency(15.0),
  m_decayFrequency(300.0),
  m_maxDisplayDistance(50000.0),
  m_pMutex(udCreateMutex())
{
  m_feedItems.Init(512);
  m_polygonModels.Init(16);

  m_lastFeedSync = 0.0;
  m_newestFeedUpdate = udGetEpochSecsUTCf() - m_decayFrequency;
  m_oldestFeedUpdate = m_newestFeedUpdate;
  m_fetchNow = true;

  m_labelLODModifier = 1.0;

  OnNodeUpdate(pProgramState);

  m_loadStatus = vcSLS_Pending;
}

void vcLiveFeed::OnNodeUpdate(vcState *pProgramState)
{
  const char *pTempStr = nullptr;

  vdkProjectNode_GetMetadataString(m_pNode, "groupid", &pTempStr, nullptr);
  udUUID_Clear(&m_groupID);
  udUUID_SetFromString(&m_groupID, pTempStr);

  vdkProjectNode_GetMetadataDouble(m_pNode, "updateFrequency", &m_updateFrequency, 30.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "maxDisplayTime", &m_decayFrequency, 300.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "maxDisplayDistance", &m_maxDisplayDistance, 50000.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "lodModifier", &m_labelLODModifier, 1.0);

  vdkProjectNode_GetMetadataBool(m_pNode, "tweenEnabled", &m_tweenPositionAndOrientation, true);

  ChangeProjection(pProgramState->geozone);
}

void vcLiveFeed::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  double now = udGetEpochSecsUTCf();
  double recently = now - m_decayFrequency;

  if (m_loadStatus != vcSLS_Loading && ((now >= m_lastFeedSync + m_updateFrequency) || (now - m_decayFrequency < m_oldestFeedUpdate) || m_fetchNow))
  {
    m_loadStatus = vcSLS_Loading;

    vcLiveFeedUpdateInfo *pInfo = udAllocType(vcLiveFeedUpdateInfo, 1, udAF_Zero);
    pInfo->pProgramState = pProgramState;
    pInfo->pFeed = this;
    pInfo->newer = ((now >= m_lastFeedSync + m_updateFrequency) || m_fetchNow);

    m_fetchNow = false;

    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcLiveFeed_UpdateFeed, pInfo);
  }

  m_visibleItems = 0;

  udLockMutex(m_pMutex);
  for (size_t i = 0; i < m_feedItems.length; ++i)
  {
    vcLiveFeedItem *pFeedItem = m_feedItems[i];

    // If its not visible or its been a while since it was visible
    if (!pFeedItem->visible || pFeedItem->lastUpdated < recently)
    {
      pFeedItem->visible = false;
      continue;
    }

    udDouble3 cameraPosition = pProgramState->camera.position;

    pFeedItem->tweenAmount = m_tweenPositionAndOrientation ? udMin(1.0, pFeedItem->tweenAmount + pProgramState->deltaTime * 0.02) : 1.0;
    udDouble3 lerpedPos = udLerp(pFeedItem->previousPositionLatLong, pFeedItem->livePositionLatLong, pFeedItem->tweenAmount);
    pFeedItem->displayPosition = udGeoZone_LatLongToCartesian(pProgramState->geozone, lerpedPos, true);

    double distanceSq = udMagSq3(pFeedItem->displayPosition - cameraPosition);

    if (distanceSq > m_maxDisplayDistance * m_maxDisplayDistance)
      continue; // Don't really want to mark !visible because it might be again soon

    // Select & Render LOD here
    for (size_t lodI = 0; lodI < pFeedItem->lodLevels.length; ++lodI)
    {
      vcLiveFeedItemLOD &lodRef = pFeedItem->lodLevels[lodI];

      if (lodI < pFeedItem->lodLevels.length - 1 && distanceSq != 0.0 && distanceSq / m_labelLODModifier > (lodRef.distance*lodRef.distance) / (pFeedItem->minBoundingRadius * pFeedItem->minBoundingRadius))
        continue;

      if (lodRef.sspixels != 0.0)
      {
        // See if its within the threshold
        //continue; // if not in threshold
      }

      if (lodRef.pLabelInfo != nullptr)
      {
        lodRef.pLabelInfo->worldPosition = pFeedItem->displayPosition;
        pRenderData->labels.PushBack(lodRef.pLabelInfo);
      }

      if (lodRef.pPinIcon != nullptr)
        pRenderData->pins.PushBack({ pFeedItem->displayPosition, lodRef.pPinIcon, 1 });

      if (lodRef.pModelAddress != nullptr)
      {
        vcPolygonModel *pModel = lodRef.pModel;

        if (pModel == nullptr) // Add to cache
        {
          vcLiveFeedPolyCache *pItem = nullptr;
          for (size_t pI = 0; pI < m_polygonModels.length; ++pI)
          {
            if (udStrEquali(m_polygonModels[pI].pModelURL, lodRef.pModelAddress))
            {
              pItem = &m_polygonModels[pI];
              break;
            }
          }

          if (pItem)
          {
            if (pItem->loadStatus == vcLiveFeedPolyCache::LS_Downloaded)
            {
              pItem->loadStatus = vcLiveFeedPolyCache::LS_Loaded;
              if (vcPolygonModel_CreateFromVSMFInMemory(&pItem->pModel, (char*)pItem->pModelData, (int)pItem->modelDataLength, pProgramState->pWorkerPool) != udR_Success)
              {
                // TODO: (EVC-570) retry? draw some error mesh?
                pItem->loadStatus = vcLiveFeedPolyCache::LS_Failed;
              }

              udFree(pItem->pModelData);
              pItem->modelDataLength = 0;
            }

            pModel = pItem->pModel;
          }
          else
          {
            m_polygonModels.PushBack({ udStrdup(lodRef.pModelAddress), nullptr, vcLiveFeedPolyCache::LS_InQueue, nullptr, 0 });
            udWorkerPool_AddTask(pProgramState->pWorkerPool, vcLiveFeed_LoadModel, this, false);
          }

          lodRef.pModel = pModel;
        }

        if (pModel != nullptr)
        {
          udDouble3 normal;
          udDouble3 terrainHeightPos = pFeedItem->displayPosition;//vcRender_QueryMapPositionAtCartesian(pProgramState, pProgramState->pRenderContext, pFeedItem->displayPosition, &normal);

          udDouble3 normalYPR = udDirectionToYPR(normal);
          udDouble3 ypr = udDouble3::create(pFeedItem->ypr.x, -normalYPR.y + UD_PI * 0.5, pFeedItem->ypr.z);
          //udDoubleQuat q = udDoubleQuat::create(normalYPR);
          //udDoubleQuat q2 = udDoubleQuat::create(pFeedItem->ypr) * q;
          //
          //udDouble4x4 mat = udDouble4x4::rotationQuat(q2, terrainHeightPos);// 
          
         // terrainHeightPos.z += 20.0f;
          pRenderData->polyModels.PushBack({ vcRenderPolyInstance::RenderType_Polygon, vcRenderPolyInstance::RenderFlags_None, { pModel }, udDouble4x4::rotationYPR(ypr, terrainHeightPos), nullptr, vcGLSCM_Back, this, (uint64_t)(i + 1) });
        }
      }

      break; // We got to the end so we should stop
    }

    ++m_visibleItems;
  }
  udReleaseMutex(m_pMutex);
}

void vcLiveFeed::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 & /*delta*/)
{
  // Do nothing
}

void vcLiveFeed::HandleImGui(vcState *pProgramState, size_t * /*pItemID*/)
{
  if (pProgramState->settings.presentation.showDiagnosticInfo)
  {
    const char *strings[] = { udTempStr("%zu", m_feedItems.length), udTempStr("%zu", m_visibleItems), udTempStr("%.2f", (m_lastFeedSync + m_updateFrequency) - udGetEpochSecsUTCf()) };
    const char *pBuffer = vcStringFormat(vcString::Get("liveFeedDiagInfo"), strings, udLengthOf(strings));
    ImGui::Text("%s", pBuffer);
    udFree(pBuffer);
  }

  // Update Frequency
  {
    const double updateFrequencyMinValue = 5.0;
    const double updateFrequencyMaxValue = 300.0;

    if (ImGui::SliderScalar(vcString::Get("liveFeedUpdateFrequency"), ImGuiDataType_Double, &m_updateFrequency, &updateFrequencyMinValue, &updateFrequencyMaxValue, "%.0f s"))
    {
      m_updateFrequency = udClamp(m_updateFrequency, updateFrequencyMinValue, updateFrequencyMaxValue);
      vdkProjectNode_SetMetadataDouble(m_pNode, "updateFrequency", m_updateFrequency);
    }
  }

  // Decay Frequency
  {
    const double decayFrequencyMinValue = 30.0;
    const double decayFrequencyMaxValue = 604800.0; // 1 week

    if (ImGui::SliderScalar(vcString::Get("liveFeedMaxDisplayTime"), ImGuiDataType_Double, &m_decayFrequency, &decayFrequencyMinValue, &decayFrequencyMaxValue, "%.0f s", 4.f))
    {
      m_decayFrequency = udClamp(m_decayFrequency, decayFrequencyMinValue, decayFrequencyMaxValue);
      vdkProjectNode_SetMetadataDouble(m_pNode, "maxDisplayTime", m_decayFrequency);

      double recently = udGetEpochSecsUTCf() - m_decayFrequency;

      udLockMutex(m_pMutex);

      for (size_t i = 0; i < m_feedItems.length; ++i)
      {
        vcLiveFeedItem *pFeedItem = m_feedItems[i];
        pFeedItem->visible = (pFeedItem->lastUpdated > recently);
      }

      udReleaseMutex(m_pMutex);
    }

    // Falloff Distance
    {
      const double displayDistanceMinValue = 1.0;
      const double displayDistanceMaxValue = 100000.0;

      if (ImGui::SliderScalar(vcString::Get("liveFeedDisplayDistance"), ImGuiDataType_Double, &m_maxDisplayDistance, &displayDistanceMinValue, &displayDistanceMaxValue, "%.0f", 3.f))
      {
        m_maxDisplayDistance = udClamp(m_maxDisplayDistance, displayDistanceMinValue, displayDistanceMaxValue);
        vdkProjectNode_SetMetadataDouble(m_pNode, "maxDisplayDistance", m_maxDisplayDistance);
      }
    }

    // LOD Distances
    {
      const double minLODModifier = 0.01;
      const double maxLODModifier = 5.0;

      if (ImGui::SliderScalar(vcString::Get("liveFeedLODModifier"), ImGuiDataType_Double, &m_labelLODModifier, &minLODModifier, &maxLODModifier, "%.2f", 2.f))
      {
        m_labelLODModifier = udClamp(m_labelLODModifier, minLODModifier, maxLODModifier);
        vdkProjectNode_SetMetadataDouble(m_pNode, "lodModifier", m_labelLODModifier);
      }
    }

    // Tween
    if (ImGui::Checkbox(vcString::Get("liveFeedTween"), &m_tweenPositionAndOrientation))
      vdkProjectNode_SetMetadataBool(m_pNode, "tweenEnabled", m_tweenPositionAndOrientation);

    char groupStr[udUUID::udUUID_Length+1];
    udStrcpy(groupStr, udUUID_GetAsString(&m_groupID));
    if (vcIGSW_InputText(vcString::Get("liveFeedGroupID"), groupStr, udLengthOf(groupStr)))
    {
      if (udUUID_IsValid(groupStr))
      {
        udUUID_SetFromString(&m_groupID, groupStr);
        vdkProjectNode_SetMetadataString(m_pNode, "groupid", groupStr);
      }
    }
  }
}

void vcLiveFeed::Cleanup(vcState * /*pProgramState*/)
{
  udLockMutex(m_pMutex);
  for (size_t i = 0; i < m_feedItems.length; ++i)
  {
    vcLiveFeedItem *pFeedItem = m_feedItems[i];

    vcLiveFeedItem_ClearLODs(pFeedItem);
    udFree(pFeedItem);
  }

  for (size_t i = 0; i < m_polygonModels.length; ++i)
  {
    while (m_polygonModels[i].loadStatus == vcLiveFeedPolyCache::LS_Downloading)
    {
      udYield(); // busy wait
    }

    udFree(m_polygonModels[i].pModelURL);
    vcPolygonModel_Destroy(&m_polygonModels[i].pModel);
    udFree(m_polygonModels[i].pModelData);
  }

  udReleaseMutex(m_pMutex);

  udDestroyMutex(&m_pMutex);

  m_feedItems.Deinit();
  m_polygonModels.Deinit();
}

void vcLiveFeed::ChangeProjection(const udGeoZone &newZone)
{
  //TODO: Handle updating everything to render in the new zone
  udUnused(newZone);
}

udDouble3 vcLiveFeed::GetLocalSpacePivot()
{
  return udDouble3::zero();
}

void vcLiveFeed::SelectSubitem(uint64_t internalId)
{
  this->m_selectedItem = internalId;
}

bool vcLiveFeed::IsSubitemSelected(uint64_t internalId)
{
  if (internalId == 0 || this->m_selectedItem == 0)
    return m_selected;

  return (m_selected && this->m_selectedItem == internalId);
}
