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
  const char *pMessage = udTempStr("{ \"position\": [153.092, -27.464, 0], \"srid\": 32756, \"radius\": 100, \"time\": %f }", pInfo->pFeed->lastUpdateTime);

  if (vdkServerAPI_Query(pInfo->pProgramState->pVDKContext, "v1/feeds/region", pMessage, &pFeedsJSON) == vE_Success)
  {
    udJSON data;
    if (data.Parse(pFeedsJSON) == udR_Success)
    {
      udJSONArray *pFeeds = data.Get("feeds").AsArray();

      pInfo->pFeed->lastUpdateTime = udMax(pInfo->pFeed->lastUpdateTime, data.Get("lastUpdate").AsDouble());

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
        udLockMutex(pInfo->pFeed->pMutex);
        for (vcLiveFeedItem *pCachedFeedItem : pInfo->pFeed->feedItems)
        {
          if (udStrcmp(uuid, pCachedFeedItem->uuid) == 0)
          {
            pFeedItem = pCachedFeedItem;
            break;
          }
        }
        udReleaseMutex(pInfo->pFeed->pMutex);

        udFloat4x4 rotation = udFloat4x4::identity();
        udDouble3 newPosition = udDouble3::create((float)temp.Get("coordinates[0]").AsDouble(), (float)temp.Get("coordinates[1]").AsDouble(), (float)temp.Get("coordinates[2]").AsDouble());
        double updated = pNode->Get("updated").AsDouble();

        if (pFeedItem == nullptr)
        {
          pFeedItem = udAllocType(vcLiveFeedItem, 1, udAF_Zero);
          udStrcpy(pFeedItem->uuid, udLengthOf(pFeedItem->uuid), uuid);
          udLockMutex(pInfo->pFeed->pMutex);
          pInfo->pFeed->feedItems.push_back(pFeedItem);
          udReleaseMutex(pInfo->pFeed->pMutex);

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
  pInfo->pFeed->lastUpdateTime = vcTime_GetEpochSecsF();
  pInfo->pFeed->loadStatus = vcSLS_Loaded;
}

void vcLiveFeed::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!visible)
    return;

  double now = vcTime_GetEpochSecsF();
  double recently = now - this->decayFrequency;

  if (this->loadStatus != vcSLS_Loading)
  {
    if (now >= this->lastUpdateTime + this->updateFrequency)
    {
      this->loadStatus = vcSLS_Loading;

      vcLiveFeedUpdateInfo *pInfo = udAllocType(vcLiveFeedUpdateInfo, 1, udAF_None);
      pInfo->pProgramState = pProgramState;
      pInfo->pFeed = this;

      vWorkerThread_AddTask(pProgramState->pWorkerPool, vcLiveFeed_UpdateFeed, pInfo);
    }
  }

  visibleItems = 0;

  udLockMutex(this->pMutex);
  for (vcLiveFeedItem *pFeedItem : this->feedItems)
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

    if (distanceSq > this->falloffDistance * this->falloffDistance)
      continue; // Don't really want to mark !visible because it will be again soon

    if (distanceSq < this->detailDistance * this->detailDistance)
      pFeedItem->labelConfig.pText = pFeedItem->pDetailedLabel;
    else
      pFeedItem->labelConfig.pText = pFeedItem->pSimplifedLabel;

    pFeedItem->labelConfig.worldPosition = pFeedItem->displayPosition;

    pRenderData->labels.PushBack(&pFeedItem->labelConfig);

    ++this->visibleItems;
  }
  udReleaseMutex(this->pMutex);
}

void vcLiveFeed::ApplyDelta(vcState * /*pProgramState*/)
{

}

void vcLiveFeed::HandleImGui(vcState * /*pProgramState*/, size_t * /*pItemID*/)
{
  ImGui::Text("Feed Items: %zu", this->feedItems.size());
  ImGui::Text("Visible Items: %zu", this->visibleItems);

  ImGui::Text("Next update in %.2f seconds", (this->lastUpdateTime + this->updateFrequency) - vcTime_GetEpochSecsF());

  // Update Frequency
  {
    const double updateFrequencyMinValue = 5.0;
    const double updateFrequencyMaxValue = 300.0;

    if (ImGui::SliderScalar("Update Frequency", ImGuiDataType_Double, &this->updateFrequency, &updateFrequencyMinValue, &updateFrequencyMaxValue, "%.0f seconds"))
      this->updateFrequency = udClamp(this->updateFrequency, updateFrequencyMinValue, updateFrequencyMaxValue);
  }

  // Decay Frequency
  {
    const double decayFrequencyMinValue = 30.0;
    const double decayFrequencyMaxValue = 604800.0; // 1 week

    if (ImGui::SliderScalar("Decay Frequency", ImGuiDataType_Double, &this->decayFrequency, &decayFrequencyMinValue, &decayFrequencyMaxValue, "%.0f seconds", 4.f))
    {
      this->decayFrequency = udClamp(this->decayFrequency, decayFrequencyMinValue, decayFrequencyMaxValue);

      double recently = vcTime_GetEpochSecsF() - this->decayFrequency;

      udLockMutex(this->pMutex);
      for (vcLiveFeedItem *pFeedItem : this->feedItems)
        pFeedItem->visible = (pFeedItem->lastUpdated > recently);
      udReleaseMutex(this->pMutex);
    }

    // Simplify Distance
    {
      const double detailDistanceMinValue = 1.0;
      const double detailDistanceMaxValue = 2500.0;

      if (ImGui::SliderScalar("Detail Distance", ImGuiDataType_Double, &this->detailDistance, &detailDistanceMinValue, &detailDistanceMaxValue, "%.0f", 1.5f))
        this->detailDistance = udClamp(this->detailDistance, detailDistanceMinValue, detailDistanceMaxValue);
    }

    // Falloff Distance
    {
      const double falloffDistanceMinValue = 1.0;
      const double falloffDistanceMaxValue = 100000.0;

      if (ImGui::SliderScalar("Falloff Distance", ImGuiDataType_Double, &this->falloffDistance, &falloffDistanceMinValue, &falloffDistanceMaxValue, "%.0f", 3.f))
        this->falloffDistance = udClamp(this->falloffDistance, falloffDistanceMinValue, falloffDistanceMaxValue);
    }

  }
}

void vcLiveFeed::Cleanup(vcState * /*pProgramState*/)
{
  udFree(pName);

  udLockMutex(this->pMutex);
  for (vcLiveFeedItem *pObject : this->feedItems)
    udFree(pObject);
  udReleaseMutex(this->pMutex);

  udDestroyMutex(&this->pMutex);

  this->feedItems.clear();

  this->vcLiveFeed::~vcLiveFeed();
}

void vcLiveFeed_AddToList(vcState *pProgramState)
{
  vcLiveFeed *pLiveFeed = udAllocType(vcLiveFeed, 1, udAF_Zero);
  pLiveFeed = new (pLiveFeed) vcLiveFeed();
  pLiveFeed->visible = true;

  pLiveFeed->pName = udStrdup("Live Feed");
  pLiveFeed->type = vcSOT_LiveFeed;

  // Just setting this as a nice default
  pLiveFeed->updateFrequency = 15.0;
  pLiveFeed->decayFrequency = 300.0;
  pLiveFeed->detailDistance = 500.0;
  pLiveFeed->falloffDistance = 25000.0;
  pLiveFeed->pMutex = udCreateMutex();

  udStrcpy(pLiveFeed->typeStr, sizeof(pLiveFeed->typeStr), "IOT");
  pLiveFeed->loadStatus = vcSLS_Pending;

  pLiveFeed->AddItem(pProgramState);
}
