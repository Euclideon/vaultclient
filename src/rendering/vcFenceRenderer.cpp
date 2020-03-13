
#include "vcFenceRenderer.h"

#include "udChunkedArray.h"

#include "gl/vcMesh.h"
#include "gl/vcTexture.h"
#include "vcSettings.h"

struct vcFenceSegment
{
  vcMesh *pMesh;
  int vertCount;

  size_t pointCount;
  udDouble3 *pCachedPoints;
  udDouble4x4 fenceOriginOffset;
};

struct vcFenceRenderer
{
  udChunkedArray<vcFenceSegment> segments;

  double totalTimePassed;
  vcFenceRendererConfig config;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_everyFrame;
    vcShaderConstantBuffer *uniform_everyObject;
    vcShaderSampler *uniform_texture;

    struct
    {
      udFloat4 bottomColour;
      udFloat4 topColour;
      float orientation; // 0 == fence, 1 == flat
      float width;
      float textureRepeatScale;
      float textureScrollSpeed;
      float time;

      char _padding[12]; // 16 byte alignment
    } everyFrameParams;

    struct
    {
      udFloat4x4 u_worldViewProjection;
    } everyObjectParams;

  } renderShader;
};

static int gRefCount = 0;
static vcTexture *gArrowTexture = nullptr;
static vcTexture *gGlowTexture = nullptr;
static vcTexture *gSolidTexture = nullptr;
static vcTexture *gDiagonalTexture = nullptr;

udResult vcFenceRenderer_CreateSegmentVertexData(vcFenceRenderer *pFenceRenderer, vcFenceSegment *pSegment);
udResult vcFenceRenderer_Init();
udResult vcFenceRenderer_Destroy();

udResult vcFenceRenderer_Init()
{
  udResult result;
  gRefCount++;
  static const uint32_t whitePixel[] = { 0xffffffff };

  UD_ERROR_IF(gRefCount != 1, udR_Success);

  UD_ERROR_IF(!vcTexture_CreateFromFilename(&gArrowTexture, "asset://assets/textures/fenceArrow.png", nullptr, nullptr, vcTFM_Linear, true), udR_InternalError);
  UD_ERROR_IF(!vcTexture_CreateFromFilename(&gGlowTexture, "asset://assets/textures/fenceGlow.png", nullptr, nullptr, vcTFM_Linear, true), udR_InternalError);
  UD_ERROR_IF(!vcTexture_CreateFromFilename(&gDiagonalTexture, "asset://assets/textures/fenceDiagonal.png", nullptr, nullptr, vcTFM_Linear, true), udR_InternalError);

  UD_ERROR_CHECK(vcTexture_Create(&gSolidTexture, 1, 1, whitePixel));

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcFenceRenderer_Destroy();
  return result;
}

