#include "vcFlythrough.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcSettings.h"

#include "vcRender.h"
#include "vcFramebuffer.h"

#include "udMath.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "imgui_internal.h"
#include "vcModals.h"
#include "vcInternalModels.h"

#include "vcFeatures.h"
#include "vcError.h"

static const int vcFlythroughExportFPS[] = { 12, 24, 30, 60, 120 };
const char *vcFlythroughExportFormats[] =
{
  "png",
  "jpg"
};
static const udUInt2 vcScreenshotResolutions[] = { { 1280, 720 }, { 1920, 1080 }, { 4096, 2160 } };
const char *vcScreenshotResolutionNameKeys[] = { "settingsScreenshotRes720p", "settingsScreenshotRes1080p", "settingsScreenshotRes4K" };

vcFlythrough::vcFlythrough(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
{
  m_loadStatus = vcSLS_Loaded;
  m_flightPoints.Init(128);
  m_state = vcFTS_None;
  m_timePosition = 0.0;
  m_timeLength = 0.0;
  m_exportPath[0] = '\0';
  m_selectedResolutionIndex = (int)udLengthOf(vcScreenshotResolutions) - 1;
  m_selectedExportFPSIndex = 3;
  m_selectedExportFormatIndex = 0;
  m_pLine = nullptr;
  m_selectedFlightPoint = -1;
  m_centerPoint = udDouble3::zero();
  memset(&m_exportInfo, 0, sizeof(m_exportInfo));
  m_pProgramState = pProgramState;

  m_exportInfo.currentFrame = 0;
  m_exportInfo.frameDelta = (1.0 / (double)vcFlythroughExportFPS[m_selectedExportFPSIndex]);

  OnNodeUpdate(pProgramState);
}

void vcFlythrough::CancelExport(vcState *pProgramState)
{
  if (m_state == vcFTS_Exporting)
  {
    m_state = vcFTS_None;
    pProgramState->screenshot.pImage = nullptr;
    m_exportInfo.currentFrame = -1;
    pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem = nullptr;
    pProgramState->exportVideo = false;
    vcModals_CloseModal(pProgramState, vcMT_FlythroughExport);
  }
}

void vcFlythrough::OnNodeUpdate(vcState *pProgramState)
{
  ChangeProjection(pProgramState->geozone);
}

void vcFlythrough::SelectSubitem(uint64_t internalId)
{
  m_selectedFlightPoint = ((int)internalId) - 1;
}

bool vcFlythrough::IsSubitemSelected(uint64_t internalId)
{
  return (m_selected && (m_selectedFlightPoint == ((int)internalId - 1) || m_selectedFlightPoint == -1));
}

void vcFlythrough_ApplyDeltaToFlightPoint(vcFlightPoint *pFlightPoint, const udDouble4x4 &delta, const udGeoZone &geozone)
{
  udDoubleQuat orientation = delta.extractQuaternion() * vcGIS_HeadingPitchToQuaternion(geozone, pFlightPoint->m_CameraPosition, pFlightPoint->m_CameraHeadingPitch);
  pFlightPoint->m_CameraHeadingPitch = vcGIS_QuaternionToHeadingPitch(geozone, pFlightPoint->m_CameraPosition, orientation);
  pFlightPoint->m_CameraPosition = (delta * udDouble4x4::translation(pFlightPoint->m_CameraPosition)).axis.t.toVector3();
};

void vcFlythrough::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  if (m_selectedFlightPoint == -1)
  {
    for (vcFlightPoint &flightPoint : m_flightPoints)
      vcFlythrough_ApplyDeltaToFlightPoint(&flightPoint, delta, pProgramState->geozone);
  }
  else
  {
    vcFlythrough_ApplyDeltaToFlightPoint(&m_flightPoints[m_selectedFlightPoint], delta, pProgramState->geozone);
  }

  UpdateLinePoints();
  UpdateCenterPoint();
  SaveFlightPoints(pProgramState);
}

udDouble4x4 vcFlythrough::GetWorldSpaceMatrix()
{
  if (m_flightPoints.length == 0)
    return udDouble4x4::identity();
  else if (m_selectedFlightPoint == -1)
    return udDouble4x4::translation(m_centerPoint);
  else
    return udDouble4x4::translation(m_flightPoints[m_selectedFlightPoint].m_CameraPosition);
}

