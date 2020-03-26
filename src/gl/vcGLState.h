#ifndef vcGLState_h__
#define vcGLState_h__

#include "udMath.h"

// Define potentially missing defines
#ifndef GRAPHICS_API_OPENGL
#  define GRAPHICS_API_OPENGL 0
#endif

#ifndef GRAPHICS_API_D3D11
#  define GRAPHICS_API_D3D11 0
#endif

#ifndef GRAPHICS_API_METAL
#  define GRAPHICS_API_METAL 0
#endif

enum
{
  // TODO: (EVC-547) This is arbitrary. Should be platform based.
  vcGLState_MaxUploadBytesPerFrame = 50 * 1024, // 50kb
};

// State enums
enum vcGLStateFillMode
{
  vcGLSFM_Solid,
  vcGLSFM_Wireframe
};

enum vcGLStateCullMode
{
  vcGLSCM_Back,
  vcGLSCM_Front,

  vcGLSCM_None
};

enum vcGLStateBlendMode
{
  vcGLSBM_None,

  vcGLSBM_Interpolative, // (source alpha * source fragment) + ((1 – source alpha) * destination fragment)
  vcGLSBM_Additive, // (1 * source fragment) + (1 * destination fragment)
  vcGLSBM_Multiplicative, // source fragment * destination fragment

  vcGLSBM_Count
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
  vcGLSDM_Always,

  vcGLSDM_Total
};

enum vcGLStateStencilFunc
{
  vcGLSSF_Always,
  vcGLSSF_Never,

  vcGLSSF_Less,
  vcGLSSF_LessOrEqual,
  vcGLSSF_Greater,
  vcGLSSF_GreaterOrEqual,

  vcGLSSF_Equal,
  vcGLSSF_NotEqual,

  vcGLSSF_Total
};

enum vcGLStateStencilOp
{
  vcGLSSOP_Keep,
  vcGLSSOP_Zero,
  vcGLSSOP_Replace,

  vcGLSSOP_Increment,
  vcGLSSOP_Decrement,

  vcGLSSOP_Total
};

struct vcGLStencilSettings
{
  bool enabled;
  uint8_t writeMask;

  vcGLStateStencilFunc compareFunc;
  uint8_t compareValue;
  uint8_t compareMask;

  vcGLStateStencilOp onStencilFail;
  vcGLStateStencilOp onDepthFail;
  vcGLStateStencilOp onStencilAndDepthPass;
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
  vcGLStencilSettings stencil;

  udInt4 viewportZone;
  udInt4 scissorZone;

  struct
  {
    size_t drawCount;
    size_t triCount;
    size_t uploadBytesCount;
  } frameInfo;
};

struct SDL_Window;
struct vcFramebuffer;

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer);
void vcGLState_Deinit();

bool vcGLState_ApplyState(vcGLState *pState);
bool vcGLState_ResetState(bool force = false);

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW = true, bool force = false);
bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force = false);
bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil = nullptr, bool force = false);

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth = 0.f, float maxDepth = 1.f);
bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth);

bool vcGLState_Present(SDL_Window *pWindow);

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height);

void vcGLState_Scissor(int left, int top, int right, int bottom, bool force = false);

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel);

void vcGLState_ReportGPUWork(size_t drawCount, size_t triCount, size_t uploadBytesCount);
bool vcGLState_IsGPUDataUploadAllowed();

template <typename T>
udMatrix4x4<T> vcGLState_ProjectionMatrix(T fovY, T aspectRatio, T znear, T zfar)
{
#if GRAPHICS_API_OPENGL
  static const udMatrix4x4<T> FlipVertically = udMatrix4x4<T>::scaleNonUniform(T(1), T(-1), T(1));
  return FlipVertically * udMatrix4x4<T>::perspectiveNO(fovY, aspectRatio, znear, zfar);
#else
  return udMatrix4x4<T>::perspectiveZO(fovY, aspectRatio, znear, zfar);
#endif
}

#endif // vcGLState_h__