udResult vcFenceRenderer_Destroy()
{
  udResult result;
  --gRefCount;

  UD_ERROR_IF(gRefCount != 0, udR_Success);

  vcTexture_Destroy(&gArrowTexture);
  vcTexture_Destroy(&gGlowTexture);
  vcTexture_Destroy(&gSolidTexture);
  vcTexture_Destroy(&gDiagonalTexture);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer)
{
  udResult result;
  vcFenceRenderer *pFenceRenderer = nullptr;

  UD_ERROR_NULL(ppFenceRenderer, udR_InvalidParameter_);

  pFenceRenderer = udAllocType(vcFenceRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pFenceRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(pFenceRenderer->segments.Init(32));

  // defaults
  pFenceRenderer->config.ribbonWidth = 2.5f;
  pFenceRenderer->config.textureRepeatScale = 1.0f;
  pFenceRenderer->config.textureScrollSpeed = 1.0f;
  pFenceRenderer->config.bottomColour = udFloat4::create(1.0f);
  pFenceRenderer->config.topColour = udFloat4::create(1.0f);
  pFenceRenderer->config.imageMode = vcRRIM_Solid;
  pFenceRenderer->config.visualMode = vcRRVM_Fence;

  UD_ERROR_IF(!vcShader_CreateFromFile(&pFenceRenderer->renderShader.pProgram, "asset://assets/shaders/fenceVertexShader", "asset://assets/shaders/fenceFragmentShader", vcP3UV2RI4VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pFenceRenderer->renderShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pFenceRenderer->renderShader.uniform_texture, pFenceRenderer->renderShader.pProgram, "u_texture"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pFenceRenderer->renderShader.uniform_everyFrame, pFenceRenderer->renderShader.pProgram, "u_EveryFrame", sizeof(pFenceRenderer->renderShader.everyFrameParams)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pFenceRenderer->renderShader.uniform_everyObject, pFenceRenderer->renderShader.pProgram, "u_EveryObject", sizeof(pFenceRenderer->renderShader.everyObjectParams)), udR_InternalError);

  UD_ERROR_CHECK(vcFenceRenderer_Init());

  *ppFenceRenderer = pFenceRenderer;
  pFenceRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pFenceRenderer != nullptr)
    vcFenceRenderer_Destroy(&pFenceRenderer);

  return result;
}

udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer)
{
  if (ppFenceRenderer == nullptr || *ppFenceRenderer == nullptr)
    return udR_Success;

  vcFenceRenderer *pFenceRenderer = *ppFenceRenderer;
  *ppFenceRenderer = nullptr;

  vcFenceRenderer_ClearPoints(pFenceRenderer);
  pFenceRenderer->segments.Deinit();

  vcShader_DestroyShader(&pFenceRenderer->renderShader.pProgram);

  udFree(pFenceRenderer);

  vcFenceRenderer_Destroy();

  return udR_Success;
}

udResult vcFenceRenderer_SetConfig(vcFenceRenderer *pFenceRenderer, const vcFenceRendererConfig &config)
{
  udResult result = udR_Success;

  float oldWidth = pFenceRenderer->config.ribbonWidth;
  pFenceRenderer->config = config;

  // need to rebuild verts on width change
  if ((oldWidth - pFenceRenderer->config.ribbonWidth) > UD_EPSILON)
  {
    for (size_t i = 0; i < pFenceRenderer->segments.length; ++i)
    {
      if (vcFenceRenderer_CreateSegmentVertexData(pFenceRenderer, &pFenceRenderer->segments[i]) != udR_Success)
      {
        result = udR_InternalError;
      }
    }
  }

  return result;
}

udFloat3 vcFenceRenderer_CreateEndJointExpandVector(const udFloat3 &previous, const udFloat3 &center, float width)
{
  return udCross(udMag3(center - previous) == 0 ? center - previous : udNormalize(center - previous), udFloat3::create(0, 0, 1)) * width * 0.5f;
}

udFloat3 vcFenceRenderer_CreateStartJointExpandVector(const udFloat3 &center, const udFloat3 &next, float width)
{
  return udCross(udMag3(next - center) == 0 ? next - center : udNormalize(next - center), udFloat3::create(0, 0, 1)) * width * 0.5f;
}

udFloat3 vcFenceRenderer_CreateSegmentJointExpandVector(const udFloat3 &previous, const udFloat3 &center, const udFloat3 &next, float width, bool *pJointFlipped)
{
  udFloat3 v1 = udMag3(center - previous) == 0 ? center - previous : udNormalize(center - previous);
  udFloat3 v2 = udMag3(next - center) == 0 ? next - center : udNormalize(next - center);
  float d = udMin(udDot(v1, v2), 1.f);
  float theta = udACos(d);

  // limit angle to something reasonable
  theta = udMin(2.85f, theta);

  // determine sign of angle
  udFloat3 cross = udCross(v1, v2);
  *pJointFlipped = udDot(udFloat3::create(0, 0, 1), cross) < 0;
  if (*pJointFlipped)
    theta *= -1;

  if (d >= 0.99f) // straight edge case
  {
    udFloat3 up = udFloat3::create(0, 0, 1);
    udFloat3 right = udCross(v1, up);
    return right * 0.5f * width;
  }

  float u = width / (2.0f * udSin(theta));
  float v = width / (2.0f * udSin(theta));

  udFloat3 u0 = udMag3(previous - center) == 0 ? previous - center : udNormalize(previous - center) * u;
  udFloat3 v0 = udMag3(next - center) == 0 ? next - center : udNormalize(next - center) * v;

  return -(u0 + v0);
}

