
#include "vcWaterRenderer.h"

#include "udChunkedArray.h"

#include "gl/vcShader.h"
#include "gl/vcMesh.h"
#include "gl/vcTexture.h"

#include "vcCamera.h"
#include "vcTriangulate.h"

struct vcWaterVolume
{
  vcMesh *pMesh;
  int vertCount;

  udDouble2 min, max;
  udDouble4x4 origin;
};

static vcTexture *pNormalMapTexture = nullptr;

struct vcWaterRenderer
{
  udChunkedArray<vcWaterVolume> volumes;

  double totalTimePassed;

  struct
  {
    vcShader *pProgram;
    vcShaderConstantBuffer *uniform_everyFrameVert;
    vcShaderConstantBuffer *uniform_everyFrameFrag;
    vcShaderConstantBuffer *uniform_everyObject;
    vcShaderSampler *uniform_normalMap;
    vcShaderSampler *uniform_skybox;

    struct
    {
      udFloat4 u_time;
    } everyFrameVertParams;

    struct
    {
      udFloat4 u_specularDir;
      udFloat4x4 u_eyeNormalMatrix;
      udFloat4x4 u_inverseViewMatrix;
    } everyFrameFragParams;

    struct
    {
      udFloat4 u_colourAndSize;
      udFloat4x4 u_modelViewMatrix;
      udFloat4x4 u_worldViewProjectionMatrix;
    } everyObjectParams;

  } renderShader;
};

static int gRefCount = 0;
udResult vcWaterRenderer_Init()
{
  udResult result;
  gRefCount++;

  UD_ERROR_IF(gRefCount != 1, udR_Success);

  UD_ERROR_IF(!vcTexture_CreateFromFilename(&pNormalMapTexture, "asset://assets/textures/waterNormalMap.jpg", nullptr, nullptr, vcTFM_Linear, true, vcTWM_Repeat), udR_InternalError);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcWaterRenderer_Destroy()
{
  udResult result;
  --gRefCount;

  UD_ERROR_IF(gRefCount != 0, udR_Success);

  vcTexture_Destroy(&pNormalMapTexture);

  result = udR_Success;

epilogue:
  return result;
}

udResult vcWaterRenderer_Create(vcWaterRenderer **ppWaterRenderer)
{
  udResult result;

  vcWaterRenderer *pWaterRenderer = nullptr;

  pWaterRenderer = udAllocType(vcWaterRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pWaterRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(pWaterRenderer->volumes.Init(32));

  UD_ERROR_IF(!vcShader_CreateFromFile(&pWaterRenderer->renderShader.pProgram, "asset://assets/shaders/waterVertexShader", "asset://assets/shaders/waterFragmentShader", vcP3UV2VertexLayout), udR_InternalError);
  UD_ERROR_IF(!vcShader_Bind(pWaterRenderer->renderShader.pProgram), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pWaterRenderer->renderShader.uniform_normalMap, pWaterRenderer->renderShader.pProgram, "SPIRV_Cross_Combinedu_normalMapsampler0"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetSamplerIndex(&pWaterRenderer->renderShader.uniform_skybox, pWaterRenderer->renderShader.pProgram, "SPIRV_Cross_Combinedu_skyboxsampler1"), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pWaterRenderer->renderShader.uniform_everyFrameVert, pWaterRenderer->renderShader.pProgram, "u_EveryFrameVert", sizeof(pWaterRenderer->renderShader.everyFrameVertParams)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pWaterRenderer->renderShader.uniform_everyFrameFrag, pWaterRenderer->renderShader.pProgram, "u_EveryFrameFrag", sizeof(pWaterRenderer->renderShader.everyFrameFragParams)), udR_InternalError);
  UD_ERROR_IF(!vcShader_GetConstantBuffer(&pWaterRenderer->renderShader.uniform_everyObject, pWaterRenderer->renderShader.pProgram, "u_EveryObject", sizeof(pWaterRenderer->renderShader.everyObjectParams)), udR_InternalError);

  UD_ERROR_CHECK(vcWaterRenderer_Init());

  *ppWaterRenderer = pWaterRenderer;
  pWaterRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pWaterRenderer != nullptr)
    vcWaterRenderer_Destroy(&pWaterRenderer);
  return result;
}

