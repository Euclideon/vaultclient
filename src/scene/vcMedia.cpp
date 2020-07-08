#include "vcMedia.h"

#include "vcState.h"
#include "vcRender.h"
#include "vcStrings.h"

#include "vcImageRenderer.h"
#include "vcPolygonModel.h"
#include "vcInternalModels.h"

#include "udMath.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include "exif.h"

#include "stb_image.h"

struct vcMediaLoadInfo
{
  vcState *pProgramState;
  vcMedia *pMedia;
};

const char *s_imageTypes[] = { "standard", "oriented", "panorama", "photosphere", "screen" };
UDCOMPILEASSERT(udLengthOf(s_imageTypes) == vcIT_Count, "Update Image Types");

const char *s_imageThumbnailSizes[] = { "native", "small", "large" };
UDCOMPILEASSERT(udLengthOf(s_imageThumbnailSizes) == vcIS_Count, "Update Image Sizes");

vcMedia::vcMedia(vcProject *pProject, vdkProjectNode *pNode, vcState *pProgramState) :
  vcSceneItem(pProject, pNode, pProgramState),
  m_pLoadedURI(nullptr),
  m_loadLoadTimeSec(0.0),
  m_reloadTimeSecs(0.0),
  m_imageDataSize(0)
 {
  memset(&m_image, 0, sizeof(m_image));
  m_loadStatus = vcSLS_Loaded;
  m_pImageData = nullptr;

  OnNodeUpdate(pProgramState);
}

vcMedia::~vcMedia()
{
  vcTexture_Destroy(&m_image.pTexture);
}

void vcMedia_LoadImage(void* pMediaPtr)
{
  vcMediaLoadInfo *pData = (vcMediaLoadInfo *)pMediaPtr;

  int32_t status = udInterlockedCompareExchange(&pData->pMedia->m_loadStatus, vcSLS_Loading, vcSLS_Pending);

  void *pFileData = nullptr;
  int64_t fileLen = 0;

  if (status == vcSLS_Pending)
  {
    udResult result = udFile_Load(pData->pMedia->m_pLoadedURI, &pFileData, &fileLen);

    if (result == udR_OpenFailure)
      result = udFile_Load(udTempStr("%s%s", pData->pProgramState->activeProject.pRelativeBase, pData->pMedia->m_pLoadedURI), &pFileData, &fileLen);

    if (result == udR_Success)
    {
      pData->pMedia->SetImageData(&pFileData, fileLen); // Ownership is passed and pFileData should be null
      pData->pMedia->m_loadStatus = vcSLS_Loaded;
    }
    else if (result == udR_OpenFailure)
    {
      pData->pMedia->m_loadStatus = vcSLS_OpenFailure;
    }
    else
    {
      pData->pMedia->m_loadStatus = vcSLS_Failed;
    }

    udFree(pFileData); // Should always be nullptr by this time
  }
}

void vcMedia::OnNodeUpdate(vcState *pProgramState)
{
  if (m_pNode->pURI == nullptr)
  {
    m_loadStatus = vcSLS_Failed;
    vcTexture_Destroy(&m_image.pTexture);
  }
  else if ((m_loadStatus == vcSLS_Loaded && !udStrEqual(m_pLoadedURI, m_pNode->pURI)) || (m_reloadTimeSecs != 0.0 && m_loadLoadTimeSec + m_reloadTimeSecs < udGetEpochSecsUTCf()) || (m_loadStatus == vcSLS_Unloaded))
  {
    m_loadStatus = vcSLS_Pending;
    udFree(m_pLoadedURI);
    m_pLoadedURI = udStrdup(m_pNode->pURI);

    vcMediaLoadInfo *pMedia = udAllocType(vcMediaLoadInfo, 1, udAF_Zero);
    pMedia->pMedia = this;
    pMedia->pProgramState = pProgramState;
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcMedia_LoadImage, pMedia, true);
    m_loadLoadTimeSec = udGetEpochSecsUTCf();
  }

  vdkProjectNode_GetMetadataDouble(m_pNode, "reloadRate", &m_reloadTimeSecs, 0.0);

  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.y", &m_image.ypr.x, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.p", &m_image.ypr.y, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.rotation.r", &m_image.ypr.z, 0.0);
  vdkProjectNode_GetMetadataDouble(m_pNode, "transform.scale", &m_image.scale, 1.0);

  m_image.colour = udFloat4::create(1.0f, 1.0f, 1.0f, 1.0f);
  m_image.size = vcIS_Large;
  m_image.type = vcIT_StandardPhoto;

  const char *pImageSize = nullptr;
  if (vdkProjectNode_GetMetadataString(m_pNode, "imagesize", &pImageSize, nullptr) == vE_Success)
  {
    for (size_t i = 0; i < udLengthOf(s_imageThumbnailSizes); ++i)
    {
      if (udStrEquali(pImageSize, s_imageThumbnailSizes[i]))
        m_image.size = (vcImageThumbnailSize)i;
    }
  }

  const char *pImageType = nullptr;
  if (vdkProjectNode_GetMetadataString(m_pNode, "imagetype", &pImageType, nullptr) == vE_Success)
  {
    for (size_t i = 0; i < udLengthOf(s_imageTypes); ++i)
    {
      if (udStrEquali(pImageType, s_imageTypes[i]))
        m_image.type = (vcImageType)i;
    }
  }

  ChangeProjection(pProgramState->geozone);
}

