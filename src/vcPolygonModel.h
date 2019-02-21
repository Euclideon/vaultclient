#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "udPlatform\udMath.h"

struct vcPolygonModel;

udResult vcPolygonModel_CreateShaders();
udResult vcPolygonModel_DestroyShaders();

udResult vcPolygonModel_CreateFromMemory(vcPolygonModel **ppModel, char *pData, int dataLength);
//udResult vcVSMF_CreateFromStream(vcVSMF **ppModel, const char *pURL); TODO

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

udResult vcPolygonModel_Render(vcPolygonModel *pModel, const udDouble4x4 &modelMatrix, const udDouble4x4 &viewProjectionMatrix);

#endif // vcPolygonModel_h__
