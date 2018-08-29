#include "vcConvert.h"

#include "imgui.h"
#include "vdkConvert.h"

#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"
#include "udPlatform/udDebug.h"

#include "vcState.h"

enum vcConvertQueueStatus
{
  vcCQS_Preparing,
  vcCQS_Queued,
  vcCQS_Running,
  vcCQS_Completed,

  vcCQS_Count
};

const char *statusNames[] =
{
  "Preparing",
  "Queued",
  "Running",
  "Completed"
};

UDCOMPILEASSERT(UDARRAYSIZE(statusNames) == vcCQS_Count, "Not Enough Status Names");

struct vcConvertItem
{
  vdkConvertContext *pConvertContext;
  const vdkConvertInfo *pConvertInfo;
  vcConvertQueueStatus status;
};

struct vcConvertContext
{
  udChunkedArray<vcConvertItem*> jobs;
  bool threadRunning;
  udSemaphore *pSemaphore;
  udMutex *pMutex;
  udThread *pThread;

  size_t selectedItem;
};

uint32_t vcConvert_Thread(void *pVoidState)
{
  vcState *pProgramState = (vcState*)pVoidState;
  vcConvertContext *pConvertContext = pProgramState->pConvertContext;

  while (pConvertContext->threadRunning)
  {
    int loadStatus = udWaitSemaphore(pConvertContext->pSemaphore, 1000);

    if (loadStatus != 0)
      continue;

    // Convert Here
    while (true)
    {
      vcConvertItem *pItem = nullptr;

      udLockMutex(pConvertContext->pMutex);
      for (size_t i = 0; i < pConvertContext->jobs.length; ++i)
      {
        if (pConvertContext->jobs[i]->status == vcCQS_Queued)
        {
          pItem = pConvertContext->jobs[i];
          pItem->status = vcCQS_Running;
          break;
        }
      }
      udReleaseMutex(pConvertContext->pMutex);

      if (pItem == nullptr)
        break;

      vdkConvert_DoConvert(pProgramState->pVDKContext, pItem->pConvertContext);
      pItem->status = vcCQS_Completed;
    }
  }

  return 0;
}

void vcConvert_Init(vcState *pProgramState)
{
  if (pProgramState->pConvertContext != nullptr)
    return;

  pProgramState->pConvertContext = udAllocType(vcConvertContext, 1, udAF_Zero);

  pProgramState->pConvertContext->jobs.Init(32);

  pProgramState->pConvertContext->pMutex = udCreateMutex();
  pProgramState->pConvertContext->pSemaphore = udCreateSemaphore();
  pProgramState->pConvertContext->threadRunning = true;
  udThread_Create(&pProgramState->pConvertContext->pThread, vcConvert_Thread, pProgramState);
}

void vcConvert_Deinit(vcState *pProgramState)
{
  if (pProgramState->pConvertContext == nullptr)
    return;

  pProgramState->pConvertContext->threadRunning = false;
  udIncrementSemaphore(pProgramState->pConvertContext->pSemaphore);

  udThread_Join(pProgramState->pConvertContext->pThread);

  udDestroyMutex(&pProgramState->pConvertContext->pMutex);
  udDestroySemaphore(&pProgramState->pConvertContext->pSemaphore);
  udThread_Destroy(&pProgramState->pConvertContext->pThread);

  pProgramState->pConvertContext->jobs.Deinit();
  udFree(pProgramState->pConvertContext);
}

void vcConvert_AddEmptyJob(vcState *pProgramState, vcConvertItem **ppNextItem)
{
  udLockMutex(pProgramState->pConvertContext->pMutex);

  vcConvertItem *pNextItem = udAllocType(vcConvertItem, 1, udAF_Zero);
  pProgramState->pConvertContext->jobs.PushBack(pNextItem);
  vdkConvert_CreateContext(pProgramState->pVDKContext, &pNextItem->pConvertContext);
  vdkConvert_GetInfo(pProgramState->pVDKContext, pNextItem->pConvertContext, &pNextItem->pConvertInfo);

  udReleaseMutex(pProgramState->pConvertContext->pMutex);

  *ppNextItem = pNextItem;
}