void vcFlythrough::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  int offset = -1; // The index of the _next_ node

  if (!m_selected)
    m_state = vcFTS_None;

  if (m_state == vcFTS_Exporting)
  {
    if (pProgramState->screenshot.pImage != nullptr)
    {
      if (m_exportInfo.currentFrame >= 0)
      {
        const char *pSavePath = udTempStr("%s/%05d.%s", m_exportPath, m_exportInfo.currentFrame, vcFlythroughExportFormats[m_selectedExportFormatIndex]);
        udResult saveResult = vcTexture_SaveImage(pProgramState->screenshot.pImage, vcRender_GetSceneFramebuffer(pProgramState->pActiveViewport->pRenderContext), pSavePath);

        if (saveResult != udR_Success)
        {
          ErrorItem flythroughSaveError = {};
          flythroughSaveError.source = vcES_File;
          flythroughSaveError.pData = udStrdup(pSavePath);
          flythroughSaveError.resultCode = saveResult;
          vcError_AddError(pProgramState, flythroughSaveError);

          m_state = vcFTS_None;
          pProgramState->exportVideo = false;
          vcModals_CloseModal(pProgramState, vcMT_FlythroughExport);
        }
      }

      ++m_exportInfo.currentFrame;
      m_timePosition = (m_exportInfo.currentFrame * m_exportInfo.frameDelta);

      // This must be run here as vcTexture_SaveImage will change which framebuffer is bound but not reset it
      vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
    }
  }

  if (m_state == vcFTS_Playing || m_state == vcFTS_Recording)
    m_timePosition += pProgramState->deltaTime;

  for (size_t i = 0; i < m_flightPoints.length; ++i)
  {
    if (m_flightPoints[i].time > m_timePosition)
    {
      offset = (int)i;
      break;
    }
  }

  if (m_state == vcFTS_Recording)
  {
    vcFlightPoint newFlightPoint = {};

    newFlightPoint.m_CameraPosition = pProgramState->pViewports[0].camera.position;
    newFlightPoint.m_CameraHeadingPitch = pProgramState->pViewports[0].camera.headingPitch;
    newFlightPoint.time = m_timePosition;

    if (offset == -1)
    {
      if (m_flightPoints.length <= 1 || m_flightPoints[m_flightPoints.length - 1].m_CameraPosition != newFlightPoint.m_CameraPosition || m_flightPoints[m_flightPoints.length - 1].m_CameraHeadingPitch != newFlightPoint.m_CameraHeadingPitch)
        m_flightPoints.PushBack(newFlightPoint);
      else
        m_flightPoints[m_flightPoints.length - 1].time = m_timePosition;

      m_timeLength = m_timePosition;
    }
    else
    {
      m_flightPoints.Insert(offset, &newFlightPoint);

      for (size_t i = offset + 1; i < m_flightPoints.length; ++i)
        m_flightPoints[i].time += pProgramState->deltaTime;

      m_timeLength = m_flightPoints[m_flightPoints.length - 1].time;
    }
  }

  if (m_state == vcFTS_Playing || m_state == vcFTS_Exporting)
  {
    if (m_timePosition > m_timeLength)
    {
      if (m_state == vcFTS_Exporting)
      {
        pProgramState->exportVideo = false;
        vcModals_CloseModal(pProgramState, vcMT_FlythroughExport);
        //system("./assets/bin/ffmpeg.exe -y -f image2 -framerate 60 -i ./_temp/%05d.png -c:v vp9 -b:v 1M output.avi");
        //udFindDir *pDir = nullptr;
        //udOpenDir(&pDir, "./_temp");
        //do
        //{
        //  if (!pDir->isDirectory)
        //    udFileDelete(udTempStr("./_temp/%s", pDir->pFilename));
        //} while (udReadDir(pDir) == udR_Success);
        //udCloseDir(&pDir);
      }

      m_state = vcFTS_None;
    }

    if (pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem == this)
    {
      UpdateCameraPosition(pProgramState);
    }
  }

  if (m_selected && m_state == vcFTS_None && m_pLine != nullptr && m_flightPoints.length > 1)
  {
    pRenderData->depthLines.PushBack(m_pLine);

    for (size_t i = 0; i < m_flightPoints.length; ++i)
    {
      if (m_selectedFlightPoint != -1 && (int)i != m_selectedFlightPoint)
        continue;

      vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();

      pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
      udDouble3 linearDistance = (pProgramState->pActiveViewport->camera.position - m_flightPoints[i].m_CameraPosition);
      pInstance->worldMat = udDouble4x4::translation(m_flightPoints[i].m_CameraPosition) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
      pInstance->pSceneItem = this;
      pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
      pInstance->sceneItemInternalId = (uint64_t)i + 1;
      pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);
      pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
      pInstance->selectable = true;
      pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.85f);
    }
  }
}

