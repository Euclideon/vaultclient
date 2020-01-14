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

bool vcFramebuffer_BeginReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || int(x + width) > pAttachment->width || int(y + height) > pAttachment->height)
    return false;

  udResult result = udR_Success;
  if ((pAttachment->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead && pAttachment->pStagingTextureD3D[pAttachment->stagingIndex] == nullptr)
  {
    // Texture not configured for pixel read back, create a single staging texture now for re-use.
    // TODO: is creating a 1*1 staging texture every query more performant than creating a single width*height and re-using?
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = pAttachment->d3dFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    // Create a full res texture, because subsequent reads must work with different regions. 
    // Might be some optimizations here, if not D24S8 format, and width != tex.width or height != tex.height,
    // recreate the staging texture then.
    desc.Width = pAttachment->width;
    desc.Height = pAttachment->height;

    UD_ERROR_IF(g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pAttachment->pStagingTextureD3D[pAttachment->stagingIndex]) != S_OK, udR_InternalError);
  }

  // Begin asynchronous copy
  if (pAttachment->format == vcTextureFormat_D24S8 || pAttachment->format == vcTextureFormat_D32F)
  {
    // If you use CopySubresourceRegion with a depth-stencil buffer or a multisampled resource, you must copy the whole subresource
    // TODO: What about D32F format? (its not a depth-stencil)
    g_pd3dDeviceContext->CopyResource(pAttachment->pStagingTextureD3D[pAttachment->stagingIndex], pAttachment->pTextureD3D);
  }
  else
  {
    // left, top, front, right, bottom, back
    D3D11_BOX srcBox = { x, y, 0, (x + width), (y + height), 1 };
    g_pd3dDeviceContext->CopySubresourceRegion(pAttachment->pStagingTextureD3D[pAttachment->stagingIndex], 0, x, y, 0, pAttachment->pTextureD3D, 0, &srcBox);
  }

  if ((pAttachment->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead)
  {
    UD_ERROR_IF(!vcFramebuffer_EndReadPixels(pFramebuffer, pAttachment, x, y, width, height, pPixels), udR_InternalError);
    pAttachment->stagingIndex = 0; // force only using single staging texture
  }

epilogue:

  return result == udR_Success;
}

bool vcFramebuffer_EndReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pFramebuffer == nullptr || pAttachment == nullptr || pPixels == nullptr || int(x + width) > pAttachment->width || int(y + height) > pAttachment->height || (pAttachment->pStagingTextureD3D[pAttachment->stagingIndex] == nullptr))
    return false;

  udResult result = udR_Success;
  int pixelBytes = 4; // assumptions
  ID3D11Texture2D *pStagingTexture = pAttachment->pStagingTextureD3D[pAttachment->stagingIndex];
  pAttachment->stagingIndex = (pAttachment->stagingIndex + 1) & 1;

  D3D11_MAPPED_SUBRESOURCE msr;
  UD_ERROR_IF(g_pd3dDeviceContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &msr) != S_OK, udR_InternalError);

  uint32_t *pPixelData = ((uint32_t*)msr.pData) + (x + y * pAttachment->width);
  memcpy(pPixels, pPixelData, width * height * pixelBytes);

  g_pd3dDeviceContext->Unmap(pStagingTexture, 0);

epilogue:
  return result == udR_Success;
}