#include "vcFolder.h"

#include "vcScene.h"
#include "vcState.h"
#include "vcStrings.h"

#include "gl/vcFenceRenderer.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include <chrono>

// TODO: Rename vcMain functions
static int64_t vcMain_GetCurrentTime(int fractionSec = 1) // This gives 1/fractionSec factions since epoch, 5=200ms, 10=100ms etc.
{
  return std::chrono::system_clock::now().time_since_epoch().count() * fractionSec / std::chrono::system_clock::period::den;
}

static void vcMain_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, bool sameLine = true)
{
  const char *loadingChars[] = { "\xE2\x96\xB2", "\xE2\x96\xB6", "\xE2\x96\xBC", "\xE2\x97\x80" };
  int64_t currentLoadingChar = vcMain_GetCurrentTime(10);

  // Load Status (if any)
  if (loadStatus == vcSLS_Pending)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Yellow Exclamation in Triangle
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("Pending"));

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Loading)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", loadingChars[currentLoadingChar % udLengthOf(loadingChars)]); // Yellow Spinning clock
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("Loading"));

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Failed || loadStatus == vcSLS_OpenFailure)
  {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    if (ImGui::IsItemHovered())
    {
      if (loadStatus == vcSLS_OpenFailure)
        ImGui::SetTooltip("%s", vcString::Get("ModelOpenFailure"));
      else
        ImGui::SetTooltip("%s", vcString::Get("ModelLoadFailure"));
    }

    if (sameLine)
      ImGui::SameLine();
  }
}

void vcFolder_Cleanup(vcState *pProgramState, vcSceneItem *pBaseItem)
{
  vcFolder *pFolder = (vcFolder*)pBaseItem;
  udFree(pFolder->pName);

  while (pFolder->children.size() > 0)
    vcScene_RemoveItem(pProgramState, pFolder, 0);

  pFolder->~vcFolder();
}

