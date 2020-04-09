#include "vcPOI.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcFenceRenderer.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

const char *vcFRVMStrings[] =
{
  "Screen Line",
  "Fence",
  "Flat",
};
UDCOMPILEASSERT(udLengthOf(vcFRVMStrings) == vcRRVM_Count, "New enum values");

static const char *vcFRIMStrings[] =
{
  "Arrow",
  "Glow",
  "Solid",
  "Diagonal"
};
UDCOMPILEASSERT(udLengthOf(vcFRIMStrings) == vcRRIM_Count, "New enum values");

static uint32_t Vec4ToUINT(udFloat4 const & vec)
{
  uint32_t val = 0;

  udFloat4 clamped = udClamp(vec, udFloat4::create(0.f, 0.f, 0.f, 0.f), udFloat4::create(1.f, 1.f, 1.f, 1.f));

  val |= ((uint32_t)(clamped[0] * 255) << 16); // Blue
  val |= ((uint32_t)(clamped[1] * 255) << 8); // Green
  val |= ((uint32_t)(clamped[2] * 255) << 0); // Red
  val |= ((uint32_t)(clamped[3] * 255) << 24); // Alpha

  return val;
}

class vcPOIState_General;
class vcPOIState_Append;

class vcPOIState
{
public:
  vcPOIState(vcPOI *pParent)
  : m_pParent(pParent)
  {};

  virtual ~vcPOIState() {};

  virtual vcPOIState *ChangeState(vcState * /*pProgramState*/) = 0;
  virtual void AddToScene(vcState * /*pProgramState*/, vcRenderData * /*pRenderData*/) = 0;

protected:
  vcPOI *m_pParent;
};

class vcPOIState_General : public vcPOIState
{
public:
  vcPOIState_General(vcPOI *pParent)
    : vcPOIState(pParent)
  {

  }

  ~vcPOIState_General()
  {

  }

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override
  {
    if (!m_pParent->IsVisible(pProgramState))
      return;

    if (m_pParent->m_selected)
    {
      for (int i = 0; i < m_pParent->m_line.numPoints; ++i)
      {
        m_pParent->AddNodeToRenderData(pProgramState, pRenderData, i);
      }
    }

    m_pParent->AddFenceToScene(pRenderData);
    m_pParent->AddLabelsToScene(pRenderData);
    m_pParent->AddAttachedModelsToScene(pProgramState, pRenderData);
    m_pParent->DoFlythrough(pProgramState);
  }

  vcPOIState *ChangeState(vcState *pProgramState) override;
};

class vcPOIState_Append : public vcPOIState
{
public:
  vcPOIState_Append(vcPOI *pParent)
    : vcPOIState(pParent)
  {

  }

  ~vcPOIState_Append()
  {

  }

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override
  {
    //Actually in append mode we always want it to be visible.
    //if (!m_pParent->IsVisible(pProgramState))
    //  return;

    if (m_pParent->m_selected)
    {
      for (int i = 0; i < m_pParent->m_line.numPoints; ++i)
      {
        vcRenderPolyInstance * pInstance = m_pParent->AddNodeToRenderData(pProgramState, pRenderData, i);
        pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
      }
    }

    m_pParent->AddFenceToScene(pRenderData);
    m_pParent->AddLabelsToScene(pRenderData);
  }

  vcPOIState *ChangeState(vcState *pProgramState) override;
};

vcPOIState *vcPOIState_General::ChangeState(vcState *pProgramState)
{
  if (pProgramState->activeTool == vcActiveTool_MeasureLine || pProgramState->activeTool == vcActiveTool_MeasureArea)
    return new vcPOIState_Append(m_pParent);

  return this;
}

vcPOIState *vcPOIState_Append::ChangeState(vcState *pProgramState)
{
  if (pProgramState->activeTool != vcActiveTool_MeasureLine && pProgramState->activeTool != vcActiveTool_MeasureArea)
  {
    if (m_pParent->m_pFence != nullptr)
    {
      vcFenceRenderer_ClearPoints(m_pParent->m_pFence);
      vcFenceRenderer_AddPoints(m_pParent->m_pFence, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints - 1, m_pParent->m_line.closed);
    }

    m_pParent->ChangeProjection(pProgramState->gis.zone);
    return new vcPOIState_General(m_pParent);
  }

  return this;
}

