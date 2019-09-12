#include "vcInternalModels.h"
#include "vcInternalModelsData.h"

#include "gl/vcMesh.h"

static int gRefCount = 0;
vcMesh *gInternalModels[vcIMT_Count] = {};

udResult vcInternalModels_Init()
{
  udResult result;

  ++gRefCount;
  UD_ERROR_IF(gRefCount != 1, udR_Success);

  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_ScreenQuad], vcP3UV2VertexLayout, int(udLengthOf(vcP3UV2VertexLayout)), screenQuadVertices, 4, screenQuadIndices, 6, vcMF_IndexShort));
  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_FlippedScreenQuad], vcP3UV2VertexLayout, int(udLengthOf(vcP3UV2VertexLayout)), flippedScreenQuadVertices, 4, flippedScreenQuadIndices, 6, vcMF_IndexShort));

  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_Billboard], vcP3UV2VertexLayout, (int)udLengthOf(vcP3UV2VertexLayout), billboardVertices, (int)udLengthOf(billboardVertices), billboardIndices, (int)udLengthOf(billboardIndices), vcMF_IndexShort));
  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_Sphere], vcP3UV2VertexLayout, (int)udLengthOf(vcP3UV2VertexLayout), pSphereVertices, (int)udLengthOf(sphereVerticesFltArray), sphereIndices, (int)udLengthOf(sphereIndices), vcMF_IndexShort));
  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_Tube], vcP3UV2VertexLayout, (int)udLengthOf(vcP3UV2VertexLayout), pTubeVertices, (int)udLengthOf(tubeVerticesFltArray), tubeIndices, (int)udLengthOf(tubeIndices), vcMF_IndexShort));

  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_Orbit], vcP3N3VertexLayout, (int)udLengthOf(vcP3N3VertexLayout), pOrbitVertices, (int)udLengthOf(orbitVerticesFltArray), orbitIndices, (int)udLengthOf(orbitIndices), vcMF_IndexShort));
  UD_ERROR_CHECK(vcMesh_Create(&gInternalModels[vcIMT_Compass], vcP3N3VertexLayout, int(udLengthOf(vcP3N3VertexLayout)), pCompassVerts, (int)udLengthOf(compassVertsFltArray), compassIndices, (int)udLengthOf(compassIndices), vcMF_IndexShort));

  result = udR_Success;
epilogue:
  if (result != udR_Success)
    vcInternalModels_Deinit();

  return result;
}

udResult vcInternalModels_Deinit()
{
  udResult result;
  --gRefCount;
  UD_ERROR_IF(gRefCount != 0, udR_Success);

  for (int i = 0; i < vcIMT_Count; ++i)
    vcMesh_Destroy(&gInternalModels[i]);

  result = udR_Success;
epilogue:
  return result;
}
