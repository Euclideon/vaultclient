#include "vcConvert.h"

#include "imgui.h"
#include "stb_image.h"
#include "vcStrings.h"
#include "vcModals.h"
#include "vcSceneItem.h"
#include "vcModel.h"
#include "vcSceneLayerConvert.h"
#include "vcStringFormat.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "vcFBX.h"

#include "udMath.h"
#include "udChunkedArray.h"
#include "udThread.h"
#include "udDebug.h"
#include "udStringUtil.h"

const char *statusNames[] =
{
  "convertAwaiting",
  "convertQueued",
  "convertAwaitingLicense",
  "convertRunning",
  "convertCompleted",
  "convertCancelled",
  "convertWriteFailed",
  "convertParseError",
  "convertImageParseError",
  "convertFailed",
};

void vcConvert_ResetConvert(vcConvertItem *pConvertItem);
void vcConvert_ProcessFile(vcState *pProgramState, vcConvertItem *pJob);

UDCOMPILEASSERT(udLengthOf(statusNames) == vcCQS_Count, "Not Enough Status Names");

uint32_t vcConvert_Thread(void *pVoidState)
{
  vcState *pProgramState = (vcState*)pVoidState;
  vcConvertContext *pConvertContext = pProgramState->pConvertContext;

  while (pConvertContext->threadsRunning)
  {
    if (udWaitSemaphore(pConvertContext->pConversionSemaphore, 1000) != 0)
      continue;

    // Convert Here
    while (true)
    {
      vcConvertItem *pItem = nullptr;
      bool wasPending = false;

      udReadLockRWLock(pConvertContext->pRWLock);
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
      udReadUnlockRWLock(pConvertContext->pRWLock);

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
        pFileExistsMsg = vcStringFormat(vcString::Get("convertFileExistsMessage"), pItem->pConvertInfo->pOutputName);
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

      vdkError conversionStatus = vdkConvert_DoConvert(pItem->pConvertContext);

      if (conversionStatus == vE_InvalidLicense || conversionStatus == vE_Pending)
        pItem->status = vcCQS_QueuedPendingLicense;
      else if (conversionStatus == vE_Cancelled)
        pItem->status = vcCQS_Cancelled;
      else if (conversionStatus == vE_WriteFailure)
        pItem->status = vcCQS_WriteFailed;
      else if (conversionStatus == vE_ParseError)
        pItem->status = vcCQS_ParseFailed;
      else if (conversionStatus == vE_ImageParseError)
        pItem->status = vcCQS_ImageParseFailed;
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

uint32_t vcConvert_ProcessFilesThread(void *pVoidState)
{
  vcState *pProgramState = (vcState*)pVoidState;
  vcConvertContext *pConvertContext = pProgramState->pConvertContext;

  while (pConvertContext->threadsRunning)
  {
    if (udWaitSemaphore(pConvertContext->pProcessingSemaphore, 1000) != 0)
      continue;

    udReadLockRWLock(pConvertContext->pRWLock);
    for (size_t i = 0; i < pConvertContext->jobs.length; ++i)
    {
      vcConvertItem *pItem = pConvertContext->jobs[i];
      if (pConvertContext->jobs[i]->status == vcCQS_Preparing && pItem->itemsToProcess.length > 0)
      {
        vcConvert_ProcessFile(pProgramState, pConvertContext->jobs[i]);

        vdkConvertItemInfo info = {};
        vdkConvert_GetItemInfo(pItem->pConvertContext, i, &info);
        pItem->detectedProjections.PushBack(info.sourceProjection);

        break;
      }
    }
    udReadUnlockRWLock(pConvertContext->pRWLock);

    udSleep(10); // This is required to allow other threads to get the write lock
  }

  return 0;
}

void vcConvert_Init(vcState *pProgramState)
{
  if (pProgramState->pConvertContext != nullptr)
    return;

  pProgramState->pConvertContext = udAllocType(vcConvertContext, 1, udAF_Zero);

  pProgramState->pConvertContext->jobs.Init(32);

  pProgramState->pConvertContext->pRWLock = udCreateRWLock();
  pProgramState->pConvertContext->pConversionSemaphore = udCreateSemaphore();
  pProgramState->pConvertContext->pProcessingSemaphore = udCreateSemaphore();
  pProgramState->pConvertContext->threadsRunning = true;

  udThread_Create(&pProgramState->pConvertContext->pConversionThread, vcConvert_Thread, pProgramState, udTCF_None, "ConvertJob");
  udThread_Create(&pProgramState->pConvertContext->pProcessingThread, vcConvert_ProcessFilesThread, pProgramState, udTCF_None, "ConvertProcessFiles");
}

void vcConvert_RemoveJob(vcState *pProgramState, size_t index)
{
  udWriteLockRWLock(pProgramState->pConvertContext->pRWLock);
  vcConvertItem *pItem = pProgramState->pConvertContext->jobs[index];
  pProgramState->pConvertContext->jobs.RemoveAt(index);
  udWriteUnlockRWLock(pProgramState->pConvertContext->pRWLock);

  //TODO: Make sure that it isn't being used, not sure if its possible that it could be getting used?

  vcTexture_Destroy(&pItem->watermark.pTexture);
  udFree(pItem->watermark.pFilename);

  udLockMutex(pItem->pMutex);
  while (pItem->itemsToProcess.length > 0)
  {
    udFree(pItem->itemsToProcess[0]);
    pItem->itemsToProcess.PopFront();
  }
  pItem->detectedProjections.Clear();
  udReleaseMutex(pItem->pMutex);

  pItem->itemsToProcess.Deinit();
  pItem->detectedProjections.Deinit();

  vdkConvert_DestroyContext(&pItem->pConvertContext);

  udDestroyMutex(&pItem->pMutex);

  udFree(pItem);
}

void vcConvert_Deinit(vcState *pProgramState)
{
  if (pProgramState->pConvertContext == nullptr)
    return;

  pProgramState->pConvertContext->threadsRunning = false;
  udIncrementSemaphore(pProgramState->pConvertContext->pConversionSemaphore);
  udIncrementSemaphore(pProgramState->pConvertContext->pProcessingSemaphore);

  for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
    vdkConvert_Cancel(pProgramState->pConvertContext->jobs[i]->pConvertContext); // This will fail for most of the jobs

  udThread_Join(pProgramState->pConvertContext->pConversionThread);
  udThread_Join(pProgramState->pConvertContext->pProcessingThread);

  udDestroyRWLock(&pProgramState->pConvertContext->pRWLock);

  udDestroySemaphore(&pProgramState->pConvertContext->pConversionSemaphore);
  udDestroySemaphore(&pProgramState->pConvertContext->pProcessingSemaphore);

  udThread_Destroy(&pProgramState->pConvertContext->pConversionThread);
  udThread_Destroy(&pProgramState->pConvertContext->pProcessingThread);

  while (pProgramState->pConvertContext->jobs.length > 0)
    vcConvert_RemoveJob(pProgramState, 0);
  pProgramState->pConvertContext->jobs.Deinit();

  udFree(pProgramState->pConvertContext);
}

void vcConvert_AddEmptyJob(vcState *pProgramState, vcConvertItem **ppNextItem)
{
  vcConvertItem *pNextItem = udAllocType(vcConvertItem, 1, udAF_Zero);

  vdkConvert_CreateContext(pProgramState->pVDKContext, &pNextItem->pConvertContext);
  vdkConvert_GetInfo(pNextItem->pConvertContext, &pNextItem->pConvertInfo);

  pNextItem->pMutex = udCreateMutex();
  pNextItem->itemsToProcess.Init(16);
  pNextItem->detectedProjections.Init(16);
  pNextItem->status = vcCQS_Preparing;

  // Update with default settings
  if (udStrlen(pProgramState->settings.convertdefaults.tempDirectory) > 0)
    vdkConvert_SetTempDirectory(pNextItem->pConvertContext, pProgramState->settings.convertdefaults.tempDirectory);

  if (udStrlen(pProgramState->settings.convertdefaults.watermark.filename) > 0)
  {
    char buffer[vcMaxPathLength];
    udStrcpy(buffer, pProgramState->settings.pSaveFilePath);
    udStrcat(buffer, pProgramState->settings.convertdefaults.watermark.filename);
    vdkConvert_AddWatermark(pNextItem->pConvertContext, buffer);
    pNextItem->watermark.pFilename = udStrdup(buffer);
    pNextItem->watermark.isDirty = true;
  }

  if (udStrlen(pProgramState->settings.convertdefaults.author) > 0)
  {
    udStrcpy(pNextItem->author, pProgramState->settings.convertdefaults.author);
    vdkConvert_SetMetadata(pNextItem->pConvertContext, "Author", pNextItem->author);
  }

  if (udStrlen(pProgramState->settings.convertdefaults.comment) > 0)
  {
    udStrcpy(pNextItem->comment, pProgramState->settings.convertdefaults.comment);
    vdkConvert_SetMetadata(pNextItem->pConvertContext, "Comment", pNextItem->comment);
  }

  if (udStrlen(pProgramState->settings.convertdefaults.copyright) > 0)
  {
    udStrcpy(pNextItem->copyright, pProgramState->settings.convertdefaults.copyright);
    vdkConvert_SetMetadata(pNextItem->pConvertContext, "Copyright", pNextItem->copyright);
  }

  if (udStrlen(pProgramState->settings.convertdefaults.license) > 0)
  {
    udStrcpy(pNextItem->license, pProgramState->settings.convertdefaults.license);
    vdkConvert_SetMetadata(pNextItem->pConvertContext, "License", pNextItem->license);
  }

  // Add it to the jobs list
  udWriteLockRWLock(pProgramState->pConvertContext->pRWLock);
  pProgramState->pConvertContext->jobs.PushBack(pNextItem);
  udWriteUnlockRWLock(pProgramState->pConvertContext->pRWLock);

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

  // Convert Jobs --------------------------------
  ImGui::Columns(2);

  if (ImGui::IsWindowAppearing())
    ImGui::SetColumnWidth(0, 300);

  ImGui::TextUnformatted(vcString::Get("convertJobs"));
  ImGui::NextColumn();

  ImGui::TextUnformatted(vcString::Get("convertSettings"));
  ImGui::NextColumn();

  ImGui::Separator();

  if (ImGui::BeginChild("ConvertJobListColumn"))
  {
    if (ImGui::Button(vcString::Get("convertAddNewJob"), ImVec2(100, 23)))
      vcConvert_AddEmptyJob(pProgramState, &pSelectedJob);

    size_t removeItem = SIZE_MAX;

    udReadLockRWLock(pProgramState->pConvertContext->pRWLock);
    for (size_t i = 0; i < pProgramState->pConvertContext->jobs.length; ++i)
    {
      bool selected = (pProgramState->pConvertContext->selectedItem == i);

      udSprintf(tempBuffer, "X##convertjob_%zu", i);
      if (ImGui::Button(tempBuffer, ImVec2(20, 20)))
      {
        if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
          vdkConvert_Cancel(pProgramState->pConvertContext->jobs[i]->pConvertContext);
        else if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Queued)
          pProgramState->pConvertContext->jobs[i]->status = vcCQS_Preparing;
        else
          removeItem = i;

        continue;
      }

      ImGui::SameLine();

      udSprintf(tempBuffer, "%s | %s##convertjob_%zu", vcString::Get(statusNames[pProgramState->pConvertContext->jobs[i]->status]), udFilename(pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName).GetFilenameWithExt(), i);

      ImVec2 selectablePos = ImVec2(ImGui::GetContentRegionMax().x - ImGui::GetStyle().ItemSpacing.x * 2, 0);

      if (pProgramState->pConvertContext->jobs[i]->status == vcCQS_Running)
        vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);

      if (ImGui::Selectable(tempBuffer, selected, ImGuiSelectableFlags_None, selectablePos))
        pProgramState->pConvertContext->selectedItem = i;

      if (vcIGSW_IsItemHovered())
        ImGui::SetTooltip("%s", pProgramState->pConvertContext->jobs[i]->pConvertInfo->pOutputName);
    }
    udReadUnlockRWLock(pProgramState->pConvertContext->pRWLock);

    if (removeItem != SIZE_MAX)
    {
      vcConvert_RemoveJob(pProgramState, removeItem);

      if (pProgramState->pConvertContext->selectedItem >= removeItem && pProgramState->pConvertContext->selectedItem != 0)
        --pProgramState->pConvertContext->selectedItem;
    }

    if (pProgramState->pConvertContext->selectedItem < pProgramState->pConvertContext->jobs.length)
      pSelectedJob = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem];
  }
  ImGui::EndChild();

  // Options pane --------------------------------
  ImGui::NextColumn();

  if (pSelectedJob == nullptr)
  {
    ImGui::TextUnformatted(vcString::Get("convertNoJobSelected"));
  }
  else
  {
    if ((pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled))
    {
      if (ImGui::Button(vcString::Get("convertAddFile"), ImVec2(200, 40)))
        vcModals_OpenModal(pProgramState, vcMT_ConvertAdd);

      ImGui::SameLine();

      if (pSelectedJob->itemsToProcess.length == 0 && pSelectedJob->pItemProcessing == nullptr)
      {
        if (ImGui::Button(vcString::Get("convertBeginConvert"), ImVec2(-1, 40)))
        {
          if (pSelectedJob->status == vcCQS_Cancelled)
            vcConvert_ResetConvert(pSelectedJob);
          pSelectedJob->status = vcCQS_Queued;
          udIncrementSemaphore(pProgramState->pConvertContext->pConversionSemaphore);
        }
      }
      else
      {
        vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertBeginPendingFiles"), udTempStr("%zu", pSelectedJob->itemsToProcess.length + (pSelectedJob->pItemProcessing == nullptr ? 0 : 1)));
        ImGui::Button(localizationBuffer, ImVec2(-1, 40));
      }
    }
    else if (pSelectedJob->status == vcCQS_Completed)
    {
      size_t selectedJob = pProgramState->pConvertContext->selectedItem;
      if (ImGui::Button(udTempStr("%s##vcConvLoad_%zu", vcString::Get("convertAddToScene"), selectedJob), ImVec2(200, 40)))
      {
        vdkProjectNode *pNode = nullptr;
        if (vdkProjectNode_Create(pProgramState->activeProject.pProject, &pNode, pProgramState->activeProject.pRoot, "UDS", nullptr, pProgramState->pConvertContext->jobs[selectedJob]->pConvertInfo->pOutputName, nullptr) == vE_Success)
        {
          udStrcpy(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID);
          pProgramState->changeActiveDock = vcDocks_Scene;
        }
        else
        {
          vcState::ErrorItem projectError;
          projectError.source = vcES_ProjectChange;
          projectError.pData = udStrdup(pProgramState->pConvertContext->jobs[selectedJob]->pConvertInfo->pOutputName);
          projectError.resultCode = udR_Failure_;

          pProgramState->errorItems.PushBack(projectError);

          vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
        }
      }

      ImGui::SameLine();

      if (ImGui::Button(vcString::Get("convertReset"), ImVec2(-1, 40)))
        vcConvert_ResetConvert(pSelectedJob);
    }
    else if (pSelectedJob->status == vcCQS_Running)
    {
      if (pSelectedJob->status == vcCQS_Running)
      {
        const float progressRatio = 0.7f; // How much reading is compared to writing (0.8 would be 80% of the progress is writing)

        if (pSelectedJob->pConvertInfo->currentInputItem != pSelectedJob->pConvertInfo->totalItems)
        {
          float perItemAmount = progressRatio / pSelectedJob->pConvertInfo->totalItems;
          float completedFileProgress = perItemAmount * pSelectedJob->pConvertInfo->currentInputItem;

          const char *fileIndexStrings[] = { udCommaInt(pSelectedJob->pConvertInfo->currentInputItem + 1), udCommaInt(pSelectedJob->pConvertInfo->totalItems) };

          ImGui::ProgressBar(completedFileProgress, ImVec2(-1, 40), vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingFile"), fileIndexStrings, udLengthOf(fileIndexStrings)));
        }
        else
        {
          uint64_t pointsWritten = (pSelectedJob->pConvertInfo->outputPointCount + pSelectedJob->pConvertInfo->discardedPointCount);
          uint64_t pointsTotal = pSelectedJob->pConvertInfo->totalPointsRead;

          const char *strings[] = { udCommaInt(pointsWritten), udCommaInt(pointsTotal) };

          ImGui::ProgressBar(progressRatio + (1.f - progressRatio) * pointsWritten / pointsTotal, ImVec2(-1, 0), vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertWritingPoints"), strings, udLengthOf(strings)));
        }
      }

      // Preview is temporarily disabled as it doesn't work in shipped VDK0.4.1
      /*
      if (ImGui::Button(udTempStr("%s##vcAddPreview", pSelectedJob->previewRequested ? vcString::Get("convertGeneratingPreview") : vcString::Get("convertAddPreviewToScene")), ImVec2(-1, 40)) || pSelectedJob->previewRequested)
      {
        vdkPointCloud *pPointCloud = nullptr;
        vdkError result = vdkConvert_GeneratePreview(pSelectedJob->pConvertContext, &pPointCloud);

        if (result == vE_Success)
        {
          pSelectedJob->previewRequested = false;
          new vcModel(pProgramState, vcString::Get("convertPreviewName"), pPointCloud);
        }
        else if (result == vE_Pending)
        {
          pSelectedJob->previewRequested = true; // This might already be true
        }
      }
      */
    }

    if (ImGui::BeginChild("ConvertJobInfoColumn"))
    {
      ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

      udSprintf(outputName, "%s", pSelectedJob->pConvertInfo->pOutputName);
      if (ImGui::InputText("##vcSetOutputFilenameText", outputName, udLengthOf(outputName)))
        vdkConvert_SetOutputFilename(pSelectedJob->pConvertContext, outputName);

      if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
      {
        ImGui::SameLine(0.f, 0.f);
        if (ImGui::Button("...##vcSetOutputFilename", ImVec2(30, 0)))
          vcModals_OpenModal(pProgramState, vcMT_ConvertOutput);
      }

      ImGui::SameLine();
      ImGui::TextUnformatted(vcString::Get("convertOutputName"));

      udSprintf(tempDirectory, "%s", pSelectedJob->pConvertInfo->pTempFilesPrefix);
      if (ImGui::InputText("##vcSetTemporaryDirectoryText", tempDirectory, udLengthOf(tempDirectory)))
        vdkConvert_SetTempDirectory(pSelectedJob->pConvertContext, tempDirectory);

      if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
      {
        ImGui::SameLine(0.f, 0.f);
        if (ImGui::Button("...##vcSetTemporaryDirectory", ImVec2(30, 0)))
          vcModals_OpenModal(pProgramState, vcMT_ConvertTempDirectory);
      }

      ImGui::SameLine();
      ImGui::TextUnformatted(vcString::Get("convertTempDirectory"));

      ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

      if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
      {
        bool skipErrorsWherePossible = pSelectedJob->pConvertInfo->skipErrorsWherePossible;
        if (ImGui::Checkbox(vcString::Get("convertContinueOnCorrupt"), &skipErrorsWherePossible))
          vdkConvert_SetSkipErrorsWherePossible(pSelectedJob->pConvertContext, skipErrorsWherePossible);
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
          vdkConvert_SetPointResolution(pSelectedJob->pConvertContext, overrideResolution, resolution);

        if (overrideResolution)
        {
          ImGui::Indent();

          if (ImGui::InputDouble(udTempStr("%s##ConvertResolution", vcString::Get("convertPointResolution")), &resolution, 0.0001, 0.01, "%.6f"))
            vdkConvert_SetPointResolution(pSelectedJob->pConvertContext, 1, resolution);

          ImGui::Unindent();
        }

        // Override SRID
        bool overrideSRID = pSelectedJob->pConvertInfo->overrideSRID;
        int srid = pSelectedJob->pConvertInfo->srid;

        if (ImGui::Checkbox(udTempStr("%s##ConvertOverrideGeolocate", vcString::Get("convertOverrideGeolocation")), &overrideSRID))
          vdkConvert_SetSRID(pSelectedJob->pConvertContext, overrideSRID, srid);

        if (overrideSRID)
        {
          if (ImGui::InputInt(udTempStr("%s##ConvertSRID", vcString::Get("convertSRID")), &srid))
            vdkConvert_SetSRID(pSelectedJob->pConvertContext, overrideSRID, srid);

          // While overrideSRID isn't required for global offset to work, in order to simplify the UI for most users, we hide global offset when override SRID is disabled
          double globalOffset[3];
          memcpy(globalOffset, pSelectedJob->pConvertInfo->globalOffset, sizeof(globalOffset));
          if (ImGui::InputScalarN(vcString::Get("convertGlobalOffset"), ImGuiDataType_Double, &globalOffset, 3))
          {
            for (int i = 0; i < 3; ++i)
              globalOffset[i] = udClamp(globalOffset[i], -vcSL_GlobalLimit, vcSL_GlobalLimit);
            vdkConvert_SetGlobalOffset(pSelectedJob->pConvertContext, globalOffset);
          }
        }
        // Quick Convert
        bool quickConvert = (pSelectedJob->pConvertInfo->everyNth != 0);
        if (ImGui::Checkbox(vcString::Get("convertQuickTest"), &quickConvert))
          vdkConvert_SetEveryNth(pSelectedJob->pConvertContext, quickConvert ? 1000 : 0);

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
        ImGui::TextUnformatted(vcString::Get("convertMetadata"));

        // Other Metadata
        if (ImGui::InputText(vcString::Get("convertAuthor"), pSelectedJob->author, udLengthOf(pSelectedJob->author)))
          vdkConvert_SetMetadata(pSelectedJob->pConvertContext, "Author", pSelectedJob->author);

        if (ImGui::InputText(vcString::Get("convertComment"), pSelectedJob->comment, udLengthOf(pSelectedJob->comment)))
          vdkConvert_SetMetadata(pSelectedJob->pConvertContext, "Comment", pSelectedJob->comment);

        if (ImGui::InputText(vcString::Get("convertCopyright"), pSelectedJob->copyright, udLengthOf(pSelectedJob->copyright)))
          vdkConvert_SetMetadata(pSelectedJob->pConvertContext, "Copyright", pSelectedJob->copyright);

        if (ImGui::InputText(vcString::Get("convertLicense"), pSelectedJob->license, udLengthOf(pSelectedJob->license)))
          vdkConvert_SetMetadata(pSelectedJob->pConvertContext, "License", pSelectedJob->license);

        // Watermark
        if (ImGui::Button(vcString::Get("convertLoadWatermark")))
          vcModals_OpenModal(pProgramState, vcMT_LoadWatermark);

        if (pSelectedJob->watermark.pTexture != nullptr)
        {
          ImGui::SameLine();
          if (ImGui::Button(vcString::Get("convertRemoveWatermark")))
          {
            vdkConvert_RemoveWatermark(pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext);
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

      ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

      uint64_t totalItems = pSelectedJob->pConvertInfo->totalItems + pSelectedJob->itemsToProcess.length + (pSelectedJob->pItemProcessing == nullptr ? 0 : 1);

      const char *sourceSpaceNames[] = { vcString::Get("convertSpaceDetected"), vcString::Get("convertSpaceCartesian"), vcString::Get("convertSpaceLatLong"), vcString::Get("convertSpaceLongLat"), vcString::Get("convertSpaceECEF") };
      UDCOMPILEASSERT(vdkCSP_Count == udLengthOf(sourceSpaceNames) - 1, "Please update to match number of convert spaces");

      if (totalItems > 0)
      {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);

        vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertInputFiles"), udCommaInt(totalItems));

        if (ImGui::TreeNodeEx(pSelectedJob->pConvertInfo, 0, "%s", localizationBuffer))
        {
          ImGui::Columns(3);

          if ((pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled) && ImGui::Button(vcString::Get("convertRemoveAll")))
          {
            while (pSelectedJob->pConvertInfo->totalItems > 0)
              vdkConvert_RemoveItem(pSelectedJob->pConvertContext, 0);

            udLockMutex(pSelectedJob->pMutex);
            while (pSelectedJob->itemsToProcess.length > 0)
            {
              udFree(pSelectedJob->itemsToProcess[0]);
              pSelectedJob->itemsToProcess.PopFront();
            }
            pSelectedJob->detectedProjections.Clear();
            udReleaseMutex(pSelectedJob->pMutex);
          }

          ImGui::NextColumn();
          // Skip the middle column
          ImGui::NextColumn();

          if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
          {
            static int globalSource = 0;
            if (ImGui::Combo(udTempStr("%s###convertallitemspace", vcString::Get("convertAllSpaceLabel")), &globalSource, sourceSpaceNames, (int)udLengthOf(sourceSpaceNames)) && globalSource > -1)
            {
              for (size_t i = 0; i < pSelectedJob->pConvertInfo->totalItems; ++i)
                vdkConvert_SetInputSourceProjection(pSelectedJob->pConvertContext, i, globalSource == 0 ? *pSelectedJob->detectedProjections.GetElement(i) : (vdkConvertSourceProjection)(globalSource - 1));
            }
          }

          ImGui::Separator();

          // New line
          ImGui::NextColumn();

          for (size_t i = 0; i < pSelectedJob->pConvertInfo->totalItems; ++i)
          {
            vdkConvert_GetItemInfo(pSelectedJob->pConvertContext, i, &itemInfo);

            if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
            {
              if (ImGui::Button(udTempStr("X##convertitemremove_%zu", i)))
              {
                vdkConvert_RemoveItem(pSelectedJob->pConvertContext, i);
                pSelectedJob->detectedProjections.RemoveAt(i);
                --i;
              }
              ImGui::SameLine();
            }

            ImGui::TextUnformatted(itemInfo.pFilename);
            ImGui::NextColumn();

            const char *ptCountStrings[] = { udCommaInt(itemInfo.pointsCount), udCommaInt(itemInfo.pointsRead) };

            if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
            {
              if (itemInfo.pointsCount == -1)
                vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
              else
                vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

              ImGui::TextUnformatted(localizationBuffer);

              ImGui::NextColumn();

              int sourceSpace = (int)itemInfo.sourceProjection + 1;
              if (ImGui::Combo(udTempStr("%s###converitemspace_%zu", vcString::Get("convertSpaceLabel"), i), &sourceSpace, sourceSpaceNames, (int)udLengthOf(sourceSpaceNames)))
                  vdkConvert_SetInputSourceProjection(pSelectedJob->pConvertContext, i, sourceSpace == 0 ? pSelectedJob->detectedProjections[i] : (vdkConvertSourceProjection)(sourceSpace - 1));
            }
            else
            {
              if (pSelectedJob->pConvertInfo->currentInputItem > i) // Already read
              {
                vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadComplete"), ptCountStrings, udLengthOf(ptCountStrings));
                ImGui::TextUnformatted(localizationBuffer);
                ImGui::NextColumn();
                ImGui::ProgressBar(1.f);
              }
              else if (pSelectedJob->pConvertInfo->currentInputItem < i) // Pending read
              {
                if (itemInfo.pointsCount == -1)
                  vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
                else
                  vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertPendingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

                ImGui::TextUnformatted(localizationBuffer);
                ImGui::NextColumn();
                ImGui::ProgressBar(0.f);
              }
              else // Currently reading
              {
                if (itemInfo.pointsCount == -1)
                  vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingNoEstimate"), ptCountStrings, udLengthOf(ptCountStrings));
                else
                  vcStringFormat(localizationBuffer, udLengthOf(localizationBuffer), vcString::Get("convertReadingEstimate"), ptCountStrings, udLengthOf(ptCountStrings));

                ImGui::TextUnformatted(localizationBuffer);
                ImGui::NextColumn();
                ImGui::ProgressBar(1.f * itemInfo.pointsRead / itemInfo.pointsCount);
              }
            }

            ImGui::NextColumn();
          }

          if (pSelectedJob->status == vcCQS_Preparing || pSelectedJob->status == vcCQS_Cancelled)
          {
            udLockMutex(pSelectedJob->pMutex);

            if (pSelectedJob->pItemProcessing != nullptr)
            {
              vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
              ImGui::TextUnformatted(pSelectedJob->pItemProcessing);
              ImGui::NextColumn();
              ImGui::TextUnformatted(vcString::Get("convertPendingProcessInProgress"));
              ImGui::NextColumn();
              // Intentionally blank column
              ImGui::NextColumn();
            }

            for (size_t i = 0; i < pSelectedJob->itemsToProcess.length; ++i)
            {
              if (ImGui::Button(udTempStr("X##convertprocessitemremove_%zu", i)))
              {
                udFree(pSelectedJob->itemsToProcess[i]);
                pSelectedJob->itemsToProcess.RemoveAt(i);
                pSelectedJob->detectedProjections.RemoveAt(i);
                --i;
              }
              else
              {
                ImGui::SameLine();
                ImGui::TextUnformatted(pSelectedJob->itemsToProcess[i]);
              }

              ImGui::NextColumn();
              ImGui::TextUnformatted(vcString::Get("convertPendingProcessing"));
              ImGui::NextColumn();
              // Intentionally blank column
              ImGui::NextColumn();
            }
            udReleaseMutex(pSelectedJob->pMutex);
          }

          ImGui::Columns(1);

          ImGui::TreePop();
        }
      }
    }

    ImGui::EndChild();
  }
}

void vcConvert_ProcessFile(vcState *pProgramState, vcConvertItem *pJob)
{
  const char *pFilename = nullptr;
  udResult result = udR_NothingToDo;

  udMutex *pMutex = udLockMutex(pJob->pMutex);

  if (pJob->itemsToProcess.PopFront(&pFilename))
  {
    pJob->pItemProcessing = pFilename;

    udFilename file(pFilename);
    const char *pExt = file.GetExt();

    udReleaseMutex(pMutex);
    pMutex = nullptr;

    if (udStrEquali(pExt, ".slpk"))
    {
      if (vcSceneLayerConvert_AddItem(pJob->pConvertContext, pFilename) == vE_Success)
        UD_ERROR_SET(udR_Success);
      UD_ERROR_SET(udR_CorruptData);
    }
#ifdef FBXSDK_ON
    else if (udStrEquali(pExt, ".fbx"))
    {
      if (vcFBX_AddItem(pJob->pConvertContext, pFilename) == vE_Success)
        UD_ERROR_SET(udR_Success);
      UD_ERROR_SET(udR_CorruptData);
    }
#endif
    else if (vdkConvert_AddItem(pJob->pConvertContext, pFilename) == vE_Success)
    {
      UD_ERROR_SET(udR_Success);
    }

    UD_ERROR_SET(udR_Unsupported);
  }

epilogue:
  if (pMutex == nullptr)
    pMutex = udLockMutex(pJob->pMutex);
  pJob->pItemProcessing = nullptr;
  udReleaseMutex(pMutex);

  if (pFilename != nullptr && result != udR_Success)
  {
    vcState::ErrorItem fileError;
    fileError.source = vcES_File;
    fileError.pData = udStrdup(pFilename);
    fileError.resultCode = result;
    pProgramState->errorItems.PushBack(fileError);
    pFilename = nullptr;
  }

  udFree(pFilename);
}

void vcConvert_QueueFile(vcState *pProgramState, const char *pFilename)
{
  vcConvertItem *pSelectedJob = nullptr;

  if (pProgramState->pConvertContext->selectedItem < pProgramState->pConvertContext->jobs.length)
    pSelectedJob = pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem];

  if (pSelectedJob == nullptr || pSelectedJob->status != vcCQS_Preparing)
    vcConvert_AddEmptyJob(pProgramState, &pSelectedJob);

  if (pSelectedJob != nullptr)
  {
    // Handle watermarks now
    if (udStrEndsWithi(pFilename, ".png") || udStrEndsWithi(pFilename, ".jpg") || udStrEndsWithi(pFilename, ".bmp"))
    {
      if (vdkConvert_AddWatermark(pSelectedJob->pConvertContext, pFilename) == vE_Success)
      {
        udFree(pSelectedJob->watermark.pFilename);
        pSelectedJob->watermark.pFilename = udStrdup(pFilename);
        pSelectedJob->watermark.isDirty = true;
      }
      else
      {
        //TODO: Handle the udResult properly
        vcState::ErrorItem fileError;
        fileError.source = vcES_File;
        fileError.pData = udStrdup(pFilename);
        fileError.resultCode = udR_Unsupported;
        pProgramState->errorItems.PushBack(fileError);
      }
    }
    else
    {
      udLockMutex(pSelectedJob->pMutex);
      pSelectedJob->itemsToProcess.PushBack(udStrdup(pFilename));
      udReleaseMutex(pSelectedJob->pMutex);

      udIncrementSemaphore(pProgramState->pConvertContext->pProcessingSemaphore);
    }
  }
}

void vcConvert_ResetConvert(vcConvertItem *pConvertItem)
{
  vdkConvert_Reset(pConvertItem->pConvertContext);
  pConvertItem->status = vcCQS_Preparing;
}
