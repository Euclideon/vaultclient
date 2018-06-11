#include "vcOpenGL.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

static vcGLState s_internalState;
vcFramebuffer g_defaultFramebuffer;

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
#if UDPLATFORM_WINDOWS
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    return false;
#endif

  glGetError(); // throw out first error

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
  success &= vcGLState_SetDepthMode(vcGLSDM_LessOrEqual, true);

  return success;
}

bool vcGLState_ResetState(bool force /*= false*/)
{
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back, true, force);
  vcGLState_SetBlendMode(vcGLSBM_None, force);
  vcGLState_SetDepthMode(vcGLSDM_LessOrEqual, true, force);

  return true;
}

bool vcGLState_SetFaceMode(vcGLStateFillMode fillMode, vcGLStateCullMode cullMode, bool isFrontCCW /*= true*/, bool force /*= false*/)
{
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
    if (fillMode == vcGLSFM_Solid)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      else if (blendMode == vcGLSBM_Additive)
        glBlendFunc(GL_ONE, GL_ONE);
      else if (blendMode == vcGLSBM_Multiplicative)
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    }

    s_internalState.blendMode = blendMode;
  }

  return true;
}

bool vcGLState_SetDepthMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, bool force /*= false*/)
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

  return true;
}

bool vcGLState_SetDepthRange(float minDepth, float maxDepth, bool force /*= false*/)
{
  if ((s_internalState.depthRange.x != minDepth || s_internalState.depthRange.y != maxDepth) || force)
  {
    glDepthRangef(minDepth, maxDepth);

    s_internalState.depthRange.x = minDepth;
    s_internalState.depthRange.y = maxDepth;
  }

  return true;
}

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
  if (x < 0 || y < 0 || width < 1 || height < 1)
    return false;

  glViewport(x, y, width, height);

  return true;
}


bool vcGLState_Present(SDL_Window *pWindow)
{
  if (pWindow == nullptr)
    return false;

  SDL_GL_SwapWindow(pWindow);
  return true;
}
