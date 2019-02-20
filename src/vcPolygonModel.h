#ifndef vcPolygonModel_h__
#define vcPolygonModel_h__

#include "udPlatform\udResult.h"

struct vcPolygonModel;

udResult vcPolygonModel_CreateFromMemory(vcPolygonModel **ppModel, char *pData, int dataLength);
//udResult vcVSMF_CreateFromStream(vcVSMF **ppModel, const char *pURL); TODO

udResult vcPolygonModel_Destroy(vcPolygonModel **ppModel);

#endif // vcPolygonModel_h__
