#include "vcD3D11.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

static vcGLState s_internalState;

// Data
ID3D11Device *g_pd3dDevice = nullptr;
ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
IDXGISwapChain *g_pSwapChain = nullptr;

vcFramebuffer g_defaultFramebuffer;

// Other Data
ID3D11SamplerState* g_pFontSampler = nullptr;
ID3D11RasterizerState *g_pRasterizerState = nullptr;
ID3D11BlendState *g_pBlendState = nullptr;
ID3D11DepthStencilState *g_pDepthStencilState = nullptr;

int32_t g_maxAnisotropy = 0;

static const D3D11_STENCIL_OP vcGLSSOPToD3D[] = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_ZERO, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_INCR, D3D11_STENCIL_OP_DECR };
UDCOMPILEASSERT(udLengthOf(vcGLSSOPToD3D) == vcGLSSOP_Total, "Not Enough DirectX Stencil Operations");
static const D3D11_COMPARISON_FUNC vcGLSSFToD3D[] = { D3D11_COMPARISON_ALWAYS, D3D11_COMPARISON_NEVER, D3D11_COMPARISON_LESS, D3D11_COMPARISON_LESS_EQUAL, D3D11_COMPARISON_GREATER, D3D11_COMPARISON_GREATER_EQUAL, D3D11_COMPARISON_EQUAL, D3D11_COMPARISON_NOT_EQUAL };
UDCOMPILEASSERT(udLengthOf(vcGLSSFToD3D) == vcGLSSF_Total, "Not Enough DirectX Stencil Functions");

bool vcGLState_Init(SDL_Window *pWindow, vcFramebuffer **ppDefaultFramebuffer)
{
  SDL_SysWMinfo windowInfo;
  SDL_VERSION(&windowInfo.version);
  SDL_GetWindowWMInfo(pWindow, &windowInfo);

  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = windowInfo.info.win.window;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  // Create Device
  // set 'createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG' for debugging
  UINT createDeviceFlags = 0;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    return false;

  // Get Default Framebuffer
  memset(&g_defaultFramebuffer, 0, sizeof(g_defaultFramebuffer));
  g_defaultFramebuffer.attachmentCount = 1;

  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_defaultFramebuffer.renderTargetViews[0]);
  pBackBuffer->Release();

  *ppDefaultFramebuffer = &g_defaultFramebuffer;

  // Set rasterizer state
  D3D11_RASTERIZER_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.FillMode = D3D11_FILL_SOLID;
  desc.CullMode = D3D11_CULL_BACK;
  desc.FrontCounterClockwise = true;
  desc.ScissorEnable = true;
  desc.DepthClipEnable = true;
  g_pd3dDevice->CreateRasterizerState(&desc, &g_pRasterizerState);

  s_internalState.fillMode = vcGLSFM_Solid;
  s_internalState.cullMode = vcGLSCM_Back;
  s_internalState.isFrontCCW = true;

  s_internalState.stencil.enabled = false;

  g_maxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;

  return vcGLState_ResetState(true);
}

