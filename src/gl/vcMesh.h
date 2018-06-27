#ifndef vcMesh_h__
#define vcMesh_h__

#include "udPlatform/udMath.h"
#include "gl/vcShader.h"

struct vcMesh;

struct vcSimpleVertex
{
  udFloat3 Position;
  udFloat2 UVs;
};

const vcShaderVertexInputTypes vcSimpleVertexLayout[] = { vcSVIT_Position3, vcSVIT_TextureCoords2 };

bool vcMesh_CreateSimple(vcMesh **ppMesh, const vcSimpleVertex *pVerts, int totalVerts, const int *pIndices, int totalIndices);
void vcMesh_Destroy(vcMesh **ppMesh);

bool vcMesh_RenderTriangles(vcMesh *pMesh, uint32_t triangleCount, uint32_t startTriangle = 0);

#endif // vcMesh_h__