void vcFlythrough::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  int removeAt = -1;
  int addAfter = -1;

  if (m_flightPoints.length > 2 && ImGui::Button(vcString::Get("flythroughSmooth")))
    SmoothFlightPoints();
  
  ImGui::Columns(7);

  for (size_t i = 0; i < m_flightPoints.length; ++i)
  {
    ImGui::PushID(udTempStr("ftpt_%zu", (*pItemID)));

    double minTime = (i == 0 ? 0 : m_flightPoints[i - 1].time);
    double maxTime = (i == 0 ? 0 : (i == m_flightPoints.length - 1 ? m_flightPoints[i].time + 10.0 : m_flightPoints[i + 1].time));

    if (ImGui::DragScalar("T", ImGuiDataType_Double, &m_flightPoints[i].time, 0.1f, &minTime, &maxTime, 0, ImGuiSliderFlags_ClampOnInput))
    {
      // ImGui::DragScalar doesn't clamp when min/max are the same value
      if (minTime == maxTime)
        m_flightPoints[i].time = minTime;

      if (i == m_flightPoints.length - 1)
        m_timeLength = m_flightPoints[i].time;
    }

    ImGui::NextColumn();
    udDouble3 positionTemp = m_flightPoints[i].m_CameraPosition;
    udDouble2 rotationTemp = m_flightPoints[i].m_CameraHeadingPitch;

    if (ImGui::InputDouble("PX", &positionTemp.x))
    {
      if (isfinite(positionTemp.x))
        m_flightPoints[i].m_CameraPosition.x = udClamp(positionTemp.x, -vcSL_GlobalLimit, vcSL_GlobalLimit);
    }

    ImGui::NextColumn();
    if (ImGui::InputDouble("PY", &positionTemp.y))
    {
      if (isfinite(positionTemp.y))
        m_flightPoints[i].m_CameraPosition.y = udClamp(positionTemp.y, -vcSL_GlobalLimit, vcSL_GlobalLimit);
    }

    ImGui::NextColumn();
    if (ImGui::InputDouble("PZ", &positionTemp.z))
    {
      if (isfinite(positionTemp.z))
        m_flightPoints[i].m_CameraPosition.z = udClamp(positionTemp.z, -vcSL_GlobalLimit, vcSL_GlobalLimit);
    }

    ImGui::NextColumn();
    if (ImGui::InputDouble("RH", &rotationTemp.x))
    {
      if (isfinite(rotationTemp.x))
        m_flightPoints[i].m_CameraHeadingPitch.x = rotationTemp.x;
    }

    ImGui::NextColumn();
    if (ImGui::InputDouble("RP", &rotationTemp.y))
    {
      if (isfinite(rotationTemp.y))
        m_flightPoints[i].m_CameraHeadingPitch.y = rotationTemp.y;
    }

    ImGui::NextColumn();

    if (ImGui::Button("C"))
    {
      m_flightPoints[i].m_CameraPosition = pProgramState->pViewports[0].camera.position;
      m_flightPoints[i].m_CameraHeadingPitch = pProgramState->pViewports[0].camera.headingPitch;
    }

    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("flythroughCopyPosition"));

    ImGui::SameLine();

    if (ImGui::Button("+V"))
      addAfter = (int)i;

    if (vcIGSW_IsItemHovered())
      ImGui::SetTooltip("%s", vcString::Get("flythroughAddAfter"));

    ImGui::SameLine();

    if (i != 0)
    {
      if (ImGui::Button("X"))
        removeAt = (int)i;

      if (vcIGSW_IsItemHovered())
        ImGui::SetTooltip("%s", vcString::Get("flythroughRemoveAt"));
    }

    ImGui::NextColumn();

    ImGui::PopID();

    ++(*pItemID);
  }

  ImGui::Columns(1);

  if (removeAt != -1)
    m_flightPoints.RemoveAt(removeAt);

  if (addAfter != -1)
  {
    vcFlightPoint point = m_flightPoints[addAfter];
    m_flightPoints.Insert(addAfter, &point);
  }
}