void vcGLState_Deinit()
{
  if (g_defaultFramebuffer.renderTargetViews[0] != nullptr)
  {
    g_defaultFramebuffer.renderTargetViews[0]->Release();
    g_defaultFramebuffer.renderTargetViews[0] = nullptr;
  }

  if (g_pRasterizerState != nullptr)
  {
    g_pRasterizerState->Release();
    g_pRasterizerState = nullptr;
  }

  if (g_pBlendState != nullptr)
  {
    g_pBlendState->Release();
    g_pBlendState = nullptr;
  }

  if (g_pSwapChain != nullptr)
  {
    g_pSwapChain->Release();
    g_pSwapChain = nullptr;
  }

  if (g_pd3dDeviceContext != nullptr)
  {
    g_pd3dDeviceContext->Release();
    g_pd3dDeviceContext = nullptr;
  }

  if (g_pd3dDevice != nullptr)
  {
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
  }
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
  if (s_internalState.fillMode != fillMode || s_internalState.cullMode != cullMode || s_internalState.isFrontCCW != isFrontCCW || force)
  {
    D3D11_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.FillMode = (fillMode == vcGLSFM_Solid) ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
    desc.FrontCounterClockwise = isFrontCCW;
    desc.ScissorEnable = true;
    desc.DepthClipEnable = true;

    switch (cullMode)
    {
    case vcGLSCM_None:
      desc.CullMode = D3D11_CULL_NONE;
      break;
    case vcGLSCM_Front:
      desc.CullMode = D3D11_CULL_FRONT;
      break;
    case vcGLSCM_Back:
      desc.CullMode = D3D11_CULL_BACK;
      break;
    }

    if (g_pRasterizerState != nullptr)
      g_pRasterizerState->Release();
    g_pd3dDevice->CreateRasterizerState(&desc, &g_pRasterizerState);

    s_internalState.fillMode = fillMode;
    s_internalState.cullMode = cullMode;
    s_internalState.isFrontCCW = isFrontCCW;
  }

  g_pd3dDeviceContext->RSSetState(g_pRasterizerState);

  return true;
}

bool vcGLState_SetBlendMode(vcGLStateBlendMode blendMode, bool force /*= false*/)
{
  if (s_internalState.blendMode != blendMode || force)
  {
    if (g_pBlendState != nullptr)
      g_pBlendState->Release();

    D3D11_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    // Disable all blending for other attachments (match webgl 2.0 functionality)
    desc.IndependentBlendEnable = false;

    desc.AlphaToCoverageEnable = false;
    desc.RenderTarget[0].BlendEnable = true;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    switch (blendMode)
    {
    case vcGLSBM_None:
      desc.RenderTarget[0].BlendEnable = false;
      break;
    case vcGLSBM_Interpolative:
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

      desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
      break;
    case vcGLSBM_Additive:
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
      break;
    case vcGLSBM_Multiplicative:
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
      break;
    case vcGLSBM_Count:
      break;
    }
    g_pd3dDevice->CreateBlendState(&desc, &g_pBlendState);

    s_internalState.blendMode = blendMode;
  }

  const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
  g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, blend_factor, 0xFFFFFFFF);

  return true;
}

