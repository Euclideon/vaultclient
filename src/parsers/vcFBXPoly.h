#ifndef vcFBXPoly_h__
#define vcFBXPoly_h__

#ifdef FBXSDK_ON

#include "vcPolygonModel.h"

// Read the FBX, optionally only reading a specific count of vertices (to test for valid format for example)
udResult vcFBX_LoadPolygonModel(vcPolygonModel **ppModel, const char *pFilename, udWorkerPool *pWorkerPool);

#endif

#endif //vcFBXPoly_h__
