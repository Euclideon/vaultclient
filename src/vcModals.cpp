#include "vcModals.h"

#include "vcState.h"
#include "vcVersion.h"
#include "vcThirdPartyLicenses.h"
#include "gl/vcTexture.h"
#include "vcRender.h"
#include "vcStrings.h"
#include "vcConvert.h"

#include "vCore/vStringFormat.h"

#include "udPlatform/udFile.h"

#include "imgui.h"
#include "imgui_ex/vcFileDialog.h"

#include "stb_image.h"

void vcModals_DrawLoggedOut(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoggedOut))
    ImGui::OpenPopup(vcString::Get("LoggedOut"));

  if (ImGui::BeginPopupModal(vcString::Get("LoggedOut"), nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::TextUnformatted(vcString::Get("Logged"));

    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_LoggedOut);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawReleaseNotes(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ReleaseNotes))
    ImGui::OpenPopup(vcString::Get("MenuReleaseNotes"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("MenuReleaseNotes")))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client / %s", vcString::Get("ReleaseNotes"));

    ImGui::Text("%s: %s", vcString::Get("CurrentVersion"), VCVERSION_PRODUCT_STRING);

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    if (pProgramState->pReleaseNotes == nullptr)
      udFile_Load("asset://releasenotes.md", (void**)&pProgramState->pReleaseNotes);

    if (pProgramState->pReleaseNotes != nullptr)
    {
      ImGui::BeginChild(vcString::Get("ReleaseShort"));
      ImGui::TextUnformatted(pProgramState->pReleaseNotes);
      ImGui::EndChild();
    }
    else
    {
      ImGui::TextUnformatted(vcString::Get("ReleaseNotesFail"));
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawAbout(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_About))
    ImGui::OpenPopup(vcString::Get("MenuAbout"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("MenuAbout")))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client / %s", vcString::Get("3rdPartyLic"));

    ImGui::Text("%s: %s", vcString::Get("Version"), VCVERSION_PRODUCT_STRING);

    char strBuf[128];
    if (pProgramState->packageInfo.Get("success").AsBool())
      ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", vStringFormat(strBuf, udLengthOf(strBuf), vcString::Get("PackageUpdate"), pProgramState->packageInfo.Get("package.versionstring").AsString()));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild("Licenses");
    for (int i = 0; i < (int)UDARRAYSIZE(ThirdPartyLicenses); i++)
    {
      // ImGui::Text has a limitation of 3072 bytes.
      ImGui::TextUnformatted(ThirdPartyLicenses[i].pName);
      ImGui::TextUnformatted(ThirdPartyLicenses[i].pLicense);
      ImGui::Separator();
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_DrawNewVersionAvailable(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_NewVersionAvailable))
    ImGui::OpenPopup(vcString::Get("NewVersionAvailable"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("NewVersionAvailable")))
  {
    ImGui::Columns(2, NULL, false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
    ImGui::Text("Euclideon Vault Client");

    ImGui::Text("%s: %s", vcString::Get("CurrentVersion"), VCVERSION_PRODUCT_STRING);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.f, 0.5f, 1.f));
    ImGui::TextWrapped("%s: %s", vcString::Get("NewVersion"), pProgramState->packageInfo.Get("package.versionstring").AsString());
    ImGui::PopStyleColor();

    ImGui::TextWrapped("%s", vcString::Get("DownloadPrompt"));

    ImGui::NextColumn();
    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
    {
      ImGui::CloseCurrentPopup();
      pProgramState->openModals &= ~(1 << vcMT_NewVersionAvailable);
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::BeginChild(vcString::Get("MenuReleaseNotes"));
    ImGui::TextUnformatted(pProgramState->packageInfo.Get("package.releasenotes").AsString(""));
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void vcModals_SetTileImage(void *pProgramStatePtr)
{
  vcState *pProgramState = (vcState*)pProgramStatePtr;

  char buf[256];
  udSprintf(buf, udLengthOf(buf), "%s/0/0/0.%s", pProgramState->settings.maptiles.tileServerAddress, pProgramState->settings.maptiles.tileServerExtension);

  int64_t imageSize;
  void *pLocalData = nullptr;

  if (udFile_Load(buf, &pLocalData, &imageSize) != udR_Success)
    imageSize = -2;

  pProgramState->tileModal.loadStatus = imageSize;
  udFree(pProgramState->tileModal.pImageData);
  udInterlockedExchangePointer(&pProgramState->tileModal.pImageData, pLocalData);
}

inline bool vcModals_TileThread(vcState *pProgramState)
{
  if (pProgramState->tileModal.loadStatus == -1)
    return false; // Avoid the thread starting if its already running

  pProgramState->tileModal.loadStatus = -1; // Loading
  vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);

  size_t urlLen = udStrlen(pProgramState->settings.maptiles.tileServerAddress);
  if (urlLen == 0)
  {
    pProgramState->tileModal.loadStatus = -2;
    return true;
  }

  if (pProgramState->settings.maptiles.tileServerAddress[urlLen - 1] == '/')
    pProgramState->settings.maptiles.tileServerAddress[urlLen - 1] = '\0';

  vWorkerThread_AddTask(pProgramState->pWorkerPool, vcModals_SetTileImage, pProgramState, false);

  return true;
}