vcPOI::vcPOI(vdkProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
  , m_pState(new vcPOIState_General(this))
{
  m_nameColour = Vec4ToUINT(pProgramState->settings.tools.label.textColour);
  m_backColour = Vec4ToUINT(pProgramState->settings.tools.label.backgroundColour);
  m_namePt = vcLFS_Medium;

  m_showArea = false;
  m_showLength = false;
  m_lengthLabels.Init(32);

  memset(&m_line, 0, sizeof(m_line));

  m_hasPreviewPoint = false;
  m_cameraFollowingAttachment = false;

  m_line.selectedPoint = -1; // Sentinel for no point selected

  m_line.colourPrimary = Vec4ToUINT(pProgramState->settings.tools.line.colour);
  m_line.colourSecondary = 0xFFFFFFFF;
  m_line.lineWidth = pProgramState->settings.tools.line.width;
  m_line.closed = (m_pNode->geomtype == vdkPGT_Polygon);
  m_line.lineStyle = (vcFenceRendererImageMode)pProgramState->settings.tools.line.style;
  m_line.fenceMode = (vcFenceRendererVisualMode)pProgramState->settings.tools.line.style;

  m_pLabelText = nullptr;
  m_pFence = nullptr;
  m_pLine = nullptr;
  m_pLabelInfo = udAllocType(vcLabelInfo, 1, udAF_Zero);

  m_pWorkerPool = pProgramState->pWorkerPool;

  memset(&m_attachment, 0, sizeof(m_attachment));
  m_attachment.segmentIndex = -1;
  m_attachment.moveSpeed = 16.667; //60km/hr

  memset(&m_flyThrough, 0, sizeof(m_flyThrough));

  OnNodeUpdate(pProgramState);

  m_loadStatus = vcSLS_Loaded;
}

vcPOI::~vcPOI()
{
  delete m_pState;
}

void vcPOI::OnNodeUpdate(vcState *pProgramState)
{
  const char *pTemp;
  vdkProjectNode_GetMetadataString(m_pNode, "textSize", &pTemp, "Medium");
  if (udStrEquali(pTemp, "x-small") || udStrEquali(pTemp, "small"))
    m_namePt = vcLFS_Small;
  else if (udStrEquali(pTemp, "large") || udStrEquali(pTemp, "x-large"))
    m_namePt = vcLFS_Large;
  else
    m_namePt = vcLFS_Medium;

  vdkProjectNode_GetMetadataUint(m_pNode, "nameColour", &m_nameColour, 0xFFFFFFFF);
  vdkProjectNode_GetMetadataUint(m_pNode, "backColour", &m_backColour, 0x7F000000);
  vdkProjectNode_GetMetadataUint(m_pNode, "lineColourPrimary", &m_line.colourPrimary, 0xFFFFFFFF);
  vdkProjectNode_GetMetadataUint(m_pNode, "lineColourSecondary", &m_line.colourSecondary, 0xFFFFFFFF);

  if (vdkProjectNode_GetMetadataBool(m_pNode, "lineDualColour", &m_line.isDualColour, false) != vE_Success)
  {
    m_line.isDualColour = (m_line.colourPrimary != m_line.colourSecondary);
    vdkProjectNode_SetMetadataBool(m_pNode, "lineDualColour", m_line.isDualColour);
  }

  vdkProjectNode_GetMetadataBool(m_pNode, "showLength", &m_showLength, false);
  vdkProjectNode_GetMetadataBool(m_pNode, "showAllLengths", &m_showAllLengths, false);
  vdkProjectNode_GetMetadataBool(m_pNode, "showArea", &m_showArea, false);

  m_line.closed = (m_pNode->geomtype == vdkPGT_Polygon);

  double tempDouble;
  vdkProjectNode_GetMetadataDouble(m_pNode, "lineWidth", (double*)&tempDouble, 1.0);
  m_line.lineWidth = (float)tempDouble;

  vdkProjectNode_GetMetadataString(m_pNode, "lineStyle", &pTemp, vcFRIMStrings[0]);
  int i = 0;
  for (; i < vcRRIM_Count; ++i)
    if (udStrEquali(pTemp, vcFRIMStrings[i]))
      break;
  m_line.lineStyle = (vcFenceRendererImageMode)i;

  vdkProjectNode_GetMetadataString(m_pNode, "lineMode", &pTemp, vcFRVMStrings[0]);
  for (i = 0; i < vcRRVM_Count; ++i)
    if (udStrEquali(pTemp, vcFRVMStrings[i]))
      break;
  m_line.fenceMode = (vcFenceRendererVisualMode)i;

  if (vdkProjectNode_GetMetadataString(m_pNode, "attachmentURI", &pTemp, nullptr) == vE_Success)
  {
    if (!LoadAttachedModel(pTemp))
      LoadAttachedModel(udTempStr("%s%s", pProgramState->activeProject.pRelativeBase, pTemp));

    vdkProjectNode_GetMetadataDouble(m_pNode, "attachmentSpeed", &m_attachment.moveSpeed, 16.667); //60km/hr

    if (vdkProjectNode_GetMetadataString(m_pNode, "attachmentCulling", &pTemp, "back") == vE_Success)
    {
      if (udStrEquali(pTemp, "none"))
        m_attachment.cullMode = vcGLSCM_None;
      else if (udStrEquali(pTemp, "front"))
        m_attachment.cullMode = vcGLSCM_Front;
      else // Default to backface
        m_attachment.cullMode = vcGLSCM_Back;
    }
  }

  ChangeProjection(pProgramState->gis.zone);
  UpdatePoints();
}

bool vcPOI::GetPointAtDistanceAlongLine(double distance, udDouble3 *pPoint, int *pSegmentIndex, double *pSegmentProgress)
{
  if (m_line.numPoints < 2)
    return false;

  double totalDist = 0.0;
  double segmentProgress = (pSegmentProgress == nullptr ? 0.0 : *pSegmentProgress);

  int startPoint = 0;

  if (pSegmentIndex != nullptr)
    startPoint = udMax(0, *pSegmentIndex);

  for (int i = startPoint; (i < m_line.numPoints - 1) || m_line.closed; ++i)
  {
    int seg0 = i % m_line.numPoints;
    int seg1 = (i + 1) % m_line.numPoints;

    udDouble3 segment = m_line.pPoints[seg1] - m_line.pPoints[seg0];
    double segmentLength = udMag3(segment) - segmentProgress;
    
    if (totalDist + segmentLength > distance)
    {
      if (pPoint != nullptr)
        *pPoint = m_line.pPoints[seg0] + udNormalize(segment) * (distance - totalDist + segmentProgress);

      if (pSegmentIndex != nullptr)
        *pSegmentIndex = seg0;

      if (pSegmentProgress != nullptr)
        *pSegmentProgress = (distance - totalDist + segmentProgress);

      return true;
    }
    else
    {
      *pPoint = m_line.pPoints[seg1];
      totalDist += segmentLength;
      segmentProgress = 0.0;
    }
  }

  if (pSegmentIndex != nullptr)
    *pSegmentIndex = 0;

  if (pSegmentProgress != nullptr)
    *pSegmentProgress = 0.0;

  return false;
}

void vcPOI::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  // if POI is invisible or if it exceeds maximum visible POI distance (unless selected)
  if (!m_visible || ((udMag3(m_pLabelInfo->worldPosition - pProgramState->camera.position) > pProgramState->settings.presentation.POIFadeDistance) && !m_selected))
    return;

  bool isMeasuring = (pProgramState->activeTool == vcActiveTool_MeasureLine || pProgramState->activeTool == vcActiveTool_MeasureArea);
  bool wasPreviouslyPreviewing = m_hasPreviewPoint;
  if (m_hasPreviewPoint && (!m_selected || !isMeasuring))
    ChangeProjection(pProgramState->gis.zone);

  //Preview has ended
  if (wasPreviouslyPreviewing && !m_hasPreviewPoint)
  {
    // TODO: 1452
    if (m_line.fenceMode != vcRRVM_ScreenLine && m_pFence != nullptr)
    {
      vcFenceRenderer_ClearPoints(m_pFence);
      vcFenceRenderer_AddPoints(m_pFence, m_line.pPoints, m_line.numPoints, m_line.closed);
    }
  }

  if (m_selected)
  {
    for (int i = 0; i < m_line.numPoints; ++i)
    {
      vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();

      udDouble3 linearDistance = (pProgramState->camera.position - m_line.pPoints[i]);

      pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
      pInstance->worldMat = udDouble4x4::translation(m_line.pPoints[i]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
      pInstance->pSceneItem = this;
      pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
      pInstance->sceneItemInternalId = (uint64_t)(i+1);

      if (isMeasuring)
        pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
    }
  }

  // TODO: 1452
  if (m_line.fenceMode != vcRRVM_ScreenLine && m_pFence != nullptr)
    pRenderData->fences.PushBack(m_pFence);
  else if (m_line.fenceMode == vcRRVM_ScreenLine && m_pLine != nullptr)
    pRenderData->lines.PushBack(m_pLine);

  if (m_pLabelInfo != nullptr)
  {
    if ((m_showLength && m_line.numPoints > 1) || (m_showArea && m_line.numPoints > 2))
      m_pLabelInfo->pText = m_pLabelText;
    else
      m_pLabelInfo->pText = m_pNode->pName;

    pRenderData->labels.PushBack(m_pLabelInfo);

    if (m_showAllLengths && m_line.numPoints > 1)
    {
      for (size_t i = 0; i < m_lengthLabels.length; ++i)
      {
        if (m_line.closed || i > 0)
          pRenderData->labels.PushBack(m_lengthLabels.GetElement(i));
      }
    }
  }

  // Model attached to the camera
  if (m_attachment.pModel != nullptr)
  {
    double remainingMovementThisFrame = m_attachment.moveSpeed * pProgramState->deltaTime;
    udDouble3 startYPR = m_attachment.eulerAngles;
    udDouble3 startPosDiff = pProgramState->camera.position - m_attachment.currentPos;

    udDouble3 updatedPosition = {};

    if (remainingMovementThisFrame > 0)
    {
      if (!GetPointAtDistanceAlongLine(remainingMovementThisFrame, &updatedPosition, &m_attachment.segmentIndex, &m_attachment.segmentProgress))
      {
        if (m_line.numPoints > 1)
          m_attachment.eulerAngles = udDirectionToYPR(m_line.pPoints[1] - m_line.pPoints[0]);

        m_attachment.currentPos = m_line.pPoints[0];
      }
      else
      {
        udDouble3 targetEuler = udDirectionToYPR(updatedPosition - m_attachment.currentPos);

        m_attachment.eulerAngles = udSlerp(udDoubleQuat::create(startYPR), udDoubleQuat::create(targetEuler), 0.2).eulerAngles();
        m_attachment.currentPos = updatedPosition;
      }
    }

    udDouble4x4 attachmentMat = udDouble4x4::rotationYPR(m_attachment.eulerAngles, m_attachment.currentPos);

    // Render the attachment
    vcRenderPolyInstance *pModel = pRenderData->polyModels.PushBack();
    pModel->pModel = m_attachment.pModel;
    pModel->pSceneItem = this;
    pModel->worldMat = attachmentMat;
    pModel->cullFace = m_attachment.cullMode;

    // Update the camera if the camera is coming along
    if (pProgramState->cameraInput.pAttachedToSceneItem == this && m_cameraFollowingAttachment)
    {
      udOrientedPoint<double> rotRay = udOrientedPoint<double>::create(startPosDiff, vcGIS_HeadingPitchToQuaternion(pProgramState->gis, pProgramState->camera.position, pProgramState->camera.headingPitch));
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.z.toVector3(), m_attachment.eulerAngles.x - startYPR.x);
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.x.toVector3(), m_attachment.eulerAngles.y - startYPR.y);
      pProgramState->camera.position = m_attachment.currentPos + rotRay.position;
      pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, rotRay.orientation);
    }
  }

  // Flythrough
  if (pProgramState->cameraInput.pAttachedToSceneItem == this && !m_cameraFollowingAttachment)
  {
    if (m_line.numPoints <= 1)
    {
      pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
    }
    else
    {
      double remainingMovementThisFrame = pProgramState->settings.camera.moveSpeed * pProgramState->deltaTime;
      udDoubleQuat startQuat = vcGIS_HeadingPitchToQuaternion(pProgramState->gis, pProgramState->camera.position, pProgramState->camera.headingPitch);

      udDouble3 updatedPosition = {};

      if (!GetPointAtDistanceAlongLine(remainingMovementThisFrame, &updatedPosition, &m_flyThrough.segmentIndex, &m_flyThrough.segmentProgress))
      {
        pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, udDoubleQuat::create(udNormalize(m_line.pPoints[1] - m_line.pPoints[0]), 0.0));
        pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
      }
      else
      {
        pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, udSlerp(startQuat, udDoubleQuat::create(udDirectionToYPR(updatedPosition - pProgramState->camera.position)), 0.2));
      }

      pProgramState->camera.position = updatedPosition;
    }
  }
}

