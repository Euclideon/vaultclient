#import "vcMetal.h"

#import "gl/vcShader.h"
#import "gl/vcTexture.h"
#import "vcSettings.h"
#import "udFile.h"
#import "udPlatformUtil.h"
#import "udStringUtil.h"

#import "vcRenderer.h"

#include "stb_image.h"

uint32_t g_textureIndex = 0;
uint32_t g_blitTargetID = 0;

void vcTexture_GetFormatAndPixelSize(const vcTextureFormat format, int *pPixelSize = nullptr, MTLPixelFormat *pPixelFormat = nullptr, MTLStorageMode *pStorageMode = nullptr, MTLTextureUsage *pUsage = nullptr)
{
  // defaults
  MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm;
  MTLStorageMode storageMode = MTLStorageModeManaged; 
  MTLTextureUsage usage = MTLTextureUsageShaderRead;
  int pixelSize = 0; // in bytes

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  pTextureDesc.storageMode = MTLStorageModeShared; // default on iOS and tvOS
#endif

  switch (format)
  {
  case vcTextureFormat_RGBA8:
    pixelFormat = MTLPixelFormatRGBA8Unorm;
    usage = MTLTextureUsagePixelFormatView | MTLTextureUsageShaderRead;
    pixelSize = 4;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    storageMode = MTLStorageModeManaged;
#endif
    break;
  case vcTextureFormat_BGRA8:
    pixelFormat = MTLPixelFormatBGRA8Unorm;
    usage = MTLTextureUsageShaderRead;
    pixelSize = 4;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    storageMode = MTLStorageModeManaged;
#endif
    break;
  case vcTextureFormat_D32F:
    pixelFormat = MTLPixelFormatDepth32Float;
    usage = MTLTextureUsageShaderRead;
    storageMode = MTLStorageModePrivate;
    pixelSize = 4;
    break;
  case vcTextureFormat_D24S8:
    usage = MTLTextureUsageShaderRead;
    storageMode = MTLStorageModePrivate;
    pixelSize = 4;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
#endif
    break;

  case vcTextureFormat_Unknown: // fall through
  case vcTextureFormat_Count:
    break;
  }

  if (pPixelFormat != nullptr)
    *pPixelFormat = pixelFormat;

  if (pStorageMode != nullptr)
    *pStorageMode = storageMode;

  if (pUsage != nullptr)
    *pUsage = usage;

  if (pPixelSize != nullptr)
    *pPixelSize = pixelSize;
}

