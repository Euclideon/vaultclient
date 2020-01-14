#ifndef vcRenderShaders_h__
#define vcRenderShaders_h__

#include "vcGLState.h"

extern const char* const g_udFragmentShader;
extern const char* const g_udSplatIdFragmentShader;
extern const char* const g_udVertexShader;
extern const char* const g_tileFragmentShader;
extern const char* const g_tileVertexShader;
extern const char* const g_vcSkyboxFragmentShaderPanarama;
extern const char* const g_vcSkyboxFragmentShaderImageColour;
extern const char* const g_vcSkyboxVertexShader;
extern const char* const g_FenceVertexShader;
extern const char* const g_FenceFragmentShader;
extern const char* const g_WaterVertexShader;
extern const char* const g_WaterFragmentShader;

extern const char* const g_CompassFragmentShader;
extern const char* const g_CompassVertexShader;

extern const char* const g_ImGuiVertexShader;
extern const char* const g_ImGuiFragmentShader;

// Polygon shaders
extern const char* const g_PolygonP1N1UV1FragmentShader;
extern const char* const g_PolygonP1N1UV1VertexShader;
extern const char* const g_PolygonP1UV1FragmentShader;
extern const char* const g_PolygonP1UV1VertexShader;
extern const char* const g_FlatColour_FragmentShader;

extern const char* const g_BillboardFragmentShader;
extern const char* const g_BillboardVertexShader;

// Utility Shaders
extern const char* const g_BlurVertexShader;
extern const char* const g_BlurFragmentShader;
extern const char* const g_HighlightVertexShader;
extern const char* const g_HighlightFragmentShader;

// GPU UD Renderer
extern const char* const g_udGPURenderQuadVertexShader;
extern const char* const g_udGPURenderQuadFragmentShader;
extern const char* const g_udGPURenderGeomVertexShader;
extern const char* const g_udGPURenderGeomFragmentShader;
extern const char* const g_udGPURenderGeomGeometryShader;

#endif//vcRenderShaders_h__
