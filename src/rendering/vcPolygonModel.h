#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "gl/vcLayout.h"

#include "udMath.h"

enum vcPolyModelPass
{
  vcPMP_Standard,
  vcPMP_ColourOnly,
  //vcPMP_Shadows,
};

struct vcPolygonModel;
struct vcTexture;

udResult vcPolygonModel_CreateShaders();
udResult vcPolygonModel_DestroyShaders();

udResult vcPolygonModel_CreateFromRawVertexData(vcPolygonModel **ppPolygonModel, void *pVerts, uint16_t vertCount, const vcVertexLayoutTypes *pMeshLayout, int totalTypes);
udResult vcPolygonModel_CreateFromURL(vcPolygonModel **ppModel, const char *pURL);

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix, const vcPolyModelPass &passType = vcPMP_Standard, vcTexture *pDiffuseOverride = nullptr, const udFloat4 *pColourOverride = nullptr);

// TODO: (EVC-570) Parsing formats should be in their own module, not here
udResult vcPolygonModel_CreateFromVSMFInMemory(vcPolygonModel **ppModel, char *pData, int dataLength);

#endif // vcPolygonModel_h__