udResult vcFenceRenderer_CreateSegmentVertexData(vcFenceRenderer *pFenceRenderer, vcFenceSegment *pSegment)
{
  udResult result = udR_Success;

  int layoutCount = (int)udLengthOf(vcP3UV2RI4VertexLayout);
  float widthSquared = pFenceRenderer->config.ribbonWidth * pFenceRenderer->config.ribbonWidth;
  float uvFenceOffset = 0;
  int vertIndex = 0;
  bool jointFlipped = false;
  float currentUV = 0;
  float previousDist0 = 0;
  float previousDist1 = 0;
  udFloat3 expandVector = udFloat3::zero();
  udFloat3 prev = udFloat3::zero();
  udFloat3 current = udFloat3::zero();
  udFloat3 next = udFloat3::zero();
  float uv0 = 0.0f;
  float uv1 = 0.0f;
  float maxUV = 0.0f;

  int vertCount = int(pSegment->pointCount * 2 + udMax<size_t>(pSegment->pointCount - 2, 0) * 2);
  vcP3UV2RI4Vertex *pVerts = udAllocType(vcP3UV2RI4Vertex, vertCount, udAF_Zero);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);
  pSegment->vertCount = vertCount;

  for (size_t i = 0; i < pSegment->pointCount; ++i)
  {
    if (i > 0)
      uvFenceOffset += float(udMag3(pSegment->pCachedPoints[i] - pSegment->pCachedPoints[i - 1])) / pFenceRenderer->config.ribbonWidth;

    udFloat3 point = udFloat3::create(pSegment->pCachedPoints[i]);

    pVerts[vertIndex + 0].position = point;
    pVerts[vertIndex + 0].uv = udFloat2::create(0, uvFenceOffset);

    pVerts[vertIndex + 1].position = point;
    pVerts[vertIndex + 1].uv = udFloat2::create(0, uvFenceOffset);

    vertIndex += 2;

    if (i > 0 && i < pSegment->pointCount - 1) // middle segments
    {
      pVerts[vertIndex + 0].position = point;
      pVerts[vertIndex + 0].uv = udFloat2::create(0, uvFenceOffset);

      pVerts[vertIndex + 1].position = point;
      pVerts[vertIndex + 1].uv = udFloat2::create(0, uvFenceOffset);

      vertIndex += 2;
    }
  }

  vertIndex = 0;

  // start segment
  current = udFloat3::create(pSegment->pCachedPoints[0]);
  next = udFloat3::create(pSegment->pCachedPoints[1]);
  expandVector = vcFenceRenderer_CreateStartJointExpandVector(current, next, pFenceRenderer->config.ribbonWidth);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-expandVector, 1.0f);
  vertIndex += 2;

  // middle segments
  for (size_t i = 1; i < pSegment->pointCount - 1; ++i)
  {
    prev = udFloat3::create(pSegment->pCachedPoints[i - 1]);
    current = udFloat3::create(pSegment->pCachedPoints[i]);
    next = udFloat3::create(pSegment->pCachedPoints[i + 1]);

    expandVector = vcFenceRenderer_CreateSegmentJointExpandVector(prev, current, next, pFenceRenderer->config.ribbonWidth, &jointFlipped);

    udFloat3 pLeft = current + expandVector;
    udFloat3 pRight = current - expandVector;

    udFloat3 pPrevLeft = prev + pVerts[vertIndex - 2].ribbonInfo.toVector3();
    udFloat3 pPrevRight = prev + pVerts[vertIndex - 1].ribbonInfo.toVector3();

    // distance between (expanded) verts
    uv0 = udMag3(pLeft - pPrevLeft);
    uv1 = udMag3(pRight - pPrevRight);

    // duplicate positions
    pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
    pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-expandVector, 1.0f);
    pVerts[vertIndex + 2].ribbonInfo = udFloat4::create(expandVector, 0.0f);
    pVerts[vertIndex + 3].ribbonInfo = udFloat4::create(-expandVector, 1.0f);

    // calculate UVs
    float hyp = udMag3(pLeft - pRight);
    float dist0 = udSqrt(udAbs(hyp * hyp - widthSquared));
    float dist1 = 0;
    if (jointFlipped)
    {
      float t = dist1;
      dist1 = dist0;
      dist0 = t;
    }

    pVerts[vertIndex + 0].uv.x = currentUV + uv0 + previousDist1;
    pVerts[vertIndex + 1].uv.x = currentUV + uv1 + previousDist0;

    currentUV += udMax(uv0 - dist0, uv1 - dist1); // take max of distances

    pVerts[vertIndex + 2].uv.x = currentUV + dist1;
    pVerts[vertIndex + 3].uv.x = currentUV + dist0;

    previousDist0 = dist0;
    previousDist1 = dist1;
    vertIndex += 4;
  }

  // end segment
  prev = udFloat3::create(pSegment->pCachedPoints[pSegment->pointCount - 2]);
  current = udFloat3::create(pSegment->pCachedPoints[pSegment->pointCount - 1]);
  expandVector = vcFenceRenderer_CreateEndJointExpandVector(prev, current, pFenceRenderer->config.ribbonWidth);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(-expandVector, 1.0f);

  uv0 = udMag3((current + pVerts[vertIndex + 0].ribbonInfo.toVector3()) - (prev + pVerts[vertIndex - 2].ribbonInfo.toVector3()));
  uv1 = udMag3((current + pVerts[vertIndex + 1].ribbonInfo.toVector3()) - (prev + pVerts[vertIndex - 1].ribbonInfo.toVector3()));

  maxUV = udMax(uv0, uv1);
  pVerts[vertIndex + 0].uv.x = currentUV + maxUV;
  pVerts[vertIndex + 1].uv.x = currentUV + maxUV;

  if (pSegment->pMesh)
    vcMesh_Destroy(&pSegment->pMesh);

  UD_ERROR_IF(vcMesh_Create(&pSegment->pMesh, vcP3UV2RI4VertexLayout, layoutCount, pVerts, (uint32_t)pSegment->vertCount, nullptr, 0, vcMF_Dynamic | vcMF_NoIndexBuffer), udR_InternalError);