void vcFolder_ShowImGui(vcState *pProgramState, vcSceneItem *pBaseItem, size_t *pItemID)
{
  vcFolder *pFolder = (vcFolder*)pBaseItem;

  // Allow to be null for root folder
  if (pFolder->pName)
    vcIGSW_InputTextWithResize(udTempStr("Label Name###FolderName%zu", *pItemID), &pFolder->pName, &pFolder->nameBufferLength);

  size_t i;
  for (i = 0; i < pFolder->children.size(); ++i)
  {
    ++(*pItemID);

    // This block is also after the loop
    if (pFolder == pProgramState->sceneExplorer.insertItem.pParent && i == pProgramState->sceneExplorer.insertItem.index)
    {
      ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 0.f, 1.f)); // RGBA
      ImVec2 pos = ImGui::GetCursorPos();
      ImGui::Separator();
      ImGui::SetCursorPos(pos);
      ImGui::PopStyleColor();
    }

    // Visibility
    ImGui::Checkbox(udTempStr("###SXIVisible%zu", *pItemID), &pFolder->children[i]->visible);
    ImGui::SameLine();

    vcMain_ShowLoadStatusIndicator((vcSceneLoadStatus)pFolder->children[i]->loadStatus);

    // The actual model
    ImGui::SetNextTreeNodeOpen(pFolder->children[i]->expanded, ImGuiCond_Always);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (pFolder->children[i]->pImGuiFunc == nullptr)
      flags |= ImGuiTreeNodeFlags_Leaf;
    if (pFolder->children[i]->selected)
      flags |= ImGuiTreeNodeFlags_Selected;

    pFolder->children[i]->expanded = ImGui::TreeNodeEx(udTempStr("%s###SXIName%zu", pFolder->children[i]->pName, *pItemID), flags);

    if ((ImGui::IsMouseReleased(0) && ImGui::IsItemHovered() && !ImGui::IsItemActive()) || (!pFolder->children[i]->selected && ImGui::IsItemActive()))
    {
      if (!ImGui::GetIO().KeyCtrl)
        vcScene_ClearSelection(pProgramState);

      if (pFolder->children[i]->selected)
      {
        vcScene_UnselectItem(pProgramState, pFolder, i);
        pProgramState->sceneExplorer.clickedItem = { nullptr, SIZE_MAX };
      }
      else
      {
        vcScene_SelectItem(pProgramState, pFolder, i);
        pProgramState->sceneExplorer.clickedItem = { pFolder, i };
      }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
      ImVec2 minPos = ImGui::GetItemRectMin();
      ImVec2 maxPos = ImGui::GetItemRectMax();
      ImVec2 mousePos = ImGui::GetMousePos();

      if (pFolder->children[i]->type == vcSOT_Folder && udAbs(mousePos.y - minPos.y) >= udAbs(mousePos.y - maxPos.y))
        pProgramState->sceneExplorer.insertItem = { (vcFolder*)pFolder->children[i], ((vcFolder*)pFolder->children[i])->children.size() };
      else if (udAbs(mousePos.y - minPos.y) < udAbs(mousePos.y - maxPos.y))
        pProgramState->sceneExplorer.insertItem = { pFolder, i };
      else
        pProgramState->sceneExplorer.insertItem = { pFolder, i + 1 };
    }

    if (ImGui::BeginPopupContextItem(udTempStr("ModelContextMenu_%zu", *pItemID)))
    {
      if (pFolder->children[i]->pZone != nullptr && ImGui::Selectable(vcString::Get("UseProjection")))
      {
        if (vcGIS_ChangeSpace(&pProgramState->gis, pFolder->children[i]->pZone->srid, &pProgramState->pCamera->position))
          vcScene_UpdateItemToCurrentProjection(pProgramState, nullptr); // Update all models to new zone
      }

      if (ImGui::Selectable(vcString::Get("MoveTo")))
      {
        udDouble3 localSpaceCenter = vcScene_GetItemWorldSpacePivotPoint(pFolder->children[i]);

        // Transform the camera position. Don't do the entire matrix as it may lead to inaccuracy/de-normalised camera
        if (pProgramState->gis.isProjected && pFolder->children[i]->pZone != nullptr && pFolder->children[i]->pZone->srid != pProgramState->gis.SRID)
          localSpaceCenter = udGeoZone_TransformPoint(localSpaceCenter, *pFolder->children[i]->pZone, pProgramState->gis.zone);

        pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
        pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
        pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
        pProgramState->cameraInput.worldAnchorPoint = localSpaceCenter;
        pProgramState->cameraInput.progress = 0.0;
      }

      ImGui::EndPopup();
    }

    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
      vcScene_UseProjectFromItem(pProgramState, pFolder->children[i]);

    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", pFolder->children[i]->pName);

    // Show additional settings from ImGui
    if (pFolder->children[i]->expanded)
    {
      ImGui::Indent();
      ImGui::PushID(udTempStr("SXIExpanded%zu", *pItemID));

      pFolder->children[i]->pImGuiFunc(pProgramState, pFolder->children[i], pItemID);

      ImGui::PopID();
      ImGui::Unindent();
      ImGui::TreePop();
    }
  }

  // This block is also in the loop above
  if (pFolder == pProgramState->sceneExplorer.insertItem.pParent && i == pProgramState->sceneExplorer.insertItem.index)
  {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.f, 1.f, 0.f, 1.f)); // RGBA
    ImGui::Separator();
    ImGui::PopStyleColor();
  }
}

void vcFolder_AddToList(vcState *pProgramState, const char *pName)
{
  vcFolder *pFolder =  udAllocType(vcFolder, 1, udAF_Zero);
  pFolder = new (pFolder) vcFolder();
  pFolder->visible = true;

  pFolder->pName = udStrdup(pName);
  pFolder->type = vcSOT_Folder;

  pFolder->pCleanupFunc = vcFolder_Cleanup;
  pFolder->pImGuiFunc = vcFolder_ShowImGui;

  pFolder->children.reserve(64);

  udStrcpy(pFolder->typeStr, sizeof(pFolder->typeStr), "Folder");
  pFolder->loadStatus = vcSLS_Loaded;

  if (pName == nullptr)
    pProgramState->sceneExplorer.pItems = pFolder;
  else
    vcScene_AddItem(pProgramState, pFolder);
}
