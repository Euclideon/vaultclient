#include "vcMedia.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcFenceRenderer.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "exif.h"

#include "stb_image.h"

vcMedia::vcMedia(vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pNode, pProgramState)
{
  memset(&m_image, 0, sizeof(m_image));
  m_loadStatus = vcSLS_Loaded;

  if (pNode->pCoordinates == nullptr)
    pNode->pCoordinates = (double*)udAllocType(udDouble3, 1, udAF_Zero);

  if (pNode->geomCount == 1)
    m_image.position = ((udDouble3*)pNode->pCoordinates)[0];

  if (pProgramState->gis.isProjected)
  {
    m_pPreferredProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    memcpy(m_pPreferredProjection, &pProgramState->gis.zone, sizeof(udGeoZone));
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);
    memcpy(m_pCurrentProjection, &pProgramState->gis.zone, sizeof(udGeoZone));
    m_image.position = udGeoZone_ToCartesian(*m_pCurrentProjection, m_image.position, true);
  }
}

vcMedia::~vcMedia()
{
  vcTexture_Destroy(&m_image.pTexture);
}

void vcMedia::OnNodeUpdate()
{
  if (m_image.pTexture != nullptr)
    vcTexture_Destroy(&m_image.pTexture);

  if (m_pNode->pURI != nullptr)
  {
    void *pFileData = nullptr;
    int64_t fileLen = 0;

    if (udFile_Load(m_pNode->pURI, &pFileData, &fileLen) == udR_Success)
    {
      if (vcTexture_CreateFromMemory(&m_image.pTexture, pFileData, fileLen))
        m_loadStatus = vcSLS_Loaded;
      else
        m_loadStatus = vcSLS_Failed;

      udFree(pFileData);
    }
    else
    {
      m_loadStatus = vcSLS_Failed;
    }
  }

  // Load this info from the node:
  if (m_pCurrentProjection == nullptr)
    m_image.position = *(udDouble3*)m_pNode->pCoordinates;
  else
    m_image.position = udGeoZone_ToCartesian(*m_pCurrentProjection, *(udDouble3*)m_pNode->pCoordinates, true);

  m_image.ypr = udDouble3::zero();
  m_image.scale = udDouble3::one();
  m_image.colour = udFloat4::create(1.0f, 1.0f, 1.0f, 1.0f);
  m_image.size = vcIS_Large;
  m_image.type = vcIT_StandardPhoto;
}

void vcMedia::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_image.pTexture != nullptr)
  {
    // For now brute force sorting (n^2)
    double distToCameraSqr = udMagSq3(m_image.position - pRenderData->pCamera->position);
    size_t i = 0;
    for (; i < pRenderData->images.length; ++i)
    {
      if (udMagSq3(pRenderData->images[i]->position - pRenderData->pCamera->position) < distToCameraSqr)
        break;
    }

    vcImageRenderInfo *pImageInfo = &m_image;
    pRenderData->images.Insert(i, &pImageInfo);
  }
}

void vcMedia::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  udDouble4x4 resultMatrix = delta * udDouble4x4::translation(m_image.position) * udDouble4x4::rotationYPR(m_image.ypr) * udDouble4x4::scaleNonUniform(m_image.scale);
  udDouble3 position, scale;
  udQuaternion<double> rotation;
  udExtractTransform(resultMatrix, position, scale, rotation);

  m_image.position = position;
  m_image.ypr = udMath_DirToYPR(rotation.apply(udDouble3::create(0, 1, 0)));
  m_image.scale = scale;

  if (m_pCurrentProjection != nullptr)
  {
    udDouble3 longLat = udGeoZone_ToLatLong(*m_pCurrentProjection, m_image.position, true);
    vdkProjectNode_SetGeometry(m_pProject, m_pNode, vdkPGT_Point, 1, &longLat.x);
  }
}

void vcMedia::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
  // Handle imageurl
  if (m_pNode->pURI != nullptr)
  {
    ImGui::TextWrapped("%s: %s", vcString::Get("scenePOILabelImageURL"), m_pNode->pURI);
    if (ImGui::Button(vcString::Get("scenePOILabelOpenImageURL")))
      pProgramState->pLoadImage = udStrdup(m_pNode->pURI);

    const char *imageTypeNames[] = { vcString::Get("scenePOILabelImageTypeStandard"), vcString::Get("scenePOILabelImageTypePanorama"), vcString::Get("scenePOILabelImageTypePhotosphere") };
    ImGui::Combo(udTempStr("%s##scenePOILabelImageType%zu", vcString::Get("scenePOILabelImageType"), *pItemID), (int*)&m_image.type, imageTypeNames, (int)udLengthOf(imageTypeNames));
  }
}

void vcMedia::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pCurrentProjection != nullptr && newZone.srid == m_pCurrentProjection->srid)
    return;

  udDouble3 *pLatLong;

  if (m_pCurrentProjection == nullptr)
    pLatLong = &m_image.position;
  else
    pLatLong = (udDouble3*)m_pNode->pCoordinates;

  if (pLatLong->y < newZone.latLongBoundMin.x || pLatLong->y > newZone.latLongBoundMax.x || pLatLong->x < newZone.latLongBoundMin.y || pLatLong->x > newZone.latLongBoundMax.y)
    return;

  if (m_pCurrentProjection == nullptr)
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);

  memcpy(m_pCurrentProjection, &newZone, sizeof(udGeoZone));

  m_image.position = udGeoZone_ToCartesian(newZone, *pLatLong, true);
}

void vcMedia::Cleanup(vcState * /*pProgramState*/)
{
  // Nothing required
}

void vcMedia::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->pCamera->position = m_image.position;
}

udDouble4x4 vcMedia::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_image.position);
}
