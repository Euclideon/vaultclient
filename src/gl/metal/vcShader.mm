#import "vcMetal.h"
#import "gl/vcShader.h"
#import <Metal/Metal.h>

#import "udPlatformUtil.h"
#import "udStringUtil.h"

uint32_t g_pipeCount = 0;
uint32_t g_bufferIndex = 0;

// Takes shader function names instead of shader description string
bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pVertLayout, uint32_t totalTypes, const char *pGeometryShader /*= nullptr*/)
{
  if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr)
    return false;

  udUnused(pGeometryShader);

  vcShader *pShader = udAllocType(vcShader, 1, udAF_Zero);

  MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];

  ptrdiff_t accumulatedOffset = 0;
  for (uint32_t i = 0; i < totalTypes; ++i)
  {
    vertexDesc.attributes[i].bufferIndex = 0;
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
        vertexDesc.attributes[i].format = MTLVertexFormatUInt;
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
  vertexDesc.layouts[0].stride = accumulatedOffset;
  vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
  vertexDesc.layouts[0].stepRate = 1;

  id<MTLFunction> vFunc = [_library newFunctionWithName:[NSString stringWithUTF8String:pVertexShader]];
  id<MTLFunction> fFunc = [_library newFunctionWithName:[NSString stringWithUTF8String:pFragmentShader]];

  MTLRenderPipelineDescriptor *pDesc = [[MTLRenderPipelineDescriptor alloc] init];
  pDesc.vertexDescriptor = vertexDesc;
  pDesc.vertexFunction = vFunc;
  pDesc.fragmentFunction = fFunc;

  pDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  pDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
  pDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
  pDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
  pDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
#else
# error "Unknown platform!"
#endif

  [_viewCon.renderer buildBlendPipelines:pDesc];
  pShader->ID = (uint32_t)g_pipeCount;
  ++g_pipeCount;

  *ppShader = pShader;
  pShader = nullptr;

  return (*ppShader != nullptr);
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
    [_viewCon.renderer bindPipeline:pShader];

  return true;
}

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler/* = nullptr*/)
{
  udUnused(pShader);
  if (pTexture == nullptr)
    return false;

  [_viewCon.renderer bindTexture:pTexture index:samplerIndex];

  if (pSampler)
  {
    [_viewCon.renderer bindSampler:pSampler index:samplerIndex];
  }
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

  NSString *bID = [NSString stringWithFormat:@"%u",g_bufferIndex];
  [_viewCon.renderer.constantBuffers setObject:[_device newBufferWithLength:bufferSize options:MTLStorageModeShared] forKey:bID];
  ++g_bufferIndex;

  vcShaderConstantBuffer *temp = udAllocType(vcShaderConstantBuffer, 1, udAF_Zero);
  temp->expectedSize = bufferSize;
  udStrcpy(temp->name, 32, pBufferName);
  udStrcpy(temp->ID, 32, bID.UTF8String);

  pShader->bufferObjects[pShader->numBufferObjects] = *temp;
  ++pShader->numBufferObjects;

  *ppBuffer = temp;
  temp = nullptr;

  return true;
}

bool vcShader_BindConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer, const void *pData, const size_t bufferSize)
{
  if (pShader == nullptr || pBuffer == nullptr || pData == nullptr || bufferSize == 0)
    return false;

  int found = -1;
  for (int i = 0; i < pShader->numBufferObjects; ++i)
    if (udStrEquali(pShader->bufferObjects[i].name, pBuffer->name))
      found = i;

  if (found < 0)
  {
    pShader->bufferObjects[pShader->numBufferObjects] = *pBuffer;
    ++pShader->numBufferObjects;
  }

  if (pBuffer->expectedSize == bufferSize)
  {
    //memcpy(_viewCon.renderer.constantBuffers[[NSString stringWithUTF8String:pBuffer->ID]].contents, pData, bufferSize);

    [_viewCon.renderer.constantBuffers setObject:[_device newBufferWithBytes:pData length:bufferSize options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pBuffer->ID]];
  }
  else
  {
    [_viewCon.renderer.constantBuffers setObject:[_device newBufferWithBytes:pData length:bufferSize options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pBuffer->ID]];
  }

  return true;
}

bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer)
{
  if (pShader == nullptr || pBuffer == nullptr)
    return false;

  [_viewCon.renderer.constantBuffers removeObjectForKey:[NSString stringWithUTF8String:pBuffer->ID]];
  pBuffer->expectedSize = 0;

  udFree(pBuffer);

  return true;
}

bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName)
{
  if (pShader == nullptr)
    return false;

  for (int i = 0; i < pShader->numBufferObjects; ++i)
  {
    if (udStrEquali(pShader->samplerIndexes[i].name, pSamplerName))
    {
      *ppSampler = &pShader->samplerIndexes[i];
      return true;
    }
  }

  return false;
}
