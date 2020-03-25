#import "vcMetal.h"
#import "gl/vcShader.h"
#import <Metal/Metal.h>

#import "udPlatformUtil.h"
#import "udStringUtil.h"
#import "udFile.h"

uint32_t g_pipeCount = 0;
uint16_t g_geomPipeCount = 0;

// Takes shader function names instead of shader description string
bool vcShader_CreateFromTextInternal(vcShader **ppShader, const char *pVertexShaderFilename, const char *pVertexShader, const char *pFragmentShaderFilename, const char *pFragmentShader, const vcVertexLayoutTypes *pVertLayout, uint32_t totalTypes)
{
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
      case vcVLT_RibbonInfo4:
        vertexDesc.attributes[i].format = MTLVertexFormatFloat4;
        accumulatedOffset += 4 * sizeof(float);
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
    pShader->vertexLibrary = [_device newLibraryWithSource:[NSString stringWithUTF8String:pVertexShader] options: {} error:nil];
    pShader->fragmentLibrary = [_device newLibraryWithSource:[NSString stringWithUTF8String:pFragmentShader] options: {} error:nil];
    
    vFunc = [pShader->vertexLibrary newFunctionWithName:@"main0"];
    fFunc = [pShader->fragmentLibrary newFunctionWithName:@"main0"];
  }

  MTLRenderPipelineDescriptor *pDesc = [[MTLRenderPipelineDescriptor alloc] init];
  pDesc.vertexDescriptor = vertexDesc;
  pDesc.vertexFunction = vFunc;
  pDesc.fragmentFunction = fFunc;

  pDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

  if (udStrBeginsWithi(pFragmentShaderFilename, "blur") || udStrBeginsWithi(pFragmentShaderFilename, "flatCol"))
  {
    pDesc.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
    pDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
  }
  else
  {
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    pDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    pDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
#else
# error "Unknown platform!"
#endif
  }
    
  if (udStrBeginsWithi(pFragmentShaderFilename, "udSplat") || udStrBeginsWithi(pFragmentShaderFilename, "blur") || udStrBeginsWithi(pFragmentShaderFilename, "flat"))
    pShader->flush = vcRFO_Flush;
  else if (udStrBeginsWithi(pFragmentShaderFilename, "udFrag"))
    pShader->flush = vcRFO_Blit;
  else
    pShader->flush = vcRFO_None;

  pShader->inititalised = false;
  
  NSError *err = nil;
  MTLRenderPipelineReflection *reflectionObj;
  MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
  [_renderer.pipelines addObject:[_device newRenderPipelineStateWithDescriptor:pDesc options:option reflection:&reflectionObj error:&err]];
#ifdef METAL_DEBUG
  if (err != nil)
    NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif
  pDesc.colorAttachments[0].blendingEnabled = YES;
  pDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

  for (int i = vcGLSBM_Interpolative; i < vcGLSBM_Count; ++i)
  {
    switch ((vcGLStateBlendMode)i)
    {
      case vcGLSBM_None:
        break;
      case vcGLSBM_Interpolative:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
        break;
      case vcGLSBM_Additive:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        break;
      case vcGLSBM_Multiplicative:
        pDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorDestinationColor;
        pDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorZero;
        pDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        break;
      case vcGLSBM_Count:
        break;
    }

    [_renderer.pipelines addObject:[_device newRenderPipelineStateWithDescriptor:pDesc error:&err]];
#ifdef METAL_DEBUG
    if (err != nil)
      NSLog(@"Error: failed to create Metal pipeline state: %@", err);
#endif
  }

  for (MTLArgument *arg in reflectionObj.vertexArguments)
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

  for (MTLArgument *arg in reflectionObj.fragmentArguments)
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

  pShader->ID = g_pipeCount;
  ++g_pipeCount;


  *ppShader = pShader;
  pShader = nullptr;

  return (*ppShader != nullptr);
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

  bool success = vcShader_CreateFromTextInternal(ppShader, pVertexShader, pVertexShaderText, pFragmentShader, pFragmentShaderText, pInputTypes, totalInputs);

  udFree(pFragmentShaderText);
  udFree(pVertexShaderText);

  return success;
}

void vcShader_DestroyShader(vcShader **ppShader)
{
  if (ppShader == nullptr || *ppShader == nullptr)
    return;

  udFree(*ppShader);
}

bool vcShader_Bind(vcShader *pShader)
{
  if (pShader != nullptr)
  {
    if (pShader->inititalised)
      [_renderer bindPipeline:pShader];
    else
      pShader->inititalised = true;
  }
  return true;
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler/* = nullptr*/, vcGLSamplerShaderStage samplerStage /*= vcGLSamplerShaderStage_Fragment*/)
{
  udUnused(pShader);
  udUnused(pSampler);
  udUnused(samplerStage);

  if (pTexture == nullptr)
    return false;
  
  [_renderer bindTexture:pTexture index:samplerIndex];

  return true;
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