void vcMedia::AddToScene(vcState *pProgramState, vcRenderData *pRenderData)
{
  if (!m_visible)
    return;

  if (m_loadStatus == vcSLS_Loaded)
  {
    udInt2 imageSize = {};
    if (m_pImageData != nullptr)
    {
      uint32_t width, height, channelCount;

      uint8_t *pData = stbi_load_from_memory((stbi_uc *)m_pImageData, (int)m_imageDataSize, (int *)& width, (int *)& height, (int *)& channelCount, 4);
      udFree(m_pImageData);
      m_imageDataSize = 0;

      if (pData != nullptr)
      {
        if (m_image.pTexture != nullptr)
          vcTexture_GetSize(m_image.pTexture, &imageSize.x, &imageSize.y);

        if ((uint32_t)imageSize.x == width && (uint32_t)imageSize.y == height) // imageSize will not == for null pTexture
        {
          vcTexture_UploadPixels(m_image.pTexture, pData, width, height);
        }
        else
        {
          vcTexture_Destroy(&m_image.pTexture);

          if (vcTexture_Create(&m_image.pTexture, width, height, pData, vcTextureFormat_RGBA8, vcTFM_Linear, vcTCF_Dynamic) != udR_Success)
            m_loadStatus = vcSLS_Failed;
        }

        stbi_image_free(pData);
      }
      else
      {
        m_loadStatus = vcSLS_Failed;
      }
    }
  }

  if (m_image.pTexture != nullptr)
  {
    if (m_image.type == vcIT_StandardPhoto || m_image.type == vcIT_ScreenPhoto)
    {
      // For now brute force sorting (n^2)
      double distToCameraSqr = udMagSq3(m_image.position - pProgramState->camera.position);
      size_t i = 0;
      for (; i < pRenderData->images.length; ++i)
      {
        if (udMagSq3(pRenderData->images[i]->position - pProgramState->camera.position) < distToCameraSqr)
          break;
      }

      vcImageRenderInfo *pImageInfo = &m_image;
      pImageInfo->pSceneItem = this;
      pRenderData->images.Insert(i, &pImageInfo);
    }
    else
    {
      udInt2 imageSize = {};
      vcTexture_GetSize(m_image.pTexture, &imageSize.x, &imageSize.y);

      vcRenderPolyInstance *pPoly = pRenderData->polyModels.PushBack();
      pPoly->worldMat = udDouble4x4::rotationYPR(m_image.ypr, m_image.position) * udDouble4x4::scaleUniform(m_image.scale);
      pPoly->renderType = vcRenderPolyInstance::RenderType_Polygon;
      pPoly->renderFlags = vcRenderPolyInstance::RenderFlags_Transparent; // so atmo doesn't affect it
      pPoly->pSceneItem = this;
      pPoly->pDiffuseOverride = m_image.pTexture;
      pPoly->selectable = true;
      pPoly->tint = udFloat4::one();

      float aspect = 1.0f;
      vcTexture_GetSize(m_image.pTexture, &imageSize.x, &imageSize.y);
      aspect = float(imageSize.y) / imageSize.x;

      double worldScale = 1.0;
      if (m_image.size == vcIS_Native)
        worldScale = (double)imageSize.x / pProgramState->sceneResolution.x;
      else
        worldScale = vcISToWorldSize[m_image.size];

      if (m_image.type == vcIT_PhotoSphere)
      {
        pPoly->pModel = gInternalModels[vcInternalModelType_Sphere];
        pPoly->worldMat *= udDouble4x4::scaleUniform(worldScale);
        pPoly->cullFace = vcGLSCM_Front;
      }
      else if (m_image.type == vcIT_Panorama)
      {
        pPoly->pModel = gInternalModels[vcInternalModelType_Tube];
        pPoly->worldMat *= udDouble4x4::scaleNonUniform(worldScale, worldScale, worldScale * aspect * UD_PI);
        pPoly->cullFace = vcGLSCM_Front;
      }
      else if (m_image.type == vcIT_OrientedPhoto)
      {
        pPoly->pModel = gInternalModels[vcInternalModelType_Quad];
        pPoly->worldMat *= udDouble4x4::scaleNonUniform(worldScale, worldScale, worldScale * aspect) * udDouble4x4::rotationZ(UD_PI);
        pPoly->cullFace = vcGLSCM_Front;
      } // TODO: Billboards, this renders at the correct relative scale and everything, but looks odd when actually rendered into the scene
      /*else if (m_image.type == vcIT_StandardPhoto)
      {
        udDouble3 baseScale = udDouble3::create(worldScale, worldScale, worldScale * aspect);
        udDouble3 distanceVector = m_image.position - pProgramState->pCamera->position;
        double distance = udMag3(distanceVector);

        pPoly->pModel = gInternalModels[vcInternalModelType_Quad];
        pPoly->worldMat *= udDouble4x4::scaleNonUniform(baseScale * (distance / 25)) * udDouble4x4::rotationYPR(udDirectionToYPR(distanceVector));
        pPoly->insideOut = true;
      }*/
    }
  }

  if (m_reloadTimeSecs != 0.0 && m_loadLoadTimeSec + m_reloadTimeSecs < udGetEpochSecsUTCf())
    OnNodeUpdate(pProgramState);
}

