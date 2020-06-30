#import "vcMetal.h"

#import "vcShader.h"
#import "vcTexture.h"
#import "udFile.h"
#import "udPlatformUtil.h"
#import "udStringUtil.h"

#include "stb_image.h"

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
  case vcTextureFormat_RGBA16F:
    pixelFormat = MTLPixelFormatRGBA16Float;
    usage = MTLTextureUsageShaderRead;
    pixelSize = 8;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    storageMode = MTLStorageModeManaged;
#endif
    break;
  case vcTextureFormat_RGBA32F:
    pixelFormat = MTLPixelFormatRGBA32Float;
    usage = MTLTextureUsageShaderRead;
    pixelSize = 16;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    storageMode = MTLStorageModeManaged;
#endif
    break;
  case vcTextureFormat_RG8:  
    pixelFormat = MTLPixelFormatRG8Unorm;
    usage = MTLTextureUsageShaderRead;
    pixelSize = 2;
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

udResult vcTexture_GetFormat(vcTexture *pTexture, vcTextureFormat *pFormat)
{
  if (pTexture == nullptr || pFormat == nullptr)
    return udR_InvalidParameter_;

  *pFormat = pTexture->format;

  return udR_Success;
}

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, vcTextureCreationFlags flags /*= vcTCF_None*/)
{
  return vcTexture_CreateAdv(ppTexture, vcTextureType_Texture2D, width, height, 1, pPixels, format, filterMode, false, vcTWM_Repeat, flags);
}

udResult vcTexture_CreateAdv(vcTexture **ppTexture, vcTextureType type, uint32_t width, uint32_t height, uint32_t depth, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
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

    if (flags & vcTCF_RenderTarget)
    {
      pTextureDesc.usage |= MTLTextureUsageRenderTarget;
    }

    switch (type)
    {
    case vcTextureType_Texture2D:
      pTextureDesc.textureType = MTLTextureType2D;
      break;
    case vcTextureType_Texture3D:
      pTextureDesc.textureType = MTLTextureType3D;
      pTextureDesc.depth = depth;
      break;
    case vcTextureType_TextureArray:
      pTextureDesc.textureType = MTLTextureType2DArray;
      pTextureDesc.arrayLength = depth;
      break;
    }

    pText->width = width;
    pText->height = height;
    pText->depth = depth;
    pText->format = format;
    pText->flags = flags;
    
    pText->texture = [g_device newTextureWithDescriptor:pTextureDesc];
    
    if ((pTextureDesc.pixelFormat == MTLPixelFormatRGBA8Unorm) && (pTextureDesc.usage & MTLTextureUsageRenderTarget))
    {
      pText->texture = [pText->texture newTextureViewWithPixelFormat:MTLPixelFormatBGRA8Unorm];
      pTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }
    MTLRegion region = MTLRegionMake3D(0, 0, 0, width, height, pTextureDesc.depth);
    NSUInteger row = pixelBytes * width;
    NSUInteger len = row * height;
    
    if (pPixels && (pTextureDesc.storageMode != MTLStorageModePrivate))
    {
      [pText->texture replaceRegion:region mipmapLevel:0 withBytes:pPixels bytesPerRow:row];
  #if UDPLATFORM_OSX
      [g_blitEncoder synchronizeTexture:pText->texture slice:0 level:0];
  #endif
    }
    else if (pPixels)
    {
      id<MTLBuffer> temp = [g_device newBufferWithBytes:pPixels length:len * pTextureDesc.depth options:MTLStorageModeShared];
      [g_blitEncoder copyFromBuffer:temp sourceOffset:0 sourceBytesPerRow:row sourceBytesPerImage:len sourceSize:MTLSizeMake(width, height, pTextureDesc.depth) toTexture:pText->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
  }
  
  // Sampler
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

  pText->sampler = [g_device newSamplerStateWithDescriptor:pSamplerDesc];

  if ((flags & vcTCF_AsynchronousRead) == vcTCF_AsynchronousRead)
    pText->blitBuffer = [g_device newBufferWithLength:pixelBytes * pText->width * pText->height options:MTLResourceStorageModeShared];

  vcGLState_ReportGPUWork(0, 0, pText->width * pText->height * pText->depth * pixelBytes);
  *ppTexture = pText;
  pText = nullptr;

  return result;
}

bool vcTexture_CreateFromMemory(struct vcTexture **ppTexture, void *pFileData, size_t fileLength, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFileData == nullptr || fileLength == 0)
    return false;

  bool result = false;

  uint32_t width, height, channelCount;
  uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLength, (int*)&width, (int*)&height, (int*)&channelCount, 4);

  if (pData)
    result = (vcTexture_CreateAdv(ppTexture, vcTextureType_Texture2D, width, height, 1, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags, aniFilter) == udR_Success);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  return result;
}

