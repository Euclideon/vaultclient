#include "vcFolder.h"

#include "vcScene.h"
#include "vcState.h"
#include "vcStrings.h"
#include "vcTime.h"

#include "vcFenceRenderer.h"

#include "vcModel.h" // Included just for "ResetPosition"
#include "vcRender.h" // Included just for "ClearTiles"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

void vcFolder_ShowLoadStatusIndicator(vcSceneLoadStatus loadStatus, bool sameLine /*= true*/)
{
  const char *loadingChars[] = { "\xE2\x96\xB2", "\xE2\x96\xB6", "\xE2\x96\xBC", "\xE2\x97\x80" };
  int64_t currentLoadingChar = (int64_t)(10*vcTime_GetEpochSecsF());

  // Load Status (if any)
  if (loadStatus == vcSLS_Pending)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Yellow Exclamation in Triangle
    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("sceneExplorerPending"));

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Loading)
  {
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", loadingChars[currentLoadingChar % udLengthOf(loadingChars)]); // Yellow Spinning clock
    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("sceneExplorerLoading"));

    if (sameLine)
      ImGui::SameLine();
  }
  else if (loadStatus == vcSLS_Failed || loadStatus == vcSLS_OpenFailure)
  {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "\xE2\x9A\xA0"); // Red Exclamation in Triangle
    if (vcIGSW_IsItemHovered())
    {
      if (loadStatus == vcSLS_OpenFailure)
        ImGui::SetTooltip("%s", vcString::Get("sceneExplorerErrorOpen"));
      else
        ImGui::SetTooltip("%s", vcString::Get("sceneExplorerErrorLoad"));
    }

    if (sameLine)
      ImGui::SameLine();
  }
}

vcFolder::vcFolder(const char *pName)
{
  m_visible = true;
  m_pName = udStrdup(pName);
  m_type = vdkPNT_Folder;
  m_children.reserve(64);
  udStrcpy(m_typeStr, sizeof(m_typeStr), "Folder");
  m_loadStatus = vcSLS_Loaded;
}

void vcFolder::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  for (size_t i = 0; i < m_children.size(); ++i)
    m_children[i]->AddToScene(pProgramState, pRenderData);
}

void vcFolder::ChangeProjection(vcState *pProgramState, const udGeoZone &newZone)
{
  for (size_t i = 0; i < m_children.size(); ++i)
    m_children[i]->ChangeProjection(pProgramState, newZone);
}

void vcFolder::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  for (size_t i = 0; i < m_children.size(); ++i)
    m_children[i]->ApplyDelta(pProgramState, delta);
}

void vcFolder_AddInsertSeparator()
{
  ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered]); //RBGA
  ImVec2 pos = ImGui::GetCursorPos();
  ImVec2 winPos = ImGui::GetWindowPos();
  ImGui::SetWindowPos({ winPos.x + pos.x, winPos.y }, ImGuiCond_Always);
  ImGui::Separator();
  ImGui::SetCursorPos(pos);
  ImGui::SetWindowPos(winPos, ImGuiCond_Always);
  ImGui::PopStyleColor();
}

