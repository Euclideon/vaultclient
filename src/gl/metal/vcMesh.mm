#import "gl/vcMesh.h"
#import "vcMetal.h"
#import "ViewCon.h"
#import "Renderer.h"
#import "udPlatformUtil.h"

int currIndex = 0;

udResult vcMesh_Create(vcMesh **ppMesh, const vcVertexLayoutTypes *pMeshLayout, int totalTypes, const void *pVerts, int currentVerts, const void *pIndices, int currentIndices, vcMeshFlags flags/* = vcMF_None*/)
{
  bool invalidIndexSetup = (flags & vcMF_NoIndexBuffer) && ((pIndices == nullptr && currentIndices > 0) || currentIndices == 0);
  if (ppMesh == nullptr || pMeshLayout == nullptr || totalTypes == 0 || pVerts == nullptr || currentVerts == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  vcMesh *pMesh = udAllocType(vcMesh, 1, udAF_Zero);

  if (!(flags & vcMF_NoIndexBuffer) || !invalidIndexSetup)
  {
    if (!(flags & vcMF_IndexShort))
    {
      pMesh->indexType = MTLIndexTypeUInt32;
      pMesh->indexBytes = sizeof(int);
    }
    else
    {
      pMesh->indexType = MTLIndexTypeUInt16;
      pMesh->indexBytes = sizeof(short);
    }
    pMesh->iBufferIndex = (uint32_t)_viewCon.renderer.indexBuffers.count;
    [_viewCon.renderer.indexBuffers addObject:[_device newBufferWithBytes:pIndices length:currentIndices * pMesh->indexBytes options:MTLStorageModeShared]];
  }

  ptrdiff_t accumulatedOffset = 0;
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
    case vcVLT_TextureCoords2:
      accumulatedOffset += 2 * sizeof(float);
      break;
    case vcVLT_RibbonInfo4: // !!!
      accumulatedOffset += 4 * sizeof(float);
      break;
    case vcVLT_ColourBGRA: // !!!
      accumulatedOffset += sizeof(uint32_t);
      break;
    case vcVLT_Normal3:
      accumulatedOffset += 3 * sizeof(float);
      break;
    case vcVLT_TotalTypes:
      break; // never reaches here due to error set above
    }
  }
  
  NSUInteger size = currentVerts * accumulatedOffset;
  
  udStrItoa(pMesh->vBufferIndex, 32, currIndex);
  ++currIndex;
  [_viewCon.renderer.vertBuffers setObject:[_device newBufferWithBytes:pVerts length:size options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
  
  pMesh->indexCount = currentIndices;
  pMesh->vertexCount = currentVerts;
  pMesh->vertexBytes = (uint32_t)size;

  *ppMesh = pMesh;

  return result;
}

void vcMesh_Destroy(struct vcMesh **ppMesh)
{
  if (ppMesh == nullptr || *ppMesh == nullptr)
    return;
  
  [_viewCon.renderer.vertBuffers removeObjectForKey:[NSString stringWithUTF8String:(*ppMesh)->vBufferIndex]];
  //if ((*ppMesh)->indexCount)
  
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
      case vcVLT_TextureCoords2:
        accumulatedOffset += 2 * sizeof(float);
        break;
      case vcVLT_RibbonInfo4: // !!!
        accumulatedOffset += 4 * sizeof(float);
        break;
      case vcVLT_ColourBGRA: // !!!
        accumulatedOffset += sizeof(uint32_t);
        break;
      case vcVLT_Normal3:
        accumulatedOffset += 3 * sizeof(float);
        break;
      case vcVLT_TotalTypes:
        break; // never reaches here due to error set above
    }
  }
  pMesh->vertexCount = totalVerts;
  
  NSUInteger size = totalVerts * accumulatedOffset;
  if (pMesh->vertexBytes != size)
  {
    [_viewCon.renderer.vertBuffers setObject:[_device newBufferWithBytes:pVerts length:size options:MTLStorageModeShared] forKey:[NSString stringWithUTF8String:pMesh->vBufferIndex]];
    pMesh->vertexBytes = (uint32_t)size;
  }
  else
  {
    memcpy(_viewCon.renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]].contents, pVerts, size);
    //[_viewCon.renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]] didModifyRange:NSMakeRange(0, size)];
  }
  
  if (totalIndices > 0)
  {
    if (pMesh->indexCount != (uint32_t)totalIndices)
    {
      [_viewCon.renderer.indexBuffers replaceObjectAtIndex:pMesh->iBufferIndex withObject:[_device newBufferWithBytes:pIndices length:totalIndices * pMesh->indexBytes options:MTLStorageModeShared]];
    }
    else
    {
      memcpy(_viewCon.renderer.indexBuffers[pMesh->iBufferIndex].contents, pIndices, pMesh->indexCount * pMesh->indexBytes);
      //[_viewCon.renderer.indexBuffers[pMesh->iBufferIndex] didModifyRange:NSMakeRange(0, pMesh->indexCount * pMesh->indexBytes)];
    }
  }
  pMesh->indexCount = totalIndices;
  
  return result;
}

bool vcMesh_Render(struct vcMesh *pMesh, uint32_t elementCount /* = 0*/, uint32_t startElement /* = 0*/, vcMeshRenderMode renderMode /*= vcMRM_Triangles*/)
{
  udUnused(elementCount);
  
  if (pMesh == nullptr)
    return false;

  MTLPrimitiveType primitiveType = MTLPrimitiveTypeTriangle;
  
  switch (renderMode)
  {
  case vcMRM_TriangleStrip:
    primitiveType = MTLPrimitiveTypeTriangleStrip;
    break;
  case vcMRM_Triangles: // fall through
  default:
    primitiveType = MTLPrimitiveTypeTriangle;
    break;
  }
  
  if (pMesh->indexCount)
    [_viewCon.renderer drawIndexedTriangles:_viewCon.renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]]
                     indexedBuffer:_viewCon.renderer.indexBuffers[pMesh->iBufferIndex]
                        indexCount:pMesh->indexCount
                       indexSize:pMesh->indexType
                     primitiveType:primitiveType];
  else
    [_viewCon.renderer drawUnindexed:_viewCon.renderer.vertBuffers[[NSString stringWithUTF8String:pMesh->vBufferIndex]]
                       vertexStart:startElement
                       vertexCount:pMesh->vertexCount
                     primitiveType:primitiveType];
  return true;
}
