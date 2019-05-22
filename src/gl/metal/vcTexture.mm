#import "vcMetal.h"

#import "gl/vcShader.h"
#import "gl/vcTexture.h"
#import "vcSettings.h"
#import "udFile.h"
#import "udPlatformUtil.h"
#import "udStringUtil.h"

#import "vcViewCon.h"
#import "vcRenderer.h"

#include "stb_image.h"

uint32_t g_textureIndex = 0;

udResult vcTexture_Create(struct vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags, int32_t aniFilter /* = 0 */)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  udUnused(hasMipmaps);

  udResult result = udR_Success;
  int pixelBytes = 4;
  vcTexture *pText = udAllocType(vcTexture, 1, udAF_Zero);

  MTLTextureDescriptor *pTextureDesc = [[MTLTextureDescriptor alloc] init];

  switch (format)
  {
  case vcTextureFormat_Unknown:
    return udR_InvalidParameter_;
    break;
  case vcTextureFormat_RGBA8:
    pTextureDesc.pixelFormat = MTLPixelFormatRGBA8Unorm;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pTextureDesc.storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    pTextureDesc.storageMode = MTLStorageModeManaged;
#else
# error "Unsupported platform!"
#endif
    pTextureDesc.usage = MTLTextureUsagePixelFormatView | MTLTextureUsageShaderRead;
    pixelBytes = 4;
    break;
  case vcTextureFormat_BGRA8:
    pTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pTextureDesc.storageMode = MTLStorageModeShared;
#elif UDPLATFORM_OSX
    pTextureDesc.storageMode = MTLStorageModeManaged;
#else
# error "Unsupported platform!"
#endif
    pTextureDesc.usage = MTLTextureUsageShaderRead;
    pixelBytes = 4;
    break;
  case vcTextureFormat_D32F:
    pTextureDesc.pixelFormat = MTLPixelFormatDepth32Float;
    pTextureDesc.usage = MTLTextureUsageShaderRead;
    pTextureDesc.storageMode = MTLStorageModePrivate;
    pixelBytes = 4;
    break;
  case vcTextureFormat_D24S8:
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pTextureDesc.pixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pTextureDesc.pixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
#else
# error "Unsupported platform!"
#endif
    pTextureDesc.usage = MTLTextureUsageShaderRead;
    pTextureDesc.storageMode = MTLStorageModePrivate;
    pixelBytes = 4;
    break;
  case vcTextureFormat_Cubemap:
    return udR_InvalidParameter_;
    break;
  case vcTextureFormat_Count:
    return udR_InvalidParameter_;
    break;
  }

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
    [_viewCon.renderer.blitEncoder synchronizeTexture:texture slice:0 level:0];
#endif
  }
  else if (pPixels)
  {
    id<MTLBuffer> temp = [_device newBufferWithBytes:pPixels length:len options:MTLStorageModeShared];
    [_viewCon.renderer.blitEncoder copyFromBuffer:temp sourceOffset:0 sourceBytesPerRow:row sourceBytesPerImage:len sourceSize:MTLSizeMake(width, height, 1) toTexture:texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
  }

  // Sampler
  NSString *key = [NSString stringWithFormat:@"%@%@", (filterMode == vcTFM_Nearest ? @"N" : @"L"), (wrapMode == vcTWM_Repeat ? @"R" : @"C")];
  id<MTLSamplerState> savedSampler = [_viewCon.renderer.samplers valueForKey:key];

  if (savedSampler)
  {
    udStrcpy(pText->samplerID, 32, key.UTF8String);
  }
  else
  {
    MTLSamplerDescriptor *pSamplerDesc = [MTLSamplerDescriptor new];
    pSamplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
    pSamplerDesc.rAddressMode = MTLSamplerAddressModeClampToEdge;
    pSamplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    pSamplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    if (filterMode == vcTFM_Nearest)
      pSamplerDesc.mipFilter = MTLSamplerMipFilterNearest;

    if (wrapMode == vcTWM_Repeat)
    {
      pSamplerDesc.rAddressMode = MTLSamplerAddressModeRepeat;
      pSamplerDesc.sAddressMode = MTLSamplerAddressModeRepeat;
      pSamplerDesc.tAddressMode = MTLSamplerAddressModeRepeat;
    }

    if (aniFilter > 0)
      pSamplerDesc.maxAnisotropy = aniFilter;
    else
      pSamplerDesc.maxAnisotropy = 1;

    pSamplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
    pSamplerDesc.magFilter = MTLSamplerMinMagFilterLinear;

    id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:pSamplerDesc];

    udStrcpy(pText->samplerID, 32, key.UTF8String);
    [_viewCon.renderer.samplers setObject:sampler forKey:key];
  }

  NSString *txID = [NSString stringWithFormat:@"%u",g_textureIndex];
  udStrcpy(pText->ID, 32, txID.UTF8String);
  [_viewCon.renderer.textures setObject:texture forKey:txID];
  ++g_textureIndex;

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
  udResult result = udR_Failure_;
  int pixelBytes = 4; // assumed

  if (pTexture == nullptr || pPixels == nullptr || width == 0 || height == 0)
    return result;

  if ((int)pTexture->width == width && (int)pTexture->height == height)
  {
    id<MTLBuffer> pData = [_device newBufferWithBytes:pPixels length:width * height * pixelBytes options:MTLStorageModeShared];

    [_viewCon.renderer.blitEncoder copyFromBuffer:pData sourceOffset:0 sourceBytesPerRow:width * pixelBytes sourceBytesPerImage:width * height * 4 sourceSize:MTLSizeMake(width, height, 1) toTexture:_viewCon.renderer.textures[[NSString stringWithUTF8String:pTexture->ID]] destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
#if UDPLATFORM_OSX
    if (_viewCon.renderer.textures[[NSString stringWithUTF8String:pTexture->ID]].storageMode != MTLStorageModePrivate)
      [_viewCon.renderer.blitEncoder synchronizeTexture:_viewCon.renderer.textures[[NSString stringWithUTF8String:pTexture->ID]] slice:0 level:0];
#endif
    result = udR_Success;
  }

  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * pixelBytes);
  return result;
}