void vcConvert_ShowUI(vcState *pProgramState)
{
  vcConvertItem *pSelectedJob = nullptr;
  vdkConvertItemInfo itemInfo;
  char tempBuffer[256];

  // Convert Jobs
  ImGui::Text("Convert Jobs");
  ImGui::Separator();

  for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
  {
    bool selected = (pProgramState->pConvertContext->selectedItem == i);

    udSprintf(tempBuffer, UDARRAYSIZE(tempBuffer), "%s (%s)##convertjob_%llu", pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName, statusNames[pProgramState->pConvertContext->jobs[i]->status], i);

    if (ImGui::Selectable(tempBuffer, &selected))
      pProgramState->pConvertContext->selectedItem = i;

    if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
    {
      ImGui::SameLine();

      //The two sections below are 50% each (hence the 0.5f's)
      if (pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem != pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems)
      {
        vdkConvert_GetItemInfo(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[i]->pConvertContext, pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem, &itemInfo);

        float perItemAmount = 0.5f / pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems;
        float currentFileProgress = perItemAmount * itemInfo.pointsRead / itemInfo.pointsCount;
        float completedFileProgress = perItemAmount * pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem;

        ImGui::ProgressBar(currentFileProgress + completedFileProgress, ImVec2(-1, 0), udTempStr("Reading File %s/%s...", udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem+1), udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems)));
      }
      else
      {
        ImGui::ProgressBar(0.5f + 0.5f * (pProgramState->pConvertContext->jobs[i]->pConvertInfo->outputPointCount + pProgramState->pConvertContext->jobs[i]->pConvertInfo->discardedPointCount) / pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalPointsRead, ImVec2(-1, 0), udTempStr("Writing Points %s/%s", udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->outputPointCount + pProgramState->pConvertContext->jobs[i]->pConvertInfo->discardedPointCount), udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalPointsRead)));
      }
    }
  }

  if (pProgramState->pConvertContext->selectedItem >= pProgramState->pConvertContext->jobs.length)
    vcConvert_AddEmptyJob(pProgramState, &pSelectedJob);
  else
    pSelectedJob = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem];

  // Options pane --------------------------------
  ImGui::Separator();
  ImGui::Separator();
  ImGui::Separator();

  ImGui::Text("Convert Settings");
  ImGui::Separator();

  ImGui::Text("Output Name: %s", pSelectedJob->pConvertInfo->pOutputName);
  ImGui::Text("TempDirectory: %s", pSelectedJob->pConvertInfo->pTempFilesPrefix);

  ImGui::Separator();

  if (pSelectedJob->status == vcCQS_Preparing)
    ImGui::CheckboxFlags("Skip errors where possible", (uint32_t*)&pSelectedJob->pConvertInfo->ignoreParseErrors, 1);

  // Resolution
  ImGui::Text("Point resolution: %.6f", pSelectedJob->pConvertInfo->pointResolution);
  if (pSelectedJob->pConvertInfo->pointResolution != pSelectedJob->pConvertInfo->minPointResolution || pSelectedJob->pConvertInfo->pointResolution != pSelectedJob->pConvertInfo->maxPointResolution)
  {
    ImGui::SameLine();
    if (pSelectedJob->pConvertInfo->minPointResolution == pSelectedJob->pConvertInfo->maxPointResolution)
      ImGui::Text("(input data is %.6f)", pSelectedJob->pConvertInfo->minPointResolution);
    else
      ImGui::Text("(input data ranges from %.6f to %.6f)", pSelectedJob->pConvertInfo->minPointResolution, pSelectedJob->pConvertInfo->maxPointResolution);
  }

  // Override Resolution
  if (pSelectedJob->status == vcCQS_Preparing)
  {
    bool overrideResolution = pSelectedJob->pConvertInfo->overrideResolution ? 1 : 0;
    double resolution = pSelectedJob->pConvertInfo->pointResolution;

    ImGui::SameLine();
    if (ImGui::Checkbox("Override##ConvertResolutionOverride", &overrideResolution))
      vdkConvert_SetPointResolution(pProgramState->pVDKContext, pSelectedJob->pConvertContext, overrideResolution ? 1 : 0, resolution);

    if (overrideResolution && ImGui::InputDouble("Point Resolution##ConvertResolution", &resolution, 0.0001, 0.01, "%.6f"))
      vdkConvert_SetPointResolution(pProgramState->pVDKContext, pSelectedJob->pConvertContext, 1, resolution);
  }

  // SRID
  ImGui::Text("SRID: %d", pSelectedJob->pConvertInfo->srid);

  // Override SRID
  if (pSelectedJob->status == vcCQS_Preparing)
  {
    bool overrideSRID = pSelectedJob->pConvertInfo->overrideSRID ? 1 : 0;
    int srid = pSelectedJob->pConvertInfo->srid;

    ImGui::SameLine();
    if (ImGui::Checkbox("Override##ConvertSRIDOverride", &overrideSRID))
      vdkConvert_SetSRID(pProgramState->pVDKContext, pSelectedJob->pConvertContext, overrideSRID ? 1 : 0, srid);

    if (overrideSRID && ImGui::InputInt("SRID##ConvertSRID", &srid))
      vdkConvert_SetSRID(pProgramState->pVDKContext, pSelectedJob->pConvertContext, 1, srid);
  }

  ImGui::Separator();
  if (pSelectedJob->pConvertInfo->totalItems > 0)
  {
    if (ImGui::TreeNodeEx(pSelectedJob->pConvertInfo, 0, "Input Files (%s files)", udCommaInt(pSelectedJob->pConvertInfo->totalItems)))
    {
      if (pSelectedJob->status == vcCQS_Preparing && ImGui::Button("Remove All Files"))
      {
        while (pSelectedJob->pConvertInfo->totalItems > 0)
          vdkConvert_RemoveItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, 0);
      }

      ImGui::Columns(3, NULL, true);

      for (size_t i = 0; i < pSelectedJob->pConvertInfo->totalItems; ++i)
      {
        vdkConvert_GetItemInfo(pProgramState->pVDKContext, pSelectedJob->pConvertContext, i, &itemInfo);

        ImGui::Text("%s", itemInfo.pFilename);
        ImGui::NextColumn();

        if (pSelectedJob->status == vcCQS_Preparing)
        {
          ImGui::Text("Points: %s", udCommaInt(itemInfo.pointsCount));

          ImGui::NextColumn();

          if (ImGui::Button(udTempStr("Remove##convertitemremove_%llu", i)))
          {
            vdkConvert_RemoveItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, i);
            --i;
          }
        }
        else
        {
          if (pSelectedJob->pConvertInfo->currentInputItem > i) // Already read
          {
            ImGui::Text("Read %spts", udCommaInt(itemInfo.pointsRead));
            ImGui::NextColumn();
            ImGui::ProgressBar(1.f);
          }
          else if (pSelectedJob->pConvertInfo->currentInputItem < i) // Pending read
          {
            ImGui::Text("%spts estimate", udCommaInt(itemInfo.pointsCount));
            ImGui::NextColumn();
            ImGui::ProgressBar(0.f);
          }
          else // Currently reading
          {
            ImGui::Text("Reading... %s/%spts", udCommaInt(itemInfo.pointsRead), udCommaInt(itemInfo.pointsCount));
            ImGui::NextColumn();
            ImGui::ProgressBar(1.f * itemInfo.pointsRead / itemInfo.pointsCount);
          }
        }

        ImGui::NextColumn();
      }

      ImGui::Columns(1);

      ImGui::TreePop();
    }

    if (pSelectedJob->status == vcCQS_Preparing && ImGui::Button("Begin Convert"))
    {
      ImGui::Separator();
      pSelectedJob->status = vcCQS_Queued;
      udIncrementSemaphore(pProgramState->pConvertContext->pSemaphore);
    }
  }
}

bool vcConvert_AddFile(vcState *pProgramState, const char *pFilename)
{
  vcConvertItem *pSelectedJob = nullptr;

  for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
  {
    if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Preparing)
      pSelectedJob = pProgramState->pConvertContext->jobs[i];
  }

  if (pSelectedJob == nullptr)
    vcConvert_AddEmptyJob(pProgramState, &pSelectedJob);

  if (vdkConvert_AddItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, pFilename) == vE_Success)
  {
    pProgramState->settings.window.windowsOpen[vcdConvert] = true;
    return true;
  }

  return false;
}