bool vcTexture_CreateFromFilename(struct vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
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

udResult vcTexture_UploadPixels(struct vcTexture *pTexture, const void *pPixels, int width, int height, int depth /*= 1*/)
{
  if (pTexture == nullptr || pPixels == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udResult result = udR_Success;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  id<MTLBuffer> pData = [g_device newBufferWithBytes:pPixels length:width * height * depth * pixelBytes options:MTLStorageModeShared];
  [g_blitEncoder copyFromBuffer:pData sourceOffset:0 sourceBytesPerRow:width * pixelBytes sourceBytesPerImage:width * height * pixelBytes sourceSize:MTLSizeMake(width, height, depth) toTexture:pTexture->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
#if UDPLATFORM_OSX
  if (pTexture->texture.storageMode != MTLStorageModePrivate)
    [g_blitEncoder synchronizeTexture:pTexture->texture slice:0 level:0];
#endif
  
  vcGLState_FlushBlit();
  
  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * pTexture->depth * pixelBytes);
  return result;
}

void vcTexture_Destroy(struct vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr || !(*ppTexture)->texture)
    return;

  @autoreleasepool
  {
    (*ppTexture)->texture = nil;
    (*ppTexture)->sampler = nil;
    (*ppTexture)->blitBuffer = nil;
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
    if (g_pCurrFramebuffer == pFramebuffer)
    {
      [g_pCurrFramebuffer->encoder endEncoding];
      [g_pCurrFramebuffer->commandBuffer commit];
      [g_pCurrFramebuffer->commandBuffer waitUntilCompleted];
      g_pCurrFramebuffer->commandBuffer = nil;
      g_pCurrFramebuffer->encoder = nil;
      g_pCurrFramebuffer->actions = 0;
    }
  
    if (pTexture->blitBuffer == nil)
      pTexture->blitBuffer = [g_device newBufferWithLength:pixelBytes * pTexture->width * pTexture->height options:MTLResourceStorageModeShared];
    else if (pTexture->blitBuffer.length < pixelBytes * pTexture->width * pTexture->height)
      pTexture->blitBuffer = [g_device newBufferWithLength:pixelBytes * pTexture->width * pTexture->height options:MTLResourceStorageModeShared];

    [g_blitEncoder copyFromTexture:pTexture->texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(pTexture->width, pTexture->height, 1) toBuffer:pTexture->blitBuffer destinationOffset:0 destinationBytesPerRow:pixelBytes * pTexture->width destinationBytesPerImage:pixelBytes * pTexture->width * pTexture->height];

    vcGLState_FlushBlit();
  }

  if ((pTexture->flags & vcTCF_AsynchronousRead) != vcTCF_AsynchronousRead)
    return vcTexture_EndReadPixels(pTexture, x, y, width, height, pPixels);
  
  return result == udR_Success;
}

bool vcTexture_EndReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels)
{
  if (pTexture == nullptr || pPixels == nullptr || (x + width) > pTexture->width || (y + height) > pTexture->height)
    return false;

  if (pTexture->format == vcTextureFormat_Unknown || pTexture->format == vcTextureFormat_Count || pTexture->blitBuffer == nil)
    return false;

  udResult result = udR_Success;
  int pixelBytes = 0;
  uint8_t *pPixelData = nullptr;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  if (x == 0 && y == 0 && width == (uint32_t)pTexture->width && height == (uint32_t)pTexture->height)
  {
    pPixelData = (uint8_t*)[pTexture->blitBuffer contents];
    memcpy((uint8_t*)pPixels, pPixelData, height * width * pixelBytes);
  }
  else
  {
    pPixelData = ((uint8_t*)[pTexture->blitBuffer contents]) + (x + y * pTexture->width) * pixelBytes;
    for (int i = 0; i < (int)height; ++i)
    {
      memcpy((uint8_t*)pPixels + (i * pTexture->width * pixelBytes), pPixelData, width * pixelBytes);
      pPixelData += pTexture->width * pixelBytes;
    }
  }

  // Flip R&B when required
  if (pTexture->format == vcTextureFormat_RGBA8 && (pTexture->flags & vcTCF_RenderTarget))
  {
    pPixelData = (uint8_t*)pPixels;
    for (int i = 0; i < (int)(height * width); ++i, pPixelData += pixelBytes)
    {
      uint8_t temp = pPixelData[0];
      pPixelData[0] = pPixelData[2];
      pPixelData[2] = temp;
    }
  }

  return result == udR_Success;
}