bool vcGLState_SetDepthStencilMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, vcGLStencilSettings *pStencil /*= nullptr*/, bool force /*= false*/)
{
  bool enableStencil = pStencil != nullptr;

  if ((s_internalState.depthReadMode != depthReadMode) || (s_internalState.doDepthWrite != doDepthWrite) || force || (s_internalState.stencil.enabled != enableStencil) ||
    (enableStencil && ((s_internalState.stencil.onStencilFail != pStencil->onStencilFail) || (s_internalState.stencil.onDepthFail != pStencil->onDepthFail) || (s_internalState.stencil.onStencilAndDepthPass != pStencil->onStencilAndDepthPass) || (s_internalState.stencil.compareFunc != pStencil->compareFunc) || (s_internalState.stencil.compareMask != pStencil->compareMask) || (s_internalState.stencil.writeMask != pStencil->writeMask))))
  {
    D3D11_DEPTH_STENCIL_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    desc.StencilEnable = enableStencil;
    if (enableStencil)
    {
      desc.FrontFace.StencilFailOp = vcGLSSOPToD3D[pStencil->onStencilFail];
      desc.FrontFace.StencilDepthFailOp = vcGLSSOPToD3D[pStencil->onDepthFail];
      desc.FrontFace.StencilPassOp = vcGLSSOPToD3D[pStencil->onStencilAndDepthPass];
      desc.FrontFace.StencilFunc = vcGLSSFToD3D[pStencil->compareFunc];
      desc.StencilReadMask = pStencil->compareMask;
      desc.StencilWriteMask = pStencil->writeMask;
    }
    desc.BackFace = desc.FrontFace;

    switch (depthReadMode)
    {
    case vcGLSDM_None:
      desc.DepthFunc = D3D11_COMPARISON_NEVER;
      break;
    case vcGLSDM_Less:
      desc.DepthFunc = D3D11_COMPARISON_LESS;
      break;
    case vcGLSDM_LessOrEqual:
      desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
      break;
    case vcGLSDM_Equal:
      desc.DepthFunc = D3D11_COMPARISON_EQUAL;
      break;
    case vcGLSDM_GreaterOrEqual:
      desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
      break;
    case vcGLSDM_Greater:
      desc.DepthFunc = D3D11_COMPARISON_GREATER;
      break;
    case vcGLSDM_NotEqual:
      desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
      break;
    case vcGLSDM_Always:
      desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
      break;
    case vcGLSDM_Total:
      desc.DepthFunc = D3D11_COMPARISON_NEVER;
      break;
    }

    if (doDepthWrite)
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    else
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

    if (g_pDepthStencilState != nullptr)
      g_pDepthStencilState->Release();
    g_pd3dDevice->CreateDepthStencilState(&desc, &g_pDepthStencilState);

    s_internalState.depthReadMode = depthReadMode;
    s_internalState.doDepthWrite = doDepthWrite;

    s_internalState.stencil.enabled = enableStencil;
    if (enableStencil)
    {
      s_internalState.stencil.writeMask = pStencil->writeMask;
      s_internalState.stencil.compareFunc = pStencil->compareFunc;
      s_internalState.stencil.compareValue = pStencil->compareValue;
      s_internalState.stencil.compareMask = pStencil->compareMask;
      s_internalState.stencil.onStencilFail = pStencil->onStencilFail;
      s_internalState.stencil.onDepthFail = pStencil->onDepthFail;
      s_internalState.stencil.onStencilAndDepthPass = pStencil->onStencilAndDepthPass;
    }
  }

  int stencilRef = 0;
  if (pStencil)
    stencilRef = pStencil->compareValue;
  g_pd3dDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, stencilRef);

  return true;
}

bool vcGLState_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth /*= 0.f*/, float maxDepth /*= 1.f*/)
{
  if (x < 0 || y < 0 || width < 1 || height < 1)
    return false;

  D3D11_VIEWPORT viewport;
  viewport.TopLeftX = (float)x;
  viewport.TopLeftY = (float)y;
  viewport.Width = (float)width;
  viewport.Height = (float)height;
  viewport.MinDepth = minDepth;
  viewport.MaxDepth = maxDepth;

  g_pd3dDeviceContext->RSSetViewports(1, &viewport);

  s_internalState.viewportZone = udInt4::create(x, y, width, height);

  vcGLState_Scissor(s_internalState.viewportZone.x, s_internalState.viewportZone.y, s_internalState.viewportZone.x + s_internalState.viewportZone.z, s_internalState.viewportZone.y + s_internalState.viewportZone.w);

  return true;
}

bool vcGLState_SetViewportDepthRange(float minDepth, float maxDepth)
{
  return vcGLState_SetViewport(s_internalState.viewportZone.x, s_internalState.viewportZone.y, s_internalState.viewportZone.x + s_internalState.viewportZone.z, s_internalState.viewportZone.y + s_internalState.viewportZone.w, minDepth, maxDepth);
}

bool vcGLState_Present(SDL_Window * /*pWindow*/)
{
  if (g_pSwapChain == nullptr)
    return false;

  g_pSwapChain->Present(1, 0); // Present with vsync

  memset(&s_internalState.frameInfo, 0, sizeof(s_internalState.frameInfo));
  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  g_defaultFramebuffer.renderTargetViews[0]->Release();
  g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_defaultFramebuffer.renderTargetViews[0]);
  pBackBuffer->Release();

  return true;
}

void vcGLState_Scissor(int left, int top, int right, int bottom, bool force /*= false*/)
{
  udInt4 newScissor = udInt4::create(left, top, right, bottom);
  const D3D11_RECT r = { (LONG)left, (LONG)top, (LONG)right, (LONG)bottom };

  if (newScissor != s_internalState.scissorZone || force)
  {
    g_pd3dDeviceContext->RSSetScissorRects(1, &r);
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
