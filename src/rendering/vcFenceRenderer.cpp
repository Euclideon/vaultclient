
#include "vcFenceRenderer.h"

#include "udChunkedArray.h"

#include "vcMesh.h"
#include "vcTexture.h"
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
      udFloat4 primaryColour;
      udFloat4 secondaryColour;
      udFloat4 worldUp;
      int options;
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

udResult vcFenceRenderer_Create(vcFenceRenderer **ppFenceRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;
  vcFenceRenderer *pFenceRenderer = nullptr;

  UD_ERROR_NULL(ppFenceRenderer, udR_InvalidParameter_);

  pFenceRenderer = udAllocType(vcFenceRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pFenceRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcFenceRenderer_ReloadShaders(pFenceRenderer, pWorkerPool));

  UD_ERROR_CHECK(pFenceRenderer->segments.Init(32));

  // defaults
  pFenceRenderer->config.ribbonWidth = 2.5f;
  pFenceRenderer->config.textureRepeatScale = 1.0f;
  pFenceRenderer->config.textureScrollSpeed = 1.0f;
  pFenceRenderer->config.isDualColour = false;
  pFenceRenderer->config.primaryColour = udFloat4::create(1.0f);
  pFenceRenderer->config.secondaryColour = udFloat4::create(1.0f);
  pFenceRenderer->config.imageMode = vcRRIM_Solid;
  pFenceRenderer->config.visualMode = vcRRVM_Fence;

  UD_ERROR_CHECK(vcFenceRenderer_Init());

  *ppFenceRenderer = pFenceRenderer;
  pFenceRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pFenceRenderer != nullptr)
    vcFenceRenderer_Destroy(&pFenceRenderer);

  return result;
}

void vcFenceRenderer_DestroyShaders(vcFenceRenderer *pFenceRenderer)
{
  vcShader_ReleaseConstantBuffer(pFenceRenderer->renderShader.pProgram, pFenceRenderer->renderShader.uniform_everyFrame);
  vcShader_ReleaseConstantBuffer(pFenceRenderer->renderShader.pProgram, pFenceRenderer->renderShader.uniform_everyObject);
  vcShader_DestroyShader(&pFenceRenderer->renderShader.pProgram);
}

udResult vcFenceRenderer_Destroy(vcFenceRenderer **ppFenceRenderer)
{
  if (ppFenceRenderer == nullptr || *ppFenceRenderer == nullptr)
    return udR_Success;

  vcFenceRenderer *pFenceRenderer = *ppFenceRenderer;
  *ppFenceRenderer = nullptr;

  vcFenceRenderer_DestroyShaders(pFenceRenderer);
  vcFenceRenderer_ClearPoints(pFenceRenderer);
  pFenceRenderer->segments.Deinit();

  udFree(pFenceRenderer);

  vcFenceRenderer_Destroy();

  return udR_Success;
}

udResult vcFenceRenderer_ReloadShaders(vcFenceRenderer *pFenceRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcFenceRenderer_DestroyShaders(pFenceRenderer);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pFenceRenderer->renderShader.pProgram, pWorkerPool, "asset://assets/shaders/fenceVertexShader", "asset://assets/shaders/fenceFragmentShader", vcP3UV2RI4VertexLayout,
    [pFenceRenderer](void *)
    {
      vcShader_Bind(pFenceRenderer->renderShader.pProgram);
      vcShader_GetSamplerIndex(&pFenceRenderer->renderShader.uniform_texture, pFenceRenderer->renderShader.pProgram, "colour");
      vcShader_GetConstantBuffer(&pFenceRenderer->renderShader.uniform_everyFrame, pFenceRenderer->renderShader.pProgram, "u_EveryFrame", sizeof(pFenceRenderer->renderShader.everyFrameParams));
      vcShader_GetConstantBuffer(&pFenceRenderer->renderShader.uniform_everyObject, pFenceRenderer->renderShader.pProgram, "u_EveryObject", sizeof(pFenceRenderer->renderShader.everyObjectParams));
    }
  ), udR_InternalError);


  result = udR_Success;
epilogue:

  return result;
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
        result = udR_InternalError;
    }
  }

  return result;
}

udFloat3 vcFenceRenderer_CreateLeftOrthogonalExpandVector(const udFloat3 &center, const udFloat3 &next, udFloat3 const & worldUp)
{
  udFloat3 Vcn = next - center;
  float magVcn = udMag3(Vcn);

  if (magVcn < UD_EPSILON)
    return udPerpendicular3(worldUp);

  Vcn /= magVcn;

  udFloat3 Vleft = udCross(worldUp, Vcn);
  float magVleft = udMag3(Vleft);

  //Just choose perpendicular vector to the world up
  if (magVleft <= UD_EPSILON)
    return udPerpendicular3(worldUp);

  return Vleft /= magVleft;
}

