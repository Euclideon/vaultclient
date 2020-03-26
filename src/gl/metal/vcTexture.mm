#import "vcMetal.h"

#import "gl/vcShader.h"
#import "gl/vcTexture.h"
#import "vcSettings.h"
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
  case vcTextureFormat_R16:  
    pixelFormat = MTLPixelFormatR16Snorm;
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
  case vcTextureFormat_D24S8:
    usage = MTLTextureUsageShaderRead;
    storageMode = MTLStorageModePrivate;
    pixelSize = 4;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pixelFormat = MTLPixelFormatDepth32Float_Stencil8;
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

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, vcTextureCreationFlags flags /*= vcTCF_None*/)
{
  return vcTexture_CreateAdv(ppTexture, vcTextureType_Texture2D, width, height, 1, pPixels, format, filterMode, false, vcTWM_Repeat, flags);
}

udResult vcTexture_CreateAdv(vcTexture **ppTexture, vcTextureType type, uint32_t width, uint32_t height, uint32_t depth, const void *pPixels /*= nullptr*/, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0 || format == vcTextureFormat_Unknown || format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udUnused(hasMipmaps);
  udUnused(depth);
  udUnused(type);

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
    pText->flags = flags;
    
    pText->texture = [g_device newTextureWithDescriptor:pTextureDesc];
    
    if ((pTextureDesc.pixelFormat == MTLPixelFormatRGBA8Unorm) && (pTextureDesc.usage & MTLTextureUsageRenderTarget))
    {
      pText->texture = [pText->texture newTextureViewWithPixelFormat:MTLPixelFormatBGRA8Unorm];
      pTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
      pText->format = vcTextureFormat_BGRA8;
    }
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
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
      id<MTLBuffer> temp = [g_device newBufferWithBytes:pPixels length:len options:MTLStorageModeShared];
      [g_blitEncoder copyFromBuffer:temp sourceOffset:0 sourceBytesPerRow:row sourceBytesPerImage:len sourceSize:MTLSizeMake(width, height, 1) toTexture:pText->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
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

  vcGLState_ReportGPUWork(0, 0, pText->width * pText->height * pixelBytes);
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

    udUnused(depth);
  udResult result = udR_Success;
  int pixelBytes = 0;
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  if ((int)pTexture->width == width && (int)pTexture->height == height)
  {
    id<MTLBuffer> pData = [g_device newBufferWithBytes:pPixels length:width * height * pixelBytes options:MTLStorageModeShared];

    [g_blitEncoder copyFromBuffer:pData sourceOffset:0 sourceBytesPerRow:width * pixelBytes sourceBytesPerImage:width * height * 4 sourceSize:MTLSizeMake(width, height, 1) toTexture:pTexture->texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
#if UDPLATFORM_OSX
    if (pTexture->texture.storageMode != MTLStorageModePrivate)
      [g_blitEncoder synchronizeTexture:pTexture->texture slice:0 level:0];
#endif
    result = udR_Success;
  }
  
  vcGLState_FlushBlit();
  
  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * pixelBytes);
  return result;
}

void vcTexture_Destroy(struct vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr || !(*ppTexture)->texture)
    return;

  @autoreleasepool
  {
    (*ppTexture)->texture = nil;
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
    if (pTexture->blitBuffer == nil)
      pTexture->blitBuffer = [g_device newBufferWithLength:pixelBytes * pTexture->width * pTexture->height options:MTLResourceStorageModeShared];
    else if (pTexture->blitBuffer.length < 4 * pTexture->width * pTexture->height)
      pTexture->blitBuffer = [g_device newBufferWithLength:4 * pTexture->width * pTexture->height options:MTLResourceStorageModeShared];

    if (pTexture->format == vcTextureFormat_D24S8)
      [g_blitEncoder copyFromTexture:pTexture->texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(pTexture->width, pTexture->height, 1) toBuffer:pTexture->blitBuffer destinationOffset:0 destinationBytesPerRow:4 * pTexture->width destinationBytesPerImage:4 * pTexture->width * pTexture->height options:MTLBlitOptionDepthFromDepthStencil];
    else
      [g_blitEncoder copyFromTexture:pTexture->texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(pTexture->width, pTexture->height, 1) toBuffer:pTexture->blitBuffer destinationOffset:0 destinationBytesPerRow:4 * pTexture->width destinationBytesPerImage:4 * pTexture->width * pTexture->height];

    vcGLState_FlushBlit();
  }
  
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
  vcTexture_GetFormatAndPixelSize(pTexture->format, &pixelBytes);

  uint32_t *pSource = (uint32_t*)[pTexture->blitBuffer contents];
  pSource += ((y * pTexture->width) + x);
  
  uint32_t rowBytes = pixelBytes * width;
  
  for (uint32_t i = 0; i < height; ++i)
  {
    for (uint32_t j = 0; j < width; ++j)
      *((uint32_t*)pPixels + (i * rowBytes)) = *(pSource + (i * pTexture->width) + j);
  }
  
  return result == udR_Success;
}
