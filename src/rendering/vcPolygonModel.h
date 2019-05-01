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

// WIP

struct vcPolygonModelVertex
{
  udFloat3 pos;
  udFloat3 normal;
  udFloat2 uv;
 // unsigned char colour[4]; // JUST BROKE ALL OTHER POLY MODELS
};
const vcVertexLayoutTypes vcPolygonModelVertexLayout[] = { vcVLT_Position3, vcVLT_Normal3, vcVLT_TextureCoords2 };//, vcVLT_ColourBGRA };

udResult vcPolygonModel_CreateFromData(vcPolygonModel **ppPolygonModel, vcPolygonModelVertex *pVerts, uint16_t vertCount);

#endif // vcPolygonModel_h__
