#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "udPlatform/udMath.h"
#include "gl/vcMesh.h"

struct vcPolygonModel;
struct vcTexture;

udResult vcPolygonModel_CreateShaders();
udResult vcPolygonModel_DestroyShaders();

udResult vcPolygonModel_CreateFromMemory(vcPolygonModel **ppModel, char *pData, int dataLength);
udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL);

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, vcTexture *pDiffuseOverride = nullptr);

udResult vcPolygonModel_CreateFromData(vcPolygonModel **ppPolygonModel, void *pVerts, uint16_t vertCount, const vcVertexLayoutTypes *pMeshLayout, int totalTypes);

#endif // vcPolygonModel_h__
