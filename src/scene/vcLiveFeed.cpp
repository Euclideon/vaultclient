#include "vcLiveFeed.h"

#include "vdkServerAPI.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcTime.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "gl/vcLabelRenderer.h"

#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"

struct vcLiveFeedItem
{
  bool visible;
  double lastUpdated;

  udDouble3 previousPosition; // Previous known location
  udDouble3 livePosition; // Latest known location
  udDouble3 displayPosition; // Where we're going to display the item
  double tweenAmount;

  char pSimplifedLabel[16];
  char pDetailedLabel[128];
  char uuid[64];

  vcLabelInfo labelConfig;
};

struct vcLiveFeedUpdateInfo
{
  vcState *pProgramState;
  vcLiveFeed *pFeed;
};

void vcLiveFeed_UpdateFeed(void *pUserData)
{
  vcLiveFeedUpdateInfo *pInfo = (vcLiveFeedUpdateInfo*)pUserData;

  const char *pFeedsJSON = nullptr;
  const char *pMessage = udTempStr("{ \"position\": [153.092, -27.464, 0], \"srid\": 32756, \"radius\": 100, \"time\": %f }", pInfo->pFeed->m_lastUpdateTime);

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

        udJSON temp, temp2;
        temp.Parse(pNode->Get("location").AsString());
        temp2.Parse(pNode->Get("data").AsString());

        char uuid[64];
        udStrcpy(uuid, udLengthOf(uuid), pNode->Get("feedid").AsString());

        vcLiveFeedItem *pFeedItem = nullptr;
        udLockMutex(pInfo->pFeed->m_pMutex);
        for (vcLiveFeedItem *pCachedFeedItem : pInfo->pFeed->m_feedItems)
        {
          if (udStrcmp(uuid, pCachedFeedItem->uuid) == 0)
          {
            pFeedItem = pCachedFeedItem;
            break;
          }
        }
        udReleaseMutex(pInfo->pFeed->m_pMutex);

        udFloat4x4 rotation = udFloat4x4::identity();
        udDouble3 newPosition = udDouble3::create((float)temp.Get("coordinates[0]").AsDouble(), (float)temp.Get("coordinates[1]").AsDouble(), (float)temp.Get("coordinates[2]").AsDouble());
        double updated = pNode->Get("updated").AsDouble();

        if (pFeedItem == nullptr)
        {
          pFeedItem = udAllocType(vcLiveFeedItem, 1, udAF_Zero);
          udStrcpy(pFeedItem->uuid, udLengthOf(pFeedItem->uuid), uuid);
          udLockMutex(pInfo->pFeed->m_pMutex);
          pInfo->pFeed->m_feedItems.push_back(pFeedItem);
          udReleaseMutex(pInfo->pFeed->m_pMutex);

          pFeedItem->previousPosition = newPosition;
          pFeedItem->tweenAmount = 1.0f;

          pFeedItem->labelConfig.pText = pFeedItem->pSimplifedLabel;
          pFeedItem->labelConfig.textSize = vcLFS_Medium;
        }
        else
        {
          udDouble3 dir = newPosition - pFeedItem->previousPosition;
          if (udMagSq3(dir) > 0)
          {
            dir = udNormalize3(dir);
            udDouble3 euler = udMath_DirToEuler(dir);
            rotation = udFloat4x4::rotationYPR(float(euler.x + UD_HALF_PI), 0, 0);
            pFeedItem->tweenAmount = 0;
            pFeedItem->previousPosition = pFeedItem->displayPosition;
          }
        }

        pFeedItem->lastUpdated = updated;
        pFeedItem->visible = true; // Just got updated

        udSprintf(pFeedItem->pDetailedLabel, udLengthOf(pFeedItem->pDetailedLabel), "%s / %s", temp2.Get("routenameshort").AsString("???"), temp2.Get("routenamelong").AsString("???"));
        udSprintf(pFeedItem->pSimplifedLabel, udLengthOf(pFeedItem->pSimplifedLabel), "%s", temp2.Get("routenameshort").AsString("???"));

        pFeedItem->labelConfig.backColourRGBA = vcIGSW_BGRAToRGBAUInt32(0xFF000000 | udStrAtou(temp2.Get("routecolor").AsString("000000"), nullptr, 16));
        pFeedItem->labelConfig.textColourRGBA = vcIGSW_BGRAToRGBAUInt32(0xFF000000 | udStrAtou(temp2.Get("routenamecolor").AsString("FF00FF"), nullptr, 16));
        pFeedItem->labelConfig.worldPosition = udDouble3::create(newPosition);

        pFeedItem->livePosition = newPosition;
      }
    }

  }

epilogue:
  vdkServerAPI_ReleaseResult(pInfo->pProgramState->pVDKContext, &pFeedsJSON);
  pInfo->pFeed->m_lastUpdateTime = vcTime_GetEpochSecsF();
  pInfo->pFeed->m_loadStatus = vcSLS_Loaded;
}

