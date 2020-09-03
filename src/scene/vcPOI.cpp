#include "vcPOI.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcFenceRenderer.h"
#include "vcInternalModels.h"
#include "vcWebFile.h"
#include "vcUnitConversion.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"
#include "vcCDT.h"

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

//----------------------------------------------------------------------------------------------------
// vcPOIState_General
//----------------------------------------------------------------------------------------------------

class vcPOIState_General
{
protected:
  vcPOI *m_pParent;
public:
  vcPOIState_General(vcPOI *pParent)
    : m_pParent(pParent)
  {

  }

  virtual ~vcPOIState_General()
  {

  }

  virtual udProjectGeometryType GetGeometryType() const
  {
    return m_pParent->m_line.closed ? udPGT_Polygon : udPGT_LineString;
  }

  virtual void HandleBasicUI(vcState *pProgramState, size_t itemID)
  {
    if (m_pParent->m_line.numPoints > 1)
    {
      if (ImGui::Checkbox(udTempStr("%s##POIShowLength%zu", vcString::Get("scenePOILineShowLength"), itemID), &m_pParent->m_showLength))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showLength", m_pParent->m_showLength);

      if (ImGui::Checkbox(udTempStr("%s##POIShowAllLengths%zu", vcString::Get("scenePOILineShowAllLengths"), itemID), &m_pParent->m_showAllLengths))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showAllLengths", m_pParent->m_showAllLengths);

      if (ImGui::Checkbox(udTempStr("%s##POIShowArea%zu", vcString::Get("scenePOILineShowArea"), itemID), &m_pParent->m_showArea))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showArea", m_pParent->m_showArea);

      if (ImGui::Checkbox(udTempStr("%s##POILineClosed%zu", vcString::Get("scenePOILineClosed"), itemID), &m_pParent->m_line.closed))
        vcProject_UpdateNodeGeometryFromCartesian(m_pParent->m_pProject, m_pParent->m_pNode, pProgramState->geozone, GetGeometryType(), m_pParent->m_line.pPoints, m_pParent->m_line.numPoints);

      if (GetGeometryType() == udPGT_Polygon)
      {
        if (ImGui::Checkbox(udTempStr("%s##POIShowFill%zu", vcString::Get("scenePOILineShowFill"), itemID), &m_pParent->m_showFill))
          udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showFill", m_pParent->m_showFill);
      }      

      if (ImGui::SliderFloat(udTempStr("%s##POILineWidth%zu", vcString::Get("scenePOILineWidth"), itemID), &m_pParent->m_line.lineWidth, 0.01f, 1000.f, "%.2f", 3.f))
        udProjectNode_SetMetadataDouble(m_pParent->m_pNode, "lineWidth", (double)m_pParent->m_line.lineWidth);

      const char *fenceOptions[] = { vcString::Get("scenePOILineOrientationScreenLine"), vcString::Get("scenePOILineOrientationVert"), vcString::Get("scenePOILineOrientationHorz") };
      if (ImGui::Combo(udTempStr("%s##POIFenceStyle%zu", vcString::Get("scenePOILineOrientation"), itemID), (int *)&m_pParent->m_line.fenceMode, fenceOptions, (int)udLengthOf(fenceOptions)))
        udProjectNode_SetMetadataString(m_pParent->m_pNode, "lineMode", vcFRVMStrings[m_pParent->m_line.fenceMode]);

      if (m_pParent->m_line.fenceMode != vcRRVM_ScreenLine)
      {
        const char *lineOptions[] = { vcString::Get("scenePOILineStyleArrow"), vcString::Get("scenePOILineStyleGlow"), vcString::Get("scenePOILineStyleSolid"), vcString::Get("scenePOILineStyleDiagonal") };
        if (ImGui::Combo(udTempStr("%s##POILineColourSecondary%zu", vcString::Get("scenePOILineStyle"), itemID), (int *)&m_pParent->m_line.lineStyle, lineOptions, (int)udLengthOf(lineOptions)))
          udProjectNode_SetMetadataString(m_pParent->m_pNode, "lineStyle", vcFRIMStrings[m_pParent->m_line.lineStyle]);
      }

      if (vcIGSW_ColorPickerU32(udTempStr("%s##POILineColourPrimary%zu", vcString::Get("scenePOILineColour1"), itemID), &m_pParent->m_line.colourPrimary, ImGuiColorEditFlags_None))
        udProjectNode_SetMetadataUint(m_pParent->m_pNode, "lineColourPrimary", m_pParent->m_line.colourPrimary);

      if (m_pParent->m_line.isDualColour && vcIGSW_ColorPickerU32(udTempStr("%s##POILineColourSecondary%zu", vcString::Get("scenePOILineColour2"), itemID), &m_pParent->m_line.colourSecondary, ImGuiColorEditFlags_None))
        udProjectNode_SetMetadataUint(m_pParent->m_pNode, "lineColourSecondary", m_pParent->m_line.colourSecondary);

      if (GetGeometryType() == udPGT_Polygon)
      {
        if (vcIGSW_ColorPickerU32(udTempStr("%s##POIMeasurementAreaFillColour%zu", vcString::Get("scenePOIFillColour"), itemID), &m_pParent->m_measurementAreaFillColour, ImGuiColorEditFlags_None))
          udProjectNode_SetMetadataUint(m_pParent->m_pNode, "measurementAreaFillColour", m_pParent->m_measurementAreaFillColour);
      }
    }
  }

  virtual void HandlePopupUI(vcState * /*pProgramState*/)
  {

  }

  virtual void AddPoint(vcState * pProgramState, const udDouble3 &position, bool /*isPreview*/)
  {
    m_pParent->InsertPoint(position);

    m_pParent->UpdatePoints(pProgramState);

    vcProject_UpdateNodeGeometryFromCartesian( m_pParent->m_pProject,  m_pParent->m_pNode, pProgramState->geozone,  m_pParent->m_line.closed ? udPGT_Polygon : udPGT_LineString,  m_pParent->m_line.pPoints,  m_pParent->m_line.numPoints);
    m_pParent->m_line.selectedPoint =  m_pParent->m_line.numPoints - 1;
  }

  virtual void AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
  {
    if (!m_pParent->IsVisible(pProgramState))
      return;

    if (m_pParent->m_selected)
    {
      for (int i = 0; i < m_pParent->m_line.numPoints; ++i)
      {
        vcRenderPolyInstance *pInstance = m_pParent->AddNodeToRenderData(pProgramState, pRenderData, i);
        pInstance->selectable = true;
        pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.85f);
      }
    }

    if (GetGeometryType() == udPGT_Polygon && m_pParent->m_pPolyModel == nullptr)
      m_pParent->GenerateLineFillPolygon(pProgramState);

    if (m_pParent->m_pPolyModel != nullptr && m_pParent->m_showFill)
    {
      if (GetGeometryType() == udPGT_LineString)
        m_pParent->m_showFill = false;

      m_pParent->AddFillPolygonToScene(pProgramState, pRenderData);
    }

    m_pParent->AddFenceToScene(pRenderData);
    m_pParent->RebuildSceneLabel(&pProgramState->settings.unitConversionData);
    m_pParent->AddLabelsToScene(pRenderData, &pProgramState->settings.unitConversionData);
    
    m_pParent->AddAttachedModelsToScene(pProgramState, pRenderData);
    m_pParent->DoFlythrough(pProgramState);
  }

  virtual vcPOIState_General *ChangeState(vcState *pProgramState);
};

//----------------------------------------------------------------------------------------------------
// vcPOIState_Annotate
//----------------------------------------------------------------------------------------------------

class vcPOIState_Annotate : public vcPOIState_General
{
public:
  vcPOIState_Annotate(vcPOI *pParent)
    : vcPOIState_General(pParent)
  {

  }

  ~vcPOIState_Annotate()
  {

  }

  udProjectGeometryType GetGeometryType() const override
  {
    return udPGT_Point;
  }

  void HandlePopupUI(vcState * /*pProgramState*/) override
  {
    size_t const bufSize = 64;
    char buf[bufSize] = {"A label"};
    if (ImGui::InputText("A label", buf, bufSize))
    {

      udProjectNode_SetMetadataString(m_pParent->m_pNode, "name", buf);
    }
  }

  void AddPoint(vcState * /*pProgramState*/, const udDouble3 & /*position*/, bool /*isPreview*/) override
  {
    //Disallow adding more points
  }

