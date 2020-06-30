#include "vcFolder.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcRender.h" // Included just for "ClearTiles"
#include "vcFenceRenderer.h"
#include "vcModals.h"
#include "vcHotkey.h"

#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

// Other includes for creating nodes
#include "vcModel.h"
#include "vcPOI.h"
#include "vcMedia.h"
#include "vcLiveFeed.h"
#include "vcUnsupportedNode.h"
#include "vcI3S.h"
#include "vcPolyModelNode.h"
#include "vcWaterNode.h"
#include "vcViewpoint.h"
#include "vcViewShed.h"
#include "vcQueryNode.h"
#include "vcPlaceLayer.h"
#include "vcVerticalMeasureTool.h"

void HandleNodeSelection(vcState* pProgramState, vdkProjectNode *pParent, vdkProjectNode* pNode)
{
  if (pProgramState->sceneExplorer.selectUUIDWhenPossible[0] == '\0' || !udStrEqual(pProgramState->sceneExplorer.selectUUIDWhenPossible, pNode->UUID) || pNode->pUserData == nullptr)
    return;

  vcSceneItem *pSceneItem = (vcSceneItem*)pNode->pUserData;

  if (!ImGui::GetIO().KeyCtrl)
    vcProject_ClearSelection(pProgramState, false);

  if (pSceneItem->m_selected)
  {
    vcProject_UnselectItem(pProgramState, pParent, pNode);
    pProgramState->sceneExplorer.clickedItem = { nullptr, nullptr };
  }
  else
  {
    vcProject_SelectItem(pProgramState, pParent, pNode);
    pProgramState->sceneExplorer.clickedItem = { pParent, pNode };
  }

  memset(pProgramState->sceneExplorer.selectUUIDWhenPossible, 0, sizeof(pProgramState->sceneExplorer.selectUUIDWhenPossible));
}

vcFolder::vcFolder(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;
}

void vcFolder::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  vdkProjectNode *pNode = m_pNode->pFirstChild;
  while (pNode != nullptr)
  {
    if (pNode->pUserData != nullptr)
    {
      vcSceneItem *pSceneItem = (vcSceneItem *)pNode->pUserData;

      if (pSceneItem->m_lastUpdateTime < pSceneItem->m_pNode->lastUpdate)
        pSceneItem->UpdateNode(pProgramState);

      pSceneItem->AddToScene(pProgramState, pRenderData);
      if (!pSceneItem->IsValid())
      {
        pSceneItem->Cleanup(pProgramState);
        vdkProjectNode *nextNode = pNode->pNextSibling;
        vcProject_RemoveItem(pProgramState, m_pNode, pNode);
        pNode = nextNode;
        continue;
      }

      if (pSceneItem->m_loadStatus == vcSLS_Loaded && pProgramState->sceneExplorer.movetoUUIDWhenPossible[0] != '\0' && udStrEqual(pProgramState->sceneExplorer.movetoUUIDWhenPossible, pNode->UUID))
      {
        vcProject_UseProjectionFromItem(pProgramState, pSceneItem);
        memset(pProgramState->sceneExplorer.movetoUUIDWhenPossible, 0, sizeof(pProgramState->sceneExplorer.movetoUUIDWhenPossible));
      }
    }
    else
    {
      // We need to create one
      if (pNode->itemtype == vdkPNT_Folder)
        pNode->pUserData = new vcFolder(&pProgramState->activeProject, pNode, pProgramState);
      else if (pNode->itemtype == vdkPNT_PointOfInterest)
        pNode->pUserData = new vcPOI(&pProgramState->activeProject, pNode, pProgramState);
      else if (pNode->itemtype == vdkPNT_PointCloud)
        pNode->pUserData = new vcModel(&pProgramState->activeProject, pNode, pProgramState);
      else if (pNode->itemtype == vdkPNT_LiveFeed)
        pNode->pUserData = new vcLiveFeed(&pProgramState->activeProject, pNode, pProgramState);
      else if (pNode->itemtype == vdkPNT_Media)
        pNode->pUserData = new vcMedia(&pProgramState->activeProject, pNode, pProgramState);
      else if (pNode->itemtype == vdkPNT_Viewpoint)
        pNode->pUserData = new vcViewpoint(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "I3S"))
        pNode->pUserData = new vcI3S(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "Water"))
        pNode->pUserData = new vcWater(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "ViewMap"))
        pNode->pUserData = new vcViewShed(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "Polygon"))
        pNode->pUserData = new vcPolyModelNode(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "QFilter"))
        pNode->pUserData = new vcQueryNode(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "Places"))
        pNode->pUserData = new vcPlaceLayer(&pProgramState->activeProject, pNode, pProgramState);
      else if (udStrEqual(pNode->itemtypeStr, "MHeight"))
        pNode->pUserData = new vcVerticalMeasureTool(&pProgramState->activeProject, pNode, pProgramState);
      else
        pNode->pUserData = new vcUnsupportedNode(&pProgramState->activeProject, pNode, pProgramState); // Catch all
    }

    HandleNodeSelection(pProgramState, m_pNode, pNode);

    pNode = pNode->pNextSibling;
  }
}