void vcFolder::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  size_t i;
  for (i = 0; i < m_children.size(); ++i)
  {
    ++(*pItemID);

    // This block is also after the loop
    if (this == pProgramState->sceneExplorer.insertItem.pParent && i == pProgramState->sceneExplorer.insertItem.index)
      vcFolder_AddInsertSeparator();

    // Can only edit the name while the item is still selected
    m_children[i]->m_editName = m_children[i]->m_editName && m_children[i]->m_selected;

    // Visibility
    ImGui::Checkbox(udTempStr("###SXIVisible%zu", *pItemID), &m_children[i]->m_visible);
    ImGui::SameLine();

    vcFolder_ShowLoadStatusIndicator((vcSceneLoadStatus)m_children[i]->m_loadStatus);

    // The actual model
    ImGui::SetNextTreeNodeOpen(m_children[i]->m_expanded, ImGuiCond_Always);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (m_children[i]->m_selected)
      flags |= ImGuiTreeNodeFlags_Selected;

    if (m_children[i]->m_editName)
    {
      m_children[i]->m_expanded = ImGui::TreeNodeEx(udTempStr("###SXIName%zu", *pItemID), flags);
      ImGui::SameLine();
      if (vcIGSW_InputTextWithResize(udTempStr("###FolderName%zu", *pItemID), &m_children[i]->m_pName, &m_children[i]->m_nameBufferLength))
        m_children[i]->OnNameChange();

      if (ImGui::IsItemDeactivated() || !(pProgramState->sceneExplorer.selectedItems.back().pParent == this && pProgramState->sceneExplorer.selectedItems.back().index == i))
        m_children[i]->m_editName = false;
      else
        ImGui::SetKeyboardFocusHere(-1); // Set focus to previous widget
    }
    else
    {
      m_children[i]->m_expanded = ImGui::TreeNodeEx(udTempStr("%s###SXIName%zu", m_children[i]->m_pName, *pItemID), flags);
      if (m_children[i]->m_selected && pProgramState->sceneExplorer.selectedItems.back().pParent == this && pProgramState->sceneExplorer.selectedItems.back().index == i && ImGui::GetIO().KeysDown[SDL_SCANCODE_F2])
        m_children[i]->m_editName = true;
    }

    if ((ImGui::IsMouseReleased(0) && ImGui::IsItemHovered() && !ImGui::IsItemActive()) || (!m_children[i]->m_selected && ImGui::IsItemActive()))
    {
      if (!ImGui::GetIO().KeyCtrl)
        vcScene_ClearSelection(pProgramState);

      if (m_children[i]->m_selected)
      {
        vcScene_UnselectItem(pProgramState, this, i);
        pProgramState->sceneExplorer.clickedItem = { nullptr, SIZE_MAX };
      }
      else
      {
        vcScene_SelectItem(pProgramState, this, i);
        pProgramState->sceneExplorer.clickedItem = { this, i };
      }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDragging())
    {
      ImVec2 minPos = ImGui::GetItemRectMin();
      ImVec2 maxPos = ImGui::GetItemRectMax();
      ImVec2 mousePos = ImGui::GetMousePos();

      if (m_children[i]->m_type == vdkPNT_Folder && udAbs(mousePos.y - minPos.y) >= udAbs(mousePos.y - maxPos.y))
        pProgramState->sceneExplorer.insertItem = { (vcFolder*)m_children[i], ((vcFolder*)m_children[i])->m_children.size() };
      else if (udAbs(mousePos.y - minPos.y) < udAbs(mousePos.y - maxPos.y))
        pProgramState->sceneExplorer.insertItem = { this, i };
      else
        pProgramState->sceneExplorer.insertItem = { this, i + 1 };
    }

    if (ImGui::BeginPopupContextItem(udTempStr("ModelContextMenu_%zu", *pItemID)))
    {
      if (!m_children[i]->m_selected)
      {
        if (!ImGui::GetIO().KeyCtrl)
          vcScene_ClearSelection(pProgramState);

        vcScene_SelectItem(pProgramState, this, i);
        pProgramState->sceneExplorer.clickedItem = { this, i };
      }

      if (ImGui::Selectable(vcString::Get("sceneExplorerEditName")))
        m_children[i]->m_editName = true;

      if (ImGui::Selectable(vcString::Get("sceneExplorerUseProjection")) && m_children[i]->m_pOriginalZone != nullptr && m_children[i]->m_pOriginalZone->srid != 0)
      {
        if (vcGIS_ChangeSpace(&pProgramState->gis, *m_children[i]->m_pOriginalZone, &pProgramState->pCamera->position))
        {
          pProgramState->sceneExplorer.pItems->ChangeProjection(pProgramState, *m_children[i]->m_pOriginalZone);
          // refresh map tiles when geozone changes
          vcRender_ClearTiles(pProgramState->pRenderContext);
        }
      }

      if (m_children[i]->m_type != vdkPNT_Folder && ImGui::Selectable(vcString::Get("sceneExplorerMoveTo")))
      {
        // Trigger a camera movement path
        pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
        pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
        pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
        pProgramState->cameraInput.worldAnchorPoint = m_children[i]->GetWorldSpacePivot();
        pProgramState->cameraInput.progress = 0.0;
      }

      // This is terrible but semi-required until we have undo
      if (m_children[i]->m_moved && m_children[i]->m_type == vdkPNT_PointCloud && ImGui::Selectable(vcString::Get("sceneExplorerResetPosition"), false))
      {
        m_children[i]->m_moved = false;
        if (m_children[i]->m_pZone == nullptr || m_children[i]->m_pOriginalZone->srid == m_children[i]->m_pZone->srid)
          ((vcModel*)m_children[i])->m_sceneMatrix = ((vcModel*)m_children[i])->m_defaultMatrix;
        else
          ((vcModel*)m_children[i])->m_sceneMatrix = udGeoZone_TransformMatrix(((vcModel*)m_children[i])->m_defaultMatrix, *m_children[i]->m_pOriginalZone, *m_children[i]->m_pZone);
      }

      if (ImGui::Selectable(vcString::Get("sceneExplorerRemoveItem")))
      {
        vcScene_RemoveItem(pProgramState, this, i);
        ImGui::EndPopup();
        return;
      }

      ImGui::EndPopup();
    }

    if (m_children[i]->m_type != vdkPNT_Folder && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
      vcScene_UseProjectFromItem(pProgramState, m_children[i]);

    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", m_children[i]->m_pName);

    if (!m_children[i]->m_expanded && m_children[i] == pProgramState->sceneExplorer.insertItem.pParent && pProgramState->sceneExplorer.insertItem.index == pProgramState->sceneExplorer.insertItem.pParent->m_children.size())
    {
      // Double indent to match open folder insertion
      ImGui::Indent();
      ImGui::Indent();
      vcFolder_AddInsertSeparator();
      ImGui::Unindent();
      ImGui::Unindent();
    }

    // Show additional settings from ImGui
    if (m_children[i]->m_expanded)
    {
      ImGui::Indent();
      ImGui::PushID(udTempStr("SXIExpanded%zu", *pItemID));

      m_children[i]->HandleImGui(pProgramState, pItemID);

      ImGui::PopID();
      ImGui::Unindent();
      ImGui::TreePop();
    }
  }

  // This block is also in the loop above
  if (this == pProgramState->sceneExplorer.insertItem.pParent && i == pProgramState->sceneExplorer.insertItem.index)
    vcFolder_AddInsertSeparator();
}

void vcFolder::Cleanup(vcState *pProgramState)
{
  udFree(m_pName);

  while (m_children.size() > 0)
    vcScene_RemoveItem(pProgramState, this, 0);
}
