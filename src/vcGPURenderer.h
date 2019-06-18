#ifndef vcGPURenderer_h__
#define vcGPURenderer_h__

#define DRAWCALL_DEBUGGING 0

#include "vdkRenderView.h"
#include "vdkGPURender.h"

#include "gl/vcGLState.h"
#include "gl/vcShader.h"
#include "gl/vcMesh.h"

enum vcGPURendererPointRenderMode
{
  vcBRPRM_Quads,
  vcBRPRM_GeometryShader,

  vcBRPRM_Count
};

struct vcGPURenderer;

udResult vcGPURenderer_Create(vcGPURenderer **ppBlockRenderer, vcGPURendererPointRenderMode renderMode, int targetPointCount, float threshold = 0.6f);
udResult vcGPURenderer_Destroy(vcGPURenderer **ppBlockRenderer);

#endif // vcBlockRender_h__
