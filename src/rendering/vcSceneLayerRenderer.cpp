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
  uint8_t grayPixel[4] = { 0x7f, 0x7f, 0x7f, 0xff };

  UD_ERROR_NULL(ppSceneLayerRenderer, udR_InvalidParameter_);
  UD_ERROR_NULL(pWorkerThreadPool, udR_InvalidParameter_);
  UD_ERROR_NULL(pSceneLayerURL, udR_InvalidParameter_);

  pSceneLayerRenderer = udAllocType(vcSceneLayerRenderer, 1, udAF_Zero);
  UD_ERROR_NULL(pSceneLayerRenderer, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(vcSceneLayer_Create(&pSceneLayerRenderer->pSceneLayer, pWorkerThreadPool, pSceneLayerURL));
  UD_ERROR_CHECK(vcSceneLayer_LoadNode(pSceneLayerRenderer->pSceneLayer, &pSceneLayerRenderer->pSceneLayer->root)); // load root data now

  UD_ERROR_CHECK(vcTexture_Create(&pSceneLayerRenderer->pEmptyTexture, 1, 1, grayPixel));

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
  if (ppSceneLayerRenderer == nullptr || *ppSceneLayerRenderer == nullptr)
    return udR_Success;

  vcSceneLayerRenderer *pSceneLayerRenderer = nullptr;

  pSceneLayerRenderer = *ppSceneLayerRenderer;
  *ppSceneLayerRenderer = nullptr;

  vcTexture_Destroy(&pSceneLayerRenderer->pEmptyTexture);

  vcSceneLayer_Destroy(&pSceneLayerRenderer->pSceneLayer);
  udFree(pSceneLayerRenderer);

  return udR_Success;
}

bool vcSceneLayerRenderer_IsNodeVisible(vcSceneLayerNode *pNode, const udDouble4 frustumPlanes[6])
{
  return -1 < udFrustumTest(frustumPlanes, pNode->minimumBoundingSphere.position, udDouble3::create(pNode->minimumBoundingSphere.radius));
}

double vcSceneLayerRenderer_CalculateNodeScreenSize(vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  udDouble4 center = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position, 1.0);
  if (center.w == 0.0) // if you're on the node position
    return (double)udMax(screenResolution.x, screenResolution.y);

  udDouble4 p1 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(pNode->minimumBoundingSphere.radius, 0.0, 0.0), 1.0);
  udDouble4 p2 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(0.0, pNode->minimumBoundingSphere.radius, 0.0), 1.0);
  center /= center.w;
  p1 /= p1.w;
  p2 /= p2.w;

  double r1 = udMag(center.toVector3() - p1.toVector3());
  double r2 = udMag(center.toVector3() - p2.toVector3());
  udDouble2 screenSize = udDouble2::create(screenResolution.x * r1, screenResolution.y * r2);
  return 2.0 * udMax(screenSize.x, screenSize.y);
}

void vcSceneLayerRenderer_RenderNode(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayerNode *pNode, const udFloat4 *pColourOverride, bool shadowsPass)
{
  udUnused(pSceneLayerRenderer);

  for (size_t geometry = 0; geometry < pNode->geometryDataCount; ++geometry)
  {
    if (!pNode->pGeometryData[geometry].loaded)
      continue;

    vcTexture *pDrawTexture = pSceneLayerRenderer->pEmptyTexture;
    if (pNode->textureDataCount > 0 && pNode->pTextureData[0].loaded)
      pDrawTexture = pNode->pTextureData[0].pTexture; // TODO: (EVC-542) read the actual material data

    vcPolyModelPass passType = vcPMP_Standard;
    if (shadowsPass)
      passType = vcPMP_Shadows;
    else if (pColourOverride != nullptr)
      passType = vcPMP_ColourOnly;

    vcPolygonModel_Render(pNode->pGeometryData[geometry].pModel, pSceneLayerRenderer->worldMatrix * pNode->pGeometryData[geometry].originMatrix, pSceneLayerRenderer->viewProjectionMatrix, passType, pDrawTexture, pColourOverride);

    // TODO: (EVC-542) other geometry types
  }
}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayerNode *pNode, const udUInt2 &screenResolution, const udFloat4 *pColourOverride, bool shadowsPass)
{
  if (!vcSceneLayer_TouchNode(pSceneLayerRenderer->pSceneLayer, pNode, pSceneLayerRenderer->cameraPosition))
    return false;

  if (!vcSceneLayerRenderer_IsNodeVisible(pNode, pSceneLayerRenderer->frustumPlanes))
  {
    vcSceneLayer_CheckNodePruneCandidancy(pSceneLayerRenderer->pSceneLayer, pNode);
    return true; // consume, but do nothing
  }

  double nodeScreenSize = vcSceneLayerRenderer_CalculateNodeScreenSize(pNode, pSceneLayerRenderer->worldViewProjectionMatrix, screenResolution);
  bool shouldRender = (pNode->childrenCount == 0 || nodeScreenSize < pNode->lodSelectionValue);
  bool canRender = (pNode->internalsLoadState >= vcSceneLayerNode::vcILS_NodeInternals);

  if (shouldRender)
  {
    if (vcSceneLayer_ExpandNodeForRendering(pSceneLayerRenderer->pSceneLayer, pNode))
    {
      vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, pColourOverride, shadowsPass);
      return true;
    }

    // continue, as child may be able to draw
  }

  bool allChildrenWereRendered = true;
  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    vcSceneLayerNode *pChildNode = &pNode->pChildren[i];
    allChildrenWereRendered = vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, pChildNode, screenResolution, pColourOverride, shadowsPass) && allChildrenWereRendered;
  }

  // If any children can't render, draw this node
  // This *may* cause some occlusion sometimes with partial children loaded, but its
  // better than sometimes no geometry being drawn.
  if (!allChildrenWereRendered && canRender)
  {
    if (vcSceneLayer_ExpandNodeForRendering(pSceneLayerRenderer->pSceneLayer, pNode))
    {
      vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, pColourOverride, shadowsPass);
      return true;
    }
  }

  return allChildrenWereRendered && !shouldRender;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, const udDouble4x4 &worldMatrix, const udDouble4x4 &viewProjectionMatrix, const udDouble3 &cameraPosition, const udUInt2 &screenResolution, const udFloat4 *pColourOverride /*= nullptr*/, bool shadowsPass /*= false*/)
{
  if (pSceneLayerRenderer == nullptr)
    return false;

  pSceneLayerRenderer->cameraPosition = cameraPosition;
  pSceneLayerRenderer->worldMatrix = worldMatrix;
  pSceneLayerRenderer->viewProjectionMatrix = viewProjectionMatrix;
  pSceneLayerRenderer->worldViewProjectionMatrix = pSceneLayerRenderer->viewProjectionMatrix * pSceneLayerRenderer->worldMatrix;

  // TODO: (EVC-548) This is duplicated work across i3s models
  // extract frustum planes
  udDouble4x4 transposedViewProjection = udTranspose(viewProjectionMatrix);
  pSceneLayerRenderer->frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  pSceneLayerRenderer->frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  pSceneLayerRenderer->frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  pSceneLayerRenderer->frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  pSceneLayerRenderer->frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  pSceneLayerRenderer->frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    pSceneLayerRenderer->frustumPlanes[j] /= udMag3(pSceneLayerRenderer->frustumPlanes[j]);

  return vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, &pSceneLayerRenderer->pSceneLayer->root, screenResolution, pColourOverride, shadowsPass);
}
