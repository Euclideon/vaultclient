#include "vcConvert.h"

#include "imgui.h"
#include "stb_image.h"
#include "vdkConvert.h"

#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"
#include "udPlatform/udDebug.h"

#include "vcState.h"
#include "gl/vcTexture.h"

enum vcConvertQueueStatus
{
  vcCQS_Preparing,
  vcCQS_Queued,
  vcCQS_QueuedPendingLicense,
  vcCQS_Running,
  vcCQS_Completed,
  vcCQS_Cancelled,
  vcCQS_Failed,

  vcCQS_Count
};

const char *statusNames[] =
{
  "Awaiting User Input",
  "Queued",
  "Waiting For Convert License",
  "Running",
  "Completed",
  "Cancelled",
  "Failed"
};

UDCOMPILEASSERT(UDARRAYSIZE(statusNames) == vcCQS_Count, "Not Enough Status Names");

struct vcConvertItem
{
  vdkConvertContext *pConvertContext;
  const vdkConvertInfo *pConvertInfo;
  vcConvertQueueStatus status;

  struct
  {
    bool isDirty;
    vcTexture *pTexture;
    int width;
    int height;
  } watermark;
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
    if (udWaitSemaphore(pConvertContext->pSemaphore, 1000) != 0)
      continue;

    // Convert Here
    while (true)
    {
      vcConvertItem *pItem = nullptr;
      bool wasPending = false;

      udLockMutex(pConvertContext->pMutex);
      for (size_t i = 0; i < pConvertContext->jobs.length; ++i)
      {
        if (pConvertContext->jobs[i]->status == vcCQS_Queued || pConvertContext->jobs[i]->status == vcCQS_QueuedPendingLicense)
        {
          wasPending = (pConvertContext->jobs[i]->status == vcCQS_QueuedPendingLicense);

          pItem = pConvertContext->jobs[i];
          pItem->status = vcCQS_Running;
          break;
        }
      }
      udReleaseMutex(pConvertContext->pMutex);

      if (pItem == nullptr)
        break;

      // If the file exists, and we weren't in the pending license state.
      // If the item was pending a license state, we already checked if the
      // user wanted to override the file. We know this because if the user
      // clicks No, the status is set to Pending and won't be processed again.
      if (!wasPending && udFileExists(pItem->pConvertInfo->pOutputName) == udR_Success)
      {
        const SDL_MessageBoxButtonData buttons[] = {
          { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
          { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
        };

        SDL_MessageBoxColorScheme colorScheme = {
          { /* .colors (.r, .g, .b) */
            /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
            { 255, 0, 0 },
            /* [SDL_MESSAGEBOX_COLOR_TEXT] */
            { 0, 255, 0 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
            { 255, 255, 0 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
            { 0, 0, 255 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
            { 255, 0, 255 }
          }
        };

        SDL_MessageBoxData messageboxdata = {
          SDL_MESSAGEBOX_INFORMATION, /* .flags */
          NULL, /* .pWindow */
          "File Exists", /* .title */
          udTempStr("The file \"%s\" already exists.\nDo you want to override the file?", pItem->pConvertInfo->pOutputName), /* .message */
          SDL_arraysize(buttons), /* .numbuttons */
          buttons, /* .buttons */
          &colorScheme /* .colorScheme */
        };

        // Skip this item if the user declines to override
        int buttonid = 0;
        if (SDL_ShowMessageBox(&messageboxdata, &buttonid) != 0 || buttonid == 0)
        {
          pItem->status = vcCQS_Preparing;
          continue;
        }
      }

      vdkError conversionStatus = vdkConvert_DoConvert(pProgramState->pVDKContext, pItem->pConvertContext);

      if (conversionStatus == vE_InvalidLicense || conversionStatus == vE_Pending)
        pItem->status = vcCQS_QueuedPendingLicense;
      else if (conversionStatus == vE_Cancelled)
        pItem->status = vcCQS_Cancelled;
      else if (conversionStatus != vE_Success)
        pItem->status = vcCQS_Failed;
      else // succeeded
        pItem->status = vcCQS_Completed;

      if (pItem->status == vcCQS_QueuedPendingLicense)
        udSleep(1000); // Sleep for a second before trying again
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

void vcConvert_RemoveJob(vcState *pProgramState, size_t index)
{
  udLockMutex(pProgramState->pConvertContext->pMutex);

  vcConvertItem *pItem = pProgramState->pConvertContext->jobs[index];
  vcTexture_Destroy(&pItem->watermark.pTexture);
  pProgramState->pConvertContext->jobs.RemoveAt(index);
  vdkConvert_DestroyContext(pProgramState->pVDKContext, &pItem->pConvertContext);
  udFree(pItem);

  udReleaseMutex(pProgramState->pConvertContext->pMutex);
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

  while (pProgramState->pConvertContext->jobs.length > 0)
    vcConvert_RemoveJob(pProgramState, 0);

  pProgramState->pConvertContext->jobs.Deinit();
  udFree(pProgramState->pConvertContext);
}

void vcConvert_AddEmptyJob(vcState *pProgramState, vcConvertItem **ppNextItem)
{
  vcConvertItem *pNextItem = udAllocType(vcConvertItem, 1, udAF_Zero);

  udLockMutex(pProgramState->pConvertContext->pMutex);

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
  char outputName[vcMaxPathLength];
  char tempDirectory[vcMaxPathLength];

  // Convert Jobs
  ImGui::Text("Convert Jobs");
  ImGui::Separator();

  for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
  {
    bool selected = (pProgramState->pConvertContext->selectedItem == i);

    udSprintf(tempBuffer, UDARRAYSIZE(tempBuffer), "X##convertjob_%llu", i);
    if (ImGui::Button(tempBuffer, ImVec2(20, 20)))
    {
      if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
      {
        vdkConvert_Cancel(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[i]->pConvertContext);
      }
      else if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Queued)
      {
        pProgramState->pConvertContext->jobs[i]->status = vcCQS_Preparing;
      }
      else
      {
        vcConvert_RemoveJob(pProgramState, i);

        if (pProgramState->pConvertContext->selectedItem >= i && pProgramState->pConvertContext->selectedItem != 0)
          --pProgramState->pConvertContext->selectedItem;

        --i;
      }
      continue;
    }
    float buttonWidth = ImGui::GetItemRectSize().x;
    ImGui::SameLine();

    udSprintf(tempBuffer, UDARRAYSIZE(tempBuffer), "%s (%s)##convertjob_%llu", pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName, statusNames[pProgramState->pConvertContext->jobs[i]->status], i);

    ImVec2 selectablePos = ImVec2(ImGui::GetContentRegionMax().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x * 2, 0);
    if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
      selectablePos.x -= ImGui::GetContentRegionMax().x / 2.f;
    else if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Completed)
      selectablePos.x -= ImGui::GetContentRegionMax().x * 0.2f;

    if (ImGui::Selectable(tempBuffer, selected, ImGuiSelectableFlags_None, selectablePos))
      pProgramState->pConvertContext->selectedItem = i;

    if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
    {
      ImGui::SameLine();
      const float progressRatio = 0.7f; // How much reading is compared to writing (0.8 would be 80% of the progress is writing)

      if (pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem != pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems)
      {
        vdkConvert_GetItemInfo(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[i]->pConvertContext, pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem, &itemInfo);

        float perItemAmount = progressRatio / pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems;
        float currentFileProgress = perItemAmount * itemInfo.pointsRead / itemInfo.pointsCount;
        float completedFileProgress = perItemAmount * pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem;

        ImGui::ProgressBar(currentFileProgress + completedFileProgress, ImVec2(-1, 0), udTempStr("Reading File %s/%s...", udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem+1), udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems)));
      }
      else
      {
        uint64_t pointsWritten = (pProgramState->pConvertContext->jobs[i]->pConvertInfo->outputPointCount + pProgramState->pConvertContext->jobs[i]->pConvertInfo->discardedPointCount);
        uint64_t pointsTotal = pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalPointsRead;

        ImGui::ProgressBar(progressRatio + (1.f - progressRatio) * pointsWritten / pointsTotal, ImVec2(-1, 0), udTempStr("Writing Points %s/%s", udCommaInt(pointsWritten), udCommaInt(pointsTotal)));
      }
    }
    else if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Completed)
    {
      ImGui::SameLine();
      if (ImGui::Button(udTempStr("Add to Scene##vcConvLoad_%llu", i), ImVec2(-1, 0)))
        pProgramState->loadList.PushBack(udStrdup(pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName));
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

  udSprintf(outputName, UDARRAYSIZE(outputName), "%s", pSelectedJob->pConvertInfo->pOutputName);
  if (ImGui::InputText("Output Name", outputName, UDARRAYSIZE(outputName)))
    vdkConvert_SetOutputFilename(pProgramState->pVDKContext, pSelectedJob->pConvertContext, outputName);

  udSprintf(tempDirectory, UDARRAYSIZE(tempDirectory), "%s", pSelectedJob->pConvertInfo->pTempFilesPrefix);
  if (ImGui::InputText("Temp Directory", tempDirectory, UDARRAYSIZE(tempDirectory)))
    vdkConvert_SetTempDirectory(pProgramState->pVDKContext, pSelectedJob->pConvertContext, tempDirectory);

  ImGui::Separator();

  if (pSelectedJob->status == vcCQS_Preparing)
    ImGui::CheckboxFlags("Continue processing after corrupt/incomplete data (where possible)", (uint32_t*)&pSelectedJob->pConvertInfo->ignoreParseErrors, 1);

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

  if (pSelectedJob->watermark.isDirty || pSelectedJob->watermark.pTexture != nullptr)
  {
    ImGui::Separator();

    if (pSelectedJob->watermark.isDirty)
    {
      vcTexture_Destroy(&pSelectedJob->watermark.pTexture);
      uint8_t *pData = nullptr;
      size_t dataSize = 0;
      if (udBase64Decode(&pData, &dataSize, pSelectedJob->pConvertInfo->pWatermark) == udR_Success)
      {
        int comp;
        stbi_uc *pImg = stbi_load_from_memory(pData, (int)dataSize, &pSelectedJob->watermark.width, &pSelectedJob->watermark.height, &comp, 4);

        vcTexture_Create(&pSelectedJob->watermark.pTexture, pSelectedJob->watermark.width, pSelectedJob->watermark.height, pImg);

        stbi_image_free(pImg);
      }

      udFree(pData);
    }

    ImGui::Image(pSelectedJob->watermark.pTexture, ImVec2((float)pSelectedJob->watermark.width, (float)pSelectedJob->watermark.height));
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
    else if (pSelectedJob->status == vcCQS_Completed && ImGui::Button("Reset"))
    {
      ImGui::Separator();
      vdkConvertContext *pConvertContext = nullptr;
      const vdkConvertInfo *pConvertInfo = nullptr;
      vdkConvert_CreateContext(pProgramState->pVDKContext, &pConvertContext);
      vdkConvert_GetInfo(pProgramState->pVDKContext, pConvertContext, &pConvertInfo);

      for (size_t i = 0; i < pSelectedJob->pConvertInfo->totalItems; i++)
      {
        vdkConvert_GetItemInfo(pProgramState->pVDKContext, pSelectedJob->pConvertContext, i, &itemInfo);
        vdkConvert_AddItem(pProgramState->pVDKContext, pConvertContext, itemInfo.pFilename);
      }

      vdkConvert_SetOutputFilename(pProgramState->pVDKContext, pConvertContext, pSelectedJob->pConvertInfo->pOutputName);
      vdkConvert_SetTempDirectory(pProgramState->pVDKContext, pConvertContext, pSelectedJob->pConvertInfo->pTempFilesPrefix);
      vdkConvert_SetPointResolution(pProgramState->pVDKContext, pConvertContext, pSelectedJob->pConvertInfo->overrideResolution != 0, pSelectedJob->pConvertInfo->pointResolution);
      vdkConvert_SetSRID(pProgramState->pVDKContext, pConvertContext, pSelectedJob->pConvertInfo->overrideSRID != 0, pSelectedJob->pConvertInfo->srid);

      vdkConvert_DestroyContext(pProgramState->pVDKContext, &pSelectedJob->pConvertContext);

      pSelectedJob->pConvertContext = pConvertContext;
      pSelectedJob->pConvertInfo = pConvertInfo;
      pSelectedJob->status = vcCQS_Preparing;
    }
  }
}

bool vcConvert_AddFile(vcState *pProgramState, const char *pFilename)
{
  vcConvertItem *pSelectedJob = nullptr;

  if (pProgramState->pConvertContext->selectedItem < pProgramState->pConvertContext->jobs.length)
  {
    pSelectedJob = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem];
    if (pSelectedJob->status != vcCQS_Preparing)
      pSelectedJob = nullptr;
  }

  for (size_t i = pProgramState->pConvertContext->jobs.length; i > 0 && pSelectedJob == nullptr; --i)
  {
    if (pProgramState->pConvertContext->jobs[i - 1]->status == vcCQS_Preparing)
      pSelectedJob = pProgramState->pConvertContext->jobs[i - 1];
  }

  if (pSelectedJob == nullptr)
    vcConvert_AddEmptyJob(pProgramState, &pSelectedJob);

  if (vdkConvert_AddItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, pFilename) == vE_Success)
  {
    pProgramState->settings.window.windowsOpen[vcdConvert] = true;
    return true;
  }

  // Long shot but maybe a watermark?
  if (vdkConvert_AddWatermark(pProgramState->pVDKContext, pSelectedJob->pConvertContext, pFilename) == vE_Success)
  {
    pSelectedJob->watermark.isDirty = true;
    pProgramState->settings.window.windowsOpen[vcdConvert] = true;
    return true;
  }

  return false;
}