udResult vcTexture_Create(struct vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags, int32_t aniFilter /* = 0 */)
{
  if (ppTexture == nullptr || width == 0 || height == 0 || format == vcTextureFormat_Unknown || format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udUnused(hasMipmaps);

  udResult result = udR_Success;
  MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm;
  MTLStorageMode storageMode = MTLStorageModeManaged; 
  MTLTextureUsage usage = MTLTextureUsageShaderRead;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(format, &pixelBytes, &pixelFormat, &storageMode, &usage);

  vcTexture *pText = udAllocType(vcTexture, 1, udAF_Zero);

  @autoreleasepool
  {
    MTLTextureDescriptor *pTextureDesc = [[MTLTextureDescriptor alloc] init];

    pTextureDesc.pixelFormat = pixelFormat;
    pTextureDesc.storageMode = storageMode;
    pTextureDesc.usage = usage;
    pTextureDesc.width = width;
    pTextureDesc.height = height;
    pTextureDesc.mipmapLevelCount = 1;

    if (flags & vcTCF_RenderTarget || flags & vcTCF_Dynamic)
    {
      pTextureDesc.usage |= MTLTextureUsageRenderTarget;
    }

    pText->width = (uint32_t)pTextureDesc.width;
    pText->height = (uint32_t)pTextureDesc.height;
    pText->format = format;
    pText->ID = g_textureIndex;
    pText->flags = flags;

    int count = (flags & vcTCF_RenderTarget) || (flags & vcTCF_Dynamic) ? DRAWABLES : 1;
    
    for (int i = 0; i < count; ++i)
    {
      id<MTLTexture> texture = [_device newTextureWithDescriptor:pTextureDesc];
      
      if ((pTextureDesc.pixelFormat == MTLPixelFormatRGBA8Unorm) && (pTextureDesc.usage & MTLTextureUsageRenderTarget))
      {
        texture = [texture newTextureViewWithPixelFormat:MTLPixelFormatBGRA8Unorm];
        pTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
        pText->format = vcTextureFormat_BGRA8;
      }
      MTLRegion region = MTLRegionMake2D(0, 0, width, height);
      NSUInteger row = pixelBytes * width;
      NSUInteger len = row * height;
      
      if (pPixels && (pTextureDesc.storageMode != MTLStorageModePrivate))
      {
        [texture replaceRegion:region mipmapLevel:0 withBytes:pPixels bytesPerRow:row];
  #if UDPLATFORM_OSX
        [_renderer.blitEncoder synchronizeTexture:texture slice:0 level:0];
  #endif
      }
      else if (pPixels)
      {
        id<MTLBuffer> temp = [_device newBufferWithBytes:pPixels length:len options:MTLStorageModeShared];
        [_renderer.blitEncoder copyFromBuffer:temp sourceOffset:0 sourceBytesPerRow:row sourceBytesPerImage:len sourceSize:MTLSizeMake(width, height, 1) toTexture:texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
      }
      
      [_renderer.textures setObject:texture forKey:[NSString stringWithFormat:@"%u",g_textureIndex++]];
    }
  }
  
  // Sampler
  NSString *key = [NSString stringWithFormat:@"%@%@", (filterMode == vcTFM_Nearest ? @"N" : @"L"), (wrapMode == vcTWM_Repeat ? @"R" : @"C")];
  id<MTLSamplerState> savedSampler = [_renderer.samplers valueForKey:key];

  if (savedSampler)
  {
    udStrcpy(pText->samplerID, key.UTF8String);
  }
  else
  {
    MTLSamplerDescriptor *pSamplerDesc = [MTLSamplerDescriptor new];

    if (filterMode == vcTFM_Nearest)
    {
      pSamplerDesc.mipFilter = MTLSamplerMipFilterNearest;
      pSamplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
      pSamplerDesc.magFilter = MTLSamplerMinMagFilterNearest;
    }
    else
    {
      pSamplerDesc.mipFilter = MTLSamplerMipFilterLinear;
      pSamplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
      pSamplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
    }

    if (wrapMode == vcTWM_Repeat)
    {
      pSamplerDesc.rAddressMode = MTLSamplerAddressModeRepeat;
      pSamplerDesc.sAddressMode = MTLSamplerAddressModeRepeat;
      pSamplerDesc.tAddressMode = MTLSamplerAddressModeRepeat;
    }
    else
    {
      pSamplerDesc.rAddressMode = MTLSamplerAddressModeClampToEdge;
      pSamplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
      pSamplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    }

    if (aniFilter > 0)
      pSamplerDesc.maxAnisotropy = aniFilter;
    else
      pSamplerDesc.maxAnisotropy = 1;

    id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:pSamplerDesc];

    udStrcpy(pText->samplerID, key.UTF8String);
    [_renderer.samplers setObject:sampler forKey:key];
  }

  vcGLState_ReportGPUWork(0, 0, pText->width * pText->height * pixelBytes);
  *ppTexture = pText;
  pText = nullptr;

  return result;
}

bool vcTexture_CreateFromMemory(struct vcTexture **ppTexture, void *pFileData, size_t fileLength, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFileData == nullptr || fileLength == 0)
    return false;

  bool result = false;

  uint32_t width, height, channelCount;
  uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLength, (int*)&width, (int*)&height, (int*)&channelCount, 4);

  if (pData)
    result = (vcTexture_Create(ppTexture, width, height, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags, aniFilter) == udR_Success);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  return result;
}

bool vcTexture_CreateFromFilename(struct vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFilename == nullptr)
    return false;

  void *pFileData;
  int64_t fileLen;

  if (udFile_Load(pFilename, &pFileData, &fileLen) != udR_Success)
    return false;

  bool result = vcTexture_CreateFromMemory(ppTexture, pFileData, fileLen, pWidth, pHeight, filterMode, hasMipmaps, wrapMode, flags, aniFilter);

  udFree(pFileData);
  return result;
}