void vcFlythrough::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  ImGui::Text("%s %.2fs / %.2fs", vcString::Get("flythroughPlayback"), m_timePosition, m_timeLength);

  double zero = 0.0;
  if (ImGui::SliderScalar(vcString::Get("flythroughPlaybackTime"), ImGuiDataType_Double, &m_timePosition, &zero, &m_timeLength, 0, ImGuiSliderFlags_ClampOnInput))
    UpdateCameraPosition(pProgramState);

  switch (m_state)
  {
  case vcFTS_None:
    if (ImGui::Button(vcString::Get("flythroughRecord")))
    {
      m_state = vcFTS_Recording;

      if (m_flightPoints.length == 0)
      {
        vcFlightPoint *pPoint = m_flightPoints.PushBack();

        pPoint->m_CameraPosition = pProgramState->pViewports[0].camera.position;
        pPoint->m_CameraHeadingPitch = pProgramState->pViewports[0].camera.headingPitch;
        pPoint->time = 0.0;
      }
      else
      {
        // Delete all keyframes after current time
        for (size_t index = m_flightPoints.length - 1; index > 0; index--)
        {
          if (m_flightPoints[index].time > m_timePosition)
            m_flightPoints.PopBack();
        }
      }
    }
    ImGui::SameLine();

    if (ImGui::ButtonEx(vcString::Get("flythroughPlay"), ImVec2(0, 0), (m_flightPoints.length == 0 ? (ImGuiButtonFlags_)ImGuiButtonFlags_Disabled : ImGuiButtonFlags_None)))
    {
      pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem = this;
      m_state = vcFTS_Playing;
      m_timePosition = 0.0;
    }

#if VC_HASCONVERT
    if (ImGui::BeginMenu(vcString::Get("flythroughExport"), m_flightPoints.length > 0))
    {
      if (ImGui::BeginCombo(vcString::Get("flythroughResolution"), vcString::Get(vcScreenshotResolutionNameKeys[m_selectedResolutionIndex])))
      {
        for (size_t i = 0; i < udLengthOf(vcScreenshotResolutions); ++i)
        {
          if (ImGui::Selectable(vcString::Get(vcScreenshotResolutionNameKeys[i])))
            m_selectedResolutionIndex = (int)i;
        }

        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo(vcString::Get("flythroughFPS"), udTempStr("%d", vcFlythroughExportFPS[m_selectedExportFPSIndex])))
      {
        for (size_t i = 0; i < udLengthOf(vcFlythroughExportFPS); ++i)
        {
          if (ImGui::Selectable(udTempStr("%d", vcFlythroughExportFPS[i])))
            m_selectedExportFPSIndex = (int)i;
        }

        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo(vcString::Get("flythroughExportFormat"), vcFlythroughExportFormats[m_selectedExportFormatIndex]))
      {
        for (size_t i = 0; i < udLengthOf(vcFlythroughExportFormats); ++i)
        {
          if (ImGui::Selectable(vcFlythroughExportFormats[i]))
            m_selectedExportFormatIndex = (int)i;
        }

        ImGui::EndCombo();
      }

      vcIGSW_FilePicker(pProgramState, vcString::Get("flythroughExportPath"), m_exportPath, udLengthOf(m_exportPath), nullptr, 0, vcFDT_SelectDirectory, nullptr);

      if (ImGui::ButtonEx(vcString::Get("flythroughExport"), ImVec2(0, 0), (m_exportPath[0] == '\0' ? (ImGuiButtonFlags_)ImGuiButtonFlags_Disabled : ImGuiButtonFlags_None)))
      {
        vcModals_OpenModal(pProgramState, vcMT_FlythroughExport);
        int frameIndex = 0;
        const char *pFilepath = GenerateFrameExportPath(frameIndex);
        if (vcModals_OverwriteExistingFile(pProgramState, pFilepath, vcString::Get("flythroughExportAlreadyExists")))
        {
          // Delete Overwritten Flythrough Images
          while (udFileExists(pFilepath) == udR_Success)
          {
            udFileDelete(pFilepath);
            ++frameIndex;
            pFilepath = GenerateFrameExportPath(frameIndex);
          }

          m_state = vcFTS_Exporting;
          pProgramState->screenshot.pImage = nullptr;
          m_exportInfo.currentFrame = -2;
          m_exportInfo.frameDelta = (1.0 / (double)vcFlythroughExportFPS[m_selectedExportFPSIndex]);
          pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem = this;
          pProgramState->exportVideo = true;
          pProgramState->exportVideoResolution = vcScreenshotResolutions[m_selectedResolutionIndex];
        }
      }

      ImGui::EndMenu();
    }
#endif // VC_HASCONVERT
    break;
  case vcFTS_Playing:
    if (ImGui::Button(vcString::Get("flythroughPlayStop")))
      m_state = vcFTS_None;

    ImGui::SameLine();

    ImGui::TextUnformatted(vcString::Get("flythroughPlaying"));
    break;
  case vcFTS_Recording:
    if (ImGui::Button(vcString::Get("flythroughRecordStop")))
    {
      UpdateLinePoints();
      SaveFlightPoints(pProgramState);
      m_state = vcFTS_None;
    }

    ImGui::SameLine();

    ImGui::TextUnformatted(vcString::Get("flythroughRecording"));
    break;
  case vcFTS_Exporting:
    break;
  }

  if (m_state != vcFTS_Playing && m_state != vcFTS_Exporting && pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem == this)
    pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem = nullptr;
}

