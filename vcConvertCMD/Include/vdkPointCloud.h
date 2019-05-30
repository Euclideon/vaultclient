#ifndef vdkPointCloud_h__
#define vdkPointCloud_h__

#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdkContext;
struct vdkPointCloud;

VDKDLL_API enum vdkError vdkPointCloud_Load(struct vdkContext *pContext, struct vdkPointCloud **ppModel, const char *modelLocation);
VDKDLL_API enum vdkError vdkPointCloud_Unload(struct vdkContext *pContext, struct vdkPointCloud **ppModel);

VDKDLL_API enum vdkError vdkPointCloud_GetMetadata(struct vdkContext *pContext, struct vdkPointCloud *pPointCloud, const char **ppJSONMetadata);
VDKDLL_API enum vdkError vdkPointCloud_GetStoredMatrix(struct vdkContext *pContext, struct vdkPointCloud *pPointCloud, double matrix[16]);
VDKDLL_API enum vdkError vdkPointCloud_GetPivotPoint(struct vdkContext *pContext, struct vdkPointCloud *pPointCloud, double centerPoint[3]);

#ifdef __cplusplus
}
#endif

#endif // vdkPointCloud_h__
