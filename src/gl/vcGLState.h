#ifndef vcGLState_h__
#define vcGLState_h__

// Define potentially missing defines
#ifndef GRAPHICS_API_OPENGL
#define GRAPHICS_API_OPENGL 0
#endif

#ifndef GRAPHICS_API_D3D11
#define GRAPHICS_API_D3D11 0
#endif

#include "udPlatform/udMath.h"

// State enums
enum vcGLStateFillMode
{
  vcGLSFM_Solid,
  vcGLSFM_Wireframe,

  vcGLSFM_TotalModes
};

enum vcGLStateCullMode
{
  vcGLSCM_None,
  vcGLSCM_Front,
  vcGLSCM_Back,

  vcGLSCM_TotalModes
};

enum vcGLStateBlendMode
{
  vcGLSBM_None,

  vcGLSBM_Interpolative, // (source alpha * source fragment) + ((1 – source alpha) * destination fragment)
  vcGLSBM_Additive, // (1 * source fragment) + (1 * destination fragment)
  vcGLSBM_Multiplicative, // source fragment * destination fragment
  vcGLSBM_AdditiveSrcInterpolativeDst, // (source fragment) + ((1 – source alpha) * destination fragment)
};

enum vcGLStateDepthMode
{
  vcGLSDM_None,

  vcGLSDM_Less,
  vcGLSDM_LessOrEqual,
  vcGLSDM_Equal,
  vcGLSDM_GreaterOrEqual,
  vcGLSDM_Greater,

  vcGLSDM_NotEqual,
  vcGLSDM_Always
};

struct vcGLState
{
  vcGLStateFillMode fillMode;
  vcGLStateCullMode cullMode;
  bool isFrontCCW;

  bool doDepthWrite;
  vcGLStateDepthMode depthReadMode;
  udFloat2 depthRange;

  vcGLStateBlendMode blendMode;

  udInt4 viewportZone;
  udInt4 scissorZone;
};

struct SDL_Window;
struct vcFramebuffer;

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer);
void vcGLState_Deinit();

bool vcGLState_ApplyState(vcGLState *pState);
bool vcGLState_ResetState(bool force = false);

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW = true, bool force = false);
bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force = false);
bool vcGLState_SetDepthMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, bool force = false);

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth = 0.f, float maxDepth = 1.f);
bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth);

bool vcGLState_Present(SDL_Window *pWindow);

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height);

void vcGLState_Scissor(int left, int top, int right, int bottom, bool force = false);

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel);

#endif // vcGLState_h__
