#ifndef vcRenderShaders_h__
#define vcRenderShaders_h__

#include "vcGLState.h"

extern const char *const g_PostEffectsVertexShader;
extern const char *const g_PostEffectsFragmentShader;

// Image Renderer shaders
extern const char *const g_ImageRendererFragmentShader;
extern const char *const g_ImageRendererMeshVertexShader;
extern const char *const g_ImageRendererBillboardVertexShader;

// Utility Shaders
extern const char *const g_BlurVertexShader;
extern const char *const g_BlurFragmentShader;
extern const char *const g_HighlightVertexShader;
extern const char *const g_HighlightFragmentShader;

// GPU UD Renderer
extern const char *const g_udGPURenderQuadVertexShader;
extern const char *const g_udGPURenderQuadFragmentShader;

#endif//vcRenderShaders_h__