  void AddToScene(vcState *pProgramState, vcRenderData *pRenderData) override
  {
    if (!m_pParent->IsVisible(pProgramState))
      return;

    if (m_pParent->m_selected)
    {
      m_pParent->AddNodeToRenderData(pProgramState, pRenderData, 0);
    }

    m_pParent->RebuildSceneLabel(&pProgramState->settings.unitConversionData);
    m_pParent->AddLabelsToScene(pRenderData, &pProgramState->settings.unitConversionData);
  }

  vcPOIState_General *ChangeState(vcState *pProgramState) override;
};

//----------------------------------------------------------------------------------------------------
// vcPOIState_MeasureLine
//----------------------------------------------------------------------------------------------------


class vcPOIState_MeasureLine : public vcPOIState_General
{
public:
  vcPOIState_MeasureLine(vcPOI *pParent)
    : vcPOIState_General(pParent)
  {
    m_pParent->m_showArea = false;
    m_pParent->m_showAllLengths = true;
    m_pParent->m_showLength = true;
    m_pParent->m_showFill = false;
  }

  ~vcPOIState_MeasureLine()
  {

  }

  udProjectGeometryType GetGeometryType() const override
  {
    return m_pParent->m_line.closed ? udPGT_Polygon : udPGT_LineString;
  }

  void HandlePopupUI(vcState *pProgramState) override
  {
    size_t itemID = size_t(-1);
    if (m_pParent->m_line.numPoints > 1)
    {
      if (ImGui::Checkbox(udTempStr("%s##POIShowLength%zu", vcString::Get("scenePOILineShowLength"), itemID), &m_pParent->m_showLength))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showLength", m_pParent->m_showLength);

      if (ImGui::Checkbox(udTempStr("%s##POIShowAllLengths%zu", vcString::Get("scenePOILineShowAllLengths"), itemID), &m_pParent->m_showAllLengths))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showAllLengths", m_pParent->m_showAllLengths);

      if (ImGui::Checkbox(udTempStr("%s##POILineClosed%zu", vcString::Get("scenePOICloseAndExit"), itemID), &m_pParent->m_line.closed))
      {
        vcProject_UpdateNodeGeometryFromCartesian(m_pParent->m_pProject, m_pParent->m_pNode, pProgramState->geozone, udPGT_LineString, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints);
        pProgramState->activeTool = vcActiveTool_Select;
      }
    }
  }

  //This and AddToScene() are essentially duplicated in vcPOIState_MeasureArea, but let's keep it this way for now.
  //When these tools grow, we'll no doubt look different.
  void AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview) override
  {
    //For new nodes, we need to add a preview point
    if (m_pParent->m_line.numPoints == 1)
      m_pParent->InsertPoint(position);

    if (isPreview)
      m_pParent->m_line.pPoints[m_pParent->m_line.numPoints - 1] = position;
    else
      m_pParent->InsertPoint(position);

    m_pParent->UpdatePoints(pProgramState);

    if (!isPreview)
    {
      vcProject_UpdateNodeGeometryFromCartesian(m_pParent->m_pProject, m_pParent->m_pNode, pProgramState->geozone, udPGT_LineString, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints);
      m_pParent->m_line.selectedPoint = m_pParent->m_line.numPoints - 1;
    }
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
    m_pParent->RebuildSceneLabel(&pProgramState->settings.unitConversionData);
    m_pParent->AddLabelsToScene(pRenderData, &pProgramState->settings.unitConversionData);
  }

  vcPOIState_General *ChangeState(vcState *pProgramState) override;
};

//----------------------------------------------------------------------------------------------------
// vcPOIState_MeasureArea
//----------------------------------------------------------------------------------------------------

class vcPOIState_MeasureArea : public vcPOIState_General
{
public:
  vcPOIState_MeasureArea(vcPOI *pParent)
    : vcPOIState_General(pParent)
  {
    m_pParent->m_showArea = true;
    m_pParent->m_showAllLengths = false;
    m_pParent->m_showLength = false;
    m_pParent->m_showFill = true;
  }

  ~vcPOIState_MeasureArea()
  {

  }

  udProjectGeometryType GetGeometryType() const override
  {
    return udPGT_Polygon;
  }

  void HandlePopupUI(vcState * /*pProgramState*/) override
  {
    size_t itemID = size_t(-1);
    if (m_pParent->m_line.numPoints > 1)
    {
      if (ImGui::Checkbox(udTempStr("%s##POIShowLength%zu", vcString::Get("scenePOILineShowLength"), itemID), &m_pParent->m_showLength))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showLength", m_pParent->m_showLength);

      if (ImGui::Checkbox(udTempStr("%s##POIShowAllLengths%zu", vcString::Get("scenePOILineShowAllLengths"), itemID), &m_pParent->m_showAllLengths))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showAllLengths", m_pParent->m_showAllLengths);

      if (ImGui::Checkbox(udTempStr("%s##POIShowArea%zu", vcString::Get("scenePOILineShowArea"), itemID), &m_pParent->m_showArea))
        udProjectNode_SetMetadataBool(m_pParent->m_pNode, "showArea", m_pParent->m_showArea);
    }
  }

  void AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview) override
  {
    //For new nodes, we need to add a preview point
    if (m_pParent->m_line.numPoints == 1)
      m_pParent->InsertPoint(position);

    if (isPreview)
      m_pParent->m_line.pPoints[m_pParent->m_line.numPoints - 1] = position;
    else
      m_pParent->InsertPoint(position);

    m_pParent->UpdatePoints(pProgramState);

    if (!isPreview)
    {
      vcProject_UpdateNodeGeometryFromCartesian(m_pParent->m_pProject, m_pParent->m_pNode, pProgramState->geozone, udPGT_Polygon, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints);
      m_pParent->m_line.selectedPoint = m_pParent->m_line.numPoints - 1;
    }
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
    m_pParent->RebuildSceneLabel(&pProgramState->settings.unitConversionData);
    m_pParent->AddLabelsToScene(pRenderData, &pProgramState->settings.unitConversionData);

    if (m_pParent->m_showFill)
    {
      m_pParent->GenerateLineFillPolygon(pProgramState);
      m_pParent->AddFillPolygonToScene(pProgramState, pRenderData);
    }
  }

  vcPOIState_General *ChangeState(vcState *pProgramState) override;
};

//----------------------------------------------------------------------------------------------------
// State changes
//----------------------------------------------------------------------------------------------------

//A general POI set cannot be promoted to a measuring tool
vcPOIState_General *vcPOIState_General::ChangeState(vcState * /*pProgramState*/)
{
  return this;
}

//An Annotate cannot change state!
vcPOIState_General *vcPOIState_Annotate::ChangeState(vcState * /*pProgramState*/)
{
  return this;
}

//We still have to query a lot of state for these state changes, but at least now it is localised in one place.
vcPOIState_General *vcPOIState_MeasureLine::ChangeState(vcState *pProgramState)
{
  bool shouldChange = pProgramState->activeTool != vcActiveTool_MeasureLine;

  //TODO Should we allow measure mode to act on a closed polygon?
  shouldChange = shouldChange || m_pParent->m_line.closed;

  if (shouldChange)
  {
    //We need to notify the program state that we have stopped measuring.
    //Ideally we shouldn't be changing program state here; data should flow in one direction (into this object for example).
    //But this would require other changes to the program framework.
    if (pProgramState->activeTool == vcActiveTool_MeasureLine)
      pProgramState->activeTool = vcActiveTool_Select;

    //We need to remove the preview point
    if (m_pParent->m_line.numPoints > 0)
      m_pParent->RemovePoint(pProgramState, m_pParent->m_line.numPoints - 1);

    // TODO: 1452
    if (m_pParent->m_line.fenceMode != vcRRVM_ScreenLine && m_pParent->m_pFence != nullptr)
    {
      vcFenceRenderer_ClearPoints(m_pParent->m_pFence);
      vcFenceRenderer_AddPoints(m_pParent->m_pFence, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints - 1, m_pParent->m_line.closed);
    }

    m_pParent->ChangeProjection(pProgramState->geozone);
    m_pParent->UpdatePoints(pProgramState);
    return new vcPOIState_General(m_pParent);
  }

  return this;
}

