#include "vcTexture.h"
#include "vcTiff.h"

#include "udWorkerPool.h"
#include "udStringUtil.h"
#include "udFile.h"

#include "stb_image.h"
#include "stb_image_write.h"
#include "udFileHandler.h"

struct AsyncTextureLoadInfo
{
  udResult loadResult;

  vcTexture *pTexture; // Used for load
  vcTexture **ppTexture; // Used for create
  vcTextureFilterMode filterMode;
  bool hasMips;
  vcTextureWrapMode wrapMode;
  uint32_t limitTextureSize;

  // optional filename to load
  const char *pFilename;

  // optional raw data to create pixels from
  void *pMemory;
  int64_t memoryLen;

  // resulting unpacked pixel data
  vcTextureType type;
  const void *pPixels;
  vcTextureFormat format;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
};

void vcTexture_AsyncLoadWorkerThreadWork(void *pTextureLoadInfo)
{
  udResult result;
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;

  if (pLoadInfo->pFilename)
    UD_ERROR_CHECK(udFile_Load(pLoadInfo->pFilename, &pLoadInfo->pMemory, &pLoadInfo->memoryLen));

  if (pLoadInfo->pMemory)
  {
    UD_ERROR_IF(pLoadInfo->memoryLen < 0, udR_InvalidParameter_);

    vcImageData tempData = {};
    uint8_t *pPixels = vcTexture_DecodePixels(pLoadInfo->pMemory, (size_t)pLoadInfo->memoryLen, &tempData);
    UD_ERROR_NULL(pPixels, udR_InternalError);

    pLoadInfo->pPixels = pPixels;
    pLoadInfo->format = vcTextureFormat_RGBA8;
    pLoadInfo->width = tempData.width;
    pLoadInfo->height = tempData.height;
    pLoadInfo->depth = 1;
  }

  if (pLoadInfo->width > pLoadInfo->limitTextureSize || pLoadInfo->height > pLoadInfo->limitTextureSize)
  {
    const void *pResizedPixels = nullptr;
    uint32_t resizedWidth = 0;
    uint32_t resizedHeight = 0;

    // On failure, just continue without resize
    if (vcTexture_ResizePixels(pLoadInfo->pPixels, pLoadInfo->width, pLoadInfo->height, pLoadInfo->limitTextureSize, &pResizedPixels, &resizedWidth, &resizedHeight) == udR_Success)
    {
      udFree(pLoadInfo->pPixels);

      pLoadInfo->pPixels = pResizedPixels;
      pLoadInfo->width = resizedWidth;
      pLoadInfo->height = resizedHeight;
      pLoadInfo->limitTextureSize = udMax(pLoadInfo->width, pLoadInfo->height); // adjust limit to match
    }
  }

  result = udR_Success;
epilogue:

  pLoadInfo->loadResult = result;
  udFree(pLoadInfo->pFilename);
  udFree(pLoadInfo->pMemory);
}

void vcTexture_AsyncLoadMainThreadWork(void *pTextureLoadInfo)
{
  udResult result;
  AsyncTextureLoadInfo *pLoadInfo = (AsyncTextureLoadInfo*)pTextureLoadInfo;
  UD_ERROR_CHECK(pLoadInfo->loadResult);

  if (pLoadInfo->pTexture)
  {
    vcTexture_UploadPixels(pLoadInfo->pTexture, pLoadInfo->pPixels, pLoadInfo->width, pLoadInfo->height, pLoadInfo->depth);
    vcTexture_UpdateSampler(pLoadInfo->pTexture, pLoadInfo->filterMode, pLoadInfo->wrapMode);
  }
  else
  {
    UD_ERROR_CHECK(vcTexture_CreateAdv(pLoadInfo->ppTexture, pLoadInfo->type, pLoadInfo->width, pLoadInfo->height, pLoadInfo->depth, pLoadInfo->pPixels, pLoadInfo->format, pLoadInfo->filterMode, pLoadInfo->hasMips, pLoadInfo->wrapMode));
  }

  result = udR_Success;
epilogue:

  if (result != udR_Success)
  {
    // texture failed to load, load a 1x1 white texture
    uint32_t whitePixel = 0xffffffff;
    vcTexture_Create(pLoadInfo->ppTexture, 1, 1, &whitePixel);
  }
  pLoadInfo->loadResult = result;
  udFree(pLoadInfo->pPixels);
}