udResult vcTexture_UploadPixels(struct vcTexture *pTexture, const void *pPixels, int width, int height)
{
  if (pTexture == nullptr || pPixels == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  if ((int)pTexture->width == width && (int)pTexture->height == height)
  {
    id<MTLBuffer> pData = [_device newBufferWithBytes:pPixels length:width * height * pixelBytes options:MTLStorageModeShared];

    [_renderer.blitEncoder copyFromBuffer:pData sourceOffset:0 sourceBytesPerRow:width * pixelBytes sourceBytesPerImage:width * height * 4 sourceSize:MTLSizeMake(width, height, 1) toTexture:_renderer.textures[[NSString stringWithFormat:@"%u",pTexture->ID]] destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
#if UDPLATFORM_OSX
    if (_renderer.textures[[NSString stringWithFormat:@"%u",pTexture->ID]].storageMode != MTLStorageModePrivate)
      [_renderer.blitEncoder synchronizeTexture:_renderer.textures[[NSString stringWithFormat:@"%u",pTexture->ID]] slice:0 level:0];
#endif
    result = udR_Success;
  }
  
  @autoreleasepool {
    [_renderer flushBlit];
  }
  
  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * pixelBytes);
  return result;
}

void vcTexture_Destroy(struct vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr || !_renderer.textures[[NSString stringWithFormat:@"%u",(*ppTexture)->ID]])
    return;

  @autoreleasepool
  {
    [_renderer.textures removeObjectForKey:[NSString stringWithFormat:@"%u",(*ppTexture)->ID]];
  }

  udFree(*ppTexture);
  *ppTexture = nullptr;
}

udResult vcTexture_GetSize(struct vcTexture *pTexture, int *pWidth, int *pHeight)
{
  if (pTexture == nullptr)
    return udR_InvalidParameter_;

  if (pWidth != nullptr)
    *pWidth = (int)pTexture->width;

  if (pHeight != nullptr)
    *pHeight = (int)pTexture->height;

  return udR_Success;
}

bool vcTexture_BeginReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels, vcFramebuffer *pFramebuffer)
{
  udUnused(pPixels);
  
  if (pFramebuffer == nullptr || pTexture == nullptr || (x + width) > pTexture->width || (y + height) > pTexture->height)
    return false;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count)
    return false;

  udResult result = udR_Success;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  @autoreleasepool
  {
    if (pTexture->blitTarget == 0)
    {
      [_renderer.blitBuffers addObject:[_device newBufferWithLength:pixelBytes * pTexture->width * pTexture->height options:MTLResourceStorageModeShared]];
      
      pTexture->blitTarget = ++g_blitTargetID;
      pFramebuffer->actions |= vcRFA_Blit;
    }
    else if (_renderer.blitBuffers[pTexture->blitTarget - 1].length < 4 * pTexture->width * pTexture->height)
    {
      [_renderer.blitBuffers replaceObjectAtIndex:pTexture->blitTarget - 1 withObject:[_device newBufferWithLength:4 * pTexture->width * pTexture->height options:MTLResourceStorageModeShared]];
    }
    
    [_renderer.blitEncoder copyFromTexture:_renderer.textures[[NSString stringWithFormat:@"%u",pTexture->ID]] sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(pTexture->width, pTexture->height, 1) toBuffer:_renderer.blitBuffers[pTexture->blitTarget - 1] destinationOffset:0 destinationBytesPerRow:4 * pTexture->width destinationBytesPerImage:4 * pTexture->width * pTexture->height];
  
    [_renderer flushBlit];
  }
  
  return result == udR_Success;
}

bool vcTexture_EndReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pTexture == nullptr || pPixels == nullptr || (x + width) > pTexture->width || (y + height) > pTexture->height)
    return false;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count || pTexture->blitTarget == 0)
    return false;

  udResult result = udR_Success;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  uint32_t *pSource = (uint32_t*)[_renderer.blitBuffers[pTexture->blitTarget - 1] contents];
  pSource += ((y * pTexture->width) + x);
  
  uint32_t rowBytes = pixelBytes * width;
  
  for (uint32_t i = 0; i < height; ++i)
  {
    for (uint32_t j = 0; j < width; ++j)
      *((uint32_t*)pPixels + (i * rowBytes)) = *(pSource + (i * pTexture->width) + j);
  }
  
  return result == udR_Success;
}
