#include "vcConvert.h"

#include "imgui.h"
#include "stb_image.h"
#include "vcStrings.h"
#include "vcModals.h"
#include "vCore/vStringFormat.h"

#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udThread.h"
#include "udPlatform/udDebug.h"

const char *statusNames[] =
{
  "convertAwaiting",
  "convertQueued",
  "convertAwaitingLicense",
  "convertRunning",
  "convertCompleted",
  "convertCancelled",
  "convertFailed"
};

void vcConvert_ResetConvert(vcState *pProgramState, vcConvertItem *pConvertItem, vdkConvertItemInfo *pItemInfo);

UDCOMPILEASSERT(UDARRAYSIZE(statusNames) == vcCQS_Count, "Not Enough Status Names");

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
      const char *pFileExistsMsg = nullptr;
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
        pFileExistsMsg = vStringFormat(vcString::Get("convertFileExistsMessage"), pItem->pConvertInfo->pOutputName);
        SDL_MessageBoxData messageboxdata = {
          SDL_MESSAGEBOX_INFORMATION, /* .flags */
          NULL, /* .pWindow */
          vcString::Get("convertFileExistsTitle"), /* .title */
          pFileExistsMsg, /* .message */
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
      udFree(pFileExistsMsg);

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
  udFree(pItem->watermark.pFilename);
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
  char localizationBuffer[512];
  char outputName[vcMaxPathLength];
  char tempDirectory[vcMaxPathLength];

  // Convert Jobs
  ImGui::Text("%s", vcString::Get("convertJobs"));
  ImGui::Separator();

  for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
  {
    bool selected = (pProgramState->pConvertContext->selectedItem == i);

    udSprintf(tempBuffer, UDARRAYSIZE(tempBuffer), "X##convertjob_%zu", i);
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

    udSprintf(tempBuffer, UDARRAYSIZE(tempBuffer), "%s (%s)##convertjob_%zu", pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName, vcString::Get(statusNames[pProgramState->pConvertContext->jobs[i]->status]), i);

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

        const char *fileIndexStrings[] = { udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->currentInputItem + 1), udCommaInt(pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalItems) };

        ImGui::ProgressBar(currentFileProgress + completedFileProgress, ImVec2(-1, 0), vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingFile"), fileIndexStrings, udLengthOf(fileIndexStrings)));
      }
      else
      {
        uint64_t pointsWritten = (pProgramState->pConvertContext->jobs[i]->pConvertInfo->outputPointCount + pProgramState->pConvertContext->jobs[i]->pConvertInfo->discardedPointCount);
        uint64_t pointsTotal = pProgramState->pConvertContext->jobs[i]->pConvertInfo->totalPointsRead;

        const char *strings[] = { udCommaInt(pointsWritten), udCommaInt(pointsTotal) };

        ImGui::ProgressBar(progressRatio + (1.f - progressRatio) * pointsWritten / pointsTotal, ImVec2(-1, 0), vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertWritingPoints"), strings, udLengthOf(strings)));
      }
    }
    else if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Completed)
    {
      ImGui::SameLine();
      if (ImGui::Button(udTempStr("%s##vcConvLoad_%zu", vcString::Get("convertAddToScene"), i), ImVec2(-1, 0)))
        pProgramState->loadList.push_back(udStrdup(pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName));
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

  ImGui::TextUnformatted(vcString::Get("convertSettings"));
  ImGui::Separator();

  udSprintf(outputName, UDARRAYSIZE(outputName), "%s", pSelectedJob->pConvertInfo->pOutputName);
  if (ImGui::InputText(vcString::Get("convertOutputName"), outputName, UDARRAYSIZE(outputName)))
    vdkConvert_SetOutputFilename(pProgramState->pVDKContext, pSelectedJob->pConvertContext, outputName);

  udSprintf(tempDirectory, UDARRAYSIZE(tempDirectory), "%s", pSelectedJob->pConvertInfo->pTempFilesPrefix);
  if (ImGui::InputText(vcString::Get("convertTempDirectory"), tempDirectory, UDARRAYSIZE(tempDirectory)))
    vdkConvert_SetTempDirectory(pProgramState->pVDKContext, pSelectedJob->pConvertContext, tempDirectory);

  ImGui::Separator();

  if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
  {
    bool skipErrorsWherePossible = pSelectedJob->pConvertInfo->skipErrorsWherePossible;
    if (ImGui::Checkbox(vcString::Get("convertContinueOnCorrupt"), &skipErrorsWherePossible))
      vdkConvert_SetSkipErrorsWherePossible(pProgramState->pVDKContext, pSelectedJob->pConvertContext, skipErrorsWherePossible);
  }

  // Resolution
  ImGui::Text("%s: %.6f", vcString::Get("convertPointResolution"), pSelectedJob->pConvertInfo->pointResolution);
  if (pSelectedJob->pConvertInfo->pointResolution != pSelectedJob->pConvertInfo->minPointResolution || pSelectedJob->pConvertInfo->pointResolution != pSelectedJob->pConvertInfo->maxPointResolution)
  {
    ImGui::SameLine();
    if (pSelectedJob->pConvertInfo->minPointResolution == pSelectedJob->pConvertInfo->maxPointResolution)
      ImGui::Text("(%s %.6f)", vcString::Get("convertPointResolution"), pSelectedJob->pConvertInfo->minPointResolution);
    else
      ImGui::Text("(%s %.6f - %.6f)", vcString::Get("convertInputDataRanges"), pSelectedJob->pConvertInfo->minPointResolution, pSelectedJob->pConvertInfo->maxPointResolution);
  }

  // Override Resolution
  if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
  {
    bool overrideResolution = pSelectedJob->pConvertInfo->overrideResolution;
    double resolution = pSelectedJob->pConvertInfo->pointResolution;

    ImGui::SameLine();
    if (ImGui::Checkbox(udTempStr("%s##ConvertResolutionOverride", vcString::Get("convertOverrideResolution")), &overrideResolution))
      vdkConvert_SetPointResolution(pProgramState->pVDKContext, pSelectedJob->pConvertContext, overrideResolution, resolution);

    if (overrideResolution)
    {
      ImGui::Indent();

      if (ImGui::InputDouble(udTempStr("%s##ConvertResolution", vcString::Get("convertPointResolution")), &resolution, 0.0001, 0.01, "%.6f"))
        vdkConvert_SetPointResolution(pProgramState->pVDKContext, pSelectedJob->pConvertContext, 1, resolution);

      ImGui::Unindent();
    }

    // Override SRID
    bool overrideSRID = pSelectedJob->pConvertInfo->overrideSRID;
    int srid = pSelectedJob->pConvertInfo->srid;

    if (ImGui::Checkbox(udTempStr("%s##ConvertOverrideGeolocate", vcString::Get("convertOverrideGeolocation")), &overrideSRID))
      vdkConvert_SetSRID(pProgramState->pVDKContext, pSelectedJob->pConvertContext, overrideSRID, srid);

    if (overrideSRID)
    {
      if (ImGui::InputInt(udTempStr("%s##ConvertSRID", vcString::Get("convertSRID")), &srid))
        vdkConvert_SetSRID(pProgramState->pVDKContext, pSelectedJob->pConvertContext, overrideSRID, srid);

      // While overrideSRID isn't required for global offset to work, in order to simplify the UI for most users, we hide global offset when override SRID is disabled
      double globalOffset[3];
      memcpy(globalOffset, pSelectedJob->pConvertInfo->globalOffset, sizeof(globalOffset));
      if (ImGui::InputScalarN(vcString::Get("convertGlobalOffset"), ImGuiDataType_Double, &globalOffset, 3))
      {
        for (int i = 0; i < 3; ++i)
          globalOffset[i] = udClamp(globalOffset[i], -vcSL_GlobalLimit, vcSL_GlobalLimit);
        vdkConvert_SetGlobalOffset(pProgramState->pVDKContext, pSelectedJob->pConvertContext, globalOffset);
      }
    }
    // Quick Convert
    bool quickConvert = (pSelectedJob->pConvertInfo->everyNth != 0);
    if (ImGui::Checkbox(vcString::Get("convertQuickTest"), &quickConvert))
      vdkConvert_SetEveryNth(pProgramState->pVDKContext, pSelectedJob->pConvertContext, quickConvert ? 1000 : 0);

    ImGui::Separator();
    ImGui::TextUnformatted(vcString::Get("convertMetadata"));

    // Other Metadata
    if (ImGui::InputText(vcString::Get("convertAuthor"), pSelectedJob->author, udLengthOf(pSelectedJob->author)))
      vdkConvert_SetMetadata(pProgramState->pVDKContext, pSelectedJob->pConvertContext, "Author", pSelectedJob->author);

    if (ImGui::InputText(vcString::Get("convertComment"), pSelectedJob->comment, udLengthOf(pSelectedJob->comment)))
      vdkConvert_SetMetadata(pProgramState->pVDKContext, pSelectedJob->pConvertContext, "Comment", pSelectedJob->comment);

    if (ImGui::InputText(vcString::Get("convertCopyright"), pSelectedJob->copyright, udLengthOf(pSelectedJob->copyright)))
      vdkConvert_SetMetadata(pProgramState->pVDKContext, pSelectedJob->pConvertContext, "Copyright", pSelectedJob->copyright);

    if (ImGui::InputText(vcString::Get("convertLicense"), pSelectedJob->license, udLengthOf(pSelectedJob->license)))
      vdkConvert_SetMetadata(pProgramState->pVDKContext, pSelectedJob->pConvertContext, "License", pSelectedJob->license);

    // Watermark
    if (ImGui::Button(vcString::Get("convertLoadWatermark")))
      vcModals_OpenModal(pProgramState, vcMT_LoadWatermark);

    if (pSelectedJob->watermark.pTexture != nullptr)
    {
      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("convertRemoveWatermark")))
      {
        vdkConvert_RemoveWatermark(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext);
        udFree(pSelectedJob->watermark.pFilename);
        pSelectedJob->watermark.width = 0;
        pSelectedJob->watermark.height = 0;
      }
    }

    if (pSelectedJob->watermark.isDirty)
    {
      pSelectedJob->watermark.isDirty = false;
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
  }
  else
  {
    ImGui::Text("%s: %d", vcString::Get("convertSRID"), pSelectedJob->pConvertInfo->srid);
  }

  if (pSelectedJob->watermark.pTexture != nullptr)
    ImGui::Image(pSelectedJob->watermark.pTexture, ImVec2((float)pSelectedJob->watermark.width, (float)pSelectedJob->watermark.height));
  else
    ImGui::TextUnformatted(vcString::Get("convertNoWatermark"));

  ImGui::Separator();
  if (pSelectedJob->pConvertInfo->totalItems > 0)
  {
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);

    vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertInputFiles"), udCommaInt(pSelectedJob->pConvertInfo->totalItems));

    if (ImGui::TreeNodeEx(pSelectedJob->pConvertInfo, 0, "%s", localizationBuffer))
    {
      if ((pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled) && ImGui::Button(vcString::Get("convertRemoveAll")))
      {
        while (pSelectedJob->pConvertInfo->totalItems > 0)
          vdkConvert_RemoveItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, 0);
      }

      ImGui::Columns(3, NULL, true);

      for (size_t i = 0; i < pSelectedJob->pConvertInfo->totalItems; ++i)
      {
        vdkConvert_GetItemInfo(pProgramState->pVDKContext, pSelectedJob->pConvertContext, i, &itemInfo);

        ImGui::TextUnformatted(itemInfo.pFilename);
        ImGui::NextColumn();

        const char *ptCountStrings[] = { udCommaInt(itemInfo.pointsCount), udCommaInt(itemInfo.pointsRead) };

        if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
        {
          if (itemInfo.pointsCount == -1)
            vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
          else
            vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

          ImGui::TextUnformatted(localizationBuffer);

          ImGui::NextColumn();

          if (ImGui::Button(udTempStr("%s##convertitemremove_%zu", vcString::Get("convertRemove"), i)))
          {
            vdkConvert_RemoveItem(pProgramState->pVDKContext, pSelectedJob->pConvertContext, i);
            --i;
          }
        }
        else
        {
          if (pSelectedJob->pConvertInfo->currentInputItem > i) // Already read
          {
            vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadComplete"), ptCountStrings, udLengthOf(ptCountStrings));
            ImGui::TextUnformatted(localizationBuffer);
            ImGui::NextColumn();
            ImGui::ProgressBar(1.f);
          }
          else if (pSelectedJob->pConvertInfo->currentInputItem < i) // Pending read
          {
            if (itemInfo.pointsCount == -1)
              vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
            else
              vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

            ImGui::TextUnformatted(localizationBuffer);
            ImGui::NextColumn();
            ImGui::ProgressBar(0.f);
          }
          else // Currently reading
          {
            if (itemInfo.pointsCount == -1)
              vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
            else
              vStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

            ImGui::TextUnformatted(localizationBuffer);
            ImGui::NextColumn();
            ImGui::ProgressBar(1.f * itemInfo.pointsRead / itemInfo.pointsCount);
          }
        }

        ImGui::NextColumn();
      }

      ImGui::Columns(1);

      ImGui::TreePop();
    }

    if ((pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled) && ImGui::Button(vcString::Get("convertBeginConvert")))
    {
      if (pSelectedJob->status == vcCQS_Cancelled)
        vcConvert_ResetConvert(pProgramState, pSelectedJob, &itemInfo);
      ImGui::Separator();
      pSelectedJob->status = vcCQS_Queued;
      udIncrementSemaphore(pProgramState->pConvertContext->pSemaphore);
    }
    else if (pSelectedJob->status == vcCQS_Completed && ImGui::Button(vcString::Get("convertReset")))
    {
      ImGui::Separator();
      vcConvert_ResetConvert(pProgramState, pSelectedJob, &itemInfo);
    }
  }
}

