#include "gl/vcFramebuffer.h"
#include "vcD3D11.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, int level /*= 0*/)
{
  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);

  D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
  memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));

  renderTargetViewDesc.Format = pTexture->d3dFormat;
  renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  renderTargetViewDesc.Texture2D.MipSlice = level;

  // Create the render target view.
  g_pd3dDevice->CreateRenderTargetView(pTexture->pTextureD3D, &renderTargetViewDesc, &pFramebuffer->pRenderTargetView);

  if (pDepth != nullptr)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    memset(&dsvDesc, 0, sizeof(dsvDesc));
    dsvDesc.Format = ((pDepth->format == vcTextureFormat_D32F) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT);
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    g_pd3dDevice->CreateDepthStencilView(pDepth->pTextureD3D, &dsvDesc, &pFramebuffer->pDepthStencilView);
  }

  *ppFramebuffer = pFramebuffer;
  return true;
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

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer)
{
  if (pFramebuffer == nullptr || pFramebuffer->pRenderTargetView == nullptr)
    return false;

  g_pd3dDeviceContext->OMSetRenderTargets(1, &pFramebuffer->pRenderTargetView, pFramebuffer->pDepthStencilView);

  return true;
}

bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour)
{
  if (pFramebuffer == nullptr || pFramebuffer->pRenderTargetView == nullptr)
    return false;

  float colours[4] = { ((colour >> 16) & 0xFF) / 255.f, ((colour >> 8) & 0xFF) / 255.f, (colour & 0xFF) / 255.f, ((colour >> 24) & 0xFF) / 255.f };

  g_pd3dDeviceContext->ClearRenderTargetView(pFramebuffer->pRenderTargetView, colours);
  if (pFramebuffer->pDepthStencilView)
    g_pd3dDeviceContext->ClearDepthStencilView(pFramebuffer->pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

  return true;
}

bool vcFramebuffer_ReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, void *pPixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || int(x + width) > pAttachment->width || int(y + height) > pAttachment->height)
    return false;

  udResult result = udR_Success;
  int pixelBytes = 4; // assumptions

  // Create D3D11 texture
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = pAttachment->d3dFormat;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

  if (pAttachment->format == vcTextureFormat_D24S8 || pAttachment->format == vcTextureFormat_D32F)
  {
    desc.Width = pAttachment->width;
    desc.Height = pAttachment->height;
  }

  ID3D11Texture2D *pTextureD3D = nullptr;
  HRESULT res = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pTextureD3D);
  UD_ERROR_IF(res != S_OK, udR_InternalError);

  if (pAttachment->format == vcTextureFormat_D24S8 || pAttachment->format == vcTextureFormat_D32F)
  {
    g_pd3dDeviceContext->CopyResource(pTextureD3D, pAttachment->pTextureD3D);
  }
  else
  {
    D3D11_BOX srcBox;
    srcBox.left = x;
    srcBox.right = srcBox.left + width;
    srcBox.top = y;
    srcBox.bottom = srcBox.top + height;
    srcBox.front = 0;
    srcBox.back = 1;
    g_pd3dDeviceContext->CopySubresourceRegion(pTextureD3D, 0, 0, 0, 0, pAttachment->pTextureD3D, 0, &srcBox);
  }

  D3D11_MAPPED_SUBRESOURCE msr;
  res = g_pd3dDeviceContext->Map(pTextureD3D, 0, D3D11_MAP_READ, 0, &msr);
  UD_ERROR_IF(res != S_OK, udR_InternalError);
 
  if (pAttachment->format == vcTextureFormat_D24S8)
  {
    uint32_t *pPixel = ((uint32_t*)msr.pData) + (x + y * pAttachment->width);

    // 24 bit unsigned int -> float
    uint8_t r = ((*pPixel) & 0xff) >> 0;
    uint8_t g = ((*pPixel) & 0xff00) >> 8;
    uint8_t b = ((*pPixel) & 0xff0000) >> 16;
    uint8_t a = ((*pPixel) & 0xff000000) >> 24; // stencil

    uint32_t t = uint32_t((b << 16) | (g << 8) | (r << 0));
    float v = t / ((1 << 24) - 1.0f);
    memcpy(pPixels, &v, 4);
  }
  else
  {
    memcpy(pPixels, msr.pData, width * height * pixelBytes);
  }

  g_pd3dDeviceContext->Unmap(pTextureD3D, 0);

epilogue:
  pTextureD3D->Release();

  return result == udR_Success;
}