//TODO This is very similar to vcPOIState_MeasureLine::ChangeState(). Consider refactoring.
vcPOIState_General *vcPOIState_MeasureArea::ChangeState(vcState *pProgramState)
{
  bool shouldChange = pProgramState->activeTool != vcActiveTool_MeasureArea;

  if (shouldChange)
  {
    if (pProgramState->activeTool == vcActiveTool_MeasureArea)
      pProgramState->activeTool = vcActiveTool_Select;

    //We need to remove the preview point
    if (m_pParent->m_line.numPoints > 0)
      m_pParent->RemovePoint(pProgramState, m_pParent->m_line.numPoints - 1);

    // TODO: 1452
    if (m_pParent->m_line.fenceMode != vcRRVM_ScreenLine && m_pParent->m_pFence != nullptr)
    {
      vcFenceRenderer_ClearPoints(m_pParent->m_pFence);
      vcFenceRenderer_AddPoints(m_pParent->m_pFence, m_pParent->m_line.pPoints, m_pParent->m_line.numPoints - 1, m_pParent->m_line.closed);
    }

    m_pParent->ChangeProjection(pProgramState->geozone);
    m_pParent->UpdatePoints(pProgramState);
    return new vcPOIState_General(m_pParent);
  }

  return this;
}

//----------------------------------------------------------------------------------------------------
// vcPOI
//----------------------------------------------------------------------------------------------------

vcPOI::vcPOI(vcProject *pProject, udProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState)
  , m_pState(nullptr)
{
  m_pProgramState = pProgramState;
  m_nameColour =  0xFFFFFFFF;
  m_backColour = 0x7F000000;

  m_showArea = false;
  m_showLength = false;
  m_lengthLabels.Init(32);

  memset(&m_line, 0, sizeof(m_line));

  m_cameraFollowingAttachment = false;

  m_line.selectedPoint = -1; // Sentinel for no point selected

  m_line.colourPrimary = 0xFF000000;
  m_line.colourSecondary = 0xFFFFFFFF;
  m_line.lineWidth = 1.0;
  m_line.closed = (m_pNode->geomtype == udPGT_Polygon);
  m_line.lineStyle = vcRRIM_Arrow;
  m_line.fenceMode = vcRRVM_Fence;

  m_pLabelText = nullptr;
  m_pPolyModel = nullptr;
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
  if (m_pState == nullptr)
  {
    switch (pProgramState->activeTool)
    {
    case vcActiveTool_MeasureLine:
    {
      m_pState = new vcPOIState_MeasureLine(this);
      break;
    }
    case vcActiveTool_MeasureArea:
    {
      m_pState = new vcPOIState_MeasureArea(this);
      break;
    }
    case vcActiveTool_Annotate:
    {
      m_pState = new vcPOIState_Annotate(this);
      break;
    }
    default:
    {
      m_pState = new vcPOIState_General(this);
    }
    }
  }

  const char *pTemp;
  udProjectNode_GetMetadataUint(m_pNode, "nameColour", &m_nameColour, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.textColour));
  udProjectNode_GetMetadataUint(m_pNode, "backColour", &m_backColour, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.label.backgroundColour));
  udProjectNode_GetMetadataUint(m_pNode, "lineColourPrimary", &m_line.colourPrimary, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.line.colour));
  udProjectNode_GetMetadataUint(m_pNode, "lineColourSecondary", &m_line.colourSecondary, 0xFFFFFFFF);
  udProjectNode_GetMetadataUint(m_pNode, "measurementAreaFillColour", &m_measurementAreaFillColour, vcIGSW_ImGuiToBGRA(pProgramState->settings.tools.fill.colour));
  udProjectNode_GetMetadataString(m_pNode, "hyperlink", &pTemp, "");
  udStrcpy(m_hyperlink, pTemp);

  udProjectNode_GetMetadataString(m_pNode, "description", &pTemp, "");
  udStrcpy(m_description, pTemp);

  if (vcProject_GetNodeMetadata(m_pNode, "lineDualColour", &m_line.isDualColour, false) != udE_Success)
  {
    m_line.isDualColour = (m_line.colourPrimary != m_line.colourSecondary);
    udProjectNode_SetMetadataBool(m_pNode, "lineDualColour", m_line.isDualColour);
  }

  vcProject_GetNodeMetadata(m_pNode, "showLength", &m_showLength, false);
  vcProject_GetNodeMetadata(m_pNode, "showAllLengths", &m_showAllLengths, false);
  vcProject_GetNodeMetadata(m_pNode, "showArea", &m_showArea, false);
  vcProject_GetNodeMetadata(m_pNode, "showFill", &m_showFill, false);

  m_line.closed = (m_pState->GetGeometryType() == udPGT_Polygon);

  double tempDouble;
  udProjectNode_GetMetadataDouble(m_pNode, "lineWidth", (double*)&tempDouble, pProgramState->settings.tools.line.width);
  m_line.lineWidth = (float)tempDouble;

  udProjectNode_GetMetadataString(m_pNode, "lineStyle", &pTemp, vcFRIMStrings[pProgramState->settings.tools.line.style]);
  int i = 0;
  for (; i < vcRRIM_Count; ++i)
    if (udStrEquali(pTemp, vcFRIMStrings[i]))
      break;
  m_line.lineStyle = (vcFenceRendererImageMode)i;

  udProjectNode_GetMetadataString(m_pNode, "lineMode", &pTemp, vcFRVMStrings[pProgramState->settings.tools.line.fenceMode]);
  for (i = 0; i < vcRRVM_Count; ++i)
    if (udStrEquali(pTemp, vcFRVMStrings[i]))
      break;
  m_line.fenceMode = (vcFenceRendererVisualMode)i;

  if (udProjectNode_GetMetadataString(m_pNode, "attachmentURI", &pTemp, nullptr) == udE_Success)
  {
    if (!LoadAttachedModel(pTemp))
      LoadAttachedModel(udTempStr("%s%s", pProgramState->activeProject.pRelativeBase, pTemp));

    udProjectNode_GetMetadataDouble(m_pNode, "attachmentSpeed", &m_attachment.moveSpeed, 16.667); //60km/hr

    if (udProjectNode_GetMetadataString(m_pNode, "attachmentCulling", &pTemp, "back") == udE_Success)
    {
      if (udStrEquali(pTemp, "none"))
        m_attachment.cullMode = vcGLSCM_None;
      else if (udStrEquali(pTemp, "front"))
        m_attachment.cullMode = vcGLSCM_Front;
      else // Default to backface
        m_attachment.cullMode = vcGLSCM_Back;
    }
  }

  UpdateState(pProgramState);
  ChangeProjection(pProgramState->geozone);
  UpdatePoints(pProgramState);
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
  UpdateState(pProgramState);
  m_pState->AddToScene(pProgramState, pRenderData);
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

  UpdatePoints(pProgramState);

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, m_pState->GetGeometryType(), m_line.pPoints, m_line.numPoints);
}

void vcPOI::RebuildSceneLabel(const vcUnitConversionData *pConversionData)
{
  char labelBuf[128] = {};
  char tempBuf[128] = {};
  udStrcat(labelBuf, m_pNode->pName);

  if (m_showLength)
  {
    udStrcat(labelBuf, "\n");
    vcUnitConversion_ConvertAndFormatDistance(tempBuf, 128, m_totalLength, vcDistance_Metres, pConversionData);
    udStrcat(labelBuf, tempBuf);
  }
  if (m_showArea)
  {
    udStrcat(labelBuf, "\n");
    vcUnitConversion_ConvertAndFormatArea(tempBuf, 128, m_area, vcArea_SquareMetres, pConversionData);
    udStrcat(labelBuf, tempBuf);
  }

  udFree(m_pLabelText);
  m_pLabelText = udStrdup(labelBuf);
}

void vcPOI::UpdatePoints(vcState *pProgramState)
{
  CalculateCentroid();

  udDouble3 worldUp = vcGIS_GetWorldLocalUp(pProgramState->geozone, m_centroid);
  CalculateArea(udDouble4::create(worldUp, 0.0));
  CalculateTotalLength();

  m_pLabelInfo->worldPosition = m_centroid;

  RebuildSceneLabel(&pProgramState->settings.unitConversionData);

  // update the fence renderer as well
  if (m_line.numPoints > 1)
  {
    if (m_line.fenceMode != vcRRVM_ScreenLine)
    {
      if (m_pFence == nullptr)
        vcFenceRenderer_Create(&m_pFence, pProgramState->pWorkerPool);

      vcFenceRendererConfig config;
      config.visualMode = m_line.fenceMode;
      config.imageMode = m_line.lineStyle;
      config.isDualColour = m_line.isDualColour;
      config.primaryColour = vcIGSW_BGRAToImGui(m_line.colourPrimary);
      config.secondaryColour = vcIGSW_BGRAToImGui(m_line.colourSecondary);
      config.ribbonWidth = m_line.lineWidth;
      config.textureScrollSpeed = 1.f;
      config.textureRepeatScale = 1.f;
      config.worldUp = udFloat3::create(vcGIS_GetWorldLocalUp(pProgramState->geozone, m_centroid)); //TODO this should be worldUp as calculated above
      
      vcFenceRenderer_SetConfig(m_pFence, config);

      vcFenceRenderer_ClearPoints(m_pFence);
      vcFenceRenderer_AddPoints(m_pFence, m_line.pPoints, m_line.numPoints, m_line.closed);
    }
    else
    {
      // TODO: 1452
      if (m_pLine == nullptr)
        vcLineRenderer_CreateLine(&m_pLine);

      vcLineRenderer_UpdatePoints(m_pLine, m_line.pPoints, m_line.numPoints, vcIGSW_BGRAToImGui(m_line.colourPrimary), m_line.lineWidth, m_line.closed);
    }
  }

  // Update the label as well
  m_pLabelInfo->pText = m_pLabelText;
  m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
  m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
  m_pLabelInfo->pSceneItem = this;

  for (size_t i = 0; i < m_lengthLabels.length; ++i)
  {
    vcLabelInfo* pLabel = m_lengthLabels.GetElement(i);
    pLabel->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
    pLabel->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
    pLabel->pSceneItem = this;
  }

  // Update Polygon Model
  if (m_pPolyModel != nullptr)
    GenerateLineFillPolygon(pProgramState);
}

