#include "vcImageRenderer.h"

#include "gl/vcMesh.h"
#include "gl/vcRenderShaders.h"
#include "vcInternalModels.h"

#include "udPlatformUtil.h"

#include "imgui.h"

// These values were picked by visual inspection
static const float vcISToPixelSize[] = { -1.f, 100.f, 250.f };
UDCOMPILEASSERT(udLengthOf(vcISToPixelSize) == vcIS_Count, "ImagePixelSize not equal size");

static const float vcISToWorldSize[] = { -1.f, 3.f, 10.f };
UDCOMPILEASSERT(udLengthOf(vcISToWorldSize) == vcIS_Count, "ImageWorldSize not equal size");

static vcInternalMeshType vcITToMeshType[] = { vcInternalMeshType_Billboard, vcInternalMeshType_Tube, vcInternalMeshType_Sphere };
UDCOMPILEASSERT(udLengthOf(vcITToMeshType) == vcIT_Count, "ImageMesh does not equal size");

static struct vcImageShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_modelViewProjectionMatrix;
    udFloat4 u_colour;
    udFloat4 u_screenSize;
  } everyObject;
} gShaders[vcIT_Count];

static int gRefCount = 0;
udResult vcImageRenderer_Init()
{
  udResult result;
  gRefCount++;

  UD_ERROR_IF(gRefCount != 1, udR_Success);

  UD_ERROR_IF(!vcShader_CreateFromText(&gShaders[vcIT_StandardPhoto].pShader, g_ImageRendererBillboardVertexShader, g_ImageRendererFragmentShader, vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromText(&gShaders[vcIT_PhotoSphere].pShader, g_ImageRendererMeshVertexShader, g_ImageRendererFragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromText(&gShaders[vcIT_Panorama].pShader, g_ImageRendererMeshVertexShader, g_ImageRendererFragmentShader, vcP3N3UV2VertexLayout), udR_InternalError);

  for (int i = 0; i < vcIT_Count; ++i)
  {
    UD_ERROR_IF(!vcShader_GetConstantBuffer(&gShaders[i].pEveryObjectConstantBuffer, gShaders[i].pShader, "u_EveryObject", sizeof(gShaders[i].everyObject)), udR_InternalError);
    UD_ERROR_IF(!vcShader_GetSamplerIndex(&gShaders[i].pDiffuseSampler, gShaders[i].pShader, "u_texture"), udR_InternalError);
  }

  result = udR_Success;
epilogue:

  if (result != udR_Success)
    vcImageRenderer_Destroy();

  return result;
}

udResult vcImageRenderer_Destroy()
{
  udResult result;
  --gRefCount;

  UD_ERROR_IF(gRefCount != 0, udR_Success);

  for (int i = 0; i < vcIT_Count; ++i)
  {
    UD_ERROR_IF(!vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryObjectConstantBuffer), udR_InternalError);
    vcShader_DestroyShader(&gShaders[i].pShader);
  }

  result = udR_Success;
epilogue:

  return result;
}

bool vcImageRenderer_Render(vcImageRenderInfo *pImageInfo, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, double zScale)
{
  // get aspect ratio
  udInt2 imageSize = {};
  float aspect = 1.0f;
  vcTexture_GetSize(pImageInfo->pTexture, &imageSize.x, &imageSize.y);
  aspect = float(imageSize.y) / imageSize.x;

  double worldScale = vcISToWorldSize[pImageInfo->size];
  udDouble4x4 mvp = viewProjectionMatrix * udDouble4x4::translation(pImageInfo->position) * udDouble4x4::rotationYPR(pImageInfo->ypr) * udDouble4x4::scaleNonUniform(pImageInfo->scale);
  if (pImageInfo->type == vcIT_Panorama)
    mvp = mvp * udDouble4x4::scaleNonUniform(worldScale, worldScale, worldScale * aspect * UD_PI);
  else if (pImageInfo->type == vcIT_PhotoSphere)
    mvp = mvp * udDouble4x4::scaleUniform(worldScale);

  vcImageShader *pShader = &gShaders[pImageInfo->type];
  vcShader_Bind(pShader->pShader);

  pShader->everyObject.u_modelViewProjectionMatrix = udFloat4x4::create(mvp);
  pShader->everyObject.u_colour = pImageInfo->colour;

  if (pImageInfo->size == vcIS_Native)
    pShader->everyObject.u_screenSize = udFloat4::create((float)imageSize.x / screenSize.x, (float)imageSize.y / screenSize.y, float(zScale), 0.0f);
  else
    pShader->everyObject.u_screenSize = udFloat4::create(vcISToPixelSize[pImageInfo->size] / screenSize.x, vcISToPixelSize[pImageInfo->size] / screenSize.y * aspect, float(zScale), 0.0f);

  vcShader_BindConstantBuffer(pShader->pShader, pShader->pEveryObjectConstantBuffer, &pShader->everyObject, sizeof(pShader->everyObject));
  vcShader_BindTexture(pShader->pShader, pImageInfo->pTexture, 0, pShader->pDiffuseSampler);

  vcMesh_Render(gInternalMeshes[vcITToMeshType[pImageInfo->type]]);

  return true;
}
