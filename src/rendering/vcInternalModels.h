#ifndef vcInternalModels_h__
#define vcInternalModels_h__

#include "udResult.h"

struct vcMesh;
struct vcPolygonModel;

enum vcInternalMeshType
{
  vcInternalMeshType_ScreenQuad,

  vcInternalMeshType_WorldQuad,
  vcInternalMeshType_Billboard,
  vcInternalMeshType_Sphere,
  vcInternalMeshType_Tube,

  vcInternalMeshType_Compass,
  vcInternalMeshType_Orbit,

  vcInternalMeshType_Count
};
extern vcMesh *gInternalMeshes[vcInternalMeshType_Count];

enum vcInternalModelType
{
  vcInternalModelType_Cube,
  vcInternalModelType_Sphere,
  vcInternalModelType_Cylinder,
  vcInternalModelType_Tube,
  vcInternalModelType_Quad,

  vcInternalModelType_Count
};
extern vcPolygonModel *gInternalModels[vcInternalModelType_Count];

udResult vcInternalModels_Init();
udResult vcInternalModels_Deinit();

#endif //vcInternalModels_h__