void vcFolder::OnNodeUpdate(vcState * /*pProgramState*/)
{
  // Does Nothing
}

void vcFolder::ChangeProjection(const udGeoZone &newZone)
{
  vdkProjectNode *pNode = m_pNode->pFirstChild;
  while (pNode != nullptr)
  {
    if (pNode->pUserData)
      ((vcSceneItem*)pNode->pUserData)->ChangeProjection(newZone);
    pNode = pNode->pNextSibling;
  }
}

void vcFolder::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  vdkProjectNode *pNode = m_pNode->pFirstChild;
  while (pNode != nullptr)
  {
    if (pNode->pUserData)
      ((vcSceneItem*)pNode->pUserData)->ApplyDelta(pProgramState, delta);
    pNode = pNode->pNextSibling;
  }
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

void vcFolder::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  vdkProjectNode *pNode = m_pNode->pFirstChild;
  while (pNode != nullptr)
  {
    ++(*pItemID);

    if (pNode->pUserData != nullptr)
    {
      vcSceneItem *pSceneItem = (vcSceneItem*)pNode->pUserData;

      // This block is also after the loop
      if (pProgramState->sceneExplorer.insertItem.pParent == m_pNode && pProgramState->sceneExplorer.insertItem.pItem == pNode)
        vcFolder_AddInsertSeparator();

      // Can only edit the name while the item is still selected
      pSceneItem->m_editName = (pSceneItem->m_editName && pSceneItem->m_selected);

      // Visibility
      ImGui::Checkbox(udTempStr("###SXIVisible%zu", *pItemID), &pSceneItem->m_visible);
      ImGui::SameLine();

      vcIGSW_ShowLoadStatusIndicator((vcSceneLoadStatus)pSceneItem->m_loadStatus);

      if (pSceneItem->m_pActiveWarningStatus != nullptr)
        vcIGSW_ShowLoadStatusIndicator(vcSLS_Pending, pSceneItem->m_pActiveWarningStatus);
      
      // The actual model
      ImGui::SetNextItemOpen(pSceneItem->m_expanded, ImGuiCond_Always);
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
      if (pSceneItem->m_selected)
        flags |= ImGuiTreeNodeFlags_Selected;

      if (pSceneItem->m_editName)
      {
        pSceneItem->m_expanded = ImGui::TreeNodeEx(udTempStr("###SXIName%zu", *pItemID), flags);
        ImGui::SameLine();

        if (pSceneItem->m_pName == nullptr)
        {
          pSceneItem->m_pName = udStrdup(pNode->pName);
          pSceneItem->m_nameCapacity = udStrlen(pSceneItem->m_pName) + 1;
        }

        vcIGSW_InputTextWithResize(udTempStr("###FolderName%zu", *pItemID), &pSceneItem->m_pName, &pSceneItem->m_nameCapacity);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
          if (vdkProjectNode_SetName(pProgramState->activeProject.pProject, pNode, pSceneItem->m_pName) != vE_Success)
          {
            vcState::ErrorItem projectError;
            projectError.source = vcES_ProjectChange;
            projectError.pData = udStrdup(pSceneItem->m_pName);
            projectError.resultCode = udR_Failure_;

            pProgramState->errorItems.PushBack(projectError);

            vcModals_OpenModal(pProgramState, vcMT_ProjectChange);
          }
        }

        if (ImGui::IsItemDeactivated() || !(pProgramState->sceneExplorer.selectedItems.back().pParent == m_pNode && pProgramState->sceneExplorer.selectedItems.back().pItem == pNode))
          pSceneItem->m_editName = false;
        else
          ImGui::SetKeyboardFocusHere(-1); // Set focus to previous widget
      }
      else
      {
        if (pSceneItem->m_pName != nullptr)
        {
          udFree(pSceneItem->m_pName);
          pSceneItem->m_nameCapacity = 0;
        }

        pSceneItem->m_expanded = ImGui::TreeNodeEx(udTempStr("%s###SXIName%zu", pNode->pName, *pItemID), flags);
        if (pSceneItem->m_selected && pProgramState->sceneExplorer.selectedItems.back().pParent == m_pNode && pProgramState->sceneExplorer.selectedItems.back().pItem == pNode && ImGui::GetIO().KeysDown[vcHotkey::Get(vcB_RenameSceneItem)])
          pSceneItem->m_editName = true;
      }

      bool sceneExplorerItemClicked = ((ImGui::IsMouseReleased(0) && ImGui::IsItemHovered() && !ImGui::IsItemActive()) || (!pSceneItem->m_selected && ImGui::IsItemActive()));
      if (sceneExplorerItemClicked)
      {
        if (ImGui::GetIO().KeyShift && pProgramState->sceneExplorer.selectStartItem.pItem == nullptr && pProgramState->sceneExplorer.selectedItems.size() > 0)
          pProgramState->sceneExplorer.selectStartItem = pProgramState->sceneExplorer.selectedItems.back();
        else if (!ImGui::GetIO().KeyShift)
          pProgramState->sceneExplorer.selectStartItem = {};

        if (!ImGui::GetIO().KeyCtrl)
          vcProject_ClearSelection(pProgramState);

        if (pSceneItem->m_selected)
        {
          vcProject_UnselectItem(pProgramState, m_pNode, pNode);
          pProgramState->sceneExplorer.clickedItem = { nullptr, nullptr };
        }
        else
        {
          vcProject_SelectItem(pProgramState, m_pNode, pNode);
          pProgramState->sceneExplorer.clickedItem = { m_pNode, pNode };
        }

        pSceneItem->SelectSubitem(0);
      }

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDragging(0))
      {
        ImVec2 minPos = ImGui::GetItemRectMin();
        ImVec2 maxPos = ImGui::GetItemRectMax();
        ImVec2 mousePos = ImGui::GetMousePos();

        if (pNode->itemtype == vdkPNT_Folder && mousePos.y > minPos.y && mousePos.y < maxPos.y)
          pProgramState->sceneExplorer.insertItem = { pNode, nullptr };
        else
          pProgramState->sceneExplorer.insertItem = { m_pNode, pNode }; // This will become pNode->pNextSibling after drop
      }

      if (ImGui::BeginPopupContextItem(udTempStr("ModelContextMenu_%zu", *pItemID)))
      {
        if (!pSceneItem->m_selected)
        {
          if (!ImGui::GetIO().KeyCtrl)
            vcProject_ClearSelection(pProgramState);

          vcProject_SelectItem(pProgramState, m_pNode, pNode);
          pProgramState->sceneExplorer.clickedItem = { m_pNode, pNode };
        }

        if (ImGui::Selectable(vcString::Get("sceneExplorerEditName")))
          pSceneItem->m_editName = true;

        if (pSceneItem->m_pPreferredProjection != nullptr && pSceneItem->m_pPreferredProjection->srid != 0 && ImGui::Selectable(vcString::Get("sceneExplorerUseProjection")))
        {
          if (vcGIS_ChangeSpace(&pProgramState->geozone, *pSceneItem->m_pPreferredProjection, &pProgramState->camera.position))
          {
            pProgramState->activeProject.pFolder->ChangeProjection(*pSceneItem->m_pPreferredProjection);
          }
        }

        if (pNode->itemtype != vdkPNT_Folder && pSceneItem->GetWorldSpacePivot() != udDouble3::zero() && ImGui::Selectable(vcString::Get("sceneExplorerMoveTo")))
        {
          // Trigger a camera movement path
          if (pNode->itemtype != vdkPNT_Viewpoint)
          {
            pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
          }
          else
          {
            pProgramState->cameraInput.inputState = vcCIS_MoveToViewpoint;
            vdkProjectNode_GetMetadataDouble(pNode, "transform.heading", &pProgramState->cameraInput.headingPitch.x, 0.0);
            vdkProjectNode_GetMetadataDouble(pNode, "transform.pitch", &pProgramState->cameraInput.headingPitch.y, 0.0);
            pProgramState->cameraInput.targetAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->camera.position, pProgramState->cameraInput.headingPitch);
          }

          pProgramState->cameraInput.startPosition = pProgramState->camera.position;
          pProgramState->cameraInput.startAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->camera.position, pProgramState->camera.headingPitch);
          pProgramState->cameraInput.progress = 0.0;

          pProgramState->isUsingAnchorPoint = true;
          pProgramState->worldAnchorPoint = pSceneItem->GetWorldSpacePivot();
        }

        pSceneItem->HandleContextMenu(pProgramState);
        
        ImGui::Separator();

        if (ImGui::Selectable(vcString::Get("sceneExplorerRemoveItem")))
        {
          ImGui::EndPopup();

          if (pSceneItem->m_expanded)
            ImGui::TreePop();

          vcProject_RemoveItem(pProgramState, m_pNode, pNode);
          pProgramState->sceneExplorer.clickedItem = { nullptr, nullptr };

          return;
        }

        ImGui::EndPopup();
      }

      if (pSceneItem->Is3DSceneObject() && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
      {
        if (pSceneItem->m_pPreferredProjection != nullptr)
          vcProject_UseProjectionFromItem(pProgramState, pSceneItem);
        else
          pSceneItem->SetCameraPosition(pProgramState);
      }

      if (vcIGSW_IsItemHovered())
        ImGui::SetTooltip("%s", pNode->pName);

      if (!pSceneItem->m_expanded && pProgramState->sceneExplorer.insertItem.pParent == pNode && pProgramState->sceneExplorer.insertItem.pItem == nullptr)
      {
        // Double indent to match open folder insertion
        ImGui::Indent();
        ImGui::Indent();
        vcFolder_AddInsertSeparator();
        ImGui::Unindent();
        ImGui::Unindent();
      }

      // Show additional settings from ImGui
      if (pSceneItem->m_expanded)
      {
        ImGui::Indent();
        ImGui::PushID(udTempStr("SXIExpanded%zu", *pItemID));

        pSceneItem->HandleSceneExplorerUI(pProgramState, pItemID);

        ImGui::PopID();
        ImGui::Unindent();
        ImGui::TreePop();
      }
    }

    pNode = pNode->pNextSibling;
  }

  // This block is also in the loop above
  if (pProgramState->sceneExplorer.insertItem.pParent == m_pNode && pNode == pProgramState->sceneExplorer.insertItem.pItem)
    vcFolder_AddInsertSeparator();
}

void vcFolder::Cleanup(vcState *pProgramState)
{
  vdkProjectNode *pNode = m_pNode->pFirstChild;

  while (pNode != nullptr)
  {
    vcProject_RemoveItem(pProgramState, m_pNode, pNode);
    pNode = pNode->pNextSibling;
  }
}
