#import "gl/vcMesh.h"
#import "vcMetal.h"
#import "vcRenderer.h"
#import "udPlatformUtil.h"
#import "udStringUtil.h"

uint32_t g_currIndex = 0;
uint32_t g_currVertex = 0;

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void *pVerts, uint32_t currentVerts, const void *pIndices, uint32_t currentIndices, vcMeshFlags flags/* = vcMF_None*/)
{
  bool invalidIndexSetup = (flags & vcMF_NoIndexBuffer) || ((pIndices == nullptr && currentIndices > 0) || currentIndices == 0);
  if (ppMesh == nullptr || pMeshLayout == nullptr || totalTypes == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  vcMesh *pMesh = nullptr;
  ptrdiff_t accumulatedOffset = 0;

  pMesh = udAllocType(vcMesh, 1, udAF_Zero);
  if (pMesh == nullptr)
    return udR_MemoryAllocationFailure;

  for (int i = 0; i < totalTypes; ++i)
  {
    switch (pMeshLayout[i])
    {
      case vcVLT_Position2:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_Position3:
        accumulatedOffset += 3 * sizeof(float);
        break;
      case vcVLT_Position4:
        accumulatedOffset += 4 * sizeof(float);
        break;
      case vcVLT_TextureCoords2:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_RibbonInfo4:
        accumulatedOffset += 4 * sizeof(float);
        break;
      case vcVLT_ColourBGRA:
        accumulatedOffset += sizeof(uint32_t);
        break;
      case vcVLT_Normal3:
        accumulatedOffset += 3 * sizeof(float);
        break;
      case vcVLT_QuadCorner:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_Unsupported: // TODO: (EVC-641) Handle unsupported attributes interleaved with supported attributes
        continue; // NOTE continue
      case vcVLT_TotalTypes:
        break;
    }
  }

  pMesh->indexCount = currentIndices;
  pMesh->vertexCount = currentVerts;
  pMesh->vertexBytes = accumulatedOffset;

  if (!invalidIndexSetup)
  {
    if ((flags & vcMF_IndexShort))
    {
      pMesh->indexType = MTLIndexTypeUInt16;
      pMesh->indexBytes = sizeof(uint16);
    }
    else
    {
      pMesh->indexType = MTLIndexTypeUInt32;
      pMesh->indexBytes = sizeof(uint32);
    }
    udStrcpy(pMesh->iBufferIndex, [NSString stringWithFormat:@"%d", g_currIndex].UTF8String);
    [_renderer.indexBuffers setObject:[_device newBufferWithBytes:pIndices length:currentIndices * pMesh->indexBytes options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->iBufferIndex]];
    ++g_currIndex;
  }

  udStrcpy(pMesh->vBufferIndex, [NSString stringWithFormat:@"%d", g_currVertex].UTF8String);
  ++g_currVertex;
  
  if (pVerts != nullptr)
  {
    [_renderer.vertBuffers setObject:[_device newBufferWithBytes:pVerts length:accumulatedOffset * currentVerts options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
  }
  else
  {
    if (currentVerts < 1)
      currentVerts = 20;
    
    [_renderer.vertBuffers setObject:[_device newBufferWithLength:accumulatedOffset * currentVerts options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
  }

  *ppMesh = pMesh;
   vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexBytes) + (pMesh->indexCount * pMesh->indexBytes));

  return result;
}

void vcMesh_Destroy(struct vcMesh **ppMesh)
{
  if (ppMesh == nullptr || *ppMesh == nullptr)
    return;
  
  @autoreleasepool
  {
    [_renderer.vertBuffers removeObjectForKey:[NSString stringWithUTF8String:(*ppMesh)->vBufferIndex]];
    if ((*ppMesh)->indexCount)
      [_renderer.indexBuffers removeObjectForKey:[NSString stringWithUTF8String:(*ppMesh)->iBufferIndex]];
  }
  
  udFree(*ppMesh);
  *ppMesh = nullptr;
}

udResult vcMesh_UploadData(struct vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  ptrdiff_t accumulatedOffset = 0;
  for (int i = 0; i < totalTypes; ++i)
  {
    switch (pLayout[i])
    {
      case vcVLT_Position2:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_Position3:
        accumulatedOffset += 3 * sizeof(float);
        break;
      case vcVLT_Position4:
        accumulatedOffset += 4 * sizeof(float);
        break;
      case vcVLT_TextureCoords2:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_RibbonInfo4:
        accumulatedOffset += 4 * sizeof(float);
        break;
      case vcVLT_ColourBGRA:
        accumulatedOffset += sizeof(uint32_t);
        break;
      case vcVLT_Normal3:
        accumulatedOffset += 3 * sizeof(float);
        break;
      case vcVLT_QuadCorner:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_Unsupported: // TODO: (EVC-641) Handle unsupported attributes interleaved with supported attributes
        continue; // NOTE continue
      case vcVLT_TotalTypes:
        break;
    }
  }

  uint32_t size = accumulatedOffset * totalVerts;

  @autoreleasepool
  {
    // if (pMesh->vertexCount * pMesh->vertexBytes < size)
    [_renderer.vertBuffers removeObjectForKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
    [_renderer.vertBuffers setObject:[_device newBufferWithBytes:pVerts length:size options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
    // else
    //memcpy([_renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]] contents], pVerts, size);
    
    pMesh->vertexCount = totalVerts;
    pMesh->vertexBytes = accumulatedOffset;
    
    if (totalIndices > 0)
    {
      uint32_t isize = totalIndices * pMesh->indexBytes;
      
      //if (pMesh->indexCount < (uint32_t)totalIndices)
      [_renderer.indexBuffers removeObjectForKey:[NSString stringWithUTF8String:pMesh->iBufferIndex]];
      [_renderer.indexBuffers setObject:[_device newBufferWithBytes:pIndices length:isize options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->iBufferIndex]];
      //else
      //memcpy([_renderer.indexBuffers[[NSString stringWithUTF8String:pMesh->iBufferIndex]] contents], pIndices, isize);
    }
  }
  
  
  pMesh->indexCount = totalIndices;
  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexBytes) + (pMesh->indexCount * pMesh->indexBytes));

  return result;
}

udResult vcMesh_UploadSubData(vcMesh *pMesh, const vcVertexLayoutTypes *pLayout, int totalTypes, int startVertex, const void* pVerts, int totalVerts, const void *pIndices, int totalIndices)
{
  if (pMesh == nullptr || pLayout == nullptr || totalTypes == 0 || pVerts == nullptr || totalVerts == 0)
    return udR_InvalidParameter_;

  id<MTLBuffer> vBuffer = _renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]];
  
  uint32_t totalSize = (startVertex + totalVerts) * pMesh->vertexBytes;
  
  if (vBuffer.length < totalSize)
  {
    @autoreleasepool
    {
      vBuffer = [_device newBufferWithLength:totalSize options:MTLStorageModeShared];
      [_renderer.vertBuffers removeObjectForKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
      [_renderer.vertBuffers setObject:vBuffer forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
    }
  }
  
  memcpy((int8_t*)[vBuffer contents] + (startVertex * pMesh->vertexBytes), pVerts, totalVerts * pMesh->vertexBytes);
  
  udUnused(pIndices);
  udUnused(totalIndices);
  /* No indices in use presently
   if (totalIndices > 0)
   {
   id<MTLBuffer> iBuffer = _renderer.indexBuffers[[NSString stringWithUTF8String:pMesh->iBufferIndex]];
   memcpy([iBuffer contents], pIndices, totalIndices * pMesh->indexBytes);
   pMesh->indexCount = totalIndices;
   }*/

  vcGLState_ReportGPUWork(0, 0, (pMesh->vertexCount * pMesh->vertexBytes) + (pMesh->indexCount * pMesh->indexBytes));

  return udR_Success;
}


bool vcMesh_Render(struct vcMesh *pMesh, uint32_t elementCount /* = 0*/, uint32_t startElement /* = 0*/, vcMeshRenderMode renderMode /*= vcMRM_Triangles*/)
{
  if (pMesh == nullptr || (pMesh->indexBytes > 0 && pMesh->indexCount < (elementCount + startElement) * 3) || (elementCount == 0 && startElement != 0))
    return false;

  MTLPrimitiveType primitiveType = MTLPrimitiveTypeTriangle;
  int elementsPerPrimitive;

  switch (renderMode)
  {
  case vcMRM_TriangleStrip:
    primitiveType = MTLPrimitiveTypeTriangleStrip;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Points:
    primitiveType = MTLPrimitiveTypePoint;
    elementsPerPrimitive = 1;
    break;
  case vcMRM_Triangles:
    primitiveType = MTLPrimitiveTypeTriangle;
    elementsPerPrimitive = 3;
    break;
  }

  @autoreleasepool
  {
    if (pMesh->indexCount > 0)
    {
      if (elementCount == 0)
        elementCount = pMesh->indexCount;
      else
        elementCount *= elementsPerPrimitive;

      [_renderer drawIndexed:_renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]] indexedBuffer:_renderer.indexBuffers[[NSString stringWithUTF8String:pMesh->iBufferIndex]] indexCount:elementCount offset:startElement * elementsPerPrimitive * pMesh->indexBytes indexSize:pMesh->indexType primitiveType:primitiveType];
    }
    else
    {
      if (elementCount == 0)
        elementCount = pMesh->vertexCount;

      [_renderer drawUnindexed:_renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]] vertexStart:startElement * pMesh->vertexBytes vertexCount:elementCount primitiveType:primitiveType];
    }
  }
  vcGLState_ReportGPUWork(1, elementCount * elementsPerPrimitive, 0);
  return true;
}