void vcLiveFeed::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  double now = vcTime_GetEpochSecsF();
  double recently = now - this->m_decayFrequency;

  if (this->m_loadStatus != vcSLS_Loading)
  {
    if (now >= this->m_lastUpdateTime + this->m_updateFrequency)
    {
      this->m_loadStatus = vcSLS_Loading;

      vcLiveFeedUpdateInfo *pInfo = udAllocType(vcLiveFeedUpdateInfo, 1, udAF_None);
      pInfo->pProgramState = pProgramState;
      pInfo->pFeed = this;

      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLiveFeed_UpdateFeed, pInfo);
    }
  }

  m_visibleItems = 0;

  udLockMutex(this->m_pMutex);
  for (vcLiveFeedItem *pFeedItem : this->m_feedItems)
  {
    // If its not visible or its been a while since it was visible
    if (!pFeedItem->visible || pFeedItem->lastUpdated < recently)
    {
      pFeedItem->visible = false;
      continue;
    }

    pFeedItem->tweenAmount = udMin(1.0, pFeedItem->tweenAmount + pRenderData->deltaTime * 0.02);
    pFeedItem->displayPosition = udLerp(pFeedItem->previousPosition, pFeedItem->livePosition, pFeedItem->tweenAmount);
    double distanceSq = udMagSq3(pFeedItem->displayPosition - pRenderData->pCamera->position);

    if (distanceSq > this->m_falloffDistance * this->m_falloffDistance)
      continue; // Don't really want to mark !visible because it will be again soon

    if (distanceSq < this->m_detailDistance * this->m_detailDistance)
      pFeedItem->labelConfig.pText = pFeedItem->pDetailedLabel;
    else
      pFeedItem->labelConfig.pText = pFeedItem->pSimplifedLabel;

    pFeedItem->labelConfig.worldPosition = pFeedItem->displayPosition;

    pRenderData->labels.PushBack(&pFeedItem->labelConfig);

    ++this->m_visibleItems;
  }
  udReleaseMutex(this->m_pMutex);
}

void vcLiveFeed::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  udUnused(pProgramState);
  udUnused(delta);
}

void vcLiveFeed::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::Text("Feed Items: %zu", this->m_feedItems.size());
  ImGui::Text("Visible Items: %zu", this->m_visibleItems);

  ImGui::Text("Next update in %.2f seconds", (this->m_lastUpdateTime + this->m_updateFrequency) - vcTime_GetEpochSecsF());

  // Update Frequency
  {
    const double updateFrequencyMinValue = 5.0;
    const double updateFrequencyMaxValue = 300.0;

    if (ImGui::SliderScalar("Update Frequency", ImGuiDataType_Double, &this->m_updateFrequency, &updateFrequencyMinValue, &updateFrequencyMaxValue, "%.0f seconds"))
      this->m_updateFrequency = udClamp(this->m_updateFrequency, updateFrequencyMinValue, updateFrequencyMaxValue);
  }

  // Decay Frequency
  {
    const double decayFrequencyMinValue = 30.0;
    const double decayFrequencyMaxValue = 604800.0; // 1 week

    if (ImGui::SliderScalar("Decay Frequency", ImGuiDataType_Double, &this->m_decayFrequency, &decayFrequencyMinValue, &decayFrequencyMaxValue, "%.0f seconds", 4.f))
    {
      this->m_decayFrequency = udClamp(this->m_decayFrequency, decayFrequencyMinValue, decayFrequencyMaxValue);

      double recently = vcTime_GetEpochSecsF() - this->m_decayFrequency;

      udLockMutex(this->m_pMutex);
      for (vcLiveFeedItem *pFeedItem : this->m_feedItems)
        pFeedItem->visible = (pFeedItem->lastUpdated > recently);
      udReleaseMutex(this->m_pMutex);
    }

    // Simplify Distance
    {
      const double detailDistanceMinValue = 1.0;
      const double detailDistanceMaxValue = 2500.0;

      if (ImGui::SliderScalar("Detail Distance", ImGuiDataType_Double, &this->m_detailDistance, &detailDistanceMinValue, &detailDistanceMaxValue, "%.0f", 1.5f))
        this->m_detailDistance = udClamp(this->m_detailDistance, detailDistanceMinValue, detailDistanceMaxValue);
    }

    // Falloff Distance
    {
      const double falloffDistanceMinValue = 1.0;
      const double falloffDistanceMaxValue = 100000.0;

      if (ImGui::SliderScalar("Falloff Distance", ImGuiDataType_Double, &this->m_falloffDistance, &falloffDistanceMinValue, &falloffDistanceMaxValue, "%.0f", 3.f))
        this->m_falloffDistance = udClamp(this->m_falloffDistance, falloffDistanceMinValue, falloffDistanceMaxValue);
    }

  }
}

void vcLiveFeed::Cleanup(vcState * /*pProgramState*/)
{
  udFree(m_pName);

  udLockMutex(this->m_pMutex);
  for (vcLiveFeedItem *pObject : this->m_feedItems)
    udFree(pObject);
  udReleaseMutex(this->m_pMutex);

  udDestroyMutex(&this->m_pMutex);

  this->m_feedItems.clear();

  this->vcLiveFeed::~vcLiveFeed();
}

void vcLiveFeed_AddToList(vcState *pProgramState)
{
  vcLiveFeed *pLiveFeed = udAllocType(vcLiveFeed, 1, udAF_Zero);
  pLiveFeed = new (pLiveFeed) vcLiveFeed();
  pLiveFeed->m_visible = true;

  pLiveFeed->m_pName = udStrdup("Live Feed");
  pLiveFeed->m_type = vcSOT_LiveFeed;

  // Just setting this as a nice default
  pLiveFeed->m_updateFrequency = 15.0;
  pLiveFeed->m_decayFrequency = 300.0;
  pLiveFeed->m_detailDistance = 500.0;
  pLiveFeed->m_falloffDistance = 25000.0;
  pLiveFeed->m_pMutex = udCreateMutex();

  udStrcpy(pLiveFeed->m_typeStr, sizeof(pLiveFeed->m_typeStr), "IOT");
  pLiveFeed->m_loadStatus = vcSLS_Pending;

  pLiveFeed->AddItem(pProgramState);
}
