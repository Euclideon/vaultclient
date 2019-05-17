#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "gl/vcLayout.h"

#include "udPlatform/udMath.h"

struct vcPolygonModel;
struct vcTexture;

udResult vcPolygonModel_CreateShaders();
udResult vcPolygonModel_DestroyShaders();

udResult vcPolygonModel_CreateFromRawVertexData(vcPolygonModel **ppPolygonModel, void *pVerts, uint16_t vertCount, const vcVertexLayoutTypes *pMeshLayout, int totalTypes);
udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL);

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, vcTexture *pDiffuseOverride = nullptr);

// TODO: (EVC-570) Parsing formats should be in their own module, not here
udResult vcPolygonModel_CreateFromVSMFInMemory(vcPolygonModel **ppModel, char *pData, int dataLength);

#endif // vcPolygonModel_h__