void vcPOI::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  if (m_line.selectedPoint == -1) // We need to update all the points
  {
    for (int i = 0; i < m_line.numPoints; ++i)
      m_line.pPoints[i] = (delta * udDouble4x4::translation(m_line.pPoints[i])).axis.t.toVector3();
  }
  else
  {
    m_line.pPoints[m_line.selectedPoint] = (delta * udDouble4x4::translation(m_line.pPoints[m_line.selectedPoint])).axis.t.toVector3();
  }

  UpdatePoints();

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, m_line.closed ? vdkPGT_Polygon : vdkPGT_LineString, m_line.pPoints, m_line.numPoints);
}

void vcPOI::UpdatePoints()
{
  // Calculate length, area and label position
  m_calculatedLength = 0;
  m_calculatedArea = 0;

  m_pLabelInfo->worldPosition = udDouble3::zero();

  // j = previous, i = current
  int j = udMax(0, m_line.numPoints - 1);
  for (int i = 0; i < m_line.numPoints; i++)
  {
    if (m_showArea && m_line.closed && m_line.numPoints > 2) // Area requires at least 3 points
      m_calculatedArea = m_calculatedArea + (m_line.pPoints[j].x + m_line.pPoints[i].x) * (m_line.pPoints[j].y - m_line.pPoints[i].y);

    double lineLength = udMag3(m_line.pPoints[j] - m_line.pPoints[i]);
    m_pLabelInfo->worldPosition += m_line.pPoints[i];

    if (m_line.closed || i > 0) // Calculate length
      m_calculatedLength += lineLength;

    if (m_showAllLengths && m_line.numPoints > 1)
    {
      int numLabelDiff = m_line.numPoints - (int)m_lengthLabels.length;
      if (numLabelDiff < 0) // Too many labels, delete one
      {
        vcLabelInfo popLabel = {};
        m_lengthLabels.PopBack(&popLabel);
        udFree(popLabel.pText);
      }
      else if (numLabelDiff > 0) // Not enough labels, add one
      {
        vcLabelInfo label = vcLabelInfo(*m_pLabelInfo);
        label.pText = nullptr;
        m_lengthLabels.PushBack(label);
      }

      vcLabelInfo* pLabel = m_lengthLabels.GetElement(i);
      pLabel->worldPosition = (m_line.pPoints[j] + m_line.pPoints[i]) / 2;
      udSprintf(&pLabel->pText, "%.3f", lineLength);
    }

    j = i;
  }

  m_calculatedArea = udAbs(m_calculatedArea) / 2;

  // update the fence renderer as well
  if (m_line.numPoints > 1)
  {
    if (m_showArea && m_showLength && m_line.numPoints > 2)
      udSprintf(&m_pLabelText, "%s\n%s: %.3f\n%s: %.3f", m_pNode->pName, vcString::Get("scenePOILineLength"), m_calculatedLength, vcString::Get("scenePOIArea"), m_calculatedArea);
    else if (m_showLength)
      udSprintf(&m_pLabelText, "%s\n%s: %.3f", m_pNode->pName, vcString::Get("scenePOILineLength"), m_calculatedLength);
    else if (m_showArea && m_line.numPoints > 2)
      udSprintf(&m_pLabelText, "%s\n%s: %.3f", m_pNode->pName, vcString::Get("scenePOIArea"), m_calculatedArea);

    if (m_line.fenceMode != vcRRVM_ScreenLine)
    {
      if (m_pFence == nullptr)
        vcFenceRenderer_Create(&m_pFence);

      vcFenceRendererConfig config;
      config.visualMode = m_line.fenceMode;
      config.imageMode = m_line.lineStyle;
      config.isDualColour = m_line.isDualColour;
      config.primaryColour = vcIGSW_BGRAToImGui(m_line.colourPrimary);
      config.secondaryColour = vcIGSW_BGRAToImGui(m_line.colourSecondary);
      config.ribbonWidth = m_line.lineWidth;
      config.textureScrollSpeed = 1.f;
      config.textureRepeatScale = 1.f;

      vcFenceRenderer_SetConfig(m_pFence, config);

      vcFenceRenderer_ClearPoints(m_pFence);
      vcFenceRenderer_AddPoints(m_pFence, m_line.pPoints, m_line.numPoints, m_line.closed);
    }
    else
    {
      // TODO: 1452
      if (m_pLine == nullptr)
        vcLineRenderer_CreateLine(&m_pLine);

      vcLineRenderer_UpdatePoints(m_pLine, m_line.pPoints, m_line.numPoints, vcIGSW_BGRAToImGui(m_line.colourPrimary), m_line.lineWidth);
    }
  }
  else
  {
    udFree(m_pLabelText);
    m_pLabelText = udStrdup(m_pNode->pName);
  }

  // Update the label as well
  m_pLabelInfo->pText = m_pLabelText;
  m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
  m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
  m_pLabelInfo->textSize = m_namePt;

  if (m_line.numPoints > 0)
    m_pLabelInfo->worldPosition /= m_line.numPoints;

  for (size_t i = 0; i < m_lengthLabels.length; ++i)
  {
    vcLabelInfo* pLabel = m_lengthLabels.GetElement(i);
    pLabel->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
    pLabel->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
    pLabel->textSize = m_namePt;
  }
}

