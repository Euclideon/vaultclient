#include "gl/vcBackbuffer.h"
#include "vcD3D11.h"

#include <SDL_syswm.h>

udResult vcBackbuffer_Create(vcBackbuffer **ppWindow, SDL_Window *pHostWindow, int width, int height)
{
  SDL_SysWMinfo windowInfo;
  SDL_VERSION(&windowInfo.version);
  SDL_GetWindowWMInfo(pHostWindow, &windowInfo);

  vcBackbuffer *pWindow = udAllocType(vcBackbuffer, 1, udAF_Zero);

  // Create swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferDesc.Width = (UINT)width;
  sd.BufferDesc.Height = (UINT)height;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 1;
  sd.OutputWindow = windowInfo.info.win.window;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  sd.Flags = 0;

  g_pd3dFactory->CreateSwapChain(g_pd3dDevice, &sd, &pWindow->pSwapChain);

  // Create the render target
  if (pWindow->pSwapChain)
  {
    ID3D11Texture2D *pBackBuffer;
    pWindow->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &pWindow->frameBuffer.pRenderTargetView);
    pBackBuffer->Release();
  }

  *ppWindow = pWindow;

  return udR_Success;
}

void vcBackbuffer_Destroy(vcBackbuffer **ppWindow)
{
  if (ppWindow == nullptr || *ppWindow == nullptr)
    return;

  vcBackbuffer *pWindow = *ppWindow;
  *ppWindow = nullptr;

  pWindow->pSwapChain->Release();
  pWindow->frameBuffer.pRenderTargetView->Release();

  udFree(pWindow);
}

udResult vcBackbuffer_WindowResized(vcBackbuffer *pWindow, int width, int height)
{
  pWindow->frameBuffer.pRenderTargetView->Release();
  pWindow->pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

  ID3D11Texture2D *pBackBuffer = nullptr;
  pWindow->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &pWindow->frameBuffer.pRenderTargetView);
  pBackBuffer->Release();

  return udR_Success;
}

udResult vcBackbuffer_Bind(vcBackbuffer *pWindow, vcFramebufferClearOperation clearOperation /*= vcFramebufferClearOperation_All*/, uint32_t clearColour /*= 0x0*/)
{
  vcFramebuffer_Bind(&pWindow->frameBuffer, clearOperation, clearColour);

  return udR_Success;
}

udResult vcBackbuffer_Present(vcBackbuffer *pWindow)
{
  pWindow->pSwapChain->Present(1, 0); // Present with vsync

  // This should only be reset on the main target
  //memset(&s_internalState.frameInfo, 0, sizeof(s_internalState.frameInfo));

  return udR_Success;
}