udResult vcWaterRenderer_Destroy(vcWaterRenderer **ppWaterRenderer)
{
  if (!ppWaterRenderer || !(*ppWaterRenderer))
    return udR_InvalidParameter_;

  vcWaterRenderer *pWaterRenderer = (*ppWaterRenderer);
  *ppWaterRenderer = nullptr;

  vcShader_DestroyShader(&pWaterRenderer->renderShader.pProgram);

  vcWaterRenderer_ClearAllVolumes(pWaterRenderer);
  pWaterRenderer->volumes.Deinit();

  udFree(pWaterRenderer);
  vcWaterRenderer_Destroy();

  return udR_Success;
}

udResult vcWaterRenderer_AddVolume(vcWaterRenderer *pWaterRenderer, const udGeoZone &geoZone, double altitude, udDouble3 *pPoints, size_t pointCount)
{
  udResult result;

  vcWaterVolume pVolume = {};
  std::vector<udDouble2> triangleList;
  udDouble2 *pLocalPoints = nullptr;
  vcP3UV2Vertex *pVerts = nullptr;
  udDouble3 p0 = udDouble3::zero();

  pLocalPoints = udAllocType(udDouble2, pointCount, udAF_Zero);
  UD_ERROR_NULL(pLocalPoints, udR_MemoryAllocationFailure);

  pVolume.min = udDouble2::zero();
  pVolume.max = udDouble2::zero();
  for (size_t i = 0; i < pointCount; ++i)
  {
    udDouble2 p = pPoints[i].toVector2() - pPoints[0].toVector2();

    pVolume.min.x = udMin(pVolume.min.x, p.x);
    pVolume.min.y = udMin(pVolume.min.y, p.y);
    pVolume.max.x = udMax(pVolume.max.x, p.x);
    pVolume.max.y = udMax(pVolume.max.y, p.y);

    pLocalPoints[i] = p;
  }

  // TODO: Consider putting this function work in another thread.
  if (!vcTriangulate_Process(pLocalPoints, (int)pointCount, &triangleList))
  {
    // Failed to triangulate the entire polygon
    // TODO: Not sure how to handle this as the polygon it generates could still be almost complete.
  }

  pVolume.vertCount = int(triangleList.size());
  pVerts = udAllocType(vcP3UV2Vertex, pVolume.vertCount, udAF_Zero);
  UD_ERROR_NULL(pVerts, udR_MemoryAllocationFailure);

  pVolume.origin = udDouble4x4::translation(udGeoZone_LatLongToCartesian(geoZone, pPoints[0], true));
  p0 = pVolume.origin.axis.t.toVector3();
  for (size_t i = 0; i < triangleList.size(); ++i)
  {
    udDouble3 p = udGeoZone_LatLongToCartesian(geoZone, pPoints[0] + udDouble3::create(triangleList[i].x, triangleList[i].y, altitude), true);
    pVerts[i].position = udFloat3::create(p - p0);
    pVerts[i].uv = udFloat2::create(triangleList[i]);
  }

  UD_ERROR_IF(vcMesh_Create(&pVolume.pMesh, vcP3UV2VertexLayout, (int)udLengthOf(vcP3UV2VertexLayout), pVerts, (uint32_t)pVolume.vertCount, nullptr, 0, vcMF_Dynamic | vcMF_NoIndexBuffer), udR_InternalError);

  UD_ERROR_CHECK(pWaterRenderer->volumes.PushBack(pVolume));

  result = udR_Success;
epilogue:
  udFree(pLocalPoints);
  udFree(pVerts);

  if (result != udR_Success)
    vcMesh_Destroy(&pVolume.pMesh);

  return result;
}

