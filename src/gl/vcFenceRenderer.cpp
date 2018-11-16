
#include "gl/vcFenceRenderer.h"
#include "gl/vcMesh.h"
#include "gl/vcRenderShaders.h"
#include "gl/vcTexture.h"
#include "vcSettings.h"

struct vcFenceRenderer
{
  vcMesh *pMesh;
  int vertCount;

  int pointCount;
  udFloat3 *pCachedPoints;

  double totalTimePassed;
  float ribbonWidth;
  vcFenceRendererConfig config;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_params;
    vcShaderSampler *uniform_texture;

    struct
    {
      udFloat4x4 viewProjection;
      udFloat4 bottomColour;
      udFloat4 topColour;
      float orientation; // 0 == fence, 1 == flat
      float width;
      float textureRepeatScale;
      float textureScrollSpeed;
      float time;

      char _padding[12]; // 16 byte alignment
    } params;

  } renderShader;
};

struct vcRibbonVertex
{
  udFloat3 position;
  udFloat2 uv;
  udFloat4 ribbonInfo; // xyz: expand vector; z: pair id (0 or 1)
};
const vcVertexLayoutTypes vcRibbonVertexLayout[] = { vcVLT_Position3, vcVLT_TextureCoords2, vcVLT_RibbonInfo4 };

static int gRefCount = 0;
static vcTexture *gArrowTexture = nullptr;
static vcTexture *gGlowTexture = nullptr;
static vcTexture *gSolidTexture = nullptr;

void vcFenceRenderer_Init()
{
  gRefCount++;
  if (gRefCount == 1)
  {
    vcTexture_CreateFromFilename(&gArrowTexture, ASSETDIR "textures/fenceArrow.png", nullptr, nullptr, vcTFM_Linear, true);
    vcTexture_CreateFromFilename(&gGlowTexture, ASSETDIR "textures/fenceGlow.png", nullptr, nullptr, vcTFM_Linear, true);

    static const uint32_t solidPixels[] = { 0xffffffff };
    vcTexture_Create(&gSolidTexture, 1, 1, solidPixels);
  }
}

void vcFenceRenderer_Destroy()
{
  --gRefCount;
  if (gRefCount == 0)
  {
    vcTexture_Destroy(&gArrowTexture);
    vcTexture_Destroy(&gGlowTexture);
    vcTexture_Destroy(&gSolidTexture);
  }
}


udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer)
{
  udResult result = udR_Success;

  vcFenceRenderer *pFenceRenderer = nullptr;

  UD_ERROR_NULL(ppFenceRenderer, udR_InvalidParameter_);

  pFenceRenderer = udAllocType(vcFenceRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pFenceRenderer, udR_MemoryAllocationFailure);

  // defaults
  pFenceRenderer->config.ribbonWidth = 0.5f;
  pFenceRenderer->config.textureRepeatScale = 1.0f;
  pFenceRenderer->config.textureScrollSpeed = 1.0f;
  pFenceRenderer->config.bottomColour = udFloat4::create(1.0f);
  pFenceRenderer->config.topColour = udFloat4::create(1.0f);
  pFenceRenderer->config.imageMode = vcRRIM_Solid;
  pFenceRenderer->config.visualMode = vcRRVM_Fence;

  UD_ERROR_IF(!vcShader_CreateFromText(&pFenceRenderer->renderShader.pProgram, g_FenceVertexShader, g_FenceFragmentShader, vcRibbonVertexLayout, (int)udLengthOf(vcRibbonVertexLayout)), udR_InternalError);
  vcShader_Bind(pFenceRenderer->renderShader.pProgram);
  vcShader_GetSamplerIndex(&pFenceRenderer->renderShader.uniform_texture, pFenceRenderer->renderShader.pProgram, "u_texture");
  vcShader_GetConstantBuffer(&pFenceRenderer->renderShader.uniform_params, pFenceRenderer->renderShader.pProgram, "u_params", sizeof(pFenceRenderer->renderShader.params));

  *ppFenceRenderer = pFenceRenderer;
  pFenceRenderer = nullptr;

epilogue:

  if (pFenceRenderer == nullptr)
    vcFenceRenderer_Init();

  return result;
}

udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer)
{
  if (ppFenceRenderer == nullptr || *ppFenceRenderer == nullptr)
    return udR_Success;

  vcFenceRenderer *pFenceRenderer = *ppFenceRenderer;
  *ppFenceRenderer = nullptr;

  vcMesh_Destroy(&pFenceRenderer->pMesh);
  vcShader_DestroyShader(&pFenceRenderer->renderShader.pProgram);

  udFree(pFenceRenderer->pCachedPoints);
  udFree(pFenceRenderer);

  vcFenceRenderer_Destroy();

  return udR_Success;
}

udResult vcFenceRenderer_SetConfig(vcFenceRenderer *pFenceRenderer, const vcFenceRendererConfig &config)
{
  udResult result = udR_Success;

  float oldWidth = pFenceRenderer->ribbonWidth;
  pFenceRenderer->config = config;

  // need to rebuild verts on width change
  if (pFenceRenderer->pointCount > 0 && ((oldWidth - pFenceRenderer->config.ribbonWidth) > UD_EPSILON))
  {
    UD_ERROR_SET(vcFenceRenderer_SetPoints(pFenceRenderer, pFenceRenderer->pCachedPoints, pFenceRenderer->pointCount));
  }

epilogue:
  return result;
}

udFloat3 vcFenceRenderer_CreateEndJointExpandVector(const udFloat3 &previous, const udFloat3 &center, float width)
{
  return udCross(udNormalize(center - previous), udFloat3::create(0, 0, 1)) * width * 0.5f;
}

udFloat3 vcFenceRenderer_CreateStartJointExpandVector(const udFloat3 &center, const udFloat3 &next, float width)
{
  return udCross(udNormalize(next - center), udFloat3::create(0, 0, 1)) * width * 0.5f;
}

udFloat3 vcFenceRenderer_CreateSegmentJointExpandVector(const udFloat3 &previous, const udFloat3 &center, const udFloat3 &next, float width, bool *pJointFlipped)
{
  udFloat3 v1 = udNormalize(center - previous);
  udFloat3 v2 = udNormalize(next - center);
  float d = udDot(v1, v2);
  float theta = udACos(d);

  // limit angle to something reasonable
  theta = udMin(2.85f, theta);

  // determine sign of angle
  udFloat3 cross = udCross(v1, v2);
  *pJointFlipped = udDot(udFloat3::create(0, 0, 1), cross) < 0;
  if (*pJointFlipped)
    theta *= -1;

  if (d == 1.0f) // straight edge case
  {
    udFloat3 up = udFloat3::create(0, 0, 1);
    udFloat3 right = udCross(v1, up);
    return right * 0.5f * width;
  }

  float u = width / (2.0f * udSin(theta));
  float v = width / (2.0f * udSin(theta));

  udFloat3 u0 = ((previous - center) / udMag(previous - center)) * u;
  udFloat3 v0 = ((next - center) / udMag(next - center)) * v;

  return -(u0 + v0);
}

