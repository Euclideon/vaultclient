#include "vcSceneLayerRenderer.h"
#include "vcSceneLayer_Internal.h"

#include "vcSceneLayer.h"
#include "vcPolygonModel.h"

#include "gl/vcTexture.h"
#include "gl/vcGLState.h"

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayerRenderer, udWorkerPool *pWorkerThreadPool, const char *pSceneLayerURL)
{
  udResult result;
  vcSceneLayerRenderer *pSceneLayerRenderer = nullptr;

  UD_ERROR_NULL(ppSceneLayerRenderer, udR_InvalidParameter_);
  UD_ERROR_NULL(pWorkerThreadPool, udR_InvalidParameter_);
  UD_ERROR_NULL(pSceneLayerURL, udR_InvalidParameter_);

  pSceneLayerRenderer = udAllocType(vcSceneLayerRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayerRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcSceneLayer_Create(&pSceneLayerRenderer->pSceneLayer, pWorkerThreadPool, pSceneLayerURL));

  *ppSceneLayerRenderer = pSceneLayerRenderer;
  pSceneLayerRenderer = nullptr;
  result = udR_Success;

epilogue:
  if (pSceneLayerRenderer != nullptr)
  {
    vcSceneLayer_Destroy(&pSceneLayerRenderer->pSceneLayer);
    udFree(pSceneLayerRenderer);
  }

  return result;
}

udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayerRenderer)
{
  udResult result;
  vcSceneLayerRenderer *pSceneLayerRenderer = nullptr;

  UD_ERROR_NULL(ppSceneLayerRenderer, udR_InvalidParameter_);

  pSceneLayerRenderer = *ppSceneLayerRenderer;
  *ppSceneLayerRenderer = nullptr;

  vcSceneLayer_Destroy(&pSceneLayerRenderer->pSceneLayer);
  udFree(pSceneLayerRenderer);

  result = udR_Success;

epilogue:
  return result;
}

bool vcSceneLayerRenderer_IsNodeVisible(vcSceneLayerNode *pNode, const udDouble4 frustumPlanes[6])
{
  return -1 < udFrustumTest(frustumPlanes, pNode->minimumBoundingSphere.position, udDouble3::create(pNode->minimumBoundingSphere.radius));
}

double vcSceneLayerRenderer_CalculateNodeScreenSize(vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  udDouble4 center = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position, 1.0);
  if (center.w == 0.0) // if you're on the node position
    return 0;

  udDouble4 p1 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(pNode->minimumBoundingSphere.radius, 0.0, 0.0), 1.0);
  udDouble4 p2 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(0.0, pNode->minimumBoundingSphere.radius, 0.0), 1.0);
  center /= center.w;
  p1 /= p1.w;
  p2 /= p2.w;

  // TODO: (EVC-548) Verify this `2.0` is correct (because I only calculate the half size above)
  // I'm 75% sure it is, but changing it to 1.0 does cause a very slight noticeable LOD pop, but is that meant to happen?
  double r1 = udMag(center.toVector3() - p1.toVector3()) * 1.0; // 2.0 or 1.0?
  double r2 = udMag(center.toVector3() - p2.toVector3()) * 1.0; // 2.0 or 1.0?
  udDouble2 screenSize = udDouble2::create(screenResolution.x * r1, screenResolution.y * r2);
  return udMax(screenSize.x, screenSize.y);
}

void vcSceneLayerRenderer_RenderNode(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udFloat4 *pColourOverride)
{
  udUnused(pSceneLayerRenderer);

  for (size_t geometry = 0; geometry < pNode->geometryDataCount; ++geometry)
  {
    if (!pNode->pGeometryData[geometry].loaded)
      continue;

    vcTexture *pDrawTexture = nullptr;
    if (pNode->textureDataCount > 0 && pNode->pTextureData[0].loaded)
      pDrawTexture = pNode->pTextureData[0].pTexture; // TODO: (EVC-542) read the actual material data

    vcPolygonModel_Render(pNode->pGeometryData[geometry].pModel, pNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, (pColourOverride == nullptr) ? vcPMP_Standard : vcPMP_ColourOnly, pDrawTexture, pColourOverride);

    // TODO: (EVC-542) other geometry types
  }
}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution, const udFloat4 *pColourOverride)
{
  if (!vcSceneLayerRenderer_IsNodeVisible(pNode, pSceneLayerRenderer->frustumPlanes))
    return true; // consume, but do nothing

  if (!vcSceneLayer_TouchNode(pSceneLayerRenderer->pSceneLayer, pNode))
    return false;

  double nodeScreenSize = vcSceneLayerRenderer_CalculateNodeScreenSize(pNode, viewProjectionMatrix, screenResolution);
  if (pNode->childrenCount == 0 || nodeScreenSize < pNode->lodSelectionValue)
  {
    vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, viewProjectionMatrix, pColourOverride);
    return true;
  }

  bool childrenAllRendered = true;
  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    vcSceneLayerNode *pChildNode = &pNode->pChildren[i];
    childrenAllRendered = vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, pChildNode, viewProjectionMatrix, screenResolution, pColourOverride) && childrenAllRendered;
  }

  // TODO: (EVC-548)
  // If children are not loaded in yet, render this LOD.
  // At the moment this can cause incorrect occlusion
  if (!childrenAllRendered)
    vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, viewProjectionMatrix, pColourOverride);

  return true;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, const udDouble4x4 &worldMatrix, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution, const udFloat4 *pColourOverride /*= nullptr*/)
{
  if (pSceneLayerRenderer == nullptr)
    return false;

  // TODO: (EVC-548) This is duplicated work across i3s models
  // extract frustum planes
  udDouble4x4 worldViewProjectionMatrix = viewProjectionMatrix * worldMatrix;
  udDouble4x4 transposedViewProjection = udTranspose(worldViewProjectionMatrix);
  pSceneLayerRenderer->frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  pSceneLayerRenderer->frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  pSceneLayerRenderer->frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  pSceneLayerRenderer->frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  pSceneLayerRenderer->frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  pSceneLayerRenderer->frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    pSceneLayerRenderer->frustumPlanes[j] /= udMag3(pSceneLayerRenderer->frustumPlanes[j]);

  return vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, &pSceneLayerRenderer->pSceneLayer->root, worldViewProjectionMatrix, screenResolution, pColourOverride);
}