bool vcConvert_AddFile(vcState *pProgramState, const char *pFilename)
{
  vcConvertItem *pSelectedJob = nullptr;

  if (pProgramState->pConvertContext->selectedItem < pProgramState->pConvertContext->jobs.length)
  {
    pSelectedJob = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem];
    if (pSelectedJob->status != vcCQS_Preparing && pSelectedJob->status != vcCQS_Cancelled)
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
    pProgramState->settings.window.windowsOpen[vcDocks_Convert] = true;
    return true;
  }

  // Long shot but maybe a watermark?
  if (vdkConvert_AddWatermark(pProgramState->pVDKContext, pSelectedJob->pConvertContext, pFilename) == vE_Success)
  {
    udFree(pSelectedJob->watermark.pFilename);
    pSelectedJob->watermark.pFilename = udStrdup(pFilename);
    pSelectedJob->watermark.isDirty = true;
    pProgramState->settings.window.windowsOpen[vcDocks_Convert] = true;
    return true;
  }

  return false;
}

void vcConvert_ResetConvert(vcState *pProgramState, vcConvertItem *pConvertItem, vdkConvertItemInfo *pItemInfo)
{
  vdkConvertContext *pConvertContext = nullptr;
  const vdkConvertInfo *pConvertInfo = nullptr;
  vdkConvert_CreateContext(pProgramState->pVDKContext, &pConvertContext);
  vdkConvert_GetInfo(pProgramState->pVDKContext, pConvertContext, &pConvertInfo);

  for (size_t i = 0; i < pConvertItem->pConvertInfo->totalItems; i++)
  {
    vdkConvert_GetItemInfo(pProgramState->pVDKContext, pConvertItem->pConvertContext, i, pItemInfo);
    vdkConvert_AddItem(pProgramState->pVDKContext, pConvertContext, pItemInfo->pFilename);
  }

  vdkConvert_SetOutputFilename(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->pOutputName);
  vdkConvert_SetTempDirectory(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->pTempFilesPrefix);
  vdkConvert_SetPointResolution(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->overrideResolution != 0, pConvertItem->pConvertInfo->pointResolution);
  vdkConvert_SetSRID(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->overrideSRID != 0, pConvertItem->pConvertInfo->srid);

  vdkConvert_SetSkipErrorsWherePossible(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->skipErrorsWherePossible);
  vdkConvert_SetEveryNth(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->everyNth);

  vdkConvert_SetGlobalOffset(pProgramState->pVDKContext, pConvertContext, pConvertItem->pConvertInfo->globalOffset);

  vdkConvert_AddWatermark(pProgramState->pVDKContext, pConvertContext, pConvertItem->watermark.pFilename);

  vdkConvert_SetMetadata(pProgramState->pVDKContext, pConvertContext, "Author", pConvertItem->author);
  vdkConvert_SetMetadata(pProgramState->pVDKContext, pConvertContext, "Comment", pConvertItem->comment);
  vdkConvert_SetMetadata(pProgramState->pVDKContext, pConvertContext, "Copyright", pConvertItem->copyright);
  vdkConvert_SetMetadata(pProgramState->pVDKContext, pConvertContext, "License", pConvertItem->license);

  vdkConvert_DestroyContext(pProgramState->pVDKContext, &pConvertItem->pConvertContext);

  pConvertItem->pConvertContext = pConvertContext;
  pConvertItem->pConvertInfo = pConvertInfo;
  pConvertItem->status = vcCQS_Preparing;
}
