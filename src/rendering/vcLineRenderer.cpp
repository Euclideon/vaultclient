#include "vcLineRenderer.h"
#include "gl/vcShader.h"
#include "udChunkedArray.h"
#include "gl/vcMesh.h"

struct vcLineRenderer
{
  struct vcLineSegment
  {
    udFloat4 colour;
    float thickness;
    udDouble4x4 originMatrix;
    int pointCount;
    vcMesh *pMesh;
  };
  udChunkedArray<vcLineSegment> segments;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_everyObject;
    vcShaderSampler *uniform_texture;

    struct
    {
      udFloat4 colour;
      udFloat4 thickness;
      udFloat4x4 worldViewProjection;
    } everyObjectParams;

  } renderShader;
};

udResult vcLineRenderer_Create(vcLineRenderer **ppLineRenderer)
{
  udResult result;

  vcLineRenderer *pLineRenderer = udAllocType(vcLineRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pLineRenderer, udR_MemoryAllocationFailure);

  pLineRenderer->segments.Init(128);

  vcLineRenderer_ReloadShaders(pLineRenderer);

  *ppLineRenderer = pLineRenderer;
  pLineRenderer = nullptr;
  result = udR_Success;
epilogue:
  if (pLineRenderer != nullptr)
    vcLineRenderer_Destroy(&pLineRenderer);

  return result;
}

void vcLineRenderer_DestroyShaders(vcLineRenderer *pLineRenderer)
{
  vcShader_ReleaseConstantBuffer(pLineRenderer->renderShader.pProgram, pLineRenderer->renderShader.uniform_everyObject);
  vcShader_DestroyShader(&pLineRenderer->renderShader.pProgram);
}

udResult vcLineRenderer_Destroy(vcLineRenderer **ppLineRenderer)
{
  vcLineRenderer *pLineRenderer = *ppLineRenderer;
  *ppLineRenderer = nullptr;

  vcLineRenderer_ClearLines(pLineRenderer);

  vcLineRenderer_DestroyShaders(pLineRenderer);
  pLineRenderer->segments.Deinit();
  udFree(pLineRenderer);
  return udR_Success;
}

udResult vcLineRenderer_ReloadShaders(vcLineRenderer *pLineRenderer)
{
  udResult result;

  vcLineRenderer_DestroyShaders(pLineRenderer);

  UD_ERROR_IF(!vcShader_CreateFromFile(&pLineRenderer->renderShader.pProgram, "asset://assets/shaders/lineVertexShader", "asset://assets/shaders/lineFragmentShader", vcLineVertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pLineRenderer->renderShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pLineRenderer->renderShader.uniform_everyObject, pLineRenderer->renderShader.pProgram, "u_EveryObject", sizeof(pLineRenderer->renderShader.everyObjectParams)), udR_InternalError);

  result = udR_Success;
epilogue:

  return result;
}

udResult vcLineRenderer_AddLine(vcLineRenderer *pLineRenderer, const vcLineData &data)
{
  udResult result;
  int vertCount = data.pointCount * 2;
  vcLineVertex *pVerts = nullptr;
  vcMesh *pMesh = nullptr;
  udDouble3 origin = data.pPoints[0];
  vcLineRenderer::vcLineSegment segment = {};

  pVerts = udAllocType(vcLineVertex, vertCount, udAF_None);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < data.pointCount; ++i)
  {
    udDouble3 p = data.pPoints[i];

    udDouble3 prev = udDouble3::zero();
    udDouble3 next = udDouble3::zero();
    if (i == 0)
      prev = p + (p - data.pPoints[i + 1]) * 0.01; // fake a vert
    else
      prev = data.pPoints[i - 1];

    if (i == data.pointCount - 1)
      next = p + (p - data.pPoints[i - 1]) * 0.01; // fake a vert
    else
      next = data.pPoints[i + 1];

    int curIndex = i * 2;
    udFloat3 p0 = udFloat3::create(p - origin);
    udFloat3 prev0 = udFloat3::create(prev - origin);
    udFloat3 next0 = udFloat3::create(next - origin);
    pVerts[curIndex + 0].position = udFloat4::create(p0, -1.0f);
    pVerts[curIndex + 1].position = udFloat4::create(p0, 1.0f);
    pVerts[curIndex + 0].previous = udFloat4::create(prev0, -1.0f);
    pVerts[curIndex + 1].previous = udFloat4::create(prev0, 1.0f);
    pVerts[curIndex + 0].next = udFloat4::create(next0, -1.0f);
    pVerts[curIndex + 1].next = udFloat4::create(next0, 1.0f);
  }

  UD_ERROR_CHECK(vcMesh_Create(&pMesh, vcLineVertexLayout, udLengthOf(vcLineVertexLayout), pVerts, vertCount, nullptr, 0, vcMF_NoIndexBuffer), udR_InternalError);

  segment.pMesh = pMesh;
  segment.originMatrix = udDouble4x4::translation(origin);
  segment.colour = data.colour;
  segment.thickness = data.thickness;
  segment.pointCount = data.pointCount;
  pLineRenderer->segments.PushBack(segment);

  result = udR_Success;
epilogue:

  udFree(pVerts);
  return result;
}

void vcLineRenderer_ClearLines(vcLineRenderer *pLineRenderer)
{
  for (size_t i = 0; i < pLineRenderer->segments.length; ++i)
    vcMesh_Destroy(&pLineRenderer->segments[i].pMesh);

  pLineRenderer->segments.Clear();
}

bool vcLineRenderer_Render(vcLineRenderer *pLineRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, double deltaTime)
{
  if (pLineRenderer->segments.length == 0)
    return true;

  vcShader_Bind(pLineRenderer->renderShader.pProgram);
  for (size_t i = 0; i < pLineRenderer->segments.length; ++i)
  {
    vcLineRenderer::vcLineSegment &segment = pLineRenderer->segments[i];

    pLineRenderer->renderShader.everyObjectParams.colour = segment.colour;
    pLineRenderer->renderShader.everyObjectParams.worldViewProjection = udFloat4x4::create(viewProjectionMatrix * segment.originMatrix);
    pLineRenderer->renderShader.everyObjectParams.thickness.y = float(screenSize.x) / screenSize.y;
    pLineRenderer->renderShader.everyObjectParams.thickness.x = segment.thickness / udMax(screenSize.x, screenSize.y);

    vcShader_BindConstantBuffer(pLineRenderer->renderShader.pProgram, pLineRenderer->renderShader.uniform_everyObject, &pLineRenderer->renderShader.everyObjectParams, sizeof(pLineRenderer->renderShader.everyObjectParams));

    vcMesh_Render(segment.pMesh, segment.pointCount * 2, 0, vcMRM_TriangleStrip);
  }

  return true;
}