epilogue:
  udFree(pVerts);
  return result;
}

udResult vcFenceRenderer_AddPoints(vcFenceRenderer *pFenceRenderer, udDouble3 *pPoints, size_t pointCount, bool closed)
{
  udResult result = udR_Success;
  vcFenceSegment newSegment = {};

  UD_ERROR_IF(pointCount < 2, udR_InvalidParameter_);

  newSegment.pointCount = pointCount + (closed ? 1 : 0);
  newSegment.pCachedPoints = udAllocType(udDouble3, newSegment.pointCount, udAF_Zero);
  UD_ERROR_NULL(newSegment.pCachedPoints, udR_MemoryAllocationFailure);

  // Reposition all the points around an origin point
  newSegment.fenceOriginOffset = udDouble4x4::translation(pPoints[0]);
  for (size_t i = 0; i < pointCount; ++i)
  {
    newSegment.pCachedPoints[i] = pPoints[i] - pPoints[0];
  }

  if (closed)
    newSegment.pCachedPoints[pointCount] = udDouble3::zero();

  vcFenceRenderer_CreateSegmentVertexData(pFenceRenderer, &newSegment);
  pFenceRenderer->segments.PushBack(newSegment);

epilogue:

  return result;
}

void vcFenceRenderer_ClearPoints(vcFenceRenderer *pFenceRenderer)
{
  for (size_t i = 0; i < pFenceRenderer->segments.length; ++i)
  {
    vcFenceSegment *pSegment = &pFenceRenderer->segments[i];

    vcMesh_Destroy(&pSegment->pMesh);
    udFree(pSegment->pCachedPoints);
  }

  pFenceRenderer->segments.Clear();
}