udResult vcFenceRenderer_SetPoints(vcFenceRenderer *pFenceRenderer, udFloat3 *pPoints, int pointCount)
{
  udResult result = udR_Success;

  int layoutCount = (int)UDARRAYSIZE(vcRibbonVertexLayout);
  float widthSquared = pFenceRenderer->ribbonWidth * pFenceRenderer->ribbonWidth;
  vcRibbonVertex *pVerts = nullptr;
  float uvFenceOffset = 0;
  int vertIndex = 0;
  bool jointFlipped = false;
  float currentUV = 0;
  float previousDist0 = 0;
  float previousDist1 = 0;
  udFloat3 p0 = udFloat3::zero();
  float uv0 = 0.0f;
  float uv1 = 0.0f;
  float maxUV = 0.0f;
  int vertCount = 0;

  UD_ERROR_IF(pointCount < 2, udR_InvalidParameter_);

  if (pFenceRenderer->pointCount != pointCount)
  {
    udFree(pFenceRenderer->pCachedPoints);
    pFenceRenderer->pCachedPoints = udAllocType(udFloat3, pointCount, udAF_Zero);
    UD_ERROR_NULL(pFenceRenderer->pCachedPoints, udR_MemoryAllocationFailure);
  }

  pFenceRenderer->pointCount = pointCount;
  memcpy(pFenceRenderer->pCachedPoints, pPoints, sizeof(udFloat3) * pointCount);

  vertCount = pointCount * 2 + udMax(pointCount - 2, 0) * 2;
  pVerts = udAllocType(vcRibbonVertex, vertCount, udAF_Zero);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);
  pFenceRenderer->vertCount = vertCount;

  for (int i = 0; i < pointCount; ++i)
  {
    if (i > 0)
      uvFenceOffset += udMag3(pPoints[i] - pPoints[i - 1]);

    pVerts[vertIndex + 0].position = pPoints[i];
    pVerts[vertIndex + 0].uv = udFloat2::create(0, uvFenceOffset);

    pVerts[vertIndex + 1].position = pPoints[i];
    pVerts[vertIndex + 1].uv = udFloat2::create(0, uvFenceOffset);

    vertIndex += 2;

    if (i > 0 && i < pointCount - 1) // middle segments
    {
      pVerts[vertIndex + 0].position = pPoints[i];
      pVerts[vertIndex + 0].uv = udFloat2::create(0, uvFenceOffset);

      pVerts[vertIndex + 1].position = pPoints[i];
      pVerts[vertIndex + 1].uv = udFloat2::create(0, uvFenceOffset);

      vertIndex += 2;
    }
  }

  vertIndex = 0;

  // start segment
  p0 = vcFenceRenderer_CreateStartJointExpandVector(pPoints[0], pPoints[1], pFenceRenderer->ribbonWidth);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(p0, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-p0, 1.0f);
  vertIndex += 2;

  // middle segments
  for (int i = 1; i < pointCount - 1; ++i)
  {
    p0 = vcFenceRenderer_CreateSegmentJointExpandVector(pPoints[i - 1], pPoints[i], pPoints[i + 1], pFenceRenderer->ribbonWidth, &jointFlipped);

    udFloat3 pLeft = pPoints[i] + p0;
    udFloat3 pRight = pPoints[i] - p0;

    udFloat3 pPrevLeft = pPoints[i - 1] + pVerts[vertIndex - 2].ribbonInfo.toVector3();
    udFloat3 pPrevRight = pPoints[i - 1] + pVerts[vertIndex - 1].ribbonInfo.toVector3();

    // distance between (expanded) verts
    uv0 = udMag3(pLeft - pPrevLeft);
    uv1 = udMag3(pRight - pPrevRight);

    // duplicate positions
    pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(p0, 0.0f);
    pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-p0, 1.0f);
    pVerts[vertIndex + 2].ribbonInfo = udFloat4::create(p0, 0.0f);
    pVerts[vertIndex + 3].ribbonInfo = udFloat4::create(-p0, 1.0f);

    // calculate UVs
    float hyp = udMag3(pLeft - pRight);
    float dist0 = udSqrt(hyp * hyp - widthSquared);
    float dist1 = 0;
    if (jointFlipped)
    {
      float t = dist1;
      dist1 = dist0;
      dist0 = t;
    }

    pVerts[vertIndex + 0].uv.x = currentUV + uv0 + previousDist1;
    pVerts[vertIndex + 1].uv.x = currentUV + uv1 + previousDist0;

    currentUV += udMax(uv0 - dist0, uv1 - dist1); // take max (offset slightly, for better tiling between segments)

    pVerts[vertIndex + 2].uv.x = currentUV + dist1;
    pVerts[vertIndex + 3].uv.x = currentUV + dist0;

    previousDist0 = dist0;
    previousDist1 = dist1;
    vertIndex += 4;
  }

  // end segment
  p0 = vcFenceRenderer_CreateEndJointExpandVector(pPoints[pointCount - 2], pPoints[pointCount - 1], pFenceRenderer->ribbonWidth);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(p0, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-p0, 1.0f);

  uv0 = udMag3((pPoints[pointCount - 1] + pVerts[vertIndex + 0].ribbonInfo.toVector3()) - (pPoints[pointCount - 2] + pVerts[vertIndex - 2].ribbonInfo.toVector3()));
  uv1 = udMag3((pPoints[pointCount - 1] + pVerts[vertIndex + 1].ribbonInfo.toVector3()) - (pPoints[pointCount - 2] + pVerts[vertIndex - 1].ribbonInfo.toVector3()));

  maxUV = udMax(uv0, uv1);
  pVerts[vertIndex + 0].uv.x = currentUV + maxUV;
  pVerts[vertIndex + 1].uv.x = currentUV + maxUV;

  if (pFenceRenderer->pMesh)
  {
    UD_ERROR_IF(vcMesh_UploadData(pFenceRenderer->pMesh, vcRibbonVertexLayout, layoutCount, pVerts, pFenceRenderer->vertCount, nullptr, 0), udR_InternalError);
  }
  else
  {
    UD_ERROR_IF(vcMesh_Create(&pFenceRenderer->pMesh, vcRibbonVertexLayout, layoutCount, pVerts, pFenceRenderer->vertCount, nullptr, 0, vcMF_Dynamic | vcMF_NoIndexBuffer), udR_InternalError);
  }

