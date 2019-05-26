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

vcMedia::vcMedia(vdkProjectNode *pNode) :
  vcSceneItem(pNode)
{
  m_pImage = nullptr;
  m_loadStatus = vcSLS_Loaded;

  if (pNode->geomCount == 1)
    m_worldPosition = ((udDouble3*)pNode->pCoordinates)[0];
}

vcMedia::~vcMedia()
{
  udFree(m_pImage);
}

void vcMedia::OnNodeUpdate()
{
  if (m_pImage != nullptr)
    vcTexture_Destroy(&m_pImage->pTexture);
  else
    m_pImage = udAllocType(vcImageRenderInfo, 1, udAF_Zero);

  udDouble3 geolocation = udDouble3::zero();
  bool hasLocation = false;
  vcImageType imageType = vcIT_StandardPhoto;

  vcTexture *pImage = nullptr;
  const unsigned char *pFileData = nullptr;
  int64_t numBytes = 0;

  udFilename filename(m_pNode->pURI);
  const char *pExt = filename.GetExt();

  if (udFile_Load(m_pNode->pURI, (void**)&pFileData, &numBytes) == udR_Success)
  {
    // Many jpg's have exif, let's process that first
    if (udStrEquali(pExt, ".jpg") || udStrEquali(pExt, ".jpeg"))
    {
      easyexif::EXIFInfo result;

      if (result.parseFrom(pFileData, (int)numBytes) == PARSE_EXIF_SUCCESS)
      {
        if (result.GeoLocation.Latitude != 0.0 || result.GeoLocation.Longitude != 0.0)
        {
          hasLocation = true;
          geolocation.x = result.GeoLocation.Latitude;
          geolocation.y = result.GeoLocation.Longitude;
          geolocation.z = result.GeoLocation.Altitude;
        }

        if (result.XMPMetadata != "")
        {
          udJSON xmp;
          if (xmp.Parse(result.XMPMetadata.c_str()) == udR_Success)
          {
            bool isPanorama = xmp.Get("x:xmpmeta.rdf:RDF.rdf:Description.xmlns:GPano").IsString();
            bool isPhotosphere = xmp.Get("x:xmpmeta.rdf:RDF.rdf:Description.GPano:IsPhotosphere").AsBool();

            if (isPanorama && isPhotosphere)
              imageType = vcIT_PhotoSphere;
            else if (isPanorama)
              imageType = vcIT_Panorama;
          }
        }
      }
    }

    // TODO: (EVC-513) Generate a thumbnail
    {
      int width, height;
      int comp;
      stbi_uc *pImgPixels = stbi_load_from_memory((stbi_uc*)pFileData, (int)numBytes, &width, &height, &comp, 4);
      if (!pImgPixels)
      {
        // TODO: (EVC-517) Image failed to load, display error image
      }

      // TODO: (EVC-515) Mip maps are broken in directX
      vcTexture_Create(&pImage, width, height, pImgPixels, vcTextureFormat_RGBA8, vcTFM_Linear, false);

      stbi_image_free(pImgPixels);
    }

    udFree(pFileData);
  }
  else
  {
    // TODO: (EVC-517) File failed to load, display error image
  }

  m_pImage->ypr = udDouble3::zero();
  m_pImage->scale = udDouble3::one();
  m_pImage->pTexture = pImage;
  m_pImage->colour = udFloat4::create(1.0f, 1.0f, 1.0f, 1.0f);
  m_pImage->size = vcIS_Large;
  m_pImage->type = vcIT_StandardPhoto;


}

void vcMedia::AddToScene(vcState * /*pProgramState*/, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_pImage != nullptr)
  {
    m_pImage->position = m_worldPosition;

    // For now brute force sorting (n^2)
    double distToCameraSqr = udMagSq3(m_pImage->position - pRenderData->pCamera->position);
    size_t i = 0;
    for (; i < pRenderData->images.length; ++i)
    {
      if (udMagSq3(pRenderData->images[i]->position - pRenderData->pCamera->position) < distToCameraSqr)
        break;
    }
    pRenderData->images.Insert(i, &m_pImage);
  }
}

void vcMedia::ApplyDelta(vcState * /*pProgramState*/, const udDouble4x4 &delta)
{
  if (m_pImage != nullptr)
  {
    udDouble4x4 resultMatrix = delta * udDouble4x4::rotationYPR(m_pImage->ypr) * udDouble4x4::scaleNonUniform(m_pImage->scale);
    udDouble3 position, scale;
    udQuaternion<double> rotation;
    udExtractTransform(resultMatrix, position, scale, rotation);

    udUnused(position);
    m_pImage->ypr = udMath_DirToYPR(rotation.apply(udDouble3::create(0, 1, 0)));
    m_pImage->scale = scale;
  }
}

void vcMedia::HandleImGui(vcState *pProgramState, size_t *pItemID)
{
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

  // Handle imageurl
  const char *pImageURL = m_metadata.Get("imageurl").AsString();
  if (pImageURL != nullptr)
  {
    ImGui::TextWrapped("%s: %s", vcString::Get("scenePOILabelImageURL"), pImageURL);
    if (ImGui::Button(vcString::Get("scenePOILabelOpenImageURL")))
    {
      pProgramState->pLoadImage = udStrdup(pImageURL);
    }

    const char *imageTypeNames[] = { vcString::Get("scenePOILabelImageTypeStandard"), vcString::Get("scenePOILabelImageTypePanorama"), vcString::Get("scenePOILabelImageTypePhotosphere") };
    ImGui::Combo(udTempStr("%s##scenePOILabelImageType%zu", vcString::Get("scenePOILabelImageType"), *pItemID), (int*)&m_pImage->type, imageTypeNames, (int)udLengthOf(imageTypeNames));
  }
}

void vcMedia::ChangeProjection(const udGeoZone &newZone)
{
  if (m_pCurrentProjection == nullptr)
    m_pCurrentProjection = udAllocType(udGeoZone, 1, udAF_Zero);

  // Change all points in the POI to the new projection
  m_worldPosition = udGeoZone_ToCartesian(newZone, ((udDouble3*)m_pNode->pCoordinates)[0], true);

  // Call the parent version
  vcSceneItem::ChangeProjection(newZone);
}

void vcMedia::Cleanup(vcState * /*pProgramState*/)
{
  if (m_pImage)
  {
    vcTexture_Destroy(&m_pImage->pTexture);
    udFree(m_pImage);
  }
}

void vcMedia::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->pCamera->position = m_worldPosition;
}

udDouble4x4 vcMedia::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_worldPosition);
}
