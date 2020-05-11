#ifndef vcMesh_h__
#define vcMesh_h__

#include "udMath.h"
#include "vcShader.h"

struct vcMesh;

enum vcMeshFlags
{
  vcMF_None = 0,
  vcMF_Dynamic = 1 << 0,
  vcMF_NoIndexBuffer = 1 << 1,
  vcMF_IndexShort = 1 << 2,
};
inline vcMeshFlags operator|(vcMeshFlags a, vcMeshFlags b) { return (vcMeshFlags)(int(a) | int(b)); }

enum vcMeshRenderMode
{
  vcMRM_Triangles,
  vcMRM_TriangleStrip,
  vcMRM_Points,
};

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void* pVerts, uint32_t currentVerts, const void *pIndices, uint32_t currentIndices, vcMeshFlags flags = vcMF_None);
//bool vcMesh_CreateSimple(vcMesh **ppMesh, const vcSimpleVertex *pVerts, int totalVerts, const int *pIndices, int totalIndices);
void vcMesh_Destroy(vcMesh **ppMesh);

udResult vcMesh_UploadData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices);
udResult vcMesh_UploadSubData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, int startVertex, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices);

bool vcMesh_Render(vcMesh *pMesh, uint32_t elementCount = 0, uint32_t startElement = 0, vcMeshRenderMode renderMode = vcMRM_Triangles);

#endif // vcMesh_h__
