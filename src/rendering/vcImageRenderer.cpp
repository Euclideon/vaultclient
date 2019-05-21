#include "vcImageRenderer.h"

#include "gl/vcMesh.h"
#include "gl/vcRenderShaders.h"
#include "vcInternalModels.h"

#include "udPlatformUtil.h"

#include "imgui.h"

// These values were picked by visual inspection
static const float vcISToPixelSize[] = { 100.0, 150.0 };
UDCOMPILEASSERT(udLengthOf(vcISToPixelSize) == vcIS_Count, "ImagePixelSize not equal size");

static const float vcISToWorldSize[] = { 3.0, 10.0 };
UDCOMPILEASSERT(udLengthOf(vcISToWorldSize) == vcIS_Count, "ImageWorldSize not equal size");

static vcMesh *vcITToMesh[] = { nullptr, nullptr, nullptr };
UDCOMPILEASSERT(udLengthOf(vcITToMesh) == vcIT_Count, "ImageMesh does not equal size");

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
void vcImageRenderer_Init()
{
  gRefCount++;
  if (gRefCount == 1)
  {
    vcMesh_Create(&vcITToMesh[vcIT_StandardPhoto], vcP1UV1VertexLayout, (int)udLengthOf(vcP1UV1VertexLayout), billboardVertices, (int)udLengthOf(billboardVertices), billboardIndices, (int)udLengthOf(billboardIndices), vcMF_Dynamic | vcMF_IndexShort);
    vcMesh_Create(&vcITToMesh[vcIT_PhotoSphere], vcP1UV1VertexLayout, (int)udLengthOf(vcP1UV1VertexLayout), pSphereVertices, (int)udLengthOf(sphereVerticesFltArray), sphereIndices, (int)udLengthOf(sphereIndices), vcMF_Dynamic | vcMF_IndexShort);
    vcMesh_Create(&vcITToMesh[vcIT_Panorama], vcP1UV1VertexLayout, (int)udLengthOf(vcP1UV1VertexLayout), pTubeVertices, (int)udLengthOf(tubeVerticesFltArray), tubeIndices, (int)udLengthOf(tubeIndices), vcMF_Dynamic | vcMF_IndexShort);

    vcShader_CreateFromText(&gShaders[vcIT_StandardPhoto].pShader, g_BillboardVertexShader, g_BillboardFragmentShader, vcP1UV1VertexLayout);
    vcShader_CreateFromText(&gShaders[vcIT_PhotoSphere].pShader, g_PolygonP1UV1VertexShader, g_PolygonP1UV1FragmentShader, vcP1UV1VertexLayout);
    vcShader_CreateFromText(&gShaders[vcIT_Panorama].pShader, g_PolygonP1UV1VertexShader, g_PolygonP1UV1FragmentShader, vcP1UV1VertexLayout);

    for (int i = 0; i < vcIT_Count; ++i)
    {
      vcShader_GetConstantBuffer(&gShaders[i].pEveryObjectConstantBuffer, gShaders[i].pShader, "u_EveryObject", sizeof(gShaders[i].everyObject));
      vcShader_GetSamplerIndex(&gShaders[i].pDiffuseSampler, gShaders[i].pShader, "u_texture");
    }
  }
}

void vcImageRenderer_Destroy()
{
  --gRefCount;
  if (gRefCount == 0)
  {
    for (int i = 0; i < vcIT_Count; ++i)
      vcMesh_Destroy(&vcITToMesh[i]);

    for (int i = 0; i < vcIT_Count; ++i)
    {
      vcShader_DestroyShader(&gShaders[i].pShader);
      vcShader_ReleaseConstantBuffer(gShaders[i].pShader, gShaders[i].pEveryObjectConstantBuffer);
    }
  }
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
  pShader->everyObject.u_screenSize = udFloat4::create(vcISToPixelSize[pImageInfo->size] / screenSize.x, vcISToPixelSize[pImageInfo->size] / screenSize.y * aspect, float(zScale), 0.0f);

  vcShader_BindConstantBuffer(pShader->pShader, pShader->pEveryObjectConstantBuffer, &pShader->everyObject, sizeof(pShader->everyObject));
  vcShader_BindTexture(pShader->pShader, pImageInfo->pTexture, 0, pShader->pDiffuseSampler);

  vcMesh_Render(vcITToMesh[pImageInfo->type]);

  return true;
}