bool vcFenceRenderer_Render(vcFenceRenderer *pFenceRenderer, const udDouble4x4 &viewProjectionMatrix, double deltaTime)
{
  bool success = true;

  if (pFenceRenderer->segments.length == 0)
    return success;

  pFenceRenderer->totalTimePassed += deltaTime;

  pFenceRenderer->renderShader.everyFrameParams.time = (float)pFenceRenderer->totalTimePassed;
  pFenceRenderer->renderShader.everyFrameParams.width = pFenceRenderer->config.ribbonWidth;
  pFenceRenderer->renderShader.everyFrameParams.textureRepeatScale = pFenceRenderer->config.textureRepeatScale;
  pFenceRenderer->renderShader.everyFrameParams.textureScrollSpeed = pFenceRenderer->config.textureScrollSpeed;
  pFenceRenderer->renderShader.everyFrameParams.bottomColour = pFenceRenderer->config.bottomColour;
  pFenceRenderer->renderShader.everyFrameParams.topColour = pFenceRenderer->config.topColour;

  switch (pFenceRenderer->config.visualMode)
  {
  case vcRRVM_Fence:
    pFenceRenderer->renderShader.everyFrameParams.orientation = 0.0f;
    break;
  case vcRRVM_Flat: // fall through
  default:
    pFenceRenderer->renderShader.everyFrameParams.orientation = 1.0f;
    break;
  }

  vcShader_Bind(pFenceRenderer->renderShader.pProgram);

  vcTexture *pDisplayTexture = nullptr;
  switch (pFenceRenderer->config.imageMode)
  {
    case vcRRIM_Arrow:
      pDisplayTexture = gArrowTexture;
      break;
    case vcRRIM_Glow:
      pDisplayTexture = gGlowTexture;
      break;
    case vcRRIM_Solid:
      pDisplayTexture = gSolidTexture;
      break;
    case vcRRIM_Diagonal:
      pDisplayTexture = gDiagonalTexture;
      break;

    case vcRRIM_Count:
      // Does nothing but is required to avoid compilation issues
      break;
  }

  vcShader_BindTexture(pFenceRenderer->renderShader.pProgram, pDisplayTexture, 0, pFenceRenderer->renderShader.uniform_texture);
  vcShader_BindConstantBuffer(pFenceRenderer->renderShader.pProgram, pFenceRenderer->renderShader.uniform_everyFrame, &pFenceRenderer->renderShader.everyFrameParams, sizeof(pFenceRenderer->renderShader.everyFrameParams));

  for (size_t i = 0; i < pFenceRenderer->segments.length; ++i)
  {
    vcFenceSegment *pSegment = &pFenceRenderer->segments[i];

    pFenceRenderer->renderShader.everyObjectParams.u_worldViewProjection = udFloat4x4::create(viewProjectionMatrix * pSegment->fenceOriginOffset);
    vcShader_BindConstantBuffer(pFenceRenderer->renderShader.pProgram, pFenceRenderer->renderShader.uniform_everyObject, &pFenceRenderer->renderShader.everyObjectParams, sizeof(pFenceRenderer->renderShader.everyObjectParams));

    if (vcMesh_Render(pSegment->pMesh, pSegment->vertCount, 0, vcMRM_TriangleStrip) != udR_Success)
      success = false;
  }

  return success;
}