udFloat3 vcFenceRenderer_CreateLeftSegmentJointExpandVector(const udFloat3 &previous, const udFloat3 &center, const udFloat3 &next, udFloat3 const &worldUp)
{
  udFloat3 Vpc = center - previous;
  udFloat3 Vcn = next - center;

  Vpc = Vpc - udDot(Vpc, worldUp) * worldUp;
  Vcn = Vcn - udDot(Vcn, worldUp) * worldUp;

  float magVpc = udMag3(Vpc);
  float magVcn = udMag3(Vcn);

  Vpc = magVpc < UD_EPSILON ? udPerpendicular3(worldUp) : Vpc / magVpc;
  Vcn = magVcn < UD_EPSILON ? udPerpendicular3(worldUp) : Vcn / magVcn;

  float d = udDot(Vpc, Vcn);

  if (d >= 0.99f) // straight edge case
  {
    udFloat3 left = udCross(worldUp, Vpc);
    return left;
  }

  float theta = udACos(d);

  // limit angle to something reasonable
  theta = udMin(2.85f, theta);

  udFloat3 cross = udCross(Vpc, Vcn);
  float flip = udDot(worldUp, cross) > 0 ? 1.f : -1.f;
  float mult = 1.f / udSin((UD_PIf - theta) * 0.5f);

  return udNormalize3((Vcn - Vpc) * 0.5f) * flip * mult;
}

udResult vcFenceRenderer_CreateSegmentVertexData(vcFenceRenderer *pFenceRenderer, vcFenceSegment *pSegment)
{
  udResult result = udR_Success;

  int layoutCount = (int)udLengthOf(vcP3UV2RI4VertexLayout);
  float uvFenceOffset = 0;
  int vertIndex = 0;
  udFloat3 expandVector = udFloat3::zero();
  udFloat3 prev = udFloat3::zero();
  udFloat3 current = udFloat3::zero();
  udFloat3 next = udFloat3::zero();
  
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

  expandVector = vcFenceRenderer_CreateLeftOrthogonalExpandVector(current, next, pFenceRenderer->config.worldUp);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(expandVector, 1.0f);
  vertIndex += 2;
  
  // middle segments
  for (size_t i = 1; i < pSegment->pointCount - 1; ++i)
  {
    prev = udFloat3::create(pSegment->pCachedPoints[i - 1]);
    current = udFloat3::create(pSegment->pCachedPoints[i]);
    next = udFloat3::create(pSegment->pCachedPoints[i + 1]);

    expandVector = vcFenceRenderer_CreateLeftSegmentJointExpandVector(prev, current, next, pFenceRenderer->config.worldUp);

    float adjustment = 0.f;
    udFloat3 Vpc = current - prev;
    float magVpc = udMag3(Vpc);
    if (magVpc > UD_EPSILON)
    {
      Vpc = Vpc / magVpc;
      adjustment = udDot(Vpc, expandVector * 0.5);
    }

    pVerts[vertIndex + 0].uv.x = pVerts[vertIndex + 0].uv.y - adjustment;
    pVerts[vertIndex + 1].uv.x = pVerts[vertIndex + 1].uv.y + adjustment;
    pVerts[vertIndex + 2].uv.x = pVerts[vertIndex + 2].uv.y + adjustment;
    pVerts[vertIndex + 3].uv.x = pVerts[vertIndex + 3].uv.y - adjustment;

    // duplicate positions
    pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
    pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(expandVector, 1.0f);
    pVerts[vertIndex + 2].ribbonInfo = udFloat4::create(expandVector, 0.0f);
    pVerts[vertIndex + 3].ribbonInfo = udFloat4::create(expandVector, 1.0f);
    
    vertIndex += 4;
  }

  // end segment
  prev = udFloat3::create(pSegment->pCachedPoints[pSegment->pointCount - 2]);
  current = udFloat3::create(pSegment->pCachedPoints[pSegment->pointCount - 1]);
  expandVector = vcFenceRenderer_CreateLeftOrthogonalExpandVector(prev, current, pFenceRenderer->config.worldUp);
  pVerts[vertIndex + 0].ribbonInfo = udFloat4::create(expandVector, 0.0f);
  pVerts[vertIndex + 1].ribbonInfo = udFloat4::create(expandVector, 1.0f);

  pVerts[vertIndex + 0].uv.x = pVerts[vertIndex + 0].uv.y;
  pVerts[vertIndex + 1].uv.x = pVerts[vertIndex + 0].uv.y;

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
    newSegment.pCachedPoints[newSegment.pointCount - 1] = udDouble3::zero();

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
  pFenceRenderer->renderShader.everyFrameParams.textureScrollSpeed = pFenceRenderer->config.textureScrollSpeed;
  pFenceRenderer->renderShader.everyFrameParams.primaryColour = pFenceRenderer->config.primaryColour;
  pFenceRenderer->renderShader.everyFrameParams.worldUp = udFloat4::create(pFenceRenderer->config.worldUp, 0.f);
  pFenceRenderer->renderShader.everyFrameParams.options = 0;
  pFenceRenderer->renderShader.everyFrameParams.textureRepeatScale = pFenceRenderer->config.textureRepeatScale;
   
  if (pFenceRenderer->config.isDualColour)
    pFenceRenderer->renderShader.everyFrameParams.secondaryColour = pFenceRenderer->config.secondaryColour;
  else
    pFenceRenderer->renderShader.everyFrameParams.secondaryColour = pFenceRenderer->config.primaryColour;
   
  switch (pFenceRenderer->config.visualMode)
  {
  case vcRRVM_Fence:
    pFenceRenderer->renderShader.everyFrameParams.options = 0;
    break;
  case vcRRVM_Flat:
    pFenceRenderer->renderShader.everyFrameParams.options |= ((1 << 2) | (2));
    break;
  case vcRRVM_ScreenLine: // do nothing
    break;
  case vcRRVM_Count:
    return false;
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