void vcPOI::HandleBasicUI(vcState *pProgramState, size_t itemID)
{
  m_pState->HandleBasicUI(pProgramState, itemID);  
}

void vcPOI::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  HandleBasicUI(pProgramState, *pItemID);

  if (m_line.numPoints > 1)
  {
    if (ImGui::SliderInt(vcString::Get("scenePOISelectedPoint"), &m_line.selectedPoint, -1, m_line.numPoints - 1))
      m_line.selectedPoint = udClamp(m_line.selectedPoint, -1, m_line.numPoints - 1);

    if (m_line.selectedPoint != -1)
    {
      ImGui::InputScalarN(udTempStr("%s##POIPointPos%zu", vcString::Get("scenePOIPointPosition"), *pItemID), ImGuiDataType_Double, &m_line.pPoints[m_line.selectedPoint].x, 3);
      if (ImGui::IsItemDeactivatedAfterEdit())
        vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, m_pState->GetGeometryType(), m_line.pPoints, m_line.numPoints);

      if (ImGui::Button(vcString::Get("scenePOIRemovePoint")))
        RemovePoint(pProgramState, m_line.selectedPoint);
    }
  }

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIColour%zu", vcString::Get("scenePOILabelColour"), *pItemID), &m_nameColour, ImGuiColorEditFlags_None))
  {
    m_pLabelInfo->textColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_nameColour);
    udProjectNode_SetMetadataUint(m_pNode, "nameColour", m_nameColour);
  }

  if (vcIGSW_ColorPickerU32(udTempStr("%s##POIBackColour%zu", vcString::Get("scenePOILabelBackgroundColour"), *pItemID), &m_backColour, ImGuiColorEditFlags_None))
  {
    m_pLabelInfo->backColourRGBA = vcIGSW_BGRAToRGBAUInt32(m_backColour);
    udProjectNode_SetMetadataUint(m_pNode, "backColour", m_backColour);
  }

  if (vcIGSW_InputText(vcString::Get("scenePOILabelHyperlink"), m_hyperlink, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue))
    udProjectNode_SetMetadataString(m_pNode, "hyperlink", m_hyperlink);

  if (m_hyperlink[0] != '\0' && ImGui::Button(vcString::Get("scenePOILabelOpenHyperlink")))
  {
    if (udStrEndsWithi(m_hyperlink, ".png") || udStrEndsWithi(m_hyperlink, ".jpg"))
      pProgramState->pLoadImage = udStrdup(m_hyperlink);
    else
      vcWebFile_OpenBrowser(m_hyperlink);
  }

  if (vcIGSW_InputText(vcString::Get("scenePOILabelDescription"), m_description, sizeof(m_description), ImGuiInputTextFlags_EnterReturnsTrue))
    udProjectNode_SetMetadataString(m_pNode, "description", m_description);
   
  if (m_attachment.pModel != nullptr)
  {
    const double minSpeed = 0.0;
    const double maxSpeed = vcSL_CameraMaxAttachSpeed;

    if (ImGui::SliderScalar(vcString::Get("scenePOIAttachmentSpeed"), ImGuiDataType_Double, &m_attachment.moveSpeed, &minSpeed, &maxSpeed))
    {
      m_attachment.moveSpeed = udClamp(m_attachment.moveSpeed, minSpeed, maxSpeed);
      udProjectNode_SetMetadataDouble(m_pNode, "attachmentSpeed", m_attachment.moveSpeed);
    }

    const char *uiStrings[] = { vcString::Get("polyModelCullFaceBack"), vcString::Get("polyModelCullFaceFront"), vcString::Get("polyModelCullFaceNone") };
    const char *optStrings[] = { "back", "front", "none" };
    if (ImGui::Combo(udTempStr("%s##%zu", vcString::Get("polyModelCullFace"), *pItemID), (int *)&m_attachment.cullMode, uiStrings, (int)udLengthOf(uiStrings)))
      udProjectNode_SetMetadataString(m_pNode, "culling", optStrings[m_attachment.cullMode]);
  }
}

void vcPOI::HandleSceneEmbeddedUI(vcState *pProgramState)
{
  char buffer[128];

  ImGui::Text("%s (%s)", m_pNode->pName, m_pNode->itemtypeStr);

  if (m_showArea)
  {
    vcUnitConversion_ConvertAndFormatArea(buffer, udLengthOf(buffer), m_area, vcArea_SquareMetres, &pProgramState->settings.unitConversionData);

    ImGui::TextUnformatted(vcString::Get("scenePOIArea"));
    ImGui::Indent();
    ImGui::PushFont(pProgramState->pBigFont);
    ImGui::TextUnformatted(buffer);
    ImGui::PopFont();
    ImGui::Unindent();
  }

  if (m_showLength)
  {
    vcUnitConversion_ConvertAndFormatDistance(buffer, udLengthOf(buffer), m_totalLength, vcDistance_Metres, &pProgramState->settings.unitConversionData);

    ImGui::TextUnformatted(vcString::Get("scenePOILineLength"));
    ImGui::Indent();
    ImGui::PushFont(pProgramState->pBigFont);
    ImGui::TextUnformatted(buffer);
    ImGui::PopFont();
    ImGui::Unindent();
  }

  if (m_hyperlink[0] != '\0' && ImGui::Button(udTempStr("%s '%s'", vcString::Get("scenePOILabelOpenHyperlink"), m_hyperlink)))
  {
    if (udStrEndsWithi(m_hyperlink, ".png") || udStrEndsWithi(m_hyperlink, ".jpg"))
      pProgramState->pLoadImage = udStrdup(m_hyperlink);
    else
      vcWebFile_OpenBrowser(m_hyperlink);
  }
}

void vcPOI::HandleContextMenu(vcState *pProgramState)
{
  if (m_line.numPoints > 1)
  {
    ImGui::Separator();

    if (ImGui::MenuItem(vcString::Get("scenePOIPerformFlyThrough")))
    {
      pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = this;
      m_cameraFollowingAttachment = false;
    }

    if (m_attachment.pModel != nullptr && ImGui::MenuItem(vcString::Get("scenePOIAttachCameraToAttachment")))
    {
      pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = this;
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
          udProjectNode_SetMetadataString(m_pNode, "attachmentURI", uriBuffer);
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
      udProjectNode_SetMetadataDouble(m_pNode, "attachmentSpeed", m_attachment.moveSpeed);
    }
  }
  else
  {
    // Settings?
  }
}

void vcPOI::HandleToolUI(vcState *pProgramState)
{
  m_pState->HandlePopupUI(pProgramState);
  HandleSceneEmbeddedUI(pProgramState);
}

void vcPOI::InsertPoint(const udDouble3 &position)
{
  m_line.numPoints++;

  udDouble3 *pNewPoints = udAllocType(udDouble3, m_line.numPoints, udAF_Zero);

  memcpy(pNewPoints, m_line.pPoints, sizeof(udDouble3) * m_line.numPoints);
  pNewPoints[m_line.numPoints - 1] = position;

  udFree(m_line.pPoints);
  m_line.pPoints = pNewPoints;
}

void vcPOI::AddPoint(vcState *pProgramState, const udDouble3 &position, bool isPreview)
{
  UpdateState(pProgramState);
  m_pState->AddPoint(pProgramState, position, isPreview);
}