void vcPOI::HandleBasicUI(vcState *pProgramState, size_t itemID)
{
  if (m_line.numPoints > 1)
  {
    if (ImGui::Checkbox(udTempStr("%s##POIShowLength%zu", vcString::Get("scenePOILineShowLength"), itemID), &m_showLength))
      vdkProjectNode_SetMetadataBool(m_pNode, "showLength", m_showLength);

    if (ImGui::Checkbox(udTempStr("%s##POIShowAllLengths%zu", vcString::Get("scenePOILineShowAllLengths"), itemID), &m_showAllLengths))
      vdkProjectNode_SetMetadataBool(m_pNode, "showAllLengths", m_showAllLengths);

    if (ImGui::Checkbox(udTempStr("%s##POIShowArea%zu", vcString::Get("scenePOILineShowArea"), itemID), &m_showArea))
      vdkProjectNode_SetMetadataBool(m_pNode, "showArea", m_showArea);

    if (ImGui::Checkbox(udTempStr("%s##POILineClosed%zu", vcString::Get("scenePOILineClosed"), itemID), &m_line.closed))
      vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, m_line.closed ? vdkPGT_Polygon : vdkPGT_LineString, m_line.pPoints, m_line.numPoints);

    if (ImGui::SliderFloat(udTempStr("%s##POILineWidth%zu", vcString::Get("scenePOILineWidth"), itemID), &m_line.lineWidth, 0.01f, 1000.f, "%.2f", 3.f))
      vdkProjectNode_SetMetadataDouble(m_pNode, "lineWidth", (double)m_line.lineWidth);

    const char *fenceOptions[] = { vcString::Get("scenePOILineOrientationScreenLine"), vcString::Get("scenePOILineOrientationVert"), vcString::Get("scenePOILineOrientationHorz") };
    if (ImGui::Combo(udTempStr("%s##POIFenceStyle%zu", vcString::Get("scenePOILineOrientation"), itemID), (int *)&m_line.fenceMode, fenceOptions, (int)udLengthOf(fenceOptions)))
      vdkProjectNode_SetMetadataString(m_pNode, "lineMode", vcFRVMStrings[m_line.fenceMode]);

    const char *lineOptions[] = { vcString::Get("scenePOILineStyleArrow"), vcString::Get("scenePOILineStyleGlow"), vcString::Get("scenePOILineStyleSolid"), vcString::Get("scenePOILineStyleDiagonal") };
    if (ImGui::Combo(udTempStr("%s##POILineColourSecondary%zu", vcString::Get("scenePOILineStyle"), itemID), (int *)&m_line.lineStyle, lineOptions, (int)udLengthOf(lineOptions)))
      vdkProjectNode_SetMetadataString(m_pNode, "lineStyle", vcFRIMStrings[m_line.lineStyle]);

    if (vcIGSW_ColorPickerU32(udTempStr("%s##POILineColourPrimary%zu", vcString::Get("scenePOILineColour1"), itemID), &m_line.colourPrimary, ImGuiColorEditFlags_None))
      vdkProjectNode_SetMetadataUint(m_pNode, "lineColourPrimary", m_line.colourPrimary);

    if (m_line.isDualColour && vcIGSW_ColorPickerU32(udTempStr("%s##POILineColourSecondary%zu", vcString::Get("scenePOILineColour2"), itemID), &m_line.colourSecondary, ImGuiColorEditFlags_None))
      vdkProjectNode_SetMetadataUint(m_pNode, "lineColourSecondary", m_line.colourSecondary);
  }
}