void vcTexture_Destroy(struct vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr)
    return;

  [_viewCon.renderer.textures removeObjectForKey:[NSString stringWithUTF8String:(*ppTexture)->ID]];

  udFree(*ppTexture);
  *ppTexture = nullptr;
}

bool vcTexture_LoadCubemap(struct vcTexture **ppTexture, const char *pFilename)
{
  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);
  udFilename fileName(pFilename);

  pTexture->format = vcTextureFormat_Cubemap;

  const char* names[] = { "_LF", "_RT", "_FR", "_BK", "_UP", "_DN" };
  int width, height, depth;

  char fileNameNoExt[256] = "";
  fileName.ExtractFilenameOnly(fileNameNoExt, (int)udLengthOf(fileNameNoExt));
  uint8_t* data = stbi_load(vcSettings_GetAssetPath(udTempStr("assets/skyboxes/%s%s%s", fileNameNoExt, names[0], fileName.GetExt())), &width, &height, &depth, 0);

  MTLTextureDescriptor *pTextureDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm size:width mipmapped:NO];

  if (data)
  {
    if (depth == 3)
      // ??? RGB No corresponding MTLPixelFormat that I can see, except in the compressed formats
      pTextureDesc.pixelFormat = MTLPixelFormatInvalid;
    else
      pTextureDesc.pixelFormat = MTLPixelFormatRGBA8Unorm;
  }

  pTextureDesc.width = width;
  pTextureDesc.height = height;

  id<MTLTexture> texture = [_device newTextureWithDescriptor:pTextureDesc];


  MTLRegion region = MTLRegionMake2D(0, 0, width, height);
  [texture replaceRegion:region mipmapLevel:0 slice:0 withBytes:data bytesPerRow:4 * width bytesPerImage:4 * width * height];

  stbi_image_free(data);

  for (int i = 1; i < 6; ++i) // for each other face of the cube map
  {
    char fileNameNoExt[256] = "";
    fileName.ExtractFilenameOnly(fileNameNoExt, (int)udLengthOf(fileNameNoExt));
    uint8_t* data = stbi_load(vcSettings_GetAssetPath(udTempStr("assets/skyboxes/%s%s%s", fileNameNoExt, names[i], fileName.GetExt())), &width, &height, &depth, 0);

    [texture replaceRegion:region mipmapLevel:0 slice:i withBytes:data bytesPerRow:4 * width bytesPerImage:4 * width * height];

    stbi_image_free(data);
  }

  NSString *key = [NSString stringWithFormat:@"CL"];
  id<MTLSamplerState> savedSampler = [_viewCon.renderer.samplers valueForKey:key];

  if (savedSampler)
  {
    udStrcpy(pTexture->samplerID, 32, key.UTF8String);
  }
  else
  {
    // Sampler stuff
    MTLSamplerDescriptor *pSamplerDesc = [MTLSamplerDescriptor new];
    pSamplerDesc.mipFilter = MTLSamplerMipFilterLinear;
    pSamplerDesc.rAddressMode = MTLSamplerAddressModeClampToEdge;
    pSamplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    pSamplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;

    pSamplerDesc.minFilter = MTLSamplerMinMagFilterNearest;
    pSamplerDesc.magFilter = MTLSamplerMinMagFilterLinear;

    id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:pSamplerDesc];

    udStrcpy(pTexture->samplerID, 32, key.UTF8String);
    [_viewCon.renderer.samplers setObject:sampler forKey:key];
  }

  NSString *txID = [NSString stringWithFormat:@"%u",g_textureIndex];
  udStrcpy(pTexture->ID, 32, txID.UTF8String);
  [_viewCon.renderer.textures setObject:texture forKey:txID];
  ++g_textureIndex;

  vcGLState_ReportGPUWork(0, 0, pTexture->width * pTexture->height * depth * 6);
  *ppTexture = pTexture;
  pTexture = nullptr;
  return true;
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
