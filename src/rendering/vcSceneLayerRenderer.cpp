#include "vcSceneLayerRenderer.h"
#include "vcSceneLayer_Internal.h"

#include "vcSceneLayer.h"
#include "vcPolygonModel.h"

#include "gl/vcTexture.h"
#include "gl/vcGLState.h"

struct vcSceneLayerRenderer
{

};

udResult vcSceneLayerRenderer_Create(vcSceneLayerRenderer **ppSceneLayerRenderer)
{
  *ppSceneLayerRenderer = udAllocType(vcSceneLayerRenderer, 1, udAF_Zero);

  return udR_Success;
}

udResult vcSceneLayerRenderer_Destroy(vcSceneLayerRenderer **ppSceneLayerRenderer)
{
  udFree(*ppSceneLayerRenderer);

  return udR_Success;
}

// TODO: This is a copy of the implementation from vcTileRenderer.cpp
// Returns -1=outside, 0=inside, >0=partial (bits of planes crossed)
static int vcSceneLayerRenderer_FrustumTest(const udDouble4 frustumPlanes[6], const udDouble3 &boundCenter, const udDouble3 &boundExtents)
{
  int partial = 0;

  for (int i = 0; i < 6; ++i)
  {
    double distToCenter = udDot4(udDouble4::create(boundCenter, 1.0), frustumPlanes[i]);
    //optimized for case where boxExtents are all same: udFloat radiusBoxAtPlane = udDot3(boxExtents, udAbs(udVec3(curPlane)));
    double radiusBoxAtPlane = udDot3(boundExtents, udAbs(frustumPlanes[i].toVector3()));
    if (distToCenter < -radiusBoxAtPlane)
      return -1; // Box is entirely behind at least one plane
    else if (distToCenter <= radiusBoxAtPlane) // If spanned (not entirely infront)
      partial |= (1 << i);
  }

  return partial;
}

bool vcSceneLayerRenderer_IsNodeVisible(vcSceneLayerNode *pNode, const udDouble4 frustumPlanes[6])
{
  return -1 < vcSceneLayerRenderer_FrustumTest(frustumPlanes, pNode->minimumBoundingSphere.position, udDouble3::create(pNode->minimumBoundingSphere.radius));
}

double vcSceneLayerRenderer_CalculateNodeScreenSize(vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  udDouble4 center = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position, 1.0);
  udDouble4 p1 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(pNode->minimumBoundingSphere.radius, 0.0, 0.0), 1.0);
  udDouble4 p2 = viewProjectionMatrix * udDouble4::create(pNode->minimumBoundingSphere.position + udDouble3::create(0.0, pNode->minimumBoundingSphere.radius, 0.0), 1.0);
  center /= center.w;
  p1 /= p1.w;
  p2 /= p2.w;

  // TODO: (EVC-548) Verify this `2.0` is correct (because I only calculate the half size above)
  // I'm 75% sure it is, but changing it to 1.0 does cause a very slight noticeable LOD pop, but is that meant to happen?
  double r1 = udMag(center.toVector3() - p1.toVector3()) * 2.0;
  double r2 = udMag(center.toVector3() - p2.toVector3()) * 2.0;
  udDouble2 screenSize = udDouble2::create(screenResolution.x * r1, screenResolution.y * r2);
  return udMax(screenSize.x, screenSize.y);
}

void vcSceneLayerRenderer_RenderNode(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix)
{
  //printf("Drawing node %s\n", pNode->id);
  udUnused(pSceneLayerRenderer);

  for (size_t geometry = 0; geometry < pNode->geometryDataCount; ++geometry)
  {
    if (!pNode->pGeometryData[geometry].loaded)
      continue;

    vcTexture *pDrawTexture = nullptr;
    if (pNode->textureDataCount > 0 && pNode->pTextureData[0].loaded)
      pDrawTexture = pNode->pTextureData[0].pTexture; // TODO: pretty sure i need to read the material for this...

    vcPolygonModel_Render(pNode->pGeometryData[geometry].pModel, pNode->pGeometryData[geometry].originMatrix, viewProjectionMatrix, pDrawTexture);

    // TODO: (EVC-542) other geometry types
  }
}

bool vcSceneLayerRenderer_RecursiveRender(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayer *pSceneLayer, vcSceneLayerNode *pNode, const udDouble4x4 &viewProjectionMatrix, const udDouble4 frustumPlanes[6], const udUInt2 &screenResolution)
{
  if (!vcSceneLayerRenderer_IsNodeVisible(pNode, frustumPlanes))
    return true; // consume, but do nothing

  if (!vcSceneLayer_TouchNode(pSceneLayer, pNode))
    return false;

  double nodeScreenSize = vcSceneLayerRenderer_CalculateNodeScreenSize(pNode, viewProjectionMatrix, screenResolution);
  if (pNode->childrenCount == 0 || nodeScreenSize < pNode->loadSelectionValue)
  {
    vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, viewProjectionMatrix);
    return true;
  }

  bool childrenAllRendered = true;
  for (size_t i = 0; i < pNode->childrenCount; ++i)
  {
    vcSceneLayerNode *pChildNode = &pNode->pChildren[i];
    childrenAllRendered = vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, pSceneLayer, pChildNode, viewProjectionMatrix, frustumPlanes, screenResolution) && childrenAllRendered;
  }

  // TODO: (EVC-548)
  // If children are not loaded in yet, render this LOD.
  // At the moment this can cause incorrect occlusion
  if (!childrenAllRendered)
    vcSceneLayerRenderer_RenderNode(pSceneLayerRenderer, pNode, viewProjectionMatrix);

  return true;
}

bool vcSceneLayerRenderer_Render(vcSceneLayerRenderer *pSceneLayerRenderer, vcSceneLayer *pSceneLayer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenResolution)
{
  if (pSceneLayerRenderer == nullptr)
    return false;

  // TODO: (EVC-548) This is duplicated work across i3s models
  // extract frustum planes and re-use
  udDouble4x4 transposedViewProjection = udTranspose(viewProjectionMatrix);
  udDouble4 frustumPlanes[6];
  frustumPlanes[0] = transposedViewProjection.c[3] + transposedViewProjection.c[0]; // Left
  frustumPlanes[1] = transposedViewProjection.c[3] - transposedViewProjection.c[0]; // Right
  frustumPlanes[2] = transposedViewProjection.c[3] + transposedViewProjection.c[1]; // Bottom
  frustumPlanes[3] = transposedViewProjection.c[3] - transposedViewProjection.c[1]; // Top
  frustumPlanes[4] = transposedViewProjection.c[3] + transposedViewProjection.c[2]; // Near
  frustumPlanes[5] = transposedViewProjection.c[3] - transposedViewProjection.c[2]; // Far
  // Normalize the planes
  for (int j = 0; j < 6; ++j)
    frustumPlanes[j] /= udMag3(frustumPlanes[j]);

  return vcSceneLayerRenderer_RecursiveRender(pSceneLayerRenderer, pSceneLayer, &pSceneLayer->root, viewProjectionMatrix, frustumPlanes, screenResolution);
}