void vcPOI::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  HandleBasicUI(pProgramState, *pItemID);

  if (m_line.numPoints > 1)
  {
    if (ImGui::SliderInt(vcString::Get("scenePOISelectedPoint"), &m_line.selectedPoint, -1, m_line.numPoints - 1))
      m_line.selectedPoint = udClamp(m_line.selectedPoint, -1, m_line.numPoints - 1);

    if (m_line.selectedPoint != -1)
    {
      if (ImGui::InputScalarN(udTempStr("%s##POIPointPos%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_line.pPoints[m_line.selectedPoint].x, 3))
        vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, m_line.closed ? vdkPGT_Polygon : vdkPGT_LineString, m_line.pPoints, m_line.numPoints);

      if (ImGui::Button(vcString::Get("scenePOIRemovePoint")))
        RemovePoint(pProgramState, m_line.selectedPoint);
    }
  }

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIColour%zu", vcString::Get("scenePOILabelColour"), *pItemID), &m_nameColour, ImGuiColorEditFlags_None))
  {
    m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
    vdkProjectNode_SetMetadataUint(m_pNode, "nameColour", m_nameColour);
  }

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIBackColour%zu", vcString::Get("scenePOILabelBackgroundColour"), *pItemID), &m_backColour, ImGuiColorEditFlags_None))
  {
    m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
    vdkProjectNode_SetMetadataUint(m_pNode, "backColour", m_backColour);
  }

  const char *labelSizeOptions[] = { vcString::Get("scenePOILabelSizeNormal"), vcString::Get("scenePOILabelSizeSmall"), vcString::Get("scenePOILabelSizeLarge") };
  if (ImGui::Combo(udTempStr("%s##POILabelSize%zu", vcString::Get("scenePOILabelSize"), *pItemID), (int *)&m_namePt, labelSizeOptions, (int)udLengthOf(labelSizeOptions)))
  {
    UpdatePoints();
    const char *pTemp;
    m_pLabelInfo->textSize = m_namePt;
    switch (m_namePt)
    {
    case vcLFS_Small:
      pTemp = "small";
      break;
    case vcLFS_Large:
      pTemp = "large";
      break;
    case vcLFS_Medium:
    default: // Falls through
      pTemp = "medium";
      break;
    }
    vdkProjectNode_SetMetadataString(m_pNode, "textSize", pTemp);
  }

  // Handle hyperlinks
  const char *pHyperlink = m_metadata.Get("hyperlink").AsString();
  if (pHyperlink != nullptr)
  {
    ImGui::TextWrapped("%s: %s", vcString::Get("scenePOILabelHyperlink"), pHyperlink);
    if (udStrEndsWithi(pHyperlink, ".png") || udStrEndsWithi(pHyperlink, ".jpg"))
    {
      ImGui::SameLine();
      if (ImGui::Button(vcString::Get("scenePOILabelOpenHyperlink")))
        pProgramState->pLoadImage = udStrdup(pHyperlink);
    }
  }

  if (m_attachment.pModel != nullptr)
  {
    const double minSpeed = 0.0;
    const double maxSpeed = vcSL_CameraMaxAttachSpeed;

    if (ImGui::SliderScalar(vcString::Get("scenePOIAttachmentSpeed"), ImGuiDataType_Double, &m_attachment.moveSpeed, &minSpeed, &maxSpeed))
    {
      m_attachment.moveSpeed = udClamp(m_attachment.moveSpeed, minSpeed, maxSpeed);
      vdkProjectNode_SetMetadataDouble(m_pNode, "attachmentSpeed", m_attachment.moveSpeed);
    }

    const char *uiStrings[] = { vcString::Get("polyModelCullFaceBack"), vcString::Get("polyModelCullFaceFront"), vcString::Get("polyModelCullFaceNone") };
    const char *optStrings[] = { "back", "front", "none" };
    if (ImGui::Combo(udTempStr("%s##%zu", vcString::Get("polyModelCullFace"), *pItemID), (int *)&m_attachment.cullMode, uiStrings, (int)udLengthOf(uiStrings)))
      vdkProjectNode_SetMetadataString(m_pNode, "culling", optStrings[m_attachment.cullMode]);
  }
}

void vcPOI::HandleContextMenu(vcState *pProgramState)
{
  if (m_line.numPoints > 1)
  {
    ImGui::Separator();

    if (ImGui::MenuItem(vcString::Get("scenePOIPerformFlyThrough")))
    {
      pProgramState->cameraInput.pAttachedToSceneItem = this;
      m_cameraFollowingAttachment = false;
    }

    if (m_attachment.pModel != nullptr && ImGui::MenuItem(vcString::Get("scenePOIAttachCameraToAttachment")))
    {
      pProgramState->cameraInput.pAttachedToSceneItem = this;
      m_cameraFollowingAttachment = true;
    }

    if (ImGui::BeginMenu(vcString::Get("scenePOIAttachModel")))
    {
      static char uriBuffer[1024];
      static const char *pErrorBuffer;

      if (ImGui::IsWindowAppearing())
        pErrorBuffer = nullptr;

      vcIGSW_FilePicker(pProgramState, vcString::Get("scenePOIAttachModelURI"), uriBuffer, SupportedFileTypes_PolygonModels, vcFDT_OpenFile, nullptr);

      if (ImGui::Button(vcString::Get("scenePOIAttachModel")))
      {
        bool result = LoadAttachedModel(uriBuffer);

        if (!result)
          result = LoadAttachedModel(udTempStr("%s%s", pProgramState->activeProject.pRelativeBase, uriBuffer));

        if (result)
        {
          vdkProjectNode_SetMetadataString(m_pNode, "attachmentURI", uriBuffer);
          ImGui::CloseCurrentPopup();
        }
        else
        {
          pErrorBuffer = vcString::Get("scenePOIAttachModelFailed");
        }
      }

      if (pErrorBuffer != nullptr)
      {
        ImGui::SameLine();
        ImGui::TextUnformatted(pErrorBuffer);
      }

      ImGui::EndMenu();
    }
  }
}

void vcPOI::HandleAttachmentUI(vcState * /*pProgramState*/)
{
  if (m_cameraFollowingAttachment)
  {
    const double minSpeed = 0.0;
    const double maxSpeed = vcSL_CameraMaxAttachSpeed;

    if (ImGui::SliderScalar(vcString::Get("scenePOIAttachmentSpeed"), ImGuiDataType_Double, &m_attachment.moveSpeed, &minSpeed, &maxSpeed))
    {
      m_attachment.moveSpeed = udClamp(m_attachment.moveSpeed, minSpeed, maxSpeed);
      vdkProjectNode_SetMetadataDouble(m_pNode, "attachmentSpeed", m_attachment.moveSpeed);
    }
  }
  else
  {
    // Settings?
  }
}

void vcPOI::HandleToolUI(vcState *pProgramState)
{
  HandleBasicUI(pProgramState, size_t(-1));
}

void vcPOI::AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview)
{
  bool addingPoint = !m_hasPreviewPoint;
  m_hasPreviewPoint = isPreview;

  int totalPoints = m_line.numPoints + (addingPoint ? 1 : 0);

  udDouble3 *pNewPoints = udAllocType(udDouble3, totalPoints, udAF_Zero);

  memcpy(pNewPoints, m_line.pPoints, sizeof(udDouble3) * m_line.numPoints);
  pNewPoints[addingPoint ? m_line.numPoints : m_line.numPoints - 1] = position;

  udFree(m_line.pPoints);
  m_line.pPoints = pNewPoints;

  if (addingPoint)
    ++m_line.numPoints;

  UpdatePoints();

  if (!isPreview)
  {
    vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, m_line.closed ? vdkPGT_Polygon : vdkPGT_LineString, m_line.pPoints, m_line.numPoints);
    m_line.selectedPoint = m_line.numPoints - 1;
  }
}