void vcFlythrough::ChangeProjection(const udGeoZone &newZone)
{
  LoadFlightPoints(m_pProgramState, newZone);
}

void vcFlythrough::Cleanup(vcState * /*pProgramState*/)
{
  vcLineRenderer_DestroyLine(&m_pLine);
}

void vcFlythrough::SetCameraPosition(vcState *pProgramState)
{
  m_state = vcFTS_Playing;
  m_timePosition = 0.0;
  pProgramState->pViewports[0].cameraInput.pAttachedToSceneItem = this;
}

void vcFlythrough::UpdateCameraPosition(vcState *pProgramState)
{
  for (size_t i = 0; i < m_flightPoints.length; ++i)
  {
    if (m_flightPoints[i].time > m_timePosition)
    {
      if (i == 0)
      {
        pProgramState->pViewports[0].camera.position = m_flightPoints[i].m_CameraPosition;
        pProgramState->pViewports[0].camera.headingPitch = m_flightPoints[i].m_CameraHeadingPitch;
      }
      else
      {
        LerpFlightPoints(m_timePosition, m_flightPoints[i - 1], m_flightPoints[i], &pProgramState->pViewports[0].camera.position, &pProgramState->pViewports[0].camera.headingPitch);
      }

      break;
    }
  }
}

void vcFlythrough::UpdateLinePoints()
{
  if (m_flightPoints.length > 1)
  {
    if (m_pLine == nullptr)
      vcLineRenderer_CreateLine(&m_pLine);
    udDouble3 *pPositions = udAllocType(udDouble3, m_flightPoints.length, udAF_Zero);
    for (size_t i = 0; i < m_flightPoints.length; ++i)
      pPositions[i] = m_flightPoints[i].m_CameraPosition;
    vcLineRenderer_UpdatePoints(m_pLine, pPositions, m_flightPoints.length, vcIGSW_BGRAToImGui(0xFFFFFF00), 8.0, false);
    udFree(pPositions);
  }
}

void vcFlythrough::UpdateCenterPoint()
{
  if (m_flightPoints.length == 0)
  {
    m_centerPoint = udDouble3::zero();
    return;
  }

  udDouble3 min = udDouble3::create(DBL_MAX);
  udDouble3 max = udDouble3::create(-DBL_MAX);

  for (const vcFlightPoint &flightPoint : m_flightPoints)
  {
    min.x = udMin(min.x, flightPoint.m_CameraPosition.x);
    min.y = udMin(min.y, flightPoint.m_CameraPosition.y);
    min.z = udMin(min.z, flightPoint.m_CameraPosition.z);
    max.x = udMax(max.x, flightPoint.m_CameraPosition.x);
    max.y = udMax(max.y, flightPoint.m_CameraPosition.y);
    max.z = udMax(max.z, flightPoint.m_CameraPosition.z);
  }

  m_centerPoint = (min + max) / 2.0;
}

