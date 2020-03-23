#import "vcMetal.h"
#import "gl/vcShader.h"
#import <Metal/Metal.h>

#import "udPlatformUtil.h"
#import "udStringUtil.h"
#import "udFile.h"

uint32_t g_pipeCount = 0;
uint16_t g_geomPipeCount = 0;

// Takes shader function names instead of shader description string
bool vcShader_CreateFromText(vcShader **ppShader, const char *pVertexShader, const char *pFragmentShader, const vcVertexLayoutTypes *pVertLayout, uint32_t totalTypes)
{
  if (ppShader == nullptr || pVertexShader == nullptr || pFragmentShader == nullptr)
    return false;

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

  if (udStrBeginsWithi(pFragmentShader, "blur") || udStrBeginsWithi(pFragmentShader, "flatCol"))
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
    
  if (udStrBeginsWithi(pFragmentShader, "udSplat") || udStrBeginsWithi(pFragmentShader, "blur") || udStrBeginsWithi(pFragmentShader, "flat"))
    pShader->flush = vcRFO_Flush;
  else if (udStrBeginsWithi(pFragmentShader, "udFrag"))
    pShader->flush = vcRFO_Blit;
  else
    pShader->flush = vcRFO_None;

  pShader->inititalised = false;
  
  [_renderer buildBlendPipelines:pDesc];
  pShader->ID = g_pipeCount;
  ++g_pipeCount;


  *ppShader = pShader;
  pShader = nullptr;

  return (*ppShader != nullptr);
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

  bool success = vcShader_CreateFromText(ppShader, pVertexShaderText, pFragmentShaderText, pInputTypes, totalInputs);

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

bool vcShader_BindTexture(vcShader *pShader, vcTexture *pTexture, uint16_t samplerIndex, vcShaderSampler *pSampler/* = nullptr*/)
{
  udUnused(pShader);
  udUnused(pSampler);

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
  
  vcShaderConstantBuffer *pTemp = udAllocType(vcShaderConstantBuffer, 1, udAF_Zero);
  pTemp->expectedSize = bufferSize;
  udStrcpy(pTemp->name, pBufferName);

  pShader->bufferObjects[pShader->numBufferObjects] = *pTemp;
  ++pShader->numBufferObjects;
  
  *ppBuffer = pTemp;
  pTemp = nullptr;

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
    found = pShader->numBufferObjects++;
  
  pBuffer->pCB = pData;
  
  pShader->bufferObjects[found] = *pBuffer;
  return true;
}

bool vcShader_ReleaseConstantBuffer(vcShader *pShader, vcShaderConstantBuffer *pBuffer)
{
  if (pShader == nullptr || pBuffer == nullptr)
    return false;

  pBuffer->expectedSize = 0;

  udFree(pBuffer);

  return true;
}

bool vcShader_GetSamplerIndex(vcShaderSampler **ppSampler, vcShader *pShader, const char *pSamplerName)
{
  udUnused(ppSampler);
  udUnused(pShader);
  udUnused(pSamplerName);

  return true;
}