void vcModals_DrawTileServer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_TileServer))
  {
    ImGui::OpenPopup(vcString::Get("TileServer"));
    if (pProgramState->tileModal.pServerIcon == nullptr)
      vcModals_TileThread(pProgramState);
  }

  ImGui::SetNextWindowSize(ImVec2(300, 342), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("TileServer")))
  {
    // If there is loaded data, we turn it into a texture:
    if (pProgramState->tileModal.loadStatus > 0)
    {
      if (pProgramState->tileModal.pServerIcon != nullptr)
        vcTexture_Destroy(&pProgramState->tileModal.pServerIcon);

      uint32_t width, height, channelCount;
      uint8_t *pData = stbi_load_from_memory((stbi_uc*)pProgramState->tileModal.pImageData, (int)pProgramState->tileModal.loadStatus, (int*)&width, (int*)&height, (int*)&channelCount, 4);
      udFree(pProgramState->tileModal.pImageData);

      if (pData)
        vcTexture_Create(&pProgramState->tileModal.pServerIcon, width, height, pData, vcTextureFormat_RGBA8, vcTFM_Linear, false, vcTWM_Repeat, vcTCF_None, 0);

      stbi_image_free(pData);
      pProgramState->tileModal.loadStatus = 0;
    }

    static bool s_isDirty = false;
    static int s_currentItem = -1;

    const char *pItems[] = { "png", "jpg" };
    if (s_currentItem == -1)
    {
      for (int i = 0; i < (int)udLengthOf(pItems); ++i)
      {
        if (udStrEquali(pItems[i], pProgramState->settings.maptiles.tileServerExtension))
          s_currentItem = i;
      }
    }

    if (ImGui::InputText(vcString::Get("TileServer"), pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength))
      s_isDirty = true;

    if (ImGui::Combo(vcString::Get("ImageFormat"), &s_currentItem, pItems, (int)udLengthOf(pItems)))
    {
      udStrcpy(pProgramState->settings.maptiles.tileServerExtension, udLengthOf(pProgramState->settings.maptiles.tileServerExtension), pItems[s_currentItem]);
      vcModals_TileThread(pProgramState);
      s_isDirty = true;
    }

    if (s_isDirty)
      s_isDirty = !vcModals_TileThread(pProgramState);

    ImGui::SetItemDefaultFocus();

    if (pProgramState->tileModal.loadStatus == -1)
      ImGui::TextUnformatted(vcString::Get("LoadingWait"));
    else if (pProgramState->tileModal.loadStatus == -2)
      ImGui::TextColored(ImVec4(255, 0, 0, 255), "%s", vcString::Get("ErrorFetching"));
    else if (pProgramState->tileModal.pServerIcon != nullptr)
      ImGui::Image((ImTextureID)pProgramState->tileModal.pServerIcon, ImVec2(200, 200), ImVec2(0, 0), ImVec2(1, 1));

    if (pProgramState->tileModal.loadStatus != -1 && (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE]))
    {
      ImGui::CloseCurrentPopup();
      udFree(pProgramState->tileModal.pImageData);
      vcRender_ClearTiles(pProgramState->pRenderContext);
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawAddUDS(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_AddUDS))
    ImGui::OpenPopup(vcString::Get("SceneAddUDS"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("SceneAddUDS")))
  {
    ImGui::InputText(vcString::Get("PathURL"), pProgramState->modelPath, vcMaxPathLength);
    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Load"), ImVec2(100.f, 0)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Cancel"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".uds", ".ssf", ".udg" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawImportUDP(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ImportUDP))
    ImGui::OpenPopup(vcString::Get("SceneImportUDP"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("SceneImportUDP")))
  {
    ImGui::InputText(vcString::Get("PathURL"), pProgramState->modelPath, vcMaxPathLength);
    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Load"), ImVec2(100.f, 0)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Cancel"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".udp" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
    {
      pProgramState->loadList.push_back(udStrdup(pProgramState->modelPath));
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawLoadWatermark(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_LoadWatermark))
    ImGui::OpenPopup(vcString::Get("LoadWatermark"));

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(vcString::Get("LoadWatermark")))
  {
    ImGui::InputText(vcString::Get("PathURL"), pProgramState->modelPath, vcMaxPathLength);
    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Load"), ImVec2(100.f, 0)))
    {
      vdkConvert_AddWatermark(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext, pProgramState->modelPath);
      pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->watermark.isDirty = true;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button(vcString::Get("Cancel"), ImVec2(100.f, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::Separator();

    const char *fileExtensions[] = { ".jpg", ".png", ".tga", ".bmp", ".gif" };
    if (vcFileDialog_Show(pProgramState->modelPath, sizeof(pProgramState->modelPath), true, fileExtensions, udLengthOf(fileExtensions)))
    {
      vdkConvert_AddWatermark(pProgramState->pVDKContext, pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->pConvertContext, pProgramState->modelPath);
      pProgramState->pConvertContext->jobs[pProgramState->pConvertContext->selectedItem]->watermark.isDirty = true;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void vcModals_DrawNotImplemented(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_NotYetImplemented))
    ImGui::OpenPopup(vcString::Get("NotImplemented"));

  if (ImGui::BeginPopupModal(vcString::Get("NotImplemented"), nullptr, ImGuiWindowFlags_NoResize))
  {
    ImGui::TextUnformatted(vcString::Get("NotAvailable"));

    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void vcModals_DrawImageViewer(vcState *pProgramState)
{
  if (pProgramState->openModals & (1 << vcMT_ImageViewer))
    ImGui::OpenPopup(vcString::Get("ImageViewer"));

  int maxX, maxY;
  SDL_GetWindowSize(pProgramState->pWindow, &maxX, &maxY);

  ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2((float)maxX, (float)maxY));
  if (ImGui::BeginPopupModal(vcString::Get("ImageViewer"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
  {
    if (ImGui::Button(vcString::Get("Close"), ImVec2(-1, 0)) || ImGui::GetIO().KeysDown[SDL_SCANCODE_ESCAPE])
      ImGui::CloseCurrentPopup();

    ImGuiIO io = ImGui::GetIO();
    ImVec2 window = ImGui::GetWindowSize();
    ImVec2 windowPos = ImGui::GetWindowPos();

    ImGui::Image(pProgramState->image.pImage, ImVec2((float)pProgramState->image.width, (float)pProgramState->image.height));

    if (ImGui::IsWindowHovered())
    {
      io.MouseWheel += ImGui::IsMouseDoubleClicked(0);
      if (io.MouseWheel != 0 && (io.MouseWheel > 0 || (pProgramState->image.width > window.x || pProgramState->image.height > window.y + 15)))
      {
        float scaleFactor = io.MouseWheel / 10;

        float xRatio = float((io.MousePos.x - windowPos.x) / window.x - .5);
        float yRatio = float((io.MousePos.y - windowPos.y) / window.y - .5);

        float deltaX = pProgramState->image.width * scaleFactor;
        float deltaY = pProgramState->image.height * scaleFactor;

        ImGui::SetScrollX(ImGui::GetScrollX() + (deltaX / 2) + (deltaX * xRatio));
        ImGui::SetScrollY(ImGui::GetScrollY() + (deltaY / 2) + (deltaY * yRatio));

        pProgramState->image.width = int(pProgramState->image.width * (1 + scaleFactor));
        pProgramState->image.height = int(pProgramState->image.height * (1 + scaleFactor));
      }

      if (io.MouseDown[0])
      {
        ImGui::SetScrollX(ImGui::GetScrollX() - (ImGui::GetScrollMaxX() * (io.MouseDelta.x / window.x * 2)));
        ImGui::SetScrollY(ImGui::GetScrollY() - (ImGui::GetScrollMaxY() * (io.MouseDelta.y / window.y * 2)));
      }
    }

    ImGui::EndPopup();
  }
}
void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type)
{
  pProgramState->openModals |= (1 << type);
}

void vcModals_DrawModals(vcState *pProgramState)
{
  vcModals_DrawLoggedOut(pProgramState);
  vcModals_DrawReleaseNotes(pProgramState);
  vcModals_DrawAbout(pProgramState);
  vcModals_DrawNewVersionAvailable(pProgramState);
  vcModals_DrawTileServer(pProgramState);
  vcModals_DrawAddUDS(pProgramState);
  vcModals_DrawImportUDP(pProgramState);
  vcModals_DrawLoadWatermark(pProgramState);
  vcModals_DrawNotImplemented(pProgramState);
  vcModals_DrawImageViewer(pProgramState);

  pProgramState->openModals &= ((1 << vcMT_NewVersionAvailable) | (1 << vcMT_LoggedOut));
}
