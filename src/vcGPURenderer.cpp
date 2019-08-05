#include "vcGPURenderer.h"

#include "vcSettings.h"
#include "gl/vcRenderShaders.h"

#if ALLOW_EXPERIMENT_GPURENDER

enum
{
  vcGPURenderer_MaxPointCountPerBlock = 65536
};

struct vcGPURenderer
{
  vdkGPURenderContext *pRenderer;
  vdkGPURenderInterface gpuInterface;
  const vdkGPURenderModel *pUploadedModelData; // For tracking which model shader current constants refer to

  struct
  {
    vcGPURendererPointRenderMode mode;
    vcVertexLayoutTypes *pVertexLayout;
    int vertexLayoutCount;
    int vertCountPerPoint;
  } pointRendering;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *pEveryObjectConstantBuffer;

    struct
    {
      udFloat4x4 u_world;
      udFloat4 u_colour; // alpha is id
    } everyObject;

  } presentShader;
};

struct vcBlockRenderVertexBuffer
{
  enum { MAX_VBOS = 9 };
  vcMesh *pMesh;

  // TEMP HARD CODED VERTEX
  // The vertex buffer is divided into "pieces", which correspond to child block
  // sizes. The main vbo is populated from pieces as required
  struct Vertex
  {
    udFloat4 pos;
    uint32_t color;
  };

  struct QuadVertex : Vertex
  {
    float corner[2];
  };

  Vertex *pDivisionData[MAX_VBOS]; // In-memory temp buffer of data waiting to be uploaded as a VBO before being freed

  int divisionPointCounts[MAX_VBOS]; // Count of points in each division
  //int divisionVertexStarts[MAX_VBOS]; // Start value of each division (sum of previous counts)
  int divisionCount;
  int totalPointCount;
};

vcVertexLayoutTypes vcGPURenderer_QuadVertexLayout[] = { vcVLT_Position4, vcVLT_ColourBGRA, vcVLT_QuadCorner };
vcVertexLayoutTypes vcGPURenderer_PointVertexLayout[] = { vcVLT_Position4, vcVLT_ColourBGRA };

udResult vcGPURenderer_CreatePointRenderingData(vcGPURenderer *pBlockRenderer)
{
  udResult result;

  if (pBlockRenderer->pointRendering.mode == vcBRPRM_Quads)
  {
    pBlockRenderer->pointRendering.vertCountPerPoint = 6;
    pBlockRenderer->pointRendering.pVertexLayout = vcGPURenderer_QuadVertexLayout;
    pBlockRenderer->pointRendering.vertexLayoutCount = (int)udLengthOf(vcGPURenderer_QuadVertexLayout);
    UD_ERROR_IF(!vcShader_CreateFromText(&pBlockRenderer->presentShader.pProgram, g_udGPURenderQuadVertexShader, g_udGPURenderQuadFragmentShader, pBlockRenderer->pointRendering.pVertexLayout, pBlockRenderer->pointRendering.vertexLayoutCount), udR_Failure_);
  }
  else if (pBlockRenderer->pointRendering.mode == vcBRPRM_GeometryShader)
  {
    pBlockRenderer->pointRendering.vertCountPerPoint = 1;
    pBlockRenderer->pointRendering.pVertexLayout = vcGPURenderer_PointVertexLayout;
    pBlockRenderer->pointRendering.vertexLayoutCount = (int)udLengthOf(vcGPURenderer_PointVertexLayout);
    UD_ERROR_IF(!vcShader_CreateFromText(&pBlockRenderer->presentShader.pProgram, g_udGPURenderGeomVertexShader, g_udGPURenderGeomFragmentShader, pBlockRenderer->pointRendering.pVertexLayout, pBlockRenderer->pointRendering.vertexLayoutCount, g_udGPURenderGeomGeometryShader), udR_Failure_);
  }

  UD_ERROR_IF(!vcShader_Bind(pBlockRenderer->presentShader.pProgram), udR_Failure_);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pBlockRenderer->presentShader.pEveryObjectConstantBuffer, pBlockRenderer->presentShader.pProgram, "u_EveryObject", sizeof(pBlockRenderer->presentShader.everyObject)), udR_Failure_);

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcShader_DestroyShader(&pBlockRenderer->presentShader.pProgram);

  return result;
}

void vcGPURenderer_BeginRender(void *pContext, vdkRenderView *pView)
{
  vcGPURenderer *pBlockRenderer = (vcGPURenderer*)pContext;
  udUnused(pView);

  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true);
  vcGLState_SetBlendMode(vcGLSBM_None);

  vcShader_Bind(pBlockRenderer->presentShader.pProgram);
}