void vcPOI::RemovePoint(vcState *pProgramState, int index)
{
  if (index < (m_line.numPoints - 1))
    memmove(m_line.pPoints + index, m_line.pPoints + index + 1, sizeof(udDouble3) * (m_line.numPoints - index - 1));

  --m_line.numPoints;

  UpdatePoints();
  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->gis.zone, m_line.closed ? vdkPGT_Polygon : vdkPGT_LineString, m_line.pPoints, m_line.numPoints);
}

void vcPOI::ChangeProjection(const udGeoZone &newZone)
{
  m_hasPreviewPoint = false;
  udFree(m_line.pPoints);
  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &m_line.pPoints, &m_line.numPoints);
  UpdatePoints();
}

void vcPOI::Cleanup(vcState *pProgramState)
{
  udFree(m_line.pPoints);
  udFree(m_pLabelText);
  for (size_t i = 0; i < m_lengthLabels.length; ++i)
    udFree(m_lengthLabels.GetElement(i)->pText);

  udFree(m_attachment.pPathLoaded);
  vcPolygonModel_Destroy(&m_attachment.pModel);

  m_lengthLabels.Deinit();
  vcFenceRenderer_Destroy(&m_pFence);
  vcLineRenderer_DestroyLine(&m_pLine);
  udFree(m_pLabelInfo);

  if (pProgramState->cameraInput.pAttachedToSceneItem == this)
    pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
}