void vcPOI::RemovePoint(vcState *pProgramState, int index)
{
  if (index < (m_line.numPoints - 1))
    memmove(m_line.pPoints + index, m_line.pPoints + index + 1, sizeof(udDouble3) * (m_line.numPoints - index - 1));

  --m_line.numPoints;

  if (m_line.selectedPoint == m_line.numPoints)
    --m_line.selectedPoint;

  UpdatePoints(pProgramState);
  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, m_pState->GetGeometryType(), m_line.pPoints, m_line.numPoints);

  if (m_line.numPoints <= 1)
  {
    vcLineRenderer_DestroyLine(&m_pLine);
  }
}

void vcPOI::ChangeProjection(const udGeoZone &newZone)
{
  udFree(m_line.pPoints);
  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &m_line.pPoints, &m_line.numPoints);
  UpdatePoints(m_pProgramState);
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
  vcPolygonModel_Destroy(&m_pPolyModel);
  vcFenceRenderer_Destroy(&m_pFence);
  vcLineRenderer_DestroyLine(&m_pLine);
  udFree(m_pLabelInfo);

  if (pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem == this)
    pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = nullptr;
}

void vcPOI::SetCameraPosition(vcState *pProgramState)
{
  udDouble3 newPosition = m_attachment.currentPos;
  if (!m_attachment.pModel)
    newPosition = m_pLabelInfo->worldPosition;

  for (int viewportIndex = 0; viewportIndex < pProgramState->settings.activeViewportCount; ++viewportIndex)
    pProgramState->pViewports[viewportIndex].camera.position = newPosition;

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

  udDouble3 linearDistance = (pProgramState->pActiveViewport->camera.position - m_line.pPoints[i]);

  pInstance->pModel = gInternalModels[vcInternalModelType_Sphere];
  pInstance->worldMat = udDouble4x4::translation(m_line.pPoints[i]) * udDouble4x4::scaleUniform(udMag3(linearDistance) / 100.0); //This makes it ~1/100th of the screen size
  pInstance->pSceneItem = this;
  pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
  pInstance->sceneItemInternalId = (uint64_t)i + 1;
  pInstance->tint = udFloat4::create(1.0f, 1.0f, 1.0f, 0.65f);
  pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
  pInstance->selectable = false;

  return pInstance;
}

void vcPOI::CalculateArea(const udDouble4 &projectionPlane)
{
  m_area = 0.0;

  if (m_line.numPoints < 3)
    return;

  m_area = udProjectedArea(projectionPlane, m_line.pPoints, (size_t)m_line.numPoints);
}

void vcPOI::CalculateTotalLength()
{
  m_totalLength = 0.0;

  int j = udMax(0, m_line.numPoints - 1);
  for (int i = 0; i < m_line.numPoints; i++)
  {
    double lineLength = udMag3(m_line.pPoints[j] - m_line.pPoints[i]);

    if (m_line.closed || i > 0)
      m_totalLength += lineLength;

    j = i;
  }
}

//TODO This is will work but not show the true centroid.
// We should find the centroid of the convex hull.
void vcPOI::CalculateCentroid()
{
  m_centroid = udDouble3::zero();

  if (m_line.numPoints == 0)
    return;

  udDouble3 aabbMin = m_line.pPoints[0];
  udDouble3 aabbMax = m_line.pPoints[0];

  for (int p = 1; p < m_line.numPoints; p++)
  {
    aabbMin = udMin(aabbMin, m_line.pPoints[p]);
    aabbMax = udMax(aabbMax, m_line.pPoints[p]);
  }

  m_centroid = (aabbMin + aabbMax) * 0.5;
}

void vcPOI::AddLengths(const vcUnitConversionData *pConversionData)
{
  // j = previous, i = current
  int j = udMax(0, m_line.numPoints - 1);
  for (int i = 0; i < m_line.numPoints; i++)
  {
    double lineLength = udMag3(m_line.pPoints[j] - m_line.pPoints[i]);

    if (m_line.numPoints > 1)
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

      char buffer[128] = {};
      vcUnitConversion_ConvertAndFormatDistance(buffer, 128, lineLength, vcDistance_Metres, pConversionData);
      udSprintf(&pLabel->pText, "%s", buffer);
    }

    j = i;
  }
}

void vcPOI::UpdateState(vcState *pProgramState)
{

  vcPOIState_General *pNewState = m_pState->ChangeState(pProgramState);
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

double vcPOI::DistanceToPoint(udDouble3 const &point)
{
  int nSegments = m_line.numPoints;
  if (!m_line.closed)
    --nSegments;

  double distanceSq = udMagSq(m_line.pPoints[0] - point);

  for (int i = 0; i < nSegments; ++i)
  {
    udLineSegment<double> seg(m_line.pPoints[i], m_line.pPoints[(i + 1) % m_line.numPoints]);
    double segDistanceSq = udDistanceSqLineSegmentPoint(seg, point);
    distanceSq = udMin(segDistanceSq, distanceSq);
  }

  return udSqrt(distanceSq);
}

bool vcPOI::IsVisible(vcState *pProgramState)
{
  // if POI is invisible or if it exceeds maximum visible POI distance (unless selected)
  bool visible = m_visible;
  visible = visible && (DistanceToPoint(pProgramState->pActiveViewport->camera.position) < pProgramState->settings.presentation.POIFadeDistance);
  visible = visible || m_selected;
  return visible;
}

void vcPOI::AddFenceToScene(vcRenderData *pRenderData)
{
  if (m_line.numPoints < 2)
    return;

  if (m_line.fenceMode != vcRRVM_ScreenLine && m_pFence != nullptr)
    pRenderData->fences.PushBack(m_pFence);
  else if (m_line.fenceMode == vcRRVM_ScreenLine && m_pLine != nullptr)
    pRenderData->lines.PushBack(m_pLine);
}

void vcPOI::AddLabelsToScene(vcRenderData *pRenderData, const vcUnitConversionData *pConversionData)
{
  if (m_pLabelInfo != nullptr)
  {
    if (m_showAllLengths && m_line.numPoints > 1)
    {
      AddLengths(pConversionData);

      for (size_t i = 0; i < m_lengthLabels.length; ++i)
      {
        if (m_line.closed || i > 0)
          pRenderData->labels.PushBack(m_lengthLabels.GetElement(i));
      }
    }

    if ((m_showLength && m_line.numPoints > 1) || (m_showArea && m_line.numPoints > 2))
      m_pLabelInfo->pText = m_pLabelText;
    else
      m_pLabelInfo->pText = m_pNode->pName;

    pRenderData->labels.PushBack(m_pLabelInfo);
  }
}

void vcPOI::AddFillPolygonToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (m_pPolyModel == nullptr || m_line.numPoints < 3)
    return;
  
  vcRenderPolyInstance *pInstance = pRenderData->polyModels.PushBack();
  pInstance->pModel = m_pPolyModel;
  pInstance->pSceneItem = this;
  pInstance->worldMat = udDouble4x4::identity();
  pInstance->sceneItemInternalId = (uint64_t)0;
  pInstance->cullFace = vcGLSCM_None;
  pInstance->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent;
  pInstance->pDiffuseOverride = pProgramState->pWhiteTexture;
  pInstance->tint = vcIGSW_BGRAToImGui(m_measurementAreaFillColour);
  pInstance->selectable = true;
}

static double Det(double a, double b, double c, double d)
{
  return a * d - b * c;
}

