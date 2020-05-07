#include "vcLineRenderer.h"
#include "vcShader.h"
#include "udChunkedArray.h"
#include "vcMesh.h"

struct vcLineInstance
{
  udFloat4 colour;
  float width;
  size_t pointCount;
  vcMesh *pMesh;
  udDouble4x4 originMatrix;
};

struct vcLineRenderer
{
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

  vcLineRenderer_DestroyShaders(pLineRenderer);
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

udResult vcLineRenderer_CreateLine(vcLineInstance **ppLine)
{
  udResult result;
  vcLineInstance *pLine = nullptr;

  pLine = udAllocType(vcLineInstance, 1, udAF_Zero);
  UD_ERROR_NULL(pLine, udR_MemoryAllocationFailure);

  *ppLine = pLine;
  pLine = nullptr;
  result = udR_Success;
epilogue:

  return result;
}

udResult vcLineRenderer_UpdatePoints(vcLineInstance *pLine, const udDouble3 *pPoints, size_t pointCount, const udFloat4 &colour, float width, bool closed)
{
  udResult result;
  size_t realPointCount = (pointCount + (closed ? 1 : 0));
  size_t vertCount = realPointCount * 2;
  vcLineVertex *pVerts = nullptr;
  vcMesh *pMesh = nullptr;
  udDouble3 origin = pPoints[0];

  // Cleanup previous state
  vcMesh_Destroy(&pLine->pMesh);

  pVerts = udAllocType(vcLineVertex, vertCount, udAF_None);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < realPointCount; ++i)
  {
    udDouble3 p = pPoints[i % pointCount];

    udDouble3 prev = udDouble3::zero();
    udDouble3 next = udDouble3::zero();
    if (i == 0 && !closed)
      prev = p + udNormalize3(p - pPoints[i + 1]) * 0.1; // fake a vert
    else
      prev = pPoints[(i - 1 + pointCount) % pointCount];

    if (i == pointCount - 1 && !closed)
      next = p + udNormalize3(p - pPoints[i - 1]) * 0.1; // fake a vert
    else
      next = pPoints[(i + 1) % pointCount];

    size_t curIndex = i * 2;
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

  UD_ERROR_CHECK(vcMesh_Create(&pMesh, vcLineVertexLayout, (int)udLengthOf(vcLineVertexLayout), pVerts, (uint32_t)vertCount, nullptr, 0, vcMF_NoIndexBuffer));

  pLine->pMesh = pMesh;
  pLine->originMatrix = udDouble4x4::translation(origin);
  pLine->colour = colour;
  pLine->width = width;
  pLine->pointCount = realPointCount;

  result = udR_Success;
epilogue:

  udFree(pVerts);
  return result;
}

udResult vcLineRenderer_DestroyLine(vcLineInstance **ppLine)
{
  if (ppLine == nullptr || *ppLine == nullptr)
    return udR_Success;

  vcLineInstance *pLine = *ppLine;
  *ppLine = nullptr;

  vcMesh_Destroy(&pLine->pMesh);
  udFree(pLine);
  return udR_Success;
}

bool vcLineRenderer_Render(vcLineRenderer *pLineRenderer, const vcLineInstance *pLine, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize)
{
  vcShader_Bind(pLineRenderer->renderShader.pProgram);

  pLineRenderer->renderShader.everyObjectParams.colour = pLine->colour;
  pLineRenderer->renderShader.everyObjectParams.worldViewProjection = udFloat4x4::create(viewProjectionMatrix * pLine->originMatrix);
  pLineRenderer->renderShader.everyObjectParams.thickness.y = float(screenSize.x) / screenSize.y;
  pLineRenderer->renderShader.everyObjectParams.thickness.x = pLine->width / udMax(screenSize.x, screenSize.y);

  vcShader_BindConstantBuffer(pLineRenderer->renderShader.pProgram, pLineRenderer->renderShader.uniform_everyObject, &pLineRenderer->renderShader.everyObjectParams, sizeof(pLineRenderer->renderShader.everyObjectParams));

  vcMesh_Render(pLine->pMesh, (int)pLine->pointCount * 2, 0, vcMRM_TriangleStrip);

  return true;
}
