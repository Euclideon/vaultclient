#include "gl/vcFramebuffer.h"
#include "vcD3D11.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, uint32_t level /*= 0*/)
{
  if (ppFramebuffer == nullptr || pTexture == nullptr)
    return false;

  udResult result = udR_Success;
  D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
  memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));

  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  UD_ERROR_NULL(pFramebuffer, udR_MemoryAllocationFailure);

  renderTargetViewDesc.Format = pTexture->d3dFormat;
  renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  renderTargetViewDesc.Texture2D.MipSlice = level;

  // Create the render target view.
  g_pd3dDevice->CreateRenderTargetView(pTexture->pTextureD3D, &renderTargetViewDesc, &pFramebuffer->pRenderTargetView);

  if (pDepth != nullptr)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    memset(&dsvDesc, 0, sizeof(dsvDesc));
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    g_pd3dDevice->CreateDepthStencilView(pDepth->pTextureD3D, &dsvDesc, &pFramebuffer->pDepthStencilView);
  }

  *ppFramebuffer = pFramebuffer;
  pFramebuffer = nullptr;

epilogue:
  if (pFramebuffer != nullptr)
    vcFramebuffer_Destroy(&pFramebuffer);

  return result == udR_Success;
}

void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer)
{
  if (ppFramebuffer == nullptr || *ppFramebuffer == nullptr)
    return;

  vcFramebuffer *pBuffer = *ppFramebuffer;
  pBuffer->pRenderTargetView->Release();
  if (pBuffer->pDepthStencilView != nullptr)
    pBuffer->pDepthStencilView->Release();

  udFree(*ppFramebuffer);
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer, const vcFramebufferClearOperation clearOperation /*= vcFramebufferClearOperation_None*/, uint32_t clearColour /*= 0x0*/, const vcFramebufferClearOperation clearPreviousOperation /*= vcFramebufferClearOperation_None*/)
{
  udUnused(clearPreviousOperation);

  if (pFramebuffer == nullptr || pFramebuffer->pRenderTargetView == nullptr)
    return false;

  g_pd3dDeviceContext->OMSetRenderTargets(1, &pFramebuffer->pRenderTargetView, pFramebuffer->pDepthStencilView);

  float colours[4] = { ((clearColour >> 16) & 0xFF) / 255.f, ((clearColour >> 8) & 0xFF) / 255.f, (clearColour & 0xFF) / 255.f, ((clearColour >> 24) & 0xFF) / 255.f };

  bool clearRenderTargetView = false;
  bool clearDepthStencilView = false;

  switch (clearOperation)
  {
  case vcFramebufferClearOperation_None:
    // Clear nothing
    break;
  case vcFramebufferClearOperation_Colour:
    clearRenderTargetView = true;
    break;
  case vcFramebufferClearOperation_DepthStencil:
    clearDepthStencilView = true;
    break;
  case vcFramebufferClearOperation_All:
    clearRenderTargetView = true;
    clearDepthStencilView = true;
    break;
  }

  if (clearRenderTargetView)
    g_pd3dDeviceContext->ClearRenderTargetView(pFramebuffer->pRenderTargetView, colours);

  if (clearDepthStencilView && pFramebuffer->pDepthStencilView)
    g_pd3dDeviceContext->ClearDepthStencilView(pFramebuffer->pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

  return true;
}