void vcWaterRenderer_ClearAllVolumes(vcWaterRenderer *pWaterRenderer)
{
  for (size_t i = 0; i < pWaterRenderer->volumes.length; ++i)
  {
    vcWaterVolume *pVolume = &pWaterRenderer->volumes[i];

    vcMesh_Destroy(&pVolume->pMesh);
  }

  pWaterRenderer->volumes.Clear();
}

bool vcWaterRenderer_Render(vcWaterRenderer *pWaterRenderer, const udDouble4x4 &view, const udDouble4x4 &viewProjection, vcTexture *pSkyboxTexture, double deltaTime)
{
  bool success = true;

  if (pWaterRenderer->volumes.length == 0)
    return success;

  static const udFloat3 specularDir = udFloat3::create(-0.5f, -0.5f, -0.5f);

  udFloat4x4 inverseView = udFloat4x4::create(udInverse(view));

  pWaterRenderer->totalTimePassed += deltaTime;

  pWaterRenderer->renderShader.everyFrameVertParams.u_time = udFloat4::create((float)pWaterRenderer->totalTimePassed, 0.0f, 0.0f, 0.0f);
  pWaterRenderer->renderShader.everyFrameFragParams.u_specularDir = udFloat4::create(specularDir.x, specularDir.y, specularDir.z, 0.0f);
  pWaterRenderer->renderShader.everyFrameFragParams.u_eyeNormalMatrix = udFloat4x4::create(udTranspose(inverseView));
  pWaterRenderer->renderShader.everyFrameFragParams.u_inverseViewMatrix = udFloat4x4::create(inverseView);
  vcShader_Bind(pWaterRenderer->renderShader.pProgram);

  vcShader_BindTexture(pWaterRenderer->renderShader.pProgram, pNormalMapTexture, 0, pWaterRenderer->renderShader.uniform_normalMap);
  vcShader_BindTexture(pWaterRenderer->renderShader.pProgram, pSkyboxTexture, 1, pWaterRenderer->renderShader.uniform_skybox);

  vcShader_BindConstantBuffer(pWaterRenderer->renderShader.pProgram, pWaterRenderer->renderShader.uniform_everyFrameVert, &pWaterRenderer->renderShader.everyFrameVertParams, sizeof(pWaterRenderer->renderShader.everyFrameVertParams));
  vcShader_BindConstantBuffer(pWaterRenderer->renderShader.pProgram, pWaterRenderer->renderShader.uniform_everyFrameFrag, &pWaterRenderer->renderShader.everyFrameFragParams, sizeof(pWaterRenderer->renderShader.everyFrameFragParams));

  for (size_t i = 0; i < pWaterRenderer->volumes.length; ++i)
  {
    vcWaterVolume *pVolume = &pWaterRenderer->volumes[i];

    pWaterRenderer->renderShader.everyObjectParams.u_colourAndSize = udFloat4::create(0.0f, 102.0f / 255.0f, 204.0f / 255.0f, 0.0f);
    pWaterRenderer->renderShader.everyObjectParams.u_colourAndSize.w = float(30000.0 * udMag2(pVolume->max - pVolume->min));
    pWaterRenderer->renderShader.everyObjectParams.u_modelViewMatrix = udFloat4x4::create(view * pVolume->origin);
    pWaterRenderer->renderShader.everyObjectParams.u_worldViewProjectionMatrix = udFloat4x4::create(viewProjection * pVolume->origin);
    vcShader_BindConstantBuffer(pWaterRenderer->renderShader.pProgram, pWaterRenderer->renderShader.uniform_everyObject, &pWaterRenderer->renderShader.everyObjectParams, sizeof(pWaterRenderer->renderShader.everyObjectParams));

    if (vcMesh_Render(pVolume->pMesh, pVolume->vertCount, 0, vcMRM_Triangles) != udR_Success)
      success = false;
  }

  return success;
}