void vcMedia::ApplyDelta(vcState *pProgramState, const udDouble4x4 &delta)
{
  udDouble4x4 resultMatrix = delta * udDouble4x4::translation(m_image.position) * udDouble4x4::rotationYPR(m_image.ypr) * udDouble4x4::scaleUniform(m_image.scale);
  udDouble3 position, scale;
  udQuaternion<double> rotation;
  resultMatrix.extractTransforms(position, scale, rotation);

  m_image.position = position;
  m_image.ypr = rotation.eulerAngles();
  m_image.scale = scale.x;

  vcProject_UpdateNodeGeometryFromCartesian(m_pProject, m_pNode, pProgramState->geozone, vdkPGT_Point, &m_image.position, 1);

  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_image.ypr.x);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_image.ypr.y);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", m_image.ypr.z);
  vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale", m_image.scale);
}

void vcMedia::HandleSceneExplorerUI(vcState *pProgramState, size_t *pItemID)
{
  // Handle imageurl
  if (m_pNode->pURI != nullptr)
  {
    if (pProgramState->settings.presentation.showDiagnosticInfo)
      ImGui::TextWrapped("%s: %s", vcString::Get("scenePOILabelImageURL"), m_pNode->pURI);

    const char *imageTypeNames[] = { vcString::Get("scenePOILabelImageTypeStandard"), vcString::Get("scenePOILabelImageTypeOriented"), vcString::Get("scenePOILabelImageTypePanorama"), vcString::Get("scenePOILabelImageTypePhotosphere"), vcString::Get("scenePOILabelImageTypeScreen") };
    UDCOMPILEASSERT(udLengthOf(imageTypeNames) == vcIT_Count, "Update image names");
    if (ImGui::Combo(udTempStr("%s##scenePOILabelImageType%zu", vcString::Get("scenePOILabelImageType"), *pItemID), (int *)&m_image.type, imageTypeNames, (int)udLengthOf(imageTypeNames)))
      vdkProjectNode_SetMetadataString(m_pNode, "imagetype", s_imageTypes[(int)m_image.type]);

    const char *imageThumbnailSizeNames[] = { vcString::Get("scenePOIThumbnailSizeNative"), vcString::Get("scenePOIThumbnailSizeSmall"), vcString::Get("scenePOIThumbnailSizeLarge") };
    if (ImGui::Combo(udTempStr("%s##scenePOIThumbnailSize%zu", vcString::Get("scenePOIThumbnailSize"), *pItemID), (int *)&m_image.size, imageThumbnailSizeNames, (int)udLengthOf(imageThumbnailSizeNames)))
      vdkProjectNode_SetMetadataString(m_pNode, "imagesize", s_imageThumbnailSizes[(int)m_image.size]);
  }

  if (m_image.type != vcIT_StandardPhoto)
  {
    if (ImGui::InputScalarN(udTempStr("%s##scenePOILabelImageYPR%zu", vcString::Get("scenePOILabelImageYPR"), *pItemID), ImGuiDataType_Double, &m_image.ypr, 3))
    {
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.y", m_image.ypr.x);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.p", m_image.ypr.y);
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.rotation.r", m_image.ypr.z);
    }

    if (ImGui::InputDouble(udTempStr("%s##scenePOILabelImageYPR%zu", vcString::Get("scenePOILabelImageYPR"), *pItemID), &m_image.scale))
      vdkProjectNode_SetMetadataDouble(m_pNode, "transform.scale", m_image.scale);
  }

  double reloadTimeMin = 0;
  double reloadTimeMax = 3600; // Hourly
  if (ImGui::SliderScalar(udTempStr("%s##scenePOILabelImageType%zu", vcString::Get("scenePOIReloadTime"), *pItemID), ImGuiDataType_Double, &m_reloadTimeSecs, &reloadTimeMin, &reloadTimeMax))
  {
    m_reloadTimeSecs = udClamp(m_reloadTimeSecs, reloadTimeMin, reloadTimeMax);
    vdkProjectNode_SetMetadataDouble(m_pNode, "reloadRate", m_reloadTimeSecs);
  }
}