vdkError vcGPURenderer_CreateVertexBuffer(void *pContext, const vdkGPURenderVertexData *pVertexData, void **ppVertexBuffer)
{
  udResult result;
  vcGPURenderer *pBlockRenderer = (vcGPURenderer*)pContext;
  vcBlockRenderVertexBuffer *pVertexBuffer = nullptr;
  int base = 0;
  float voxelHalfSize = 0.0f;
  uint32_t colorOffset = vdkGPURender_GetAttributeOffset(vdkAC_ARGB, pVertexData);
  size_t intensityOffset = vdkGPURender_GetAttributeOffset(vdkAC_Intensity, pVertexData);

  pVertexBuffer = udAllocType(vcBlockRenderVertexBuffer, 1, udAF_Zero);

  UD_ERROR_NULL(pVertexBuffer, udR_MemoryAllocationFailure);
  pVertexBuffer->totalPointCount = (int)vdkGPURender_GetPointCount(pVertexData);
  voxelHalfSize = (float)vdkGPURender_GetVoxelHalfSize(pVertexData);

  pVertexBuffer->divisionCount = vdkGPURender_GetDivisionCount(pVertexData);
  for (int div = 0; div < pVertexBuffer->divisionCount; ++div)
  {
    pVertexBuffer->divisionPointCounts[div] = vdkGPURender_GetDivisionPointCount(pVertexData, div);

    // HARD CODING TO fixed vert format for the moment
    if (pBlockRenderer->pointRendering.mode == vcBRPRM_Quads)
    {
      vcBlockRenderVertexBuffer::QuadVertex *pVerts = udAllocType(vcBlockRenderVertexBuffer::QuadVertex, pVertexBuffer->divisionPointCounts[div] * pBlockRenderer->pointRendering.vertCountPerPoint, udAF_None);
      UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

      for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
      {
        vcBlockRenderVertexBuffer::QuadVertex vert = {};
        double position[3] = {};

        vdkGPURender_GetPosition(pVertexData, base + j, position);
        vert.pos = udFloat4::create(udFloat3::create((float)position[0], (float)position[1], (float)position[2]), voxelHalfSize);

        // triangle #1
        vert.corner[0] = -1;
        vert.corner[1] = -1;
        pVerts[j * 6 + 0] = vert;

        vert.corner[0] = 1;
        vert.corner[1] = -1;
        pVerts[j * 6 + 1] = vert;

        vert.corner[0] = -1;
        vert.corner[1] = 1;
        pVerts[j * 6 + 2] = vert;

        // triangle #2
        vert.corner[0] = -1;
        vert.corner[1] = 1;
        pVerts[j * 6 + 3] = vert;

        vert.corner[0] = 1;
        vert.corner[1] = 1;
        pVerts[j * 6 + 4] = vert;

        vert.corner[0] = 1;
        vert.corner[1] = -1;
        pVerts[j * 6 + 5] = vert;
      }

      pVertexBuffer->pDivisionData[div] = pVerts;
    }
    else if (pBlockRenderer->pointRendering.mode == vcBRPRM_GeometryShader)
    {
      vcBlockRenderVertexBuffer::Vertex *pVerts = udAllocType(vcBlockRenderVertexBuffer::Vertex, pVertexBuffer->divisionPointCounts[div] * pBlockRenderer->pointRendering.vertCountPerPoint, udAF_None);
      UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

      for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
      {
        double position[3] = {};
        vdkGPURender_GetPosition(pVertexData, base + j, position);
        pVerts[j].pos = udFloat4::create(udFloat3::create((float)position[0], (float)position[1], (float)position[2]), voxelHalfSize);
      }

      pVertexBuffer->pDivisionData[div] = pVerts;
    }

    if (vdkGPURender_HasAttribute(vdkAC_ARGB, pVertexData))
    {
      if (pBlockRenderer->pointRendering.mode == vcBRPRM_Quads)
      {
        vcBlockRenderVertexBuffer::QuadVertex **ppBuffer = (vcBlockRenderVertexBuffer::QuadVertex**)(pVertexBuffer->pDivisionData);

        for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
        {
          uint32_t col = *(uint32_t*)(vdkGPURender_GetAttributes(pVertexData, (base + j)) + colorOffset);

          for (int v = 0; v < pBlockRenderer->pointRendering.vertCountPerPoint; ++v)
          {
            ppBuffer[div][j * pBlockRenderer->pointRendering.vertCountPerPoint + v].color = col;
          }
        }
      }
      else if (pBlockRenderer->pointRendering.mode == vcBRPRM_GeometryShader)
      {
        for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
        {
          uint32_t col = *(uint32_t*)(vdkGPURender_GetAttributes(pVertexData, (base + j)) + colorOffset);

          for (int v = 0; v < pBlockRenderer->pointRendering.vertCountPerPoint; ++v)
          {
            pVertexBuffer->pDivisionData[div][j * pBlockRenderer->pointRendering.vertCountPerPoint + v].color = col;
          }
        }
      }
    }
    else if (vdkGPURender_HasAttribute(vdkAC_Intensity, pVertexData))
    {
      if (pBlockRenderer->pointRendering.mode == vcBRPRM_Quads)
      {
        vcBlockRenderVertexBuffer::QuadVertex **ppBuffer = (vcBlockRenderVertexBuffer::QuadVertex**)(pVertexBuffer->pDivisionData);

        for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
        {
          uint32_t col = 0xff000000 | *(uint16_t*)(vdkGPURender_GetAttributes(pVertexData, (base + j)) + intensityOffset);

          for (int v = 0; v < pBlockRenderer->pointRendering.vertCountPerPoint; ++v)
          {
            ppBuffer[div][j * pBlockRenderer->pointRendering.vertCountPerPoint + v].color = col;
          }
        }
      }
      else if (pBlockRenderer->pointRendering.mode == vcBRPRM_GeometryShader)
      {
        for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
        {
          uint32_t col = 0xff000000 | *(uint16_t*)(vdkGPURender_GetAttributes(pVertexData, (base + j)) + intensityOffset);

          for (int v = 0; v < pBlockRenderer->pointRendering.vertCountPerPoint; ++v)
          {
            pVertexBuffer->pDivisionData[div][j * pBlockRenderer->pointRendering.vertCountPerPoint + v].color = col;
          }
        }
      }
    }
    else // Colour by height in the absence of anything else
    {
      for (int j = 0; j < pVertexBuffer->divisionPointCounts[div]; ++j)
      {
        double res[3] = {};
        vdkGPURender_GetPosition(pVertexData, (base + j), res);
        uint32_t col = 0xff000000 | (0x010101 * int(res[2] * 255));
        for (int v = 0; v < pBlockRenderer->pointRendering.vertCountPerPoint; ++v)
        {
          pVertexBuffer->pDivisionData[div][j * pBlockRenderer->pointRendering.vertCountPerPoint + v].color = col;
        }
      }
    }

    base += pVertexBuffer->divisionPointCounts[div];
  }
  UD_ERROR_IF((size_t)base != vdkGPURender_GetPointCount(pVertexData), udR_InternalError);

  *ppVertexBuffer = (void*)pVertexBuffer;
  pVertexBuffer = nullptr;
  result = udR_Success;

epilogue:
  if (pVertexBuffer)
  {
    for (size_t i = 0; i < udLengthOf(pVertexBuffer->pDivisionData); ++i)
      udFree(pVertexBuffer->pDivisionData[i]);
    udFree(pVertexBuffer);
  }

  return (result == udR_Success) ? vE_Success : vE_Failure;
}