static bool LineLineIntersection2D(const udDouble3 &line1Point1, const udDouble3 &line1Point2, const udDouble3 &line2Point1, const udDouble3 &line2Point2, udDouble3 *pIntersectPoint = nullptr)
{
  double detL1 = Det(line1Point1.x, line1Point1.y, line1Point2.x, line1Point2.y);
  double detL2 = Det(line2Point1.x, line2Point1.y, line2Point2.x, line2Point2.y);
  udDouble3 line1Dif = line1Point1 - line1Point2;
  udDouble3 line2Dif = line2Point1 - line2Point2;

  double denom = Det(line1Dif.x, line1Dif.y, line2Dif.x, line2Dif.y);

  if (denom == 0.0)
    return false;

  udDouble3 intersectPoint = udDouble3::create(
    Det(detL1, line1Dif.x, detL2, line2Dif.x) / denom,
    Det(detL1, line1Dif.y, detL2, line2Dif.y) / denom,
    0.0);

  // Calculate Z
  {
    const udDouble2 intersectPoint2 = udDouble2::create(intersectPoint.x, intersectPoint.y);

    udDouble3 pointPositions[4];
    pointPositions[0] = line1Point1;
    pointPositions[1] = line1Point2;
    pointPositions[2] = line2Point1;
    pointPositions[3] = line2Point2;

    double influence[4];
    double totalInfluence = 0.0;
    for (int64_t i = 0; i < 4; ++i)
    {
      influence[i] = 1.0 / udMag2(intersectPoint2 - udDouble2::create(pointPositions[i].x, pointPositions[i].y));
      totalInfluence += influence[i];
    }

    for (int64_t i = 0; i < 4; ++i)
      intersectPoint.z += pointPositions[i].z * (influence[i] / totalInfluence);
  }
  
  // Calculate Bounds
  udDouble4 line1Bounds;
  line1Bounds.x = udMin(line1Point1.x, line1Point2.x);
  line1Bounds.y = udMin(line1Point1.y, line1Point2.y);
  line1Bounds.z = udMax(line1Point1.x, line1Point2.x);
  line1Bounds.w = udMax(line1Point1.y, line1Point2.y);

  udDouble4 line2Bounds;
  line2Bounds.x = udMin(line2Point1.x, line2Point2.x);
  line2Bounds.y = udMin(line2Point1.y, line2Point2.y);
  line2Bounds.z = udMax(line2Point1.x, line2Point2.x);
  line2Bounds.w = udMax(line2Point1.y, line2Point2.y);

  bool intersectPointInLine1Bounds = intersectPoint.x >= line1Bounds.x && intersectPoint.y >= line1Bounds.y && intersectPoint.x <= line1Bounds.z && intersectPoint.y <= line1Bounds.w;
  bool intersectPointInLine2Bounds = intersectPoint.x >= line2Bounds.x && intersectPoint.y >= line2Bounds.y && intersectPoint.x <= line2Bounds.z && intersectPoint.y <= line2Bounds.w;

  if (!intersectPointInLine1Bounds || !intersectPointInLine2Bounds)
    return false;

  if (pIntersectPoint)
  {
    pIntersectPoint->x = Det(detL1, line1Dif.x, detL2, line2Dif.x) / denom;
    pIntersectPoint->y = Det(detL1, line1Dif.y, detL2, line2Dif.y) / denom;
    pIntersectPoint->z = intersectPoint.z;
  }

  return true;
}

// static double DistancePointToLine(const udDouble3 &linePoint1, const udDouble3 &linePoint2, const udDouble3 &point)
// {
//   const udDouble3 p2Subp1 = linePoint2 - linePoint1;
//   const udDouble3 pointSubP1 = point - linePoint1;
// 
//   return udMag3(udCross3(p2Subp1, pointSubP1)) / udMag3(p2Subp1);
// }

static double DistancePointToLine(const udDouble2 &linePoint1, const udDouble2 &linePoint2, const udDouble2 &point)
{
  double m = (linePoint2.y - linePoint1.y) / (linePoint2.x - linePoint1.x);
  double b = linePoint1.y - (m * linePoint1.x);

  double x = (m * point.y + point.x - m * b) / (m * m + 1.0);
  double y = (m * m * point.y + m * point.x + b) / (m * m + 1.0);

  double ret = udMag2(udDouble2::create(x, y) - point);

  return ret;
}

void CalculatePerimeter(const udChunkedArray<udDouble3> &originalPoints, udChunkedArray<udDouble3> *pPerimeterPoints)
{
  struct Point
  {
    void Init()
    {
      pointConnections.Init(4); // Unlikely to have more than 4 connections
    }

    void Deinit()
    {
      pointConnections.Deinit();
    }

    void RemoveConnection(const int &pointConnection)
    {
      for (int i = 0; i < pointConnections.length; ++i)
        if (pointConnections[i] == pointConnection)
        {
          pointConnections.RemoveAt(i);
          break;
        }
    }

    udDouble3 position;
    udChunkedArray<int> pointConnections;
  };
  udChunkedArray<Point> points;
  points.Init(64);

  // Connect points circularly in list order
  for (int i = 0; i < originalPoints.length; ++i)
  {
    Point newPoint;
    newPoint.Init();
    newPoint.position = originalPoints[i];
    newPoint.pointConnections.PushBack(i == 0 ? originalPoints.length - 1 : i - 1);
    newPoint.pointConnections.PushBack(i == originalPoints.length - 1 ? 0 : i + 1);

    points.PushBack(newPoint);
  }

  // Search for Line crosses and Split
  bool continueSplitting = true;
  while (continueSplitting)
  {
    continueSplitting = false;
    udChunkedArray<Point> newPointsToAdd;
    newPointsToAdd.Init(64);
    for (int i = 0; i < points.length; ++i)
      for (int pointConnection : points[i].pointConnections)
      {
        bool splitFound = false;
        udInt2 line1 = udInt2::create(i, pointConnection);
        for (int i2 = i + 1; i2 < points.length; ++i2)
        {
          if (pointConnection != i2)
            for (int pointConnection2 : points[i2].pointConnections)
            {
              if (i != pointConnection2 && pointConnection != pointConnection2)
              {
                udInt2 line2 = udInt2::create(i2, pointConnection2);

                udDouble3 splitPoint;
                if (LineLineIntersection2D(
                  points[line1.x].position,
                  points[line1.y].position,
                  points[line2.x].position,
                  points[line2.y].position, &splitPoint))
                {
                  // New point to connect the 4 other points
                  Point newPoint;
                  newPoint.Init();
                  newPoint.position = splitPoint;
                  newPoint.pointConnections.PushBack(line1.x);
                  newPoint.pointConnections.PushBack(line1.y);
                  newPoint.pointConnections.PushBack(line2.x);
                  newPoint.pointConnections.PushBack(line2.y);
                  newPointsToAdd.PushBack(newPoint);

                  // Remove Connections between crossing points
                  points[line1.x].RemoveConnection(line1.y);
                  points[line1.y].RemoveConnection(line1.x);
                  points[line2.x].RemoveConnection(line2.y);
                  points[line2.y].RemoveConnection(line2.x);

                  splitFound = true;
                  break;
                }
              }
            }

          // Need to break because line1 is now invalid
          if (splitFound)
          {
            continueSplitting = true;
            break;
          }
        }

        if (splitFound)
        {
          i = (int)points.length;
          break;
        }
      }

    for (const Point &newPoint : newPointsToAdd)
    {
      points.PushBack(newPoint);
      int newPointIndex = (int)points.length - 1;
      for (int64_t connectionIndex = 0; connectionIndex < newPoint.pointConnections.length; ++connectionIndex)
        points[newPoint.pointConnections[connectionIndex]].pointConnections.PushBack(newPointIndex);
    }

    newPointsToAdd.Deinit();
  }

  // Trace the edge of the shape to get an outline

  // Find StartPoint
  int64_t startPoint = -1;
  double highestPoint = -DBL_MAX;
  for (int64_t i = 0; i < points.length; ++i)
    if (points[i].position.y > highestPoint)
    {
      startPoint = i;
      highestPoint = points[i].position.y;
    }

  udChunkedArray<int64_t> perimeterPoints;
  perimeterPoints.Init(64);

  int64_t prevPoint = startPoint;
  int64_t currentPoint = startPoint;
  double prevClosestAngle = DBL_MAX;
  for (const int64_t &pointIndex : points[startPoint].pointConnections)
  {
    const udDouble3 &pos = points[startPoint].position;
    const udDouble3 &target = points[pointIndex].position;

    double angle = -atan2(pos.y - target.y, pos.x - target.x);

    // Ensure the angle is positive
    if (angle < 0.0)
      angle += UD_2PI;

    if (angle < prevClosestAngle)
    {
      prevClosestAngle = angle;
      currentPoint = pointIndex;
    }
  }

  perimeterPoints.PushBack(startPoint);
  while (currentPoint != startPoint)
  {
    perimeterPoints.PushBack(currentPoint);

    // Reverse the previous angle
    double prevAngle = -atan2(points[currentPoint].position.y - points[prevPoint].position.y, points[currentPoint].position.x - points[prevPoint].position.x);

    int64_t closestPoint = -1;
    double closestAngle = DBL_MAX;
    const udDouble3 &pos = points[currentPoint].position;
    for (const int64_t &pointIndex : points[currentPoint].pointConnections)
      if (pointIndex != prevPoint)
      {
        const udDouble3 &target = points[pointIndex].position;
        double angle = -atan2(pos.y - target.y, pos.x - target.x);

        // Ensure the angle is positive relative to the previous angle
        if (angle < prevAngle)
          angle += UD_2PI;

        if (angle < closestAngle)
        {
          closestAngle = angle;
          closestPoint = pointIndex;
        }
      }

    prevPoint = currentPoint;
    currentPoint = closestPoint;
  }

  // Finalize
  for (const int64_t &pointIndex : perimeterPoints)
    pPerimeterPoints->PushBack(points[pointIndex].position);

  perimeterPoints.Deinit();
}

