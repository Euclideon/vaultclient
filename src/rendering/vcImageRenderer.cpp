#include "vcImageRenderer.h"

#include "vcMesh.h"
#include "vcInternalModels.h"

#include "udPlatformUtil.h"

#include "imgui.h"

// These values were picked by visual inspection
static const float vcISToPixelSize[] = { -1.f, 100.f, 250.f };
UDCOMPILEASSERT(udLengthOf(vcISToPixelSize) == vcIS_Count, "ImagePixelSize not equal size");

static vcInternalMeshType vcITToMeshType[] = { vcInternalMeshType_Billboard, vcInternalMeshType_WorldQuad, vcInternalMeshType_Tube, vcInternalMeshType_Sphere, vcInternalMeshType_Billboard };
UDCOMPILEASSERT(udLengthOf(vcITToMeshType) == vcIT_Count, "ImageMesh does not equal size");

static int vcITToShaderIndex[] = { 0, 1, 1, 1, 2 };
UDCOMPILEASSERT(udLengthOf(vcITToMeshType) == vcIT_Count, "ImageMesh does not equal index size");

static struct vcImageShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_worldViewProjectionMatrix;
    udFloat4 u_colour;
    udFloat4 u_screenSize;
  } everyObject;
} gShaders[3];

static int gRefCount = 0;
udResult vcImageRenderer_Init()
{
  udResult result;
  gRefCount++;

  UD_ERROR_IF(gRefCount != 1, udR_Success);

  UD_ERROR_IF(!vcShader_CreateFromFile(&gShaders[0].pShader, "asset://assets/shaders/imageRendererBillboardVertexShader", "asset://assets/shaders/imageRendererFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&gShaders[1].pShader, "asset://assets/shaders/imageRendererMeshVertexShader", "asset://assets/shaders/imageRendererFragmentShader", vcP3N3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromFile(&gShaders[2].pShader, "asset://assets/shaders/imageRendererScreenVertexShader", "asset://assets/shaders/imageRendererScreenFragmentShader", vcP3UV2VertexLayout), udR_InternalError);

  for (size_t i = 0; i < udLengthOf(gShaders); ++i)
  {
    UD_ERROR_IF(!vcShader_GetConstantBuffer(&gShaders[i].pEveryObjectConstantBuffer, gShaders[i].pShader, "u_EveryObject", sizeof(gShaders[i].everyObject)), udR_InternalError);
    UD_ERROR_IF(!vcShader_GetSamplerIndex(&gShaders[i].pDiffuseSampler, gShaders[i].pShader, "albedo"), udR_InternalError);
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

  for (size_t i = 0; i < udLengthOf(gShaders); ++i)
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

  udDouble4x4 mvp = viewProjectionMatrix * udDouble4x4::translation(pImageInfo->position) * udDouble4x4::rotationYPR(pImageInfo->ypr);
  if (pImageInfo->type == vcIT_StandardPhoto)
  {
    // scale acts as screen scale
    imageSize.x = (int)(imageSize.x * pImageInfo->scale);
    imageSize.y = (int)(imageSize.y * pImageInfo->scale);
  }
  else
  {
    // scale is world scale
    mvp = mvp * udDouble4x4::scaleUniform(pImageInfo->scale); 
  }

  vcImageShader *pShader = &gShaders[vcITToShaderIndex[pImageInfo->type]];
  vcShader_Bind(pShader->pShader);

  pShader->everyObject.u_worldViewProjectionMatrix = udFloat4x4::create(mvp);
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
