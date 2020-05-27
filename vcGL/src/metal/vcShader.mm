#import "vcMetal.h"
#import "../../src/vcConstants.h" // TODO: Fix this
#import "vcShader.h"
#import <Metal/Metal.h>

#import "udPlatformUtil.h"
#import "udStringUtil.h"
#import "udFile.h"

void vcShader_CreatePipelineDesc(vcShader *pShader, int pipelineIndex, id<MTLFunction> vFunc, id<MTLFunction> fFunc, MTLVertexDescriptor *vertexDesc)
{
  @autoreleasepool {
    MTLRenderPipelineDescriptor *pDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pDesc.vertexFunction = vFunc;
    pDesc.fragmentFunction = fFunc;
    pDesc.vertexDescriptor = vertexDesc;

    pDesc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
    pDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    pDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;

    pDesc.colorAttachments[0].blendingEnabled = YES;
    pDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

    pShader->pDesc = pDesc;
  }
}

void vcShader_UpdatePipeline(vcShader *pShader)
{
  bool modified = false;

  // Update render target colour format
  for (int i = 0; i < g_pCurrFramebuffer->attachmentCount; ++i)
  {
    if (pShader->pDesc.colorAttachments[i].pixelFormat != g_pCurrFramebuffer->attachments[i]->texture.pixelFormat)
    {
      pShader->pDesc.colorAttachments[i].pixelFormat = g_pCurrFramebuffer->attachments[i]->texture.pixelFormat;
      modified = true;
    }
  }

  // Update unused render target colour formats if needed
  for (int i = g_pCurrFramebuffer->attachmentCount; ; ++i)
  {
    if (pShader->pDesc.colorAttachments[i].pixelFormat == MTLPixelFormatInvalid)
      break;

    pShader->pDesc.colorAttachments[i].pixelFormat = MTLPixelFormatInvalid;
    modified = true;
  }

  // Update render target depth format
  if (g_pCurrFramebuffer->pDepth != nullptr && pShader->pDesc.depthAttachmentPixelFormat != g_pCurrFramebuffer->pDepth->texture.pixelFormat)
  {
    pShader->pDesc.depthAttachmentPixelFormat = g_pCurrFramebuffer->pDepth->texture.pixelFormat;
    modified = true;
  }
  else if (g_pCurrFramebuffer->pDepth == nullptr && pShader->pDesc.depthAttachmentPixelFormat != MTLPixelFormatInvalid)
  {
    pShader->pDesc.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
    modified = true;
  }

  // Update blend mode
  switch (g_internalState.blendMode)
  {
    case vcGLSBM_None:
      if (pShader->pDesc.colorAttachments[0].blendingEnabled != NO)
      {
        pShader->pDesc.colorAttachments[0].blendingEnabled = NO;
        modified = true;
      }
      break;
    case vcGLSBM_Interpolative:
      if (pShader->pDesc.colorAttachments[0].blendingEnabled != YES || pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor != MTLBlendFactorSourceAlpha)
      {
        pShader->pDesc.colorAttachments[0].blendingEnabled = YES;
        pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pShader->pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pShader->pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pShader->pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
        modified = true;
      }
      break;
    case vcGLSBM_Additive:
      if (pShader->pDesc.colorAttachments[0].blendingEnabled != YES || pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor != MTLBlendFactorOne)
      {
        pShader->pDesc.colorAttachments[0].blendingEnabled = YES;
        pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        pShader->pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pShader->pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
        pShader->pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        modified = true;
      }
      break;
    case vcGLSBM_Multiplicative:
      if (pShader->pDesc.colorAttachments[0].blendingEnabled != YES || pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor != MTLBlendFactorDestinationColor)
      {
        pShader->pDesc.colorAttachments[0].blendingEnabled = YES;
        pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorDestinationColor;
        pShader->pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pShader->pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorZero;
        pShader->pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        modified = true;
      }
      break;
    case vcGLSBM_Count:
      if (pShader->pDesc.colorAttachments[0].blendingEnabled != YES || pShader->pDesc.colorAttachments[0].sourceRGBBlendFactor != MTLBlendOperationAdd)
      {
        pShader->pDesc.colorAttachments[0].blendingEnabled = YES;
        pShader->pDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pShader->pDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        modified = true;
      }
      break;
  }

  NSError *err = nil;
  if (modified)
    pShader->pipeline = [g_device newRenderPipelineStateWithDescriptor:pShader->pDesc error:&err];

#ifdef METAL_DEBUG
  if (err != nil)
    NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif
}