void vcFlythrough::LoadFlightPoints(vcState *pProgramState, const udGeoZone &zone)
{
  udDouble3 *pFPPositions;
  int numFrames;
  vcProject_FetchNodeGeometryAsCartesian(&pProgramState->activeProject, m_pNode, zone, &pFPPositions, &numFrames);

  m_flightPoints.Clear();

  for (size_t i = 0; i < (size_t)numFrames; ++i)
  {
    vcFlightPoint flightPoint;
    flightPoint.m_CameraPosition = pFPPositions[i];
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("time[%zu]", i), &flightPoint.time, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("cameraHeadingPitch[%zu].x", i), &flightPoint.m_CameraHeadingPitch.x, 0.0);
    udProjectNode_GetMetadataDouble(m_pNode, udTempStr("cameraHeadingPitch[%zu].y", i), &flightPoint.m_CameraHeadingPitch.y, 0.0);

    m_flightPoints.PushBack(flightPoint);
  }

  UpdateCenterPoint();
  
  udFree(pFPPositions);
  m_timeLength = m_flightPoints.length > 0 ? m_flightPoints[m_flightPoints.length - 1].time : 0.0;
}

void vcFlythrough::SaveFlightPoints(vcState *pProgramState)
{
  for (size_t i = 0; i < m_flightPoints.length; ++i)
  {
    udProjectNode_SetMetadataDouble(m_pNode, udTempStr("time[%zu]", i), m_flightPoints[i].time);
    udProjectNode_SetMetadataDouble(m_pNode, udTempStr("cameraHeadingPitch[%zu].x", i), m_flightPoints[i].m_CameraHeadingPitch.x);
    udProjectNode_SetMetadataDouble(m_pNode, udTempStr("cameraHeadingPitch[%zu].y", i), m_flightPoints[i].m_CameraHeadingPitch.y);
  }

  udDouble3 *pFPPositions = udAllocType(udDouble3, m_flightPoints.length, udAF_Zero);
  for (size_t i = 0; i < m_flightPoints.length; ++i)
    pFPPositions[i] = m_flightPoints[i].m_CameraPosition;
  vcProject_UpdateNodeGeometryFromCartesian(&pProgramState->activeProject, m_pNode, pProgramState->geozone, udPGT_LineString, pFPPositions, (int)m_flightPoints.length);
  udFree(pFPPositions);
}

void vcFlythrough::SmoothFlightPoints()
{
  for (size_t flightPointIndex = 1; flightPointIndex < m_flightPoints.length - 1; ++flightPointIndex)
  {
    udDouble3 expectedPos = udDouble3::zero();
    LerpFlightPoints(m_flightPoints[flightPointIndex].time, m_flightPoints[flightPointIndex - 1], m_flightPoints[flightPointIndex + 1], &expectedPos, nullptr);

    double distanceSq = udMagSq(m_flightPoints[flightPointIndex - 1].m_CameraPosition - m_flightPoints[flightPointIndex + 1].m_CameraPosition);

    // Used 1000th of the distance for the offset and eyeballed for good results 
    double maxOffsetDistanceSq = udMax(UD_EPSILON, distanceSq / 1000000.0);
    if (udMagSq(m_flightPoints[flightPointIndex].m_CameraPosition - expectedPos) < maxOffsetDistanceSq)
    {
      m_flightPoints.RemoveAt(flightPointIndex);
      --flightPointIndex;
    }
  }
}

void vcFlythrough::LerpFlightPoints(double timePosition, const vcFlightPoint &flightPoint1, const vcFlightPoint &flightPoint2, udDouble3 *pLerpedPosition, udDouble2 *pLerpedHeadingPitch)
{
  double lerp = (timePosition - flightPoint1.time) / (flightPoint2.time - flightPoint1.time);

  if (pLerpedPosition != nullptr)
    *pLerpedPosition = udLerp(flightPoint1.m_CameraPosition, flightPoint2.m_CameraPosition, lerp);

  if (pLerpedHeadingPitch != nullptr)
    *pLerpedHeadingPitch = udLerp(flightPoint1.m_CameraHeadingPitch, flightPoint2.m_CameraHeadingPitch, lerp);
}

const char* vcFlythrough::GenerateFrameExportPath(int frameIndex)
{
  return udTempStr("%s/%05d.%s", m_exportPath, frameIndex, vcFlythroughExportFormats[m_selectedExportFormatIndex]);
}
