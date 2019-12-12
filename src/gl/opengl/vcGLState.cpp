#include "vcOpenGL.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

static vcGLState s_internalState;
vcFramebuffer g_defaultFramebuffer;
int32_t g_maxAnisotropy = 0;

static const GLenum vcGLSSOPToGL[] = { GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR };
UDCOMPILEASSERT(udLengthOf(vcGLSSOPToGL) == vcGLSSOP_Total, "Not Enough OpenGL Stencil Operations");
static const GLenum vcGLSSFToGL[]{ GL_ALWAYS, GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL };
UDCOMPILEASSERT(udLengthOf(vcGLSSFToGL) == vcGLSSF_Total, "Not Enough OpenGL Stencil Functions");

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
  SDL_GLContext ctx = SDL_GL_CreateContext(pWindow);
  if (ctx == nullptr)
    return false;

#if UDPLATFORM_EMSCRIPTEN
  SDL_GL_MakeCurrent(pWindow, ctx);
#endif

#if UDPLATFORM_WINDOWS
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    return false;
#endif

  glGetError(); // throw out first error

#if !UDPLATFORM_ANDROID
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &g_maxAnisotropy);
#endif

  memset(&g_defaultFramebuffer, 0, sizeof(vcFramebuffer));

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // Rendering to framebuffer 0 isn't valid when using UIKit in SDL2
  // so we need to use the framebuffer that SDL2 provides us.
  {
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(pWindow, &wminfo);
    g_defaultFramebuffer.id = wminfo.info.uikit.framebuffer;
  }
#else
  udUnused(pWindow);
#endif

  *ppDefaultFramebuffer = &g_defaultFramebuffer;

  glEnable(GL_SCISSOR_TEST);

  // match opengl stencil defaults
  s_internalState.stencil.compareFunc = vcGLSSF_Always;
  s_internalState.stencil.compareValue = 0;
  s_internalState.stencil.compareMask = 0xff;
  s_internalState.stencil.onDepthFail = vcGLSSOP_Keep;
  s_internalState.stencil.onStencilFail = vcGLSSOP_Keep;
  s_internalState.stencil.onStencilAndDepthPass = vcGLSSOP_Keep;
  s_internalState.stencil.writeMask = 0xff;

  SDL_GL_SetSwapInterval(0); // disable v-sync

  return vcGLState_ResetState(true);
}

void vcGLState_Deinit()
{
  // Not (yet?) required for OpenGL
}

bool vcGLState_ApplyState(vcGLState *pState)
{
  bool success = true;

  success &= vcGLState_SetFaceMode(pState->fillMode, pState->cullMode, pState->isFrontCCW);
  success &= vcGLState_SetBlendMode(pState->blendMode);
  success &= vcGLState_SetDepthStencilMode(pState->depthReadMode, pState->doDepthWrite, &pState->stencil);

  return success;
}

bool vcGLState_ResetState(bool force /*= false*/)
{
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back, true, force);
  vcGLState_SetBlendMode(vcGLSBM_None, force);
  vcGLState_SetDepthStencilMode(vcGLSDM_LessOrEqual, true, nullptr, force);

  return true;
}

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW /*= true*/, bool force /*= false*/)
{
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  if (fillMode != vcGLSFM_Solid)
    return false;
#endif

  if (s_internalState.cullMode != cullMode || force)
  {
    if (cullMode == vcGLSCM_None)
      glDisable(GL_CULL_FACE);
    else
      glEnable(GL_CULL_FACE);

    if (cullMode == vcGLSCM_Front)
      glCullFace(GL_FRONT);
    else if (cullMode == vcGLSCM_Back)
      glCullFace(GL_BACK);

    s_internalState.cullMode = cullMode;
  }

  if (s_internalState.fillMode != fillMode || force)
  {
#if !UDPLATFORM_IOS && !UDPLATFORM_IOS_SIMULATOR && !UDPLATFORM_EMSCRIPTEN && !UDPLATFORM_ANDROID
    if (fillMode == vcGLSFM_Solid)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    s_internalState.fillMode = fillMode;
  }

  if (s_internalState.isFrontCCW != isFrontCCW || force)
  {
    if (isFrontCCW)
      glFrontFace(GL_CCW);
    else
      glFrontFace(GL_CW);

    s_internalState.isFrontCCW = isFrontCCW;
  }

  return true;
}

bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force /*= false*/)
{
  if (s_internalState.blendMode != blendMode || force)
  {
    if (blendMode == vcGLSBM_None)
    {
      glDisable(GL_BLEND);
    }
    else
    {
      glEnable(GL_BLEND);

      if (blendMode == vcGLSBM_Interpolative)
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
      else if (blendMode == vcGLSBM_Additive)
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
      else if (blendMode == vcGLSBM_Multiplicative)
        glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ONE, GL_ZERO);
    }

    s_internalState.blendMode = blendMode;
  }

  return true;
}

bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil /* = nullptr */, bool force /*= false*/)
{
  if (s_internalState.depthReadMode != depthReadMode || force)
  {
    if (depthReadMode == vcGLSDM_None)
    {
      glDisable(GL_DEPTH_TEST);
    }
    else
    {
      glEnable(GL_DEPTH_TEST);

      if (depthReadMode == vcGLSDM_Less)
        glDepthFunc(GL_LESS);
      else if (depthReadMode == vcGLSDM_LessOrEqual)
        glDepthFunc(GL_LEQUAL);
      else if (depthReadMode == vcGLSDM_Equal)
        glDepthFunc(GL_EQUAL);
      else if (depthReadMode == vcGLSDM_GreaterOrEqual)
        glDepthFunc(GL_GEQUAL);
      else if (depthReadMode == vcGLSDM_Greater)
        glDepthFunc(GL_GREATER);
      else if (depthReadMode == vcGLSDM_NotEqual)
        glDepthFunc(GL_NOTEQUAL);
      else if (depthReadMode == vcGLSDM_Always)
        glDepthFunc(GL_ALWAYS);
    }

    s_internalState.depthReadMode = depthReadMode;
  }

  if (s_internalState.doDepthWrite != doDepthWrite || force)
  {
    if (doDepthWrite)
      glDepthMask(GL_TRUE);
    else
      glDepthMask(GL_FALSE);

    s_internalState.doDepthWrite = doDepthWrite;
  }

  bool enableStencil = pStencil != nullptr;
  if ((s_internalState.stencil.enabled != enableStencil) || force)
  {
    if (enableStencil)
      glEnable(GL_STENCIL_TEST);
    else
      glDisable(GL_STENCIL_TEST);

    s_internalState.stencil.enabled = enableStencil;
  }

  if (enableStencil)
  {
    if ((s_internalState.stencil.writeMask != pStencil->writeMask) || force)
    {
      glStencilMask(pStencil->writeMask);
      s_internalState.stencil.writeMask = pStencil->writeMask;
    }

    if ((s_internalState.stencil.compareFunc != pStencil->compareFunc) || (s_internalState.stencil.compareValue != pStencil->compareValue) || (s_internalState.stencil.compareMask != pStencil->compareMask) || force)
    {
      glStencilFunc(vcGLSSFToGL[pStencil->compareFunc], pStencil->compareValue, pStencil->compareMask);

      s_internalState.stencil.compareFunc = pStencil->compareFunc;
      s_internalState.stencil.compareValue = pStencil->compareValue;
      s_internalState.stencil.compareMask = pStencil->compareMask;
    }

    if ((s_internalState.stencil.onStencilFail != pStencil->onStencilFail) || (s_internalState.stencil.onDepthFail != pStencil->onDepthFail) || (s_internalState.stencil.onStencilAndDepthPass != pStencil->onStencilAndDepthPass) || force)
    {
      glStencilOp(vcGLSSOPToGL[pStencil->onStencilFail], vcGLSSOPToGL[pStencil->onDepthFail], vcGLSSOPToGL[pStencil->onStencilAndDepthPass]);

      s_internalState.stencil.onStencilFail = pStencil->onStencilFail;
      s_internalState.stencil.onDepthFail = pStencil->onDepthFail;
      s_internalState.stencil.onStencilAndDepthPass = pStencil->onStencilAndDepthPass;
    }
  }
  return true;
}

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth /*= 0.f*/, float maxDepth /*= 1.f*/)
{
  if (x < 0 || y < 0 || width < 1 || height < 1)
    return false;

  glDepthRangef(minDepth, maxDepth);
  glViewport(x, y, width, height);

  s_internalState.viewportZone = udInt4::create(x, y, width, height);

  vcGLState_Scissor(s_internalState.viewportZone.x, s_internalState.viewportZone.y, s_internalState.viewportZone.x + s_internalState.viewportZone.z, s_internalState.viewportZone.y + s_internalState.viewportZone.w);

  return true;
}

bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth)
{
  return vcGLState_SetViewport(s_internalState.viewportZone.x, s_internalState.viewportZone.y, s_internalState.viewportZone.x + s_internalState.viewportZone.z, s_internalState.viewportZone.y + s_internalState.viewportZone.w, minDepth, maxDepth);
}

bool vcGLState_Present(SDL_Window *pWindow)
{
  if (pWindow == nullptr)
    return false;

  SDL_GL_SwapWindow(pWindow);

  memset(&s_internalState.frameInfo, 0, sizeof(s_internalState.frameInfo));
  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  udUnused(width);
  udUnused(height);
  return true;
}

void vcGLState_Scissor(int left, int top, int right, int bottom, bool force /*= false*/)
{
  udInt4 newScissor = udInt4::create(udMax(0, left), udMax(s_internalState.viewportZone.w - bottom, 0), right - left, bottom - top);

  if (s_internalState.scissorZone != newScissor || force)
  {
    glScissor(newScissor.x, newScissor.y, newScissor.z, newScissor.w);
    s_internalState.scissorZone = newScissor;
  }
}

int32_t vcGLState_GetMaxAnisotropy(int32_t desiredAniLevel)
{
  return udMin(desiredAniLevel, g_maxAnisotropy);
}

void vcGLState_ReportGPUWork(size_t drawCount, size_t triCount, size_t uploadBytesCount)
{
  s_internalState.frameInfo.drawCount += drawCount;
  s_internalState.frameInfo.triCount += triCount;
  s_internalState.frameInfo.uploadBytesCount += uploadBytesCount;
}

bool vcGLState_IsGPUDataUploadAllowed()
{
  return (s_internalState.frameInfo.uploadBytesCount < vcGLState_MaxUploadBytesPerFrame);
}
