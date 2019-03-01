#include "vcLiveFeed.h"

#include "vdkServerAPI.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcTime.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "gl/vcLabelRenderer.h"
#include "vcPolygonModel.h"

#include "vCore/vUUID.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udChunkedArray.h"

struct vcLiveFeedItemLOD
{
  double distance; // Normalized Distance
  double sspixels; // Screenspace Pixels

  const char *pModelAddress;
  vcPolygonModel *pModel; // The LOD does not own this though

  vcTexture *pIcon;
  vcLabelInfo *pLabelInfo;
  const char *pLabelText;
};

struct vcLiveFeedItem
{
  vUUID uuid;

  bool visible;
  double lastUpdated;

  udDouble3 ypr; // Estimated for many IOTs

  udDouble3 previousPosition; // Previous known location
  udDouble3 livePosition; // Latest known location
  udDouble3 displayPosition; // Where we're going to display the item
  double tweenAmount;

  double minBoundingRadius;
  udChunkedArray<vcLiveFeedItemLOD> lodLevels;
};

struct vcLiveFeedUpdateInfo
{
  vcState *pProgramState;
  vcLiveFeed *pFeed;
};

void vcLiveFeedItem_ClearLODs(vcLiveFeedItem *pFeedItem)
{
  for (size_t lodLevelIndex = 0; lodLevelIndex < pFeedItem->lodLevels.length; ++lodLevelIndex)
  {
    vcLiveFeedItemLOD &ref = pFeedItem->lodLevels[lodLevelIndex];

    udFree(ref.pLabelText);
    udFree(ref.pLabelInfo);
    udFree(ref.pModelAddress);
  }

  pFeedItem->lodLevels.Deinit();
}

void vcLiveFeed_UpdateFeed(void *pUserData)
{
  vcLiveFeedUpdateInfo *pInfo = (vcLiveFeedUpdateInfo*)pUserData;

  if (!pInfo->pProgramState->gis.isProjected || pInfo->pProgramState->gis.SRID == 0)
  {
    pInfo->pFeed->m_loadStatus = vcSLS_Failed;
    return;
  }

  const char *pFeedsJSON = nullptr;
  const char *pMessage = udTempStr("{ \"position\": [%f, %f, %f], \"srid\": %d, \"radius\": %f, \"time\": %f }", pInfo->pProgramState->pCamera->position.x, pInfo->pProgramState->pCamera->position.y, pInfo->pProgramState->pCamera->position.z, pInfo->pProgramState->gis.SRID, pInfo->pFeed->m_falloffDistance, 0.f);

  if (vdkServerAPI_Query(pInfo->pProgramState->pVDKContext, "v1/feeds/region", pMessage, &pFeedsJSON) == vE_Success)
  {
    udJSON data;
    if (data.Parse(pFeedsJSON) == udR_Success)
    {
      udJSONArray *pFeeds = data.Get("feeds").AsArray();

      pInfo->pFeed->m_lastUpdateTime = udMax(pInfo->pFeed->m_lastUpdateTime, data.Get("lastUpdate").AsDouble());

      if (!pFeeds)
        goto epilogue;

      for (size_t i = 0; i < pFeeds->length; ++i)
      {
        udJSON *pNode = pFeeds->GetElement(i);

        vUUID uuid;
        if (vUUID_SetFromString(&uuid, pNode->Get("feedid").AsString()) != udR_Success)
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
        udReleaseMutex(pInfo->pFeed->m_pMutex);

        udDouble3 newPosition = pNode->Get("geometry.coordinates").AsDouble3();
        double updated = pNode->Get("updated").AsDouble();

        if (pFeedItem == nullptr)
        {
          pFeedItem = udAllocType(vcLiveFeedItem, 1, udAF_Zero);
          pFeedItem->lodLevels.Init(4);
          pFeedItem->uuid = uuid;
          udLockMutex(pInfo->pFeed->m_pMutex);
          pInfo->pFeed->m_feedItems.PushBack(pFeedItem);
          udReleaseMutex(pInfo->pFeed->m_pMutex);

          pFeedItem->previousPosition = newPosition;
          pFeedItem->tweenAmount = 1.0f;
        }
        else
        {
          udDouble3 dir = newPosition - pFeedItem->previousPosition;
          if (udMagSq3(dir) > 0)
          {
            dir = udNormalize3(dir);
            pFeedItem->ypr = udMath_DirToYPR(dir);
            pFeedItem->tweenAmount = 0;
            pFeedItem->previousPosition = pFeedItem->displayPosition;
          }
        }

        pFeedItem->lastUpdated = updated;
        pFeedItem->visible = true; // Just got updated

        pFeedItem->minBoundingRadius = pNode->Get("display.minBoundingRadius").AsDouble(1.0);

        udJSONArray *pLODS = pNode->Get("display.lods").AsArray();

        if (pLODS != nullptr)
        {
          udLockMutex(pInfo->pFeed->m_pMutex);

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
          }

          udReleaseMutex(pInfo->pFeed->m_pMutex);
        }

        pFeedItem->livePosition = newPosition;
      }
    }

  }

epilogue:
  vdkServerAPI_ReleaseResult(pInfo->pProgramState->pVDKContext, &pFeedsJSON);

  pInfo->pFeed->m_lastUpdateTime = vcTime_GetEpochSecsF();
  pInfo->pFeed->m_loadStatus = vcSLS_Loaded;
}