void vcMedia::HandleSceneEmbeddedUI(vcState * /*pProgramState*/)
{
  const char *imageTypeNames[] = {vcString::Get("scenePOILabelImageTypeStandard"), vcString::Get("scenePOILabelImageTypeOriented"), vcString::Get("scenePOILabelImageTypePanorama"), vcString::Get("scenePOILabelImageTypePhotosphere"), vcString::Get("scenePOILabelImageTypeScreen")};
  UDCOMPILEASSERT(udLengthOf(imageTypeNames) == vcIT_Count, "Update image names");
  if (ImGui::Combo(udTempStr("%s##ImageEmbeddedUI", vcString::Get("scenePOILabelImageType")), (int *)&m_image.type, imageTypeNames, (int)udLengthOf(imageTypeNames)))
    vdkProjectNode_SetMetadataString(m_pNode, "imagetype", s_imageTypes[(int)m_image.type]);
}

void vcMedia::HandleContextMenu(vcState *pProgramState)
{
  ImGui::Separator();

  if (ImGui::MenuItem(vcString::Get("scenePOILabelOpenImageURL")))
    pProgramState->pLoadImage = udStrdup(m_pNode->pURI);
}

void vcMedia::ChangeProjection(const udGeoZone &newZone)
{
  udDouble3 *pPoint = nullptr;
  int numPoints = 0;

  vcProject_FetchNodeGeometryAsCartesian(m_pProject, m_pNode, newZone, &pPoint, &numPoints);
  if (numPoints == 1)
    m_image.position = pPoint[0];

  udFree(pPoint);
}

void vcMedia::Cleanup(vcState * /*pProgramState*/)
{
  udFree(m_pLoadedURI);
  udFree(m_pImageData);
}

void vcMedia::SetCameraPosition(vcState *pProgramState)
{
  pProgramState->camera.position = m_image.position;
}

udDouble4x4 vcMedia::GetWorldSpaceMatrix()
{
  return udDouble4x4::translation(m_image.position);
}

void vcMedia::SetImageData(void **ppImageData, int64_t imageDataSize)
{
  // Don't do the actual loading here, this gets called from a (single) worker thread
  if (m_pImageData == nullptr)
  {
    m_pImageData = *ppImageData;
    *ppImageData = nullptr;
    m_imageDataSize = imageDataSize;
  }
}

vcGizmoAllowedControls vcMedia::GetAllowedControls()
{
  return (vcGizmoAllowedControls)(vcGAC_ScaleUniform | vcGAC_Translation | vcGAC_Rotation);
}
