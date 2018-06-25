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
int g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

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
  UINT createDeviceFlags = 0;
#if UD_DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    return false;

  // Get Default Framebuffer
  memset(&g_defaultFramebuffer, 0, sizeof(vcFramebuffer));

  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_defaultFramebuffer.pRenderTargetView);
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

  return vcGLState_ResetState(true);
}

void vcGLState_Deinit()
{
  if (g_defaultFramebuffer.pRenderTargetView != nullptr)
  {
    g_defaultFramebuffer.pRenderTargetView->Release();
    g_defaultFramebuffer.pRenderTargetView = nullptr;
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
  success &= vcGLState_SetDepthMode(pState->depthReadMode, pState->doDepthWrite);

  return success;
}

bool vcGLState_ResetState(bool force /*= false*/)
{
  vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_Back, true, force);
  vcGLState_SetBlendMode(vcGLSBM_None, force);
  vcGLState_SetDepthMode(vcGLSDM_Less, true, force);

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

    if (cullMode == vcGLSCM_None)
      desc.CullMode = D3D11_CULL_NONE;
    else if (cullMode == vcGLSCM_Front)
      desc.CullMode = D3D11_CULL_NONE;
    else if (cullMode == vcGLSCM_Back)
      desc.CullMode = D3D11_CULL_BACK;

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
    desc.AlphaToCoverageEnable = false;
    desc.RenderTarget[0].BlendEnable = true;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (blendMode == vcGLSBM_None)
    {
      desc.RenderTarget[0].BlendEnable = false;
    }
    else if (blendMode == vcGLSBM_Interpolative)
    {
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
      desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    }
    else if (blendMode == vcGLSBM_Additive)
    {
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    }
    else if (blendMode == vcGLSBM_Multiplicative)
    {
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    }

    g_pd3dDevice->CreateBlendState(&desc, &g_pBlendState);

    s_internalState.blendMode = blendMode;
  }

  const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
  g_pd3dDeviceContext->OMSetBlendState(g_pBlendState, blend_factor, 0xFFFFFFFF);

  return true;
}

bool vcGLState_SetDepthMode(vcGLStateDepthMode depthReadMode, bool doDepthWrite, bool force /*= false*/)
{
  if (s_internalState.depthReadMode != depthReadMode || s_internalState.doDepthWrite != doDepthWrite || force)
  {
    D3D11_DEPTH_STENCIL_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable = false;
    desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.BackFace = desc.FrontFace;

    if (depthReadMode == vcGLSDM_None)
      desc.DepthFunc = D3D11_COMPARISON_NEVER;
    else if (depthReadMode == vcGLSDM_Less)
      desc.DepthFunc = D3D11_COMPARISON_LESS;
    else if (depthReadMode == vcGLSDM_LessOrEqual)
      desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    else if (depthReadMode == vcGLSDM_Equal)
      desc.DepthFunc = D3D11_COMPARISON_EQUAL;
    else if (depthReadMode == vcGLSDM_GreaterOrEqual)
      desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    else if (depthReadMode == vcGLSDM_Greater)
      desc.DepthFunc = D3D11_COMPARISON_GREATER;
    else if (depthReadMode == vcGLSDM_NotEqual)
      desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
    else if (depthReadMode == vcGLSDM_Always)
      desc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    if (doDepthWrite)
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    else
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

    if (g_pDepthStencilState != nullptr)
      g_pDepthStencilState->Release();
    g_pd3dDevice->CreateDepthStencilState(&desc, &g_pDepthStencilState);

    s_internalState.depthReadMode = depthReadMode;
    s_internalState.doDepthWrite = doDepthWrite;
  }

  g_pd3dDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

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

  //Reset the scissor back to the full viewport
  D3D11_RECT scissorRect = { (LONG)0, (LONG)0, (LONG)width, (LONG)height };
  g_pd3dDeviceContext->RSSetScissorRects(1, &scissorRect);

  return true;
}

bool vcGLState_Present(SDL_Window * /*pWindow*/)
{
  if (g_pSwapChain == nullptr)
    return false;

  g_pSwapChain->Present(1, 0); // Present with vsync
  return true;
}

bool vcGLState_ResizeBackBuffer(const uint32_t width, const uint32_t height)
{
  g_defaultFramebuffer.pRenderTargetView->Release();
  g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_defaultFramebuffer.pRenderTargetView);
  pBackBuffer->Release();

  return true;
}