vcLiveFeed::vcLiveFeed() :
  m_lastUpdateTime(0.0), m_visibleItems(0), m_updateFrequency(15.0), m_decayFrequency(300.0),
  m_falloffDistance(50000.0), m_pMutex(udCreateMutex())
{
  m_feedItems.Init(512);
  m_polygonModels.Init(16);

  m_visible = true;
  m_pName = udStrdup("Live Feed");
  m_type = vcSOT_LiveFeed;
  udStrcpy(m_typeStr, sizeof(m_typeStr), "IOT");
  m_loadStatus = vcSLS_Pending;
}

void vcLiveFeed::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  double now = vcTime_GetEpochSecsF();
  double recently = now - m_decayFrequency;

  if (m_loadStatus != vcSLS_Loading)
  {
    if (now >= m_lastUpdateTime + m_updateFrequency)
    {
      m_loadStatus = vcSLS_Loading;

      vcLiveFeedUpdateInfo *pInfo = udAllocType(vcLiveFeedUpdateInfo, 1, udAF_None);
      pInfo->pProgramState = pProgramState;
      pInfo->pFeed = this;

      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLiveFeed_UpdateFeed, pInfo);
    }
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

    pFeedItem->tweenAmount = udMin(1.0, pFeedItem->tweenAmount + pRenderData->deltaTime * 0.02);
    pFeedItem->displayPosition = udLerp(pFeedItem->previousPosition, pFeedItem->livePosition, pFeedItem->tweenAmount);
    double distanceSq = udMagSq3(pFeedItem->displayPosition - pRenderData->pCamera->position);

    if (distanceSq > m_falloffDistance * m_falloffDistance)
      continue; // Don't really want to mark !visible because it will be again soon

    // Select & Render LOD here
    for (size_t lodI = 0; lodI < pFeedItem->lodLevels.length; ++lodI)
    {
      vcLiveFeedItemLOD &lodRef = pFeedItem->lodLevels[lodI];

      if (lodRef.distance != 0.0 && distanceSq > (lodRef.distance*lodRef.distance) / (pFeedItem->minBoundingRadius * pFeedItem->minBoundingRadius))
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

      if (lodRef.pModelAddress != nullptr)
      {
        vcPolygonModel *pModel = lodRef.pModel;

        if (pModel == nullptr) // Add to cache
        {
          bool found = false;
          for (size_t pI = 0; pI < m_polygonModels.length; ++pI)
          {
            if (udStrEquali(m_polygonModels[pI].pModelURL, lodRef.pModelAddress))
            {
              found = true;
              pModel = m_polygonModels[pI].pModel;
              break;
            }
          }

          if (!found)
          {
            vcPolygonModel_CreateFromURL(&pModel, lodRef.pModelAddress);
            m_polygonModels.PushBack({ udStrdup(lodRef.pModelAddress), pModel }); // Even if the load failed we should add it
          }

          lodRef.pModel = pModel;
        }

        if (pModel != nullptr)
          pRenderData->polyModels.PushBack({ pModel, udDouble4x4::rotationYPR(pFeedItem->ypr, pFeedItem->displayPosition) });
      }

      break; // We got to the end so we should stop
    }

    ++m_visibleItems;
  }
  udReleaseMutex(m_pMutex);
}

void vcLiveFeed::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  udUnused(pProgramState);
  udUnused(delta);
}

void vcLiveFeed::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::Text("Feed Items: %zu", m_feedItems.length);
  ImGui::Text("Visible Items: %zu", m_visibleItems);

  ImGui::Text("Next update in %.2f seconds", (m_lastUpdateTime + m_updateFrequency) - vcTime_GetEpochSecsF());

  // Update Frequency
  {
    const double updateFrequencyMinValue = 5.0;
    const double updateFrequencyMaxValue = 300.0;

    if (ImGui::SliderScalar("Update Frequency", ImGuiDataType_Double, &m_updateFrequency, &updateFrequencyMinValue, &updateFrequencyMaxValue, "%.0f seconds"))
      m_updateFrequency = udClamp(m_updateFrequency, updateFrequencyMinValue, updateFrequencyMaxValue);
  }

  // Decay Frequency
  {
    const double decayFrequencyMinValue = 30.0;
    const double decayFrequencyMaxValue = 604800.0; // 1 week

    if (ImGui::SliderScalar("Decay Frequency", ImGuiDataType_Double, &m_decayFrequency, &decayFrequencyMinValue, &decayFrequencyMaxValue, "%.0f seconds", 4.f))
    {
      m_decayFrequency = udClamp(m_decayFrequency, decayFrequencyMinValue, decayFrequencyMaxValue);

      double recently = vcTime_GetEpochSecsF() - m_decayFrequency;

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
      const double falloffDistanceMinValue = 1.0;
      const double falloffDistanceMaxValue = 100000.0;

      if (ImGui::SliderScalar("Falloff Distance", ImGuiDataType_Double, &m_falloffDistance, &falloffDistanceMinValue, &falloffDistanceMaxValue, "%.0f", 3.f))
        m_falloffDistance = udClamp(m_falloffDistance, falloffDistanceMinValue, falloffDistanceMaxValue);
    }

  }
}

void vcLiveFeed::Cleanup(vcState * /*pProgramState*/)
{
  udFree(m_pName);

  udLockMutex(m_pMutex);
  for (size_t i = 0; i < m_feedItems.length; ++i)
  {
    vcLiveFeedItem *pFeedItem = m_feedItems[i];

    vcLiveFeedItem_ClearLODs(pFeedItem);
    udFree(pFeedItem);
  }
  udReleaseMutex(m_pMutex);

  udDestroyMutex(&m_pMutex);

  for (size_t i = 0; i < m_polygonModels.length; ++i)
  {
    udFree(m_polygonModels[i].pModelURL);
    vcPolygonModel_Destroy(&m_polygonModels[i].pModel);
  }

  m_feedItems.Deinit();
  m_polygonModels.Deinit();
}