void vcPOI::SetCameraPosition(vcState *pProgramState)
{
  if (m_attachment.pModel)
    pProgramState->camera.position = m_attachment.currentPos;
  else
    pProgramState->camera.position = m_pLabelInfo->worldPosition;
}

udDouble4x4 vcPOI::GetWorldSpaceMatrix()
{
  if (m_line.selectedPoint == -1)
    return udDouble4x4::translation(m_pLabelInfo->worldPosition);
  else
    return udDouble4x4::translation(m_line.pPoints[m_line.selectedPoint]);
}

void vcPOI::SelectSubitem(uint64_t internalId)
{
  m_line.selectedPoint = ((int)internalId) - 1;
}

bool vcPOI::IsSubitemSelected(uint64_t internalId)
{
  return (m_selected && (m_line.selectedPoint == ((int)internalId - 1) || m_line.selectedPoint == -1));
}

vcRenderPolyInstance *vcPOI::AddNodeToRenderData(vcState *pProgramState, vcRenderData *pRenderData, size_t i)
{
  vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();

  udDouble3 linearDistance = (pProgramState->camera.position - m_line.pPoints[i]);

  pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
  pInstance->worldMat = udDouble4x4::translation(m_line.pPoints[i]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
  pInstance->pSceneItem = this;
  pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
  pInstance->sceneItemInternalId = (uint64_t)i + 1;
  return pInstance;
}

void vcPOI::UpdateState(vcState *pProgramState)
{
  vcPOIState *pNewState = m_pState->ChangeState(pProgramState);
  if (pNewState != m_pState)
    delete m_pState;
  m_pState = pNewState;
}

bool vcPOI::LoadAttachedModel(const char *pNewPath)
{
  if (pNewPath == nullptr)
    return false;

  if (udStrEqual(m_attachment.pPathLoaded, pNewPath))
    return true;

  vcPolygonModel_Destroy(&m_attachment.pModel);
  udFree(m_attachment.pPathLoaded);

  if (vcPolygonModel_CreateFromURL(&m_attachment.pModel, pNewPath, m_pWorkerPool) == udR_Success)
  {
    m_attachment.pPathLoaded = udStrdup(pNewPath);
    return true;
  }

  return false;
}

bool vcPOI::IsVisible(vcState *pProgramState)
{
  // if POI is invisible or if it exceeds maximum visible POI distance (unless selected)
  bool visible = m_visible;
  visible = visible && m_selected;
  visible = visible && (udMag3(m_pLabelInfo->worldPosition - pProgramState->camera.position) < pProgramState->settings.presentation.POIFadeDistance);
  return visible;
}

void vcPOI::AddFenceToScene(vcRenderData *pRenderData)
{
  if (m_pFence != nullptr)
    pRenderData->fences.PushBack(m_pFence);
}

void vcPOI::AddLabelsToScene(vcRenderData *pRenderData)
{
  if (m_pLabelInfo != nullptr)
  {
    if ((m_showLength && m_line.numPoints > 1) || (m_showArea && m_line.numPoints > 2))
      m_pLabelInfo->pText = m_pLabelText;
    else
      m_pLabelInfo->pText = m_pNode->pName;

    pRenderData->labels.PushBack(m_pLabelInfo);

    if (m_showAllLengths && m_line.numPoints > 1)
    {
      for (size_t i = 0; i < m_lengthLabels.length; ++i)
      {
        if (m_line.closed || i > 0)
          pRenderData->labels.PushBack(m_lengthLabels.GetElement(i));
      }
    }
  }
}

void vcPOI::AddAttachedModelsToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (m_attachment.pModel != nullptr)
  {
    double remainingMovementThisFrame = m_attachment.moveSpeed * pProgramState->deltaTime;
    udDouble3 startYPR = m_attachment.eulerAngles;
    udDouble3 startPosDiff = pProgramState->camera.position - m_attachment.currentPos;

    udDouble3 updatedPosition = {};

    if (remainingMovementThisFrame > 0)
    {
      if (!GetPointAtDistanceAlongLine(remainingMovementThisFrame, &updatedPosition, &m_attachment.segmentIndex, &m_attachment.segmentProgress))
      {
        if (m_line.numPoints > 1)
          m_attachment.eulerAngles = udDirectionToYPR(m_line.pPoints[1] - m_line.pPoints[0]);

        m_attachment.currentPos = m_line.pPoints[0];
      }
      else
      {
        udDouble3 targetEuler = udDirectionToYPR(updatedPosition - m_attachment.currentPos);

        m_attachment.eulerAngles = udSlerp(udDoubleQuat::create(startYPR), udDoubleQuat::create(targetEuler), 0.2).eulerAngles();
        m_attachment.currentPos = updatedPosition;
      }
    }

    udDouble4x4 attachmentMat = udDouble4x4::rotationYPR(m_attachment.eulerAngles, m_attachment.currentPos);

    // Render the attachment
    vcRenderPolyInstance *pModel = pRenderData->polyModels.PushBack();
    pModel->pModel = m_attachment.pModel;
    pModel->pSceneItem = this;
    pModel->worldMat = attachmentMat;
    pModel->cullFace = m_attachment.cullMode;

    // Update the camera if the camera is coming along
    if (pProgramState->cameraInput.pAttachedToSceneItem == this && m_cameraFollowingAttachment)
    {
      udOrientedPoint<double> rotRay = udOrientedPoint<double>::create(startPosDiff, vcGIS_HeadingPitchToQuaternion(pProgramState->gis, pProgramState->camera.position, pProgramState->camera.headingPitch));
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.z.toVector3(), m_attachment.eulerAngles.x - startYPR.x);
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.x.toVector3(), m_attachment.eulerAngles.y - startYPR.y);
      pProgramState->camera.position = m_attachment.currentPos + rotRay.position;
      pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, rotRay.orientation);
    }
  }
}

