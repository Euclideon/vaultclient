#import "vcFramebuffer.h"
#import "vcMetal.h"

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth /*= nullptr*/, uint32_t level /*= 0*/, vcTexture *pAttachment2 /*= nullptr*/)
{
  udUnused(level);
  udUnused(pAttachment2);

  if (ppFramebuffer == nullptr || pTexture == nullptr)
    return false;

  udResult result = udR_Success;

  vcFramebuffer *pFramebuffer = udAllocType(vcFramebuffer, 1, udAF_Zero);
  UD_ERROR_NULL(pFramebuffer, udR_MemoryAllocationFailure);

  pFramebuffer->attachmentCount = 1 + (pAttachment2 ? 1 : 0);
  pFramebuffer->attachments[0] = pTexture;
  pFramebuffer->attachments[1] = pAttachment2;
  pFramebuffer->pDepth = pDepth;
  pFramebuffer->clear = 0;
  pFramebuffer->actions = 0;
  
  @autoreleasepool
  {
    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];

    for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
    {
      pass.colorAttachments[i].loadAction = MTLLoadActionClear;
      pass.colorAttachments[i].storeAction = MTLStoreActionStore;
      pass.colorAttachments[i].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
      pass.colorAttachments[i].texture = pFramebuffer->attachments[i]->texture;
    }
    
    pass.depthAttachment.texture = nil;
    pass.stencilAttachment.texture = nil;
    if (pFramebuffer->pDepth != nullptr)
    {
      pass.depthAttachment.texture = pFramebuffer->pDepth->texture;
      pass.depthAttachment.loadAction = MTLLoadActionClear;
      pass.depthAttachment.storeAction = MTLStoreActionStore;
      pass.depthAttachment.clearDepth = 1.0;
    }

    pFramebuffer->pRenderPass = pass;
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

  @autoreleasepool
  {
    if (g_pCurrFramebuffer == (*ppFramebuffer))
      g_pCurrFramebuffer = nullptr;

    (*ppFramebuffer)->pRenderPass = nil;

    [(*ppFramebuffer)->encoder endEncoding];
    (*ppFramebuffer)->encoder = nil;

    [(*ppFramebuffer)->commandBuffer commit];
    [(*ppFramebuffer)->commandBuffer waitUntilCompleted];
    (*ppFramebuffer)->commandBuffer = nil;
  }
  
  udFree(*ppFramebuffer);
  *ppFramebuffer = nullptr;
}

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer, const vcFramebufferClearOperation clearOperation /*= vcFramebufferClearOperation_None*/, uint32_t clearColour /*= 0x0*/, const vcFramebufferClearOperation clearPreviousOperation /*= vcFramebufferClearOperation_None*/)
{
  udUnused(clearOperation);
  udUnused(clearPreviousOperation);

  if (pFramebuffer == nullptr)
    return false;

  // Finalise current framebuffer before binding this framebuffer
  if (g_pCurrFramebuffer != nullptr && g_pCurrFramebuffer != g_pDefaultFramebuffer)
  {
    @autoreleasepool {
      if (g_pCurrFramebuffer->encoder != nil)
      {
        [g_pCurrFramebuffer->encoder endEncoding];
        [g_pCurrFramebuffer->commandBuffer commit];
        [g_pCurrFramebuffer->commandBuffer waitUntilCompleted];
      }

      g_pCurrFramebuffer->commandBuffer = nil;
      g_pCurrFramebuffer->encoder = nil;
    }
    g_pCurrFramebuffer->actions = 0;
  }

  g_pCurrFramebuffer = pFramebuffer;

  switch (clearOperation)
  {
    case vcFramebufferClearOperation_None:
      for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
        pFramebuffer->pRenderPass.colorAttachments[i].loadAction = MTLLoadActionLoad;

      pFramebuffer->pRenderPass.depthAttachment.loadAction = MTLLoadActionLoad;
      pFramebuffer->pRenderPass.stencilAttachment.loadAction = MTLLoadActionLoad;
      break;
    case vcFramebufferClearOperation_Colour:
      for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
        pFramebuffer->pRenderPass.colorAttachments[i].loadAction = MTLLoadActionClear;

      pFramebuffer->pRenderPass.depthAttachment.loadAction = MTLLoadActionLoad;
      pFramebuffer->pRenderPass.stencilAttachment.loadAction = MTLLoadActionLoad;
      break;
    case vcFramebufferClearOperation_DepthStencil:
      for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
        pFramebuffer->pRenderPass.colorAttachments[i].loadAction = MTLLoadActionLoad;

      pFramebuffer->pRenderPass.depthAttachment.loadAction = MTLLoadActionClear;
      pFramebuffer->pRenderPass.stencilAttachment.loadAction = MTLLoadActionClear;
      break;
    case vcFramebufferClearOperation_All:
      for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
        pFramebuffer->pRenderPass.colorAttachments[i].loadAction = MTLLoadActionClear;

      pFramebuffer->pRenderPass.depthAttachment.loadAction = MTLLoadActionClear;
      pFramebuffer->pRenderPass.stencilAttachment.loadAction = MTLLoadActionClear;
      break;
  }

  if (pFramebuffer->clear != clearColour)
  {
    udFloat4 col = udFloat4::create(((clearColour >> 16) & 0xFF) / 255.f, ((clearColour >> 8) & 0xFF) / 255.f, (clearColour & 0xFF) / 255.f, ((clearColour >> 24) & 0xFF) / 255.f);
    for (int i = 0; i < pFramebuffer->attachmentCount; ++i)
      pFramebuffer->pRenderPass.colorAttachments[i].clearColor = MTLClearColorMake(col.x,col.y,col.z,col.w);

    pFramebuffer->pRenderPass.depthAttachment.clearDepth = 1.0;
  }

  if (g_pCurrFramebuffer != g_pDefaultFramebuffer)
  {
    @autoreleasepool {
      g_pCurrFramebuffer->commandBuffer = [g_queue commandBuffer];
      g_pCurrFramebuffer->encoder = [g_pCurrFramebuffer->commandBuffer renderCommandEncoderWithDescriptor:g_pCurrFramebuffer->pRenderPass];
    }
  }

  [pFramebuffer->encoder setViewport:{
    .originX = double(g_internalState.viewportZone.x),
    .originY = double(g_internalState.viewportZone.y),
    .width = double(g_internalState.viewportZone.z),
    .height = double(g_internalState.viewportZone.w),
    .znear = g_internalState.depthRange.x,
    .zfar = g_internalState.depthRange.y
  }];

  // Set variables from global state
  vcGLState_SetFaceMode(g_internalState.fillMode, g_internalState.cullMode, g_internalState.isFrontCCW, true);
  vcGLState_SetDepthStencilMode(g_internalState.depthReadMode, g_internalState.doDepthWrite, (g_internalState.stencil.enabled ? &g_internalState.stencil : nullptr), true);

  return true;
}