bool vcShader_CreateFromTextInternal(vcShader **ppShader, const char *pVertexShaderFilename, const char *pVertexShader, const char *pFragmentShaderFilename, const char *pFragmentShader, const vcVertexLayoutTypes *pVertLayout, uint32_t totalTypes)
{
  @autoreleasepool {
    if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr)
      return false;

    vcShader *pShader = udAllocType(vcShader, 1, udAF_Zero);

    MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];

    ptrdiff_t accumulatedOffset = 0;
    for (uint32_t i = 0; i < totalTypes; ++i)
    {
      vertexDesc.attributes[i].bufferIndex = 30;
      vertexDesc.attributes[i].offset = accumulatedOffset;

      switch (pVertLayout[i])
      {
        case vcVLT_Position2:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat2;
          accumulatedOffset += 2 * sizeof(float);
          break;
        case vcVLT_Position3:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat3;
          accumulatedOffset += 3 * sizeof(float);
          break;
       case vcVLT_Position4:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat4;
          accumulatedOffset += 4 * sizeof(float);
          break;
        case vcVLT_TextureCoords2:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat2;
          accumulatedOffset += 2 * sizeof(float);
          break;
        case vcVLT_ColourBGRA:
          vertexDesc.attributes[i].format = MTLVertexFormatUChar4Normalized;
          accumulatedOffset += 1 * sizeof(uint32_t);
          break;
        case vcVLT_Normal3:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat3;
          accumulatedOffset += 3 * sizeof(float);
          break;
       case vcVLT_QuadCorner:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat2;
          accumulatedOffset += 2 * sizeof(float);
          break;
       case vcVLT_Color0:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat4;
          accumulatedOffset += 4 * sizeof(float);
          break;
       case vcVLT_Color1:
          vertexDesc.attributes[i].format = MTLVertexFormatFloat4;
          accumulatedOffset += 4 * sizeof(float);
          break;
        case vcVLT_Unsupported: // TODO: (EVC-641) Handle unsupported attributes interleaved with supported attributes
          break;
        case vcVLT_TotalTypes:
          break;
      }
    }
    vertexDesc.layouts[30].stride = accumulatedOffset;
    vertexDesc.layouts[30].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDesc.layouts[30].stepRate = 1;

    id<MTLFunction> vFunc = nil;
    id<MTLFunction> fFunc = nil;

    if (pVertexShaderFilename != nullptr && udStrchr(pVertexShader, "\n") != nullptr)
    {
      pShader->vertexLibrary = [g_device newLibraryWithSource:[NSString stringWithUTF8String:pVertexShader] options: {} error:nil];
      pShader->fragmentLibrary = [g_device newLibraryWithSource:[NSString stringWithUTF8String:pFragmentShader] options: {} error:nil];
      
      vFunc = [pShader->vertexLibrary newFunctionWithName:@"main0"];
      fFunc = [pShader->fragmentLibrary newFunctionWithName:@"main0"];
    }

    pShader->inititalised = false;

    vcShader_CreatePipelineDesc(pShader, 0, vFunc, fFunc, vertexDesc);

    NSError *err = nil;
    MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
    MTLRenderPipelineReflection *pReflectionObj = nil;
    pShader->pipeline = [g_device newRenderPipelineStateWithDescriptor:pShader->pDesc options:option reflection:&pReflectionObj error:&err];
#ifdef METAL_DEBUG
  if (err != nil)
    NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif

    for (MTLArgument *arg in pReflectionObj.vertexArguments)
    {
      if (arg.type != MTLArgumentTypeBuffer)
        continue;

      vcShaderConstantBuffer *pBuffer = &pShader->bufferObjects[pShader->numBufferObjects];
      pBuffer->buffers[1].index = -1;
      ++pShader->numBufferObjects;
      
      udStrcpy(pBuffer->name, arg.name.UTF8String);
      pBuffer->expectedSize = arg.bufferDataSize;
      pBuffer->buffers[0].index = arg.index;
    }

    for (MTLArgument *arg in pReflectionObj.fragmentArguments)
    {
      if (arg.type != MTLArgumentTypeBuffer)
        continue;

      vcShaderConstantBuffer *pBuffer = nullptr;

      for (size_t i = 0; i < udLengthOf(pShader->bufferObjects); ++i)
      {
        if (udStrEqual(pShader->bufferObjects[i].name, arg.name.UTF8String))
        {
          pBuffer = &pShader->bufferObjects[i];
          break;
        }
      }

      if (pBuffer == nullptr)
      {
        pBuffer = &pShader->bufferObjects[pShader->numBufferObjects];
        pBuffer->buffers[0].index = -1;
        ++pShader->numBufferObjects;
      }

      udStrcpy(pBuffer->name, arg.name.UTF8String);
      pBuffer->expectedSize = arg.bufferDataSize;
      pBuffer->buffers[1].index = arg.index;
    }

    pReflectionObj = nil;

    vcShader_GetConstantBuffer(&pShader->pCameraPlaneParams, pShader, "u_cameraPlaneParams", sizeof(float) * 4);

    *ppShader = pShader;
    pShader = nullptr;

    return (*ppShader != nullptr);
  }
}

bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pVertLayout, uint32_t totalTypes)
{
  return vcShader_CreateFromTextInternal(ppShader, nullptr, pVertexShader, nullptr, pFragmentShader, pVertLayout, totalTypes);
}

bool vcShader_CreateFromFile(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pInputTypes, uint32_t totalInputs)
{
  char *pVertexShaderText = nullptr;
  char *pFragmentShaderText = nullptr;

  udFile_Load(udTempStr("%s.metal", pVertexShader), &pVertexShaderText);
  udFile_Load(udTempStr("%s.metal", pFragmentShader), &pFragmentShaderText);

  char *pTemp = (char*)udStrchr(pVertexShaderText, "\n");
  if (pTemp && pTemp[1] == '\0') // This file contains the name of a shader
    *pTemp = '\0'; // Zero the new line so the shader can be found

  pTemp = (char *)udStrchr(pFragmentShaderText, "\n");
  if (pTemp && pTemp[1] == '\0') // This file contains the name of a shader
    *pTemp = '\0'; // Zero the new line so the shader can be found

  bool success = vcShader_CreateFromTextInternal(ppShader, udStrrchr(pVertexShader, "/") + 1, pVertexShaderText, udStrrchr(pFragmentShader, "/") + 1, pFragmentShaderText, pInputTypes, totalInputs);

  udFree(pFragmentShaderText);
  udFree(pVertexShaderText);

  return success;
}

void vcShader_DestroyShader(vcShader **ppShader)
{
  if (ppShader == nullptr || *ppShader == nullptr)
    return;

  @autoreleasepool
  {
    (*ppShader)->vertexLibrary = nil;
    (*ppShader)->fragmentLibrary = nil;
    (*ppShader)->pDesc = nil;
    (*ppShader)->pipeline = nil;
  }

  udFree(*ppShader);
}

bool vcShader_Bind(vcShader *pShader)
{
  @autoreleasepool {
    if (pShader != nullptr)
    {
      if (pShader->inititalised)
      {
        g_pCurrShader = pShader;

        vcShader_UpdatePipeline(pShader);

        if (g_pCurrShader != nullptr && g_pCurrFramebuffer != nullptr)
          [g_pCurrFramebuffer->encoder setRenderPipelineState:g_pCurrShader->pipeline];

        pShader->cameraPlane = { s_CameraNearPlane, s_CameraFarPlane, 0.f, 1.f };
        vcShader_BindConstantBuffer(pShader, pShader->pCameraPlaneParams, &pShader->cameraPlane, sizeof(pShader->cameraPlane));
      }
      else
      {
        pShader->inititalised = true;
      }
    }
    return true;
  }
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler/* = nullptr*/, vcGLSamplerShaderStage samplerStage /*= vcGLSamplerShaderStage_Fragment*/)
{
  @autoreleasepool {
    udUnused(pShader);
    udUnused(pSampler);

    if (pTexture == nullptr)
      return false;

    if (samplerStage == vcGLSamplerShaderStage_Fragment)
    {
      [g_pCurrFramebuffer->encoder setFragmentTexture:pTexture->texture atIndex:samplerIndex];
      [g_pCurrFramebuffer->encoder setFragmentSamplerState:pTexture->sampler atIndex:samplerIndex];
    }
    else
    {
      [g_pCurrFramebuffer->encoder setVertexTexture:pTexture->texture atIndex:samplerIndex];
      [g_pCurrFramebuffer->encoder setVertexSamplerState:pTexture->sampler atIndex:samplerIndex];
    }

    return true;
  }
}

bool vcShader_GetConstantBuffer(vcShaderConstantBuffer **ppBuffer, vcShader *pShader, const char *pBufferName, const size_t bufferSize)
{
  if (ppBuffer == nullptr || pShader == nullptr || bufferSize == 0)
    return false;

  *ppBuffer = nullptr;

  for (int i = 0; i < pShader->numBufferObjects; ++i)
  {
    if (udStrEquali(pShader->bufferObjects[i].name, pBufferName))
    {
      *ppBuffer = &pShader->bufferObjects[i];
      return true;
    }
  }

  return false;
}

bool vcShader_BindConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer, const void *pData, const size_t bufferSize)
{
  if (pShader == nullptr || pBuffer == nullptr || pData == nullptr || bufferSize == 0)
    return false;

  pBuffer->buffers[0].pCB = pData;
  pBuffer->buffers[1].pCB = pData;

  return true;
}

bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer)
{
  if (pShader == nullptr || pBuffer == nullptr)
    return false;

  pBuffer->expectedSize = 0;

  return true;
}

bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName)
{
  udUnused(ppSampler);
  udUnused(pShader);
  udUnused(pSamplerName);

  return true;
}