udResult vcTexture_AsyncCreate(vcTexture **ppTexture, udWorkerPool *pPool, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || width == 0 || height == 0 || pPixels == nullptr || format == vcTextureFormat_Unknown || format == vcTextureFormat_Count)
    return udR_InvalidParameter_;

  udResult result;
  int pixelBytes = 4; // assumptions

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pPixels = udMemDup(pPixels, width * height * pixelBytes, 0, udAF_None);
  pLoadInfo->width = width;
  pLoadInfo->height = height;
  pLoadInfo->depth = 1;
  pLoadInfo->type = vcTextureType_Texture2D;
  pLoadInfo->format = format;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pFilename = udStrdup(pFilename);
  pLoadInfo->type = vcTextureType_Texture2D;
  pLoadInfo->format = vcTextureFormat_RGBA8;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_AsyncLoadFromFilename(vcTexture *pTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, uint32_t limitTextureSize /*= -1*/)
{
  if (pTexture == nullptr || pPool == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->pTexture = pTexture;
  pLoadInfo->pFilename = udStrdup(pFilename);
  pLoadInfo->type = vcTextureType_Texture2D;
  pLoadInfo->format = vcTextureFormat_RGBA8;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;

epilogue:
  return result;
}


udResult vcTexture_AsyncCreateFromMemory(vcTexture **ppTexture, udWorkerPool *pPool, void *pFileData, size_t fileLength, vcTextureFilterMode filterMode /*= vcTFM_Nearest */, bool hasMipmaps /*= false */, vcTextureWrapMode wrapMode /*= vcTWM_Repeat */, uint32_t limitTextureSize /*= -1*/)
{
  if (ppTexture == nullptr || pPool == nullptr || pFileData == nullptr || fileLength == 0)
    return udR_InvalidParameter_;

  udResult result;

  AsyncTextureLoadInfo *pLoadInfo = udAllocType(AsyncTextureLoadInfo, 1, udAF_Zero);
  UD_ERROR_NULL(pLoadInfo, udR_MemoryAllocationFailure);

  pLoadInfo->ppTexture = ppTexture;
  pLoadInfo->pMemory = udMemDup(pFileData, fileLength, 0, udAF_None);
  pLoadInfo->memoryLen = fileLength;
  pLoadInfo->type = vcTextureType_Texture2D;
  pLoadInfo->format = vcTextureFormat_RGBA8;
  pLoadInfo->filterMode = filterMode;
  pLoadInfo->hasMips = hasMipmaps;
  pLoadInfo->wrapMode = wrapMode;
  pLoadInfo->limitTextureSize = limitTextureSize;

  UD_ERROR_CHECK(udWorkerPool_AddTask(pPool, vcTexture_AsyncLoadWorkerThreadWork, pLoadInfo, true, vcTexture_AsyncLoadMainThreadWork));

  result = udR_Success;
  *ppTexture = nullptr;
epilogue:

  return result;
}

udResult vcTexture_ResizePixels(const void *pPixels, uint32_t width, uint32_t height, uint32_t targetSize, const void **ppResultPixels, uint32_t *pResultWidth, uint32_t *pResultHeight)
{
  // TODO: Add ability to upscale
  udResult result;

  const void *pLastPixels = pPixels;
  uint32_t lastWidth = width;
  uint32_t lastHeight = height;

  uint32_t totalPassCount = uint32_t(udLog2(float(udMax(width, height))) - udLog2(float(targetSize)));
  UD_ERROR_IF(totalPassCount <= 0, udR_Failure_);

  for (uint32_t p = 0; p < totalPassCount; ++p)
  {
    lastWidth >>= 1;
    lastHeight >>= 1;
    uint32_t *pDownsampledPixels = udAllocType(uint32_t, lastWidth * lastHeight, udAF_Zero);
    UD_ERROR_NULL(pDownsampledPixels, udR_MemoryAllocationFailure);

    for (uint32_t y = 0; y < lastHeight; ++y)
    {
      for (uint32_t x = 0; x < lastWidth; ++x)
      {
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // 4x4 bilinear sampling
        for (int s = 0; s < 4; ++s)
        {
          uint32_t sample = ((uint32_t*)pLastPixels)[(y * 2 + s / 2) * lastWidth * 2 + (x * 2 + s % 2)];
          r += ((sample >> 0) & 0xff) >> 2;
          g += ((sample >> 8) & 0xff) >> 2;
          b += ((sample >> 16) & 0xff) >> 2;
          a += ((sample >> 24) & 0xff) >> 2;
        }
        pDownsampledPixels[y * lastWidth + x] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
      }
    }

    if (p != 0)
      udFree(pLastPixels);

    pLastPixels = pDownsampledPixels;
  }

  result = udR_Success;
  *ppResultPixels = pLastPixels;
  *pResultWidth = lastWidth;
  *pResultHeight = lastHeight;

epilogue:
  return result;
}

udResult vcTexture_GetPixelSize(vcTextureFormat format, size_t *pSize)
{
  static const int ByteCount[vcTextureFormat_Count] = {-1, 4, 8, 16, 2, 4};

  if (format < 0 || format >= vcTextureFormat_Count || pSize == nullptr)
    return udR_InvalidParameter_;

  *pSize = ByteCount[format];
  return udR_Success;
}

// TODO: This is duplicated from vaultClient: vcRender.cpp, which is to be moved to udCore (AB#1573).
static float Float16ToFloat32(uint16_t float16)
{
  uint16_t sign_bit = (float16 & 0b1000000000000000) >> 15;
  uint16_t exponent = (float16 & 0b0111110000000000) >> 10;
  uint16_t fraction = (float16 & 0b0000001111111111) >> 0;

  float sign = (sign_bit) ? -1.0f : 1.0f;
  float m = 1.0f;
  uint16_t exponentBias = 15;

  // The exponents '00000' and '11111' are interpreted specially
  if (exponent == 31)
    return sign * INFINITY;

  if (exponent == 0)
  {
    m = 0.0f;
    exponentBias = 14;
  }

  return sign * udPow(2.0f, float(exponent - exponentBias)) * (m + (fraction / 1024.0f));
}

udResult vcTexture_ConvertPixels(const void *pPixels, vcTextureFormat formatIn, uint8_t **ppPixelsOut, vcTextureFormat formatOut, size_t pixelCount)
{
  udResult result = udR_Failure_;

  UD_ERROR_IF((pPixels == nullptr) || (ppPixelsOut == nullptr) || (pixelCount == 0) || (formatIn == vcTextureFormat_Unknown) || (formatIn == vcTextureFormat_Count) || (formatOut == vcTextureFormat_Unknown) || (formatOut == vcTextureFormat_Count), udR_Failure_);

  if (formatIn == formatOut)
  {
    size_t pixelSize;
    UD_ERROR_CHECK(vcTexture_GetPixelSize(formatIn, &pixelSize));
    *ppPixelsOut = (uint8_t*)udMemDup(pPixels, pixelCount * pixelSize, 0, udAF_None);
    UD_ERROR_NULL(ppPixelsOut, udR_MemoryAllocationFailure);
  }
  else if (formatIn == vcTextureFormat_RGBA16F && formatOut == vcTextureFormat_RGBA8)
  {
    *ppPixelsOut = udAllocType(uint8_t, pixelCount * 4, udAF_Zero);
    UD_ERROR_NULL(ppPixelsOut, udR_MemoryAllocationFailure);
    for (size_t i = 0; i < pixelCount; ++i)
    {
      for (int e = 0; e < 4; ++e)
      {
        float temp = Float16ToFloat32(((uint16_t*)pPixels)[i * 4 + e]);
        temp = udClamp(temp, 0.f, 1.f);
        ((uint8_t *)*ppPixelsOut)[i * 4 + e] = uint8_t(temp * 255.f);
      }
    }
  }
  else
  {
    UD_ERROR_SET(udR_Unsupported);
  }

  result = udR_Success;

epilogue:
  return result;
}

struct vcTextureCallbackResult
{
  udFile *pFile;
  udResult result;
};

void vcTexture_StbCallback(void *context, void *data, int size)
{
  vcTextureCallbackResult *pData = (vcTextureCallbackResult *)context;
  pData->result = udFile_Write(pData->pFile, data, size);
}

udResult vcTexture_SaveImage(vcTexture *pTexture, vcFramebuffer *pFramebuffer, const char *pFilename)
{
  if (pTexture == nullptr || pFramebuffer == nullptr || pFilename == nullptr)
    return udR_InvalidParameter_;

  udResult result;
  vcTextureFormat format;
  size_t pixelSize;
  uint8_t *pFrameBufferPixels = nullptr;
  uint8_t *pPixelsOut = nullptr;
  vcTextureCallbackResult callbackResult = { nullptr, udR_Success };

  udInt2 currSize = udInt2::zero();
  vcTexture_GetSize(pTexture, &currSize.x, &currSize.y);

  UD_ERROR_CHECK(udFile_Open(&callbackResult.pFile, pFilename, udFOF_Create | udFOF_Write));

  //This assumes the framebuffer and the texture are the same format
  UD_ERROR_CHECK(vcTexture_GetFormat(pTexture, &format));
  UD_ERROR_CHECK(vcTexture_GetPixelSize(format, &pixelSize));

  pFrameBufferPixels = udAllocType(uint8_t, currSize.x * currSize.y * pixelSize, udAF_Zero);
  UD_ERROR_NULL(pFrameBufferPixels, udR_MemoryAllocationFailure);

  vcTexture_BeginReadPixels(pTexture, 0, 0, currSize.x, currSize.y, pFrameBufferPixels, pFramebuffer);
  UD_ERROR_CHECK(vcTexture_ConvertPixels(pFrameBufferPixels, format, &pPixelsOut, vcTextureFormat_RGBA8, currSize.x * currSize.y));
  
  if (udStrEndsWithi(pFilename, "png"))
    UD_ERROR_IF(stbi_write_png_to_func(vcTexture_StbCallback, &callbackResult, (int)currSize.x, (int)currSize.y, 4, pPixelsOut, 0) == 0, udR_InternalError);
  else if (udStrEndsWithi(pFilename, "jpg"))
    UD_ERROR_IF(stbi_write_jpg_to_func(vcTexture_StbCallback, &callbackResult, (int)currSize.x, (int)currSize.y, 4, pPixelsOut, 0) == 0, udR_InternalError);
  else
    UD_ERROR_SET(udR_Unsupported);

  UD_ERROR_CHECK(callbackResult.result);

  result = udR_Success;
epilogue:

  udFile_Close(&callbackResult.pFile);

  udFree(pFrameBufferPixels);
  udFree(pPixelsOut);

  return result;
}


uint8_t *vcTexture_DecodePixels_stbi(void *pFileContents, size_t fileLength, vcImageData *pImageData = nullptr)
{
  if (pFileContents == nullptr)
    return nullptr;

  vcImageData tempData = {};
  uint8_t *pData_stbi = nullptr;
  uint8_t *pPixelData = nullptr;

  pData_stbi = stbi_load_from_memory((stbi_uc *)pFileContents, (int)fileLength, (int *)&tempData.width, (int *)&tempData.height, (int *)&tempData.channelCount, 4);
  if (pData_stbi != nullptr)
  {
    pPixelData = (uint8_t *)udMemDup(pData_stbi, (size_t)tempData.width * tempData.height * 4, 0, udAF_None);
    stbi_image_free(pData_stbi);
    if (pImageData != nullptr)
      *pImageData = tempData;
  }

  return pPixelData;
}

uint8_t *vcTexture_DecodePixels_tiff(void *pFileContents, size_t fileLength, vcImageData *pImageData = nullptr)
{
  if (pFileContents == nullptr)
    return nullptr;

  vcImageData tempData = {};
  uint8_t *pPixelData = nullptr;

  if (vcTiff_LoadFromMemory((uint8_t *)pFileContents, fileLength, &tempData.width, &tempData.height, &pPixelData) == udR_Success)
  {
    tempData.channelCount = 4;
    if (pImageData != nullptr)
      *pImageData = tempData;
  }

  return pPixelData;
}

uint8_t *vcTexture_DecodePixels(void *pFileContents, size_t fileLength, vcImageData *pImageData)
{
  uint8_t *pPixelData = nullptr;

  do
  {
    pPixelData = vcTexture_DecodePixels_stbi(pFileContents, fileLength, pImageData);
    if (pPixelData != nullptr)
      break;

    pPixelData = vcTexture_DecodePixels_tiff(pFileContents, fileLength, pImageData);
    if (pPixelData != nullptr)
      break;

  } while (false);

  return pPixelData;
}
