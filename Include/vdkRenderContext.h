#ifndef vdkRenderContext_h__
#define vdkRenderContext_h__

#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdkContext;
struct vdkRenderContext;
struct vdkRenderView;
struct vdkPointCloud;

enum vdkRenderContextPointMode
{
  vdkRCPM_Rectangles, // This is the default, renders the voxels expanded as screen space rectangles
  vdkRCPM_Cubes, // Renders the voxels as cubes
  vdkRCPM_Points, // Renders voxels as a single point (Note: does not accurately reflect the 'size' of voxels)

  vdkRCPM_Count
};

struct vdkRenderPicking
{
  // Input variables
  unsigned int x; // View Mouse X
  unsigned int y; // View Mouse Y

  // Output variables
  int hit; // Non-zero if hit something
  unsigned int modelIndex; // Index of the model in the ppModels list
  int isHighestLOD; // Non-zero if this position is as precise as possible
  double pointCenter[3]; // The center of the highlighted point in world space
};

struct vdkRenderInstance
{
  struct vdkPointCloud *pPointCloud;
  double matrix[16];
};

VDKDLL_API enum vdkError vdkRenderContext_Create(struct vdkContext *pContext, struct vdkRenderContext **ppRenderer);
VDKDLL_API enum vdkError vdkRenderContext_Destroy(struct vdkContext *pContext, struct vdkRenderContext **ppRenderer);

VDKDLL_API enum vdkError vdkRenderContext_ShowColor(struct vdkContext *pContext, struct vdkRenderContext *pRenderer);
VDKDLL_API enum vdkError vdkRenderContext_ShowIntensity(struct vdkContext *pContext, struct vdkRenderContext *pRenderer, int minIntensity, int maxIntensity);
VDKDLL_API enum vdkError vdkRenderContext_ShowClassification(struct vdkContext *pContext, struct vdkRenderContext *pRenderer, int *pColors, int totalColors);

VDKDLL_API enum vdkError vdkRenderContext_SetPointMode(struct vdkContext *pContext, struct vdkRenderContext *pRenderer, enum vdkRenderContextPointMode newPointMode);

VDKDLL_API enum vdkError vdkRenderContext_Render(struct vdkContext *pContext, struct vdkRenderContext *pRenderer, struct vdkRenderView *pRenderView, struct vdkRenderInstance *pModels, int modelCount);
VDKDLL_API enum vdkError vdkRenderContext_RenderAdv(struct vdkContext *pContext, struct vdkRenderContext *pRenderer, struct vdkRenderView *pRenderView, struct vdkRenderInstance *pModels, int modelCount, struct vdkRenderPicking *pPickInformation);

#ifdef __cplusplus
}
#endif

#endif // vdkRenderContext_h__