void vcPOI::DoFlythrough(vcState *pProgramState)
{
  if (pProgramState->cameraInput.pAttachedToSceneItem == this && !m_cameraFollowingAttachment)
  {
    if (m_line.numPoints <= 1)
    {
      pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
    }
    else
    {
      double remainingMovementThisFrame = pProgramState->settings.camera.moveSpeed * pProgramState->deltaTime;
      udDoubleQuat startQuat = vcGIS_HeadingPitchToQuaternion(pProgramState->gis, pProgramState->camera.position, pProgramState->camera.headingPitch);

      udDouble3 updatedPosition = {};

      if (!GetPointAtDistanceAlongLine(remainingMovementThisFrame, &updatedPosition, &m_flyThrough.segmentIndex, &m_flyThrough.segmentProgress))
      {
        pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, udDoubleQuat::create(udNormalize(m_line.pPoints[1] - m_line.pPoints[0]), 0.0));
        pProgramState->cameraInput.pAttachedToSceneItem = nullptr;
      }
      else
      {
        pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->gis, pProgramState->camera.position, udSlerp(startQuat, udDoubleQuat::create(udDirectionToYPR(updatedPosition - pProgramState->camera.position)), 0.2));
      }

      pProgramState->camera.position = updatedPosition;
    }
  }
}