bool CalculateTriangles(udDouble3 *pPoints, int numPoints, udChunkedArray<udDouble3> *pTrianglePoints)
{
  double bias = 0.000001;

  udDouble3 pivotPoint = pPoints[0];

  // Point Info
  struct PointInfo
  {
    void Init() { lines.Init(512); }
    void DeInit() { lines.Deinit(); }

    udDouble3 position;
    udChunkedArray<int> lines;
  };

  udChunkedArray<PointInfo> points;
  points.Init(512);
  for (int i = 0; i < numPoints; ++i)
  {
    int prevIndex = (i == 0 ? numPoints : i) - 1;
    if (udMag3(pPoints[i] - pPoints[prevIndex]) < bias)
      continue;

    PointInfo pointInfo;
    pointInfo.Init();
    pointInfo.position = pPoints[i];
    points.PushBack(pointInfo);
  }
    
  // Remove duplicate Points
  while (true)
  {
    bool unnecessaryPointRemoved = false;
    int64_t lastIndex = points.length - 1;
    for (int64_t i = 0; i < points.length; ++i)
    {
      int64_t prevIndex = i == 0 ? lastIndex : i - 1;
      int64_t nextIndex = i == lastIndex ? 0 : i + 1;

      udDouble2 curPointPos2 = udDouble2::create(points[i].position.x, points[i].position.y);
      udDouble2 nextPointPos2 = udDouble2::create(points[nextIndex].position.x, points[nextIndex].position.y);
      udDouble2 prevPointPos2 = udDouble2::create(points[prevIndex].position.x, points[prevIndex].position.y);

      if (udMag2(curPointPos2 - prevPointPos2) < bias
        || udMag2(curPointPos2 - nextPointPos2) < bias
        || udMag2(prevPointPos2 - nextPointPos2) < bias
        || DistancePointToLine(prevPointPos2, nextPointPos2, curPointPos2) < bias)
      {
        points.RemoveAt(i);
        unnecessaryPointRemoved = true;
        break;
      }
    }

    if (!unnecessaryPointRemoved)
      break;
  }

  if (points.length < 3)
    return false;


  udChunkedArray<udDouble3> pointPositions;
  pointPositions.Init(64);
  for (const PointInfo &point : points)
    pointPositions.PushBack(point.position);

  udChunkedArray<udDouble3> perimeterPoints;
  perimeterPoints.Init(64);
  CalculatePerimeter(pointPositions, &perimeterPoints);

  
  // Separate pinch-points in the mesh
  struct PointMesh
  {
    void Init()
    {
      pointIndices.Init(64);
    }

    void Deinit()
    {
      pointIndices.Deinit();
    }

    udChunkedArray<int> pointIndices;
  };

  udChunkedArray<int> perimeterPointsIndices;
  perimeterPointsIndices.Init(64);
  for (int64_t pointIndex = 0; pointIndex < perimeterPoints.length; ++pointIndex)
    perimeterPointsIndices.PushBack(pointIndex);

  udChunkedArray<PointMesh> pointMeshes;
  pointMeshes.Init(64);

  while (true)
  {
    bool cutFound = false;
    for (int64_t pointListIndex = 1; pointListIndex < perimeterPointsIndices.length; ++pointListIndex)
      for (int64_t pointListIndex2 = pointListIndex - 1; pointListIndex2 >= 0; --pointListIndex2)
      {
        int64_t pointIndex = perimeterPointsIndices[pointListIndex];
        int64_t pointIndex2 = perimeterPointsIndices[pointListIndex2];
        if (perimeterPoints[pointIndex] == perimeterPoints[pointIndex2])
        {
          // Ensure Loop ends
          cutFound = true;
          pointListIndex = perimeterPointsIndices.length;

          PointMesh newPointMesh;
          newPointMesh.Init();
          for (int64_t i = pointListIndex - 1; i >= pointListIndex2; --i)
          {
            newPointMesh.pointIndices.PushBack(perimeterPointsIndices[i]);
            if (i < pointListIndex)
              perimeterPointsIndices.RemoveAt(i);
          }
          pointMeshes.PushBack(newPointMesh);
          break;
        }
      }

    if (!cutFound)
      break;
  }

  // if (!pointMeshes.empty())
  // {
  //   int f = 7;
  //   f += 8;
  // }
  
  PointMesh remainingPointMesh;
  remainingPointMesh.Init();
  for (const int64_t &pointIndex : perimeterPointsIndices)
    remainingPointMesh.pointIndices.PushBack(pointIndex);
  pointMeshes.PushBack(remainingPointMesh); // At least 1 Point Mesh

  

  // Create Triangle Points
  std::vector<udDouble2> trianglePointList;
  for (const PointMesh &pointMesh : pointMeshes)
  {
    udDouble3 *pPointsForTriangleGeneration = udAllocType(udDouble3, pointMesh.pointIndices.length, udAF_Zero);

    for (int64_t i = 0; i < pointMesh.pointIndices.length; ++i)
      pPointsForTriangleGeneration[i] = perimeterPoints[pointMesh.pointIndices[i]];

    std::vector<udDouble2> trianglePointListTmp;
    udDouble2 min = {};
    udDouble2 max = {};
    vcCDT_ProcessOrignal(pPointsForTriangleGeneration, pointMesh.pointIndices.length, std::vector< std::pair<const udDouble3 *, size_t> >(), min, max, &trianglePointListTmp);
    udFree(pPointsForTriangleGeneration);

    for (const udDouble2 &point : trianglePointListTmp)
      trianglePointList.push_back(point);
  }
  
  /*
  udDouble2 min = {};
  udDouble2 max = {};
  std::vector<udDouble2> trianglePointList;
  vcCDT_ProcessOrignal(pPointsForTriangleGeneration, perimeterPoints.length, std::vector< std::pair<const udDouble3 *, size_t> >(), min, max, &trianglePointList);
  udFree(pPointsForTriangleGeneration);
  */

  // Create Triangle Points
  for (int64_t i = 0; i < (int64_t)trianglePointList.size(); i += 3)
  {
    udDouble3 triPoints[3] = {
      udDouble3::create(trianglePointList[i].x, trianglePointList[i].y, 0),
      udDouble3::create(trianglePointList[i + 1].x, trianglePointList[i + 1].y, 0),
      udDouble3::create(trianglePointList[i + 2].x, trianglePointList[i + 2].y, 0)};

    if (triPoints[0] == triPoints[1] || triPoints[0] == triPoints[2] || triPoints[1] == triPoints[2])
      continue;

    pTrianglePoints->PushBack(triPoints[0]);
    pTrianglePoints->PushBack(triPoints[1]);
    pTrianglePoints->PushBack(triPoints[2]);
  }

  // Un-flatten 2D Result
  for (int64_t i = 0; i < (int64_t)pTrianglePoints->length; ++i)
  {
    double closestDist = FLT_MAX;
    double closestZ = 0;
    for (const udDouble3 &point : perimeterPoints)
    {
      udDouble2 triPoint2 = udDouble2::create(pTrianglePoints->GetElement(i)->x, pTrianglePoints->GetElement(i)->y);
      udDouble2 pos = (point - pivotPoint).toVector2();

      double dist = udMag2<double>(pos - triPoint2);
      if (dist < closestDist)
      {
        closestDist = dist;
        closestZ = point.z - pivotPoint.z;
      }
    }

    pTrianglePoints->GetElement(i)->z = closestZ;
  }

  // Cleanup
  for (PointInfo &point : points)
    point.DeInit();
  points.Deinit();
  perimeterPoints.Deinit();
  return pTrianglePoints->length > 0;
}

