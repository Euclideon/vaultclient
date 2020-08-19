#include "vcInspect.h"

#include "vcState.h"
#include "vcStrings.h"
#include "vcSettingsUI.h"

#include "imgui_ex/imgui_udValue.h"

#include "udStringUtil.h"

#include "imgui.h"

vcInspect vcInspect::m_instance;

void vcInspect::SceneUI(vcState *pProgramState)
{
  ImGui::TextUnformatted(vcString::Get("toolInspectRunning"));
  ImGui::Separator();

  if (pProgramState->pActiveViewport->udModelNodeAttributes.IsObject())
    vcImGuiValueTreeObject(&pProgramState->pActiveViewport->udModelNodeAttributes);
}

void vcInspect::PreviewPicking(vcState *pProgramState, vcRenderData &renderData, const vcRenderPickResult & /*pickResult*/)
{
  if (pProgramState->pActiveViewport->udModelPickedIndex >= 0 && pProgramState->pActiveViewport->udModelPickedNode.pTrav)
  {
    uint8_t *pAttributePtr = nullptr;
    static int lastIndex = -1;
    static udVoxelID lastNode = {};

    if ((lastIndex != pProgramState->pActiveViewport->udModelPickedIndex || memcmp(&lastNode, &pProgramState->pActiveViewport->udModelPickedNode, sizeof(lastNode))) && udPointCloud_GetAttributeAddress(renderData.models[pProgramState->pActiveViewport->udModelPickedIndex]->m_pPointCloud, &pProgramState->pActiveViewport->udModelPickedNode, 0, (const void **)&pAttributePtr) == udE_Success)
    {
      pProgramState->pActiveViewport->udModelNodeAttributes.SetVoid();

      udPointCloudHeader *pHeader = &renderData.models[pProgramState->pActiveViewport->udModelPickedIndex]->m_pointCloudHeader;

      for (uint32_t i = 0; i < pHeader->attributes.count; ++i)
      {
        if (pHeader->attributes.pDescriptors[i].typeInfo == udATI_uint8 && udStrEqual(pHeader->attributes.pDescriptors[i].name, "udClassification"))
        {
          uint8_t classificationID;
          udReadFromPointer(&classificationID, pAttributePtr);

          const char *pClassificationName = vcSettingsUI_GetClassificationName(pProgramState, classificationID);

          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = '%s'", pHeader->attributes.pDescriptors[i].name, pClassificationName);
          continue;
        }

        if (pHeader->attributes.pDescriptors[i].typeInfo == udATI_float64 && udStrEqual(pHeader->attributes.pDescriptors[i].name, "udGPSTime"))
        {
          double GPSTime;
          udReadFromPointer(&GPSTime, pAttributePtr);

          char buffer[128];
          vcTimeReferenceData timeData;
          timeData.seconds = GPSTime;
          vcUnitConversion_ConvertAndFormatTimeReference(buffer, 128, timeData, pProgramState->settings.visualization.GPSTime.inputFormat, &pProgramState->settings.unitConversionData);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = '%s'", pHeader->attributes.pDescriptors[i].name, buffer);
          continue;
        }

        // Do Stuff
        switch (pHeader->attributes.pDescriptors[i].typeInfo)
        {
        case udATI_uint8:
        {
          uint8_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %u", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_uint16:
        {
          uint16_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %u", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_uint32:
        {
          uint32_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %u", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_uint64:
        {
          uint64_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %" PRIu64, pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_int8:
        {
          int8_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %d", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_int16:
        {
          int16_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %d", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_int32:
        {
          int16_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %d", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_int64:
        {
          int64_t val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %" PRId64, pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_float32:
        {
          float val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %f", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_float64:
        {
          double val;
          udReadFromPointer(&val, pAttributePtr);
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = %f", pHeader->attributes.pDescriptors[i].name, val);
          break;
        }
        case udATI_color32:
        {
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = \"RGB(%u, %u, %u)\"", pHeader->attributes.pDescriptors[i].name, pAttributePtr[2], pAttributePtr[1], pAttributePtr[0]); //BGRA internally
          pAttributePtr += 4;
          break;
        }
        case udATI_normal32: // Not currently supported
        case udATI_vec3f32: // Not currently supported
        default:
        {
          pProgramState->pActiveViewport->udModelNodeAttributes.Set("%s = 'UNKNOWN'", pHeader->attributes.pDescriptors[i].name);
          pAttributePtr += ((pHeader->attributes.pDescriptors[i].typeInfo & udATI_SizeMask) >> udATI_SizeShift);
          break;
        }
        }
      }
    }

    lastIndex = pProgramState->pActiveViewport->udModelPickedIndex;
    lastNode = pProgramState->pActiveViewport->udModelPickedNode;
  }
}