epilogue:

  udFree(pVerts);
  return udR_Success;
}

bool vcFenceRenderer_Render(vcFenceRenderer *pFenceRenderer, const udDouble4x4 &viewProjectionMatrix, double deltaTime)
{
  if (pFenceRenderer->pointCount == 0)
    return false;

  pFenceRenderer->totalTimePassed += deltaTime;

  pFenceRenderer->renderShader.params.viewProjection = udFloat4x4::create(viewProjectionMatrix);
  pFenceRenderer->renderShader.params.time = (float)pFenceRenderer->totalTimePassed;
  pFenceRenderer->renderShader.params.width = pFenceRenderer->ribbonWidth;
  pFenceRenderer->renderShader.params.textureRepeatScale = pFenceRenderer->config.textureRepeatScale;
  pFenceRenderer->renderShader.params.textureScrollSpeed = pFenceRenderer->config.textureScrollSpeed;
  pFenceRenderer->renderShader.params.bottomColour = pFenceRenderer->config.bottomColour;
  pFenceRenderer->renderShader.params.topColour = pFenceRenderer->config.topColour;

  switch (pFenceRenderer->config.visualMode)
  {
  case vcRRVM_Fence:
    pFenceRenderer->renderShader.params.orientation = 0.0f;
    break;
  case vcRRVM_Flat: // fall through
  default:
    pFenceRenderer->renderShader.params.orientation = 1.0f;
    break;
  }

  vcShader_Bind(pFenceRenderer->renderShader.pProgram);

  vcTexture *pDisplayTexture = nullptr;
  switch (pFenceRenderer->config.imageMode)
  {
    case vcRRIM_Glow:
      pDisplayTexture = gGlowTexture;
      break;
    case vcRRIM_Solid:
      pDisplayTexture = gSolidTexture;
      break;
    case vcRRIM_Arrow: // fall through
    default:
      pDisplayTexture = gArrowTexture;
      break;
  }

  vcShader_BindTexture(pFenceRenderer->renderShader.pProgram, pDisplayTexture, 0, pFenceRenderer->renderShader.uniform_texture);
  vcShader_BindConstantBuffer(pFenceRenderer->renderShader.pProgram, pFenceRenderer->renderShader.uniform_params, &pFenceRenderer->renderShader.params, sizeof(pFenceRenderer->renderShader.params));

  return vcMesh_Render(pFenceRenderer->pMesh, pFenceRenderer->vertCount, 0, vcMRM_TriangleStrip);
}