void vcPOI::GenerateLineFillPolygon(vcState *pProgramState)
{
  if (m_line.numPoints >= 3)
  {
    udFloat3 defaultNormal = udFloat3::create(0.0f, 0.0f, 1.0f);
    udFloat2 defaultUV = udFloat2::create(0.0f, 0.0f);
    
    udDouble3 centerPoint = udDouble3::zero();
    double invNumPoints = 1.0 / (double)m_line.numPoints;
    for (int64_t i = 0; i < m_line.numPoints; ++i)
      centerPoint += m_line.pPoints[i] * invNumPoints;
    
    // Rotate
    udDoubleQuat rotate = vcGIS_GetQuaternion(pProgramState->geozone, centerPoint);
    udDoubleQuat rotateInverse = udInverse(rotate);

    udDouble3 *pModifiedVerts = udAllocType(udDouble3, m_line.numPoints, udAF_Zero);
    for (int i = 0; i < m_line.numPoints; ++i)
      pModifiedVerts[i] = m_line.pPoints[0] + rotateInverse.apply(m_line.pPoints[i] - m_line.pPoints[0]);

    udChunkedArray<udDouble3> triPoints;
    triPoints.Init(512);
    if (!CalculateTriangles(pModifiedVerts, m_line.numPoints, &triPoints))
    {
      triPoints.Deinit();
      return;
    }

    /*
    int lineNumber = 0;

    // Split Crossing Lines
    struct PointInfo
    {
      udDouble3 positon;
      udChunkedArray<int> lines;
    };

    udChunkedArray<int> splitPoints;
    udChunkedArray<PointInfo> finalPoints;
    udChunkedArray<udInt2> lines;
    for (int i = 0; i < m_line.numPoints; ++i)
    {
      PointInfo pointInfo;
      pointInfo.positon = pModifiedVerts[i];
      finalPoints.PushBack(pointInfo);
    }

    for (int i = 0; i < m_line.numPoints; ++i)
    {
      int pointIndex1 = i;
      int pointIndex2 = i == (m_line.numPoints - 1) ? 0 : i + 1);
      lines.PushBack(udInt2::create(pointIndex1, pointIndex2));

      int lineIndex = lines.ElementSize() - 1;
      finalPoints[pointIndex1].lines.PushBack(lineIndex);
      finalPoints[pointIndex2].lines.PushBack(lineIndex);
    }
    
    int startLine = 0;
    while (true)
    {
      bool splitFound = false;
      udDouble2 splitPoint = udDouble2::create(0);
      int line1 = startLine;
      int line2 = line1 + 1;
      for (; line1 < lines.ElementSize(); ++line1)
      {
        const udDouble2 &line1Point1 = finalPoints[lines[line1].x].positon.toVector2();
        const udDouble2 &line1Point2 = finalPoints[lines[line1].y].positon.toVector2();

        for (; line2 < lines.ElementSize(); ++line2)
        {
          if (lines[line1].x == lines[line2].x
            || lines[line1].x == lines[line2].y
            || lines[line1].y == lines[line2].x
            || lines[line1].y == lines[line2].y)
            continue;

          const udDouble2 &line2Point1 = finalPoints[lines[line2].x].positon.toVector2();
          const udDouble2 &line2Point2 = finalPoints[lines[line2].y].positon.toVector2();

          if (LineLineIntersection2D(line1Point1, line1Point2, line2Point1, line2Point2, &splitPoint))
          {
            // Add new split point
            int64_t newPointIndex = finalPoints.ElementSize();
            finalPoints.PushBack(udDouble3::create(splitPoint.x, splitPoint.y, 0.0));

            // remove the 2 lines, and add 4 new ones
            lines.RemoveAt(line2);
            lines.RemoveAt(line1);

            lines.PushBack(udInt2::create(lines[line1].x, newPointIndex));
            lines.PushBack(udInt2::create(lines[line1].y, newPointIndex));
            lines.PushBack(udInt2::create(lines[line2].x, newPointIndex));
            lines.PushBack(udInt2::create(lines[line2].y, newPointIndex));

            splitFound = true;
            break;
          }
        }

        if (splitFound)
          break;
      }

      if (splitFound)
      {
        PointInfo newPointInfo;
        newPointInfo.positon = splitPoint;
        finalPoints();

        udInt2 newLines[4];
        newLines[0] = udInt2::create(lines[line1].x, );
      }
      else
        break;
    }

    // Start from Top-most point to ensure it's not inside the shape
    int startPoint = -1;
    double highestY = -DBL_MAX;



    // Generate Triangles
    vcCDT_ProcessOrignal(pModifiedVerts, m_line.numPoints, std::vector< std::pair<const udDouble3 *, size_t> >(), min, max, &trianglePointList);
       
    int numPoints = (int)trianglePointList.size();

    udDouble3 *pPositions = udAllocType(udDouble3, numPoints, udAF_Zero);
    for (int64_t i = 0; i < (int64_t)trianglePointList.size(); ++i)
      pPositions[i] = udDouble3::create(trianglePointList[i].x, trianglePointList[i].y, 0);
    
    // Un-flatten 2D Result
    for (int64_t i = 0; i < (int64_t)trianglePointList.size(); ++i)
    {
      double closestDist = FLT_MAX;
      double closestZ = 0;
      for (int pointIndex = 0; pointIndex < m_line.numPoints; ++pointIndex)
      {
        udDouble2 pos = (pModifiedVerts[pointIndex] - pModifiedVerts[0]).toVector2();

        double dist = udMag2<double>(pos - trianglePointList[i]);
        if (dist < closestDist)
        {
          closestDist = dist;
          closestZ = pModifiedVerts[pointIndex].z - pModifiedVerts[0].z;
        }
      }

      pPositions[i].z = closestZ;
    }
    */

    // Un-Rotate
    vcP3N3UV2Vertex *pVerts = udAllocType(vcP3N3UV2Vertex, triPoints.length, udAF_Zero);
    udFloatQuat rotatef = udFloatQuat::create(rotate);
    for (int64_t i = 0; i < (int64_t)triPoints.length; ++i)
    {
      pVerts[i].position = rotatef.apply(udFloat3::create(triPoints[i]));
      pVerts[i].uv = defaultUV;
      pVerts[i].normal = defaultNormal;
    }

    vcPolygonModel_Destroy(&m_pPolyModel);
    vcPolygonModel_CreateFromRawVertexData(&m_pPolyModel, pVerts, (uint32_t)triPoints.length, vcP3N3UV2VertexLayout, (int)(udLengthOf(vcP3N3UV2VertexLayout)));

    m_pPolyModel->modelOffset = udDouble4x4::translation(m_line.pPoints[0]);

    triPoints.Deinit();

    udFree(pModifiedVerts);
    udFree(pVerts);
  }
}

void vcPOI::AddAttachedModelsToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (m_attachment.pModel != nullptr)
  {
    double remainingMovementThisFrame = m_attachment.moveSpeed * pProgramState->deltaTime;
    udDouble3 startYPR = m_attachment.eulerAngles;
    udDouble3 startPosDiff = pProgramState->pActiveViewport->camera.position - m_attachment.currentPos;

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
    pModel->selectable = true;
    pModel->tint = udFloat4::one();

    // Update the camera if the camera is coming along
    if (pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem == this && m_cameraFollowingAttachment)
    {
      udOrientedPoint<double> rotRay = udOrientedPoint<double>::create(startPosDiff, vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, pProgramState->pActiveViewport->camera.headingPitch));
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.z.toVector3(), m_attachment.eulerAngles.x - startYPR.x);
      rotRay = rotRay.rotationAround(rotRay, udDouble3::zero(), attachmentMat.axis.x.toVector3(), m_attachment.eulerAngles.y - startYPR.y);
      pProgramState->pActiveViewport->camera.position = m_attachment.currentPos + rotRay.position;
      pProgramState->pActiveViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, rotRay.orientation);
    }
  }
}

void vcPOI::DoFlythrough(vcState *pProgramState)
{
  if (pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem == this && !m_cameraFollowingAttachment)
  {
    if (m_line.numPoints <= 1)
    {
      pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = nullptr;
    }
    else
    {
      double remainingMovementThisFrame = pProgramState->settings.camera.moveSpeed * pProgramState->deltaTime;
      udDoubleQuat startQuat = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, pProgramState->pActiveViewport->camera.headingPitch);

      udDouble3 updatedPosition = {};

      if (!GetPointAtDistanceAlongLine(remainingMovementThisFrame, &updatedPosition, &m_flyThrough.segmentIndex, &m_flyThrough.segmentProgress))
      {
        pProgramState->pActiveViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, udDoubleQuat::create(udNormalize(m_line.pPoints[1] - m_line.pPoints[0]), 0.0));
        pProgramState->pActiveViewport->cameraInput.pAttachedToSceneItem = nullptr;
      }
      else
      {
        pProgramState->pActiveViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pProgramState->pActiveViewport->camera.position, udSlerp(startQuat, udDoubleQuat::create(udDirectionToYPR(updatedPosition - pProgramState->pActiveViewport->camera.position)), 0.2));
      }

      pProgramState->pActiveViewport->camera.position = updatedPosition;
    }
  }
}