vdkError vcGPURenderer_UploadVertexBuffer(void *pContext, const vdkGPURenderModel * /*pModel*/, void *pVertexBuffer)
{
  udResult result;
  vcGPURenderer *pBlockRenderer = (vcGPURenderer*)pContext;
  vcBlockRenderVertexBuffer *pVB = (vcBlockRenderVertexBuffer*)pVertexBuffer;

  if (pVB->pMesh == nullptr)
  {
    UD_ERROR_CHECK(vcMesh_Create(&pVB->pMesh, pBlockRenderer->pointRendering.pVertexLayout, pBlockRenderer->pointRendering.vertexLayoutCount, nullptr, pVB->totalPointCount * pBlockRenderer->pointRendering.vertCountPerPoint, nullptr, 0, vcMF_Dynamic | vcMF_NoIndexBuffer));

    int base = 0;
    for (int div = 0; div < pVB->divisionCount; ++div)
    {
      UD_ERROR_CHECK(vcMesh_UploadSubData(pVB->pMesh, pBlockRenderer->pointRendering.pVertexLayout, pBlockRenderer->pointRendering.vertexLayoutCount, base * pBlockRenderer->pointRendering.vertCountPerPoint, pVB->pDivisionData[div], pVB->divisionPointCounts[div] * pBlockRenderer->pointRendering.vertCountPerPoint, nullptr, 0));
      base += pVB->divisionPointCounts[div];
    }
  }

  result = udR_Success;

epilogue:
  return (result == udR_Success) ? vE_Success : vE_Failure;
}

