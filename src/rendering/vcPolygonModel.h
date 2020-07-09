#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "vcLayout.h"

#include "udMath.h"

enum vcPolyModelPass
{
  vcPMP_Standard,
  vcPMP_ColourOnly,
  vcPMP_Shadows, // depth only
};

struct vcTexture;
struct vcMesh;
struct udWorkerPool;

struct vcPolygonModelMaterial
{
  uint16_t flags;
  uint32_t colour; // bgra

  const char *pName;
  vcTexture *pTexture;
};

struct vcPolygonModelMesh
{
  uint16_t flags;
  uint16_t materialID;
  uint16_t LOD;
  uint32_t numVertices;
  uint32_t numElements;

  vcPolygonModelMaterial material; // TODO: materialID should reference a container of there. These should be shared between meshes, and rendering should be organized by material.
  vcMesh *pMesh;
};

struct vcPolygonModel
{
  int meshCount;
  vcPolygonModelMesh *pMeshes;

  udDouble3 origin;
  udDouble4x4 modelOffset;

  udInterlockedBool keepLoading;
};

udResult vcPolygonModel_CreateShaders(udWorkerPool *pWorkerPool);
udResult vcPolygonModel_DestroyShaders();

udResult vcPolygonModel_CreateFromRawVertexData(vcPolygonModel **ppPolygonModel, const void *pVerts, const uint32_t vertCount, const vcVertexLayoutTypes *pMeshLayout, const int totalTypes, const uint32_t *pIndices = nullptr, const uint32_t indexCount = 0);
udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL, udWorkerPool *pWorkerPool);

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const float encodedObjectId, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, const vcPolyModelPass &passType = vcPMP_Standard, const udFloat4 &tint = udFloat4::one(), vcTexture *pDiffuseOverride = nullptr, const udFloat4 *pColourOverride = nullptr);

// TODO: (EVC-570) Parsing formats should be in their own module, not here
udResult vcPolygonModel_CreateFromVSMFInMemory(vcPolygonModel **ppModel, char *pData, int dataLength, udWorkerPool *pWorkerPool);

#endif // vcPolygonModel_h__
