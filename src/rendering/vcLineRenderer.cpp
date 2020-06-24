#include "vcLineRenderer.h"
#include "vcShader.h"
#include "udChunkedArray.h"
#include "vcMesh.h"

struct vcLineInstance
{
  udFloat4 colour;
  float width;
  size_t segmentCount;
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
      udFloat4 nearPlane;
      udFloat4x4 worldViewProjection;
    } everyObjectParams;

  } renderShader;
};

udResult vcLineRenderer_Create(vcLineRenderer **ppLineRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcLineRenderer *pLineRenderer = udAllocType(vcLineRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pLineRenderer, udR_MemoryAllocationFailure);

  vcLineRenderer_ReloadShaders(pLineRenderer, pWorkerPool);

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

udResult vcLineRenderer_ReloadShaders(vcLineRenderer *pLineRenderer, udWorkerPool *pWorkerPool)
{
  udResult result;

  vcLineRenderer_DestroyShaders(pLineRenderer);

  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&pLineRenderer->renderShader.pProgram, pWorkerPool, "asset://assets/shaders/lineVertexShader", "asset://assets/shaders/lineFragmentShader", vcLineVertexLayout,
    [pLineRenderer](void  *)
    {
      vcShader_Bind(pLineRenderer->renderShader.pProgram);
      vcShader_GetConstantBuffer(&pLineRenderer->renderShader.uniform_everyObject, pLineRenderer->renderShader.pProgram, "u_EveryObject", sizeof(pLineRenderer->renderShader.everyObjectParams));
    }
  ), udR_InternalError);

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
  // Cleanup previous state
  vcMesh_Destroy(&pLine->pMesh);

  if (pointCount < 2)
    return udR_Success;

  udResult result;

  size_t segmentCount = (pointCount - (closed ? 0 : 1));

  //The basic idea is to break the line up into segments.
  size_t vertCount = segmentCount * 4;
  vcLineVertex *pVerts = nullptr;
  vcMesh *pMesh = nullptr;
  udDouble3 origin = pPoints[0];
  size_t indexCount = segmentCount * 6;
  int *pIndices = nullptr;
  size_t vertIndex = 0;
  size_t indIndex = 0;
  
  pVerts = udAllocType(vcLineVertex, vertCount, udAF_None);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);
  
  pIndices = udAllocType(int, indexCount, udAF_None);
  UD_ERROR_NULL(pIndices, udR_MemoryAllocationFailure);

  for (size_t i = 0; i < segmentCount; i++)
  {
    //Verts
    udDouble3 p0 = pPoints[i] - origin;
    udDouble3 p1 = pPoints[(i + 1) % pointCount] - origin;
    udDouble4 p0_neighbour = udDouble4::create(pPoints[(i + pointCount - 1) % pointCount] - origin, 1.0);
    udDouble4 p1_neighbour = udDouble4::create(pPoints[(i + 2) % pointCount] - origin, 1.0);

    if (i == 0 && !closed)
      p0_neighbour.w = 0.0; //Flag endpoint as open with w == 0

    if (i == pointCount - 2 && !closed)
        p0_neighbour.w = 0.0; //Flag endpoint as open with w == 0

    //p0
    pVerts[vertIndex + 0].position = udFloat4::create(udFloat3::create(p0), -1.0f);
    pVerts[vertIndex + 1].position = udFloat4::create(udFloat3::create(p0), 1.0f);
    pVerts[vertIndex + 0].partner = udFloat4::create(udFloat3::create(p1), -1.0f);
    pVerts[vertIndex + 1].partner = udFloat4::create(udFloat3::create(p1), 1.0f);
    pVerts[vertIndex + 0].neighbour = udFloat4::create(p0_neighbour);
    pVerts[vertIndex + 1].neighbour = udFloat4::create(p0_neighbour);

    //p1
    pVerts[vertIndex + 2].position = udFloat4::create(udFloat3::create(p1), 1.0f);
    pVerts[vertIndex + 3].position = udFloat4::create(udFloat3::create(p1), -1.0f);
    pVerts[vertIndex + 2].partner = udFloat4::create(udFloat3::create(p0), 1.0f);
    pVerts[vertIndex + 3].partner = udFloat4::create(udFloat3::create(p0), -1.0f);
    pVerts[vertIndex + 2].neighbour = udFloat4::create(p1_neighbour);
    pVerts[vertIndex + 3].neighbour = udFloat4::create(p1_neighbour);
    
    //Indics
    pIndices[indIndex + 0] = (int)vertIndex + 0;
    pIndices[indIndex + 1] = (int)vertIndex + 1;
    pIndices[indIndex + 2] = (int)vertIndex + 3;
    pIndices[indIndex + 3] = (int)vertIndex + 0;
    pIndices[indIndex + 4] = (int)vertIndex + 3;
    pIndices[indIndex + 5] = (int)vertIndex + 2;

    vertIndex += 4;
    indIndex += 6;
  }

  UD_ERROR_CHECK(vcMesh_Create(&pMesh, vcLineVertexLayout, (int)udLengthOf(vcLineVertexLayout), pVerts, (uint32_t)vertCount, pIndices, (uint32_t)indexCount, vcMF_None));

  pLine->pMesh = pMesh;
  pLine->originMatrix = udDouble4x4::translation(origin);
  pLine->colour = colour;
  pLine->width = width;
  pLine->segmentCount = segmentCount;

  result = udR_Success;
epilogue:

  udFree(pVerts);
  udFree(pIndices);
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

bool vcLineRenderer_Render(vcLineRenderer *pLineRenderer, const vcLineInstance *pLine, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, const udDouble4 &nearPlane)
{
  //Original camera near plane
  udDouble4 planeNormal = udDouble4::create(nearPlane.x, nearPlane.y, nearPlane.z, 0.0);
  double planeOffset = nearPlane.w;
  udDouble4 pointOnNearPlane = planeNormal * planeOffset;
  
  //Translated near plane
  pointOnNearPlane = pointOnNearPlane - pLine->originMatrix.axis.t;
  planeOffset = udDot4(pointOnNearPlane, planeNormal);
  udFloat4 newNearPlane = udFloat4::create(planeNormal);
  newNearPlane.w = float(planeOffset);

  vcShader_Bind(pLineRenderer->renderShader.pProgram);

  pLineRenderer->renderShader.everyObjectParams.colour = pLine->colour;
  pLineRenderer->renderShader.everyObjectParams.thickness.y = float(screenSize.x) / screenSize.y;
  pLineRenderer->renderShader.everyObjectParams.thickness.x = pLine->width / udMax(screenSize.x, screenSize.y);
  pLineRenderer->renderShader.everyObjectParams.nearPlane = newNearPlane;
  pLineRenderer->renderShader.everyObjectParams.worldViewProjection = udFloat4x4::create(viewProjectionMatrix * pLine->originMatrix);

  vcShader_BindConstantBuffer(pLineRenderer->renderShader.pProgram, pLineRenderer->renderShader.uniform_everyObject, &pLineRenderer->renderShader.everyObjectParams, sizeof(pLineRenderer->renderShader.everyObjectParams));

  vcMesh_Render(pLine->pMesh);

  return true;
}