/*----------------------------------------------------------------------------------------------------*/
vdkError vcGPURenderer_RenderVertexBuffer(void *pContext, void *pVertexBuffer, uint16_t divisionsMask, const double matrix[16], const int modelIndex)
{
  udUnused(divisionsMask);

  udResult result;
  vcGPURenderer *pBlockRenderer = (vcGPURenderer*)pContext;
  vcBlockRenderVertexBuffer *pVB = (vcBlockRenderVertexBuffer*)pVertexBuffer;

  // Upload shader constants
  pBlockRenderer->presentShader.everyObject.u_world = udFloat4x4::create(udDouble4x4::create(matrix));
  pBlockRenderer->presentShader.everyObject.u_colour = udFloat4::create(1, 1, 1, float(modelIndex + 1) / 255.0f); // encode id in alpha
  vcShader_BindConstantBuffer(pBlockRenderer->presentShader.pProgram, pBlockRenderer->presentShader.pEveryObjectConstantBuffer, &pBlockRenderer->presentShader.everyObject, sizeof(pBlockRenderer->presentShader.everyObject));

  int drawElementCount = 0;
  vcMeshRenderMode renderMode = vcMRM_Triangles;
  if (pBlockRenderer->pointRendering.mode == vcBRPRM_Quads)
  {
    drawElementCount = pVB->totalPointCount * 2;
    renderMode = vcMRM_Triangles;
  }
  else if (pBlockRenderer->pointRendering.mode == vcBRPRM_GeometryShader)
  {
    drawElementCount = pVB->totalPointCount;
    renderMode = vcMRM_Points;
  }
  UD_ERROR_IF(!vcMesh_Render(pVB->pMesh, drawElementCount, 0, renderMode), udR_Failure_);

  result = udR_Success;

epilogue:
  return (result == udR_Success) ? vE_Success : vE_Failure;
}

void vcGPURenderer_EndRender(void * /*pGPUContext*/)
{
  vcShader_Bind(nullptr);
}

vdkError vcGPURenderer_DestroyVertexBuffer(void * /*pContext*/, void *pVertexBuffer)
{
  vcBlockRenderVertexBuffer *pVB = (vcBlockRenderVertexBuffer*)pVertexBuffer;
  vcMesh_Destroy(&pVB->pMesh);
  for (int i = 0; i < pVB->divisionCount; ++i)
    udFree(pVB->pDivisionData[i]);
  udFree(pVertexBuffer);
  return vE_Success;
}

udResult vcGPURenderer_Create(vcGPURenderer **ppBlockRenderer, vcGPURendererPointRenderMode renderMode, int targetPointCount)
{
  udResult result;
  vcGPURenderer *pBlockRenderer = nullptr;

  pBlockRenderer = udAllocType(vcGPURenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pBlockRenderer, udR_MemoryAllocationFailure);

  pBlockRenderer->pointRendering.mode = renderMode;

  pBlockRenderer->gpuInterface.pBeginRender = vcGPURenderer_BeginRender;
  pBlockRenderer->gpuInterface.pEndRender = vcGPURenderer_EndRender;
  pBlockRenderer->gpuInterface.pCreateVertexBuffer = vcGPURenderer_CreateVertexBuffer;
  pBlockRenderer->gpuInterface.pUploadVertexBuffer = vcGPURenderer_UploadVertexBuffer;
  pBlockRenderer->gpuInterface.pRenderVertexBuffer = vcGPURenderer_RenderVertexBuffer;
  pBlockRenderer->gpuInterface.pDestroyVertexBuffer = vcGPURenderer_DestroyVertexBuffer;

  if (vdkGPURender_Create(&pBlockRenderer->pRenderer, &pBlockRenderer->gpuInterface, pBlockRenderer, targetPointCount) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  UD_ERROR_CHECK(vcGPURenderer_CreatePointRenderingData(pBlockRenderer));

  *ppBlockRenderer = pBlockRenderer;
  result = udR_Success;

epilogue:
  if (result != udR_Success)
  {
    vdkGPURender_Destroy(&pBlockRenderer->pRenderer);
    udFree(pBlockRenderer);
  }
  return result;
}

udResult vcGPURenderer_Destroy(vcGPURenderer **ppBlockRender)
{
  if (ppBlockRender == nullptr || *ppBlockRender == nullptr)
    return udR_Success;

  udResult result = udR_Success;

  vcGPURenderer *pBlockRenderer = *ppBlockRender;
  *ppBlockRender = nullptr;

  vcShader_DestroyShader(&pBlockRenderer->presentShader.pProgram);

  if (vdkGPURender_Destroy(&pBlockRenderer->pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  udFree(pBlockRenderer);
  return result;
}

#endif // ALLOW_EXPERIMENT_GPURENDER
