#ifndef vdkRenderView_h__
#define vdkRenderView_h__

#include <stdint.h>
#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdkContext;
struct vdkRenderContext;
struct vdkRenderView;

enum vdkRenderViewMatrix
{
  vdkRVM_Camera,     // The local to world-space transform of the camera (View is implicitly set as the inverse)
  vdkRVM_View,       // The view-space transform for the model (does not need to be set explicitly)
  vdkRVM_Projection, // The projection matrix (default is 60 degree LH)
  vdkRVM_Viewport,   // Viewport scaling matrix (default width and height of viewport)

  vdkRVM_Count
};

VDKDLL_API enum vdkError vdkRenderView_Create(struct vdkContext *pContext, struct vdkRenderView **ppRenderView, struct vdkRenderContext *pRenderer, uint32_t width, uint32_t height);
VDKDLL_API enum vdkError vdkRenderView_Destroy(struct vdkContext *pContext, struct vdkRenderView **ppRenderView);

// Set a target buffer to a view
// Concurrency: Guarded by mutex at vdkRenderContext level
// pColorBuffer: The color buffer, if null the buffer will not be rendered to anymore
// pDepthBuffer: The depth buffer, if null the buffer will not be rendered to anymore
VDKDLL_API enum vdkError vdkRenderView_SetTargets(struct vdkContext *pContext, struct vdkRenderView *pRenderView, void *pColorBuffer, uint32_t colorClearValue, void *pDepthBuffer);

VDKDLL_API enum vdkError vdkRenderView_GetMatrix(struct vdkContext *pContext, const struct vdkRenderView *pRenderView, enum vdkRenderViewMatrix matrixType, double cameraMatrix[16]);
VDKDLL_API enum vdkError vdkRenderView_SetMatrix(struct vdkContext *pContext, struct vdkRenderView *pRenderView, enum vdkRenderViewMatrix matrixType, const double cameraMatrix[16]);

#ifdef __cplusplus
}
#endif

#endif // vdkRenderView_h__
