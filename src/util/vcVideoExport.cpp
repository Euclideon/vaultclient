#include "vcVideoExport.h"

#include "udPlatform.h"
#include "udStringUtil.h"
#include "udFile.h"
#include "udPlatformUtil.h"

#include "vcTexture.h"

#include "stb_image_write.h"

#if VC_HAS_WINDOWSMEDIAFOUNDATION
# include <Windows.h>
# include <mfapi.h>
# include <mfidl.h>
# include <Mfreadwrite.h>
# include <mferror.h>
# pragma comment(lib, "mfreadwrite")
# pragma comment(lib, "mfplat")
# pragma comment(lib, "mfuuid")

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = NULL;
  }
}
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION

struct vcVideoExport
{
  int totalFrames;
  vcVideoExportSettings settings;

#if VC_HAS_WINDOWSMEDIAFOUNDATION
  LONGLONG rtStart;
  uint64_t frameDuration;

  IMFSinkWriter *pSinkWriter;
  DWORD stream;
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION
};

enum vcVideoExportValues
{
  vcVEV_BitRate = 65'000'000, // Paul doesn't know what this value actually represents
};

#if VC_HAS_WINDOWSMEDIAFOUNDATION
HRESULT InitializeSinkWriter(IMFSinkWriter **ppWriter, DWORD *pStreamIndex, const vcVideoExportSettings &settings)
{
  *ppWriter = NULL;
  *pStreamIndex = NULL;

  IMFSinkWriter   *pSinkWriter = NULL;
  IMFMediaType    *pMediaTypeOut = NULL;
  IMFMediaType    *pMediaTypeIn = NULL;
  DWORD           streamIndex = 0;

  HRESULT hr = MFCreateSinkWriterFromURL(udOSString(settings.filename), NULL, NULL, &pSinkWriter);

  // Set the output media type.
  if (SUCCEEDED(hr))
    hr = MFCreateMediaType(&pMediaTypeOut);

  if (SUCCEEDED(hr))
    hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

  if (SUCCEEDED(hr))
    hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);

  if (SUCCEEDED(hr))
    hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, vcVEV_BitRate);

  if (SUCCEEDED(hr))
    hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, settings.width, settings.height);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, settings.fps, 1);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

  if (SUCCEEDED(hr))
    hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);

  // Set the input media type.
  if (SUCCEEDED(hr))
    hr = MFCreateMediaType(&pMediaTypeIn);

  if (SUCCEEDED(hr))
    hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

  if (SUCCEEDED(hr))
    hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

  if (SUCCEEDED(hr))
    hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, settings.width, settings.height);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, settings.fps, 1);

  if (SUCCEEDED(hr))
    hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

  if (SUCCEEDED(hr))
    hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);

  // Tell the sink writer to start accepting data.
  if (SUCCEEDED(hr))
    hr = pSinkWriter->BeginWriting();

  // Return the pointer to the caller.
  if (SUCCEEDED(hr))
  {
    *ppWriter = pSinkWriter;
    (*ppWriter)->AddRef();
    *pStreamIndex = streamIndex;
  }

  SafeRelease(&pSinkWriter);
  SafeRelease(&pMediaTypeOut);
  SafeRelease(&pMediaTypeIn);

  return hr;
}

HRESULT WriteFrame(vcVideoExport *pExport, const uint8_t *pImageBuffer)
{
  IMFSample *pSample = NULL;
  IMFMediaBuffer *pBuffer = NULL;

  const LONG cbWidth = 4 * pExport->settings.width;
  const DWORD cbBuffer = cbWidth * pExport->settings.height;

  BYTE *pData = NULL;

  // Create a new memory buffer.
  HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

  uint32_t *pPixels = (uint32_t*)pImageBuffer;
  for (uint32_t i = 0; i < pExport->settings.width * pExport->settings.height; ++i)
    pPixels[i] = (pPixels[i] & 0xFF00FF00) | ((pPixels[i] & 0xFF0000) >> 16) | ((pPixels[i] & 0xFF) << 16);

  // Lock the buffer and copy the video frame to the buffer.
  if (SUCCEEDED(hr))
    hr = pBuffer->Lock(&pData, NULL, NULL);

  if (SUCCEEDED(hr))
    hr = MFCopyImage(pData, cbWidth, (BYTE*)(pImageBuffer + cbWidth * (pExport->settings.height-1)), cbWidth * -1, cbWidth, pExport->settings.height);

  if (pBuffer)
    pBuffer->Unlock();

  // Set the data length of the buffer.
  if (SUCCEEDED(hr))
    hr = pBuffer->SetCurrentLength(cbBuffer);

  // Create a media sample and add the buffer to the sample.
  if (SUCCEEDED(hr))
    hr = MFCreateSample(&pSample);

  if (SUCCEEDED(hr))
    hr = pSample->AddBuffer(pBuffer);

  // Set the time stamp and the duration.
  if (SUCCEEDED(hr))
    hr = pSample->SetSampleTime(pExport->rtStart);

  if (SUCCEEDED(hr))
    hr = pSample->SetSampleDuration(pExport->frameDuration);

  // Send the sample to the Sink Writer.
  if (SUCCEEDED(hr))
    hr = pExport->pSinkWriter->WriteSample(pExport->stream, pSample);

  SafeRelease(&pSample);
  SafeRelease(&pBuffer);

  return hr;
}
#endif //VC_HAS_WINDOWSMEDIAFOUNDATION

udResult vcVideoExport_BeginExport(vcVideoExport **ppExport, const vcVideoExportSettings &settings)
{
  if (ppExport == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Failure_;

#if VC_HAS_WINDOWSMEDIAFOUNDATION
  HRESULT hr = 0;
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION

  vcVideoExport *pExport = udAllocType(vcVideoExport, 1, udAF_Zero);
  UD_ERROR_NULL(pExport, udR_MemoryAllocationFailure);

  pExport->settings = settings;

  switch (settings.format)
  {
  case vcVideoExportFormat_JPGSequence: // Fallthrough
  case vcVideoExportFormat_PNGSequence:
  {
    break;
  }
#if VC_HAS_WINDOWSMEDIAFOUNDATION
  case vcVideoExportFormat_MP4_H264:
  {
    pExport->rtStart = 0;
    pExport->frameDuration = 10 * 1000 * 1000 / settings.fps;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
      return udR_Failure_;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
      return udR_Failure_;

    hr = InitializeSinkWriter(&pExport->pSinkWriter, &pExport->stream, settings);
    if (FAILED(hr))
      return udR_Failure_;
  }
  break;
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION
  case vcVideoExportFormat_Count:
    break;
  }

  *ppExport = pExport;
  pExport = nullptr;

epilogue:
  vcVideoExport_Complete(&pExport);

  return result;
}

udResult vcVideoExport_Complete(vcVideoExport **ppExport)
{
  if (ppExport == nullptr || *ppExport == nullptr)
    return udR_Success;

  vcVideoExport *pExport = *ppExport;
  *ppExport = nullptr;

#if VC_HAS_WINDOWSMEDIAFOUNDATION
  if (pExport->settings.format == vcVideoExportFormat_MP4_H264)
  {
    pExport->pSinkWriter->Finalize();

    SafeRelease(&pExport->pSinkWriter);
    MFShutdown();
    CoUninitialize();
  }
#endif

  udFree(pExport);

  return udR_Success;
}

struct vcVideoExportCallbackResult
{
  udFile *pFile;
  udResult result;
};

void vcVideoExport_StbCallback(void *context, void *data, int size)
{
  vcVideoExportCallbackResult *pData = (vcVideoExportCallbackResult*)context;
  pData->result = udFile_Write(pData->pFile, data, size);
}

udResult vcVideoExport_AddFrame(vcVideoExport *pExport, vcTexture *pTexture, vcFramebuffer *pFramebuffer)
{
  uint8_t *pFrameBufferPixels = nullptr;
  vcTextureFormat format = vcTextureFormat_Unknown;
  const size_t pixelSize = 4;

  udInt2 currSize = udInt2::zero();

  udResult result = udR_Failure_;
  vcVideoExportCallbackResult callbackResult = { nullptr, udR_Success };

  vcTexture_GetSize(pTexture, &currSize.x, &currSize.y);

  //This assumes the framebuffer and the texture are the same format
  UD_ERROR_CHECK(vcTexture_GetFormat(pTexture, &format));

  UD_ERROR_IF(format != vcTextureFormat_RGBA8, udR_InvalidConfiguration);

  pFrameBufferPixels = udAllocType(uint8_t, currSize.x * currSize.y * pixelSize, udAF_Zero);
  UD_ERROR_NULL(pFrameBufferPixels, udR_MemoryAllocationFailure);

  vcTexture_BeginReadPixels(pTexture, 0, 0, currSize.x, currSize.y, pFrameBufferPixels, pFramebuffer);

  switch (pExport->settings.format)
  {
  case vcVideoExportFormat_JPGSequence:
  {
    const char *pFilename = udTempStr("%s/%05d.jpg", pExport->settings.filename, pExport->totalFrames);
    UD_ERROR_CHECK(udFile_Open(&callbackResult.pFile, pFilename, udFOF_Write));

    UD_ERROR_IF(stbi_write_jpg_to_func(vcVideoExport_StbCallback, &callbackResult, (int)currSize.x, (int)currSize.y, pixelSize, pFrameBufferPixels, 0) == 0, udR_InternalError);

    break;
  }
  case vcVideoExportFormat_PNGSequence:
  {
    const char *pFilename = udTempStr("%s/%05d.png", pExport->settings.filename, pExport->totalFrames);
    UD_ERROR_CHECK(udFile_Open(&callbackResult.pFile, pFilename, udFOF_Write));

    UD_ERROR_IF(stbi_write_png_to_func(vcVideoExport_StbCallback, &callbackResult, (int)currSize.x, (int)currSize.y, pixelSize, pFrameBufferPixels, 0) == 0, udR_InternalError);

    break;
  }
#if VC_HAS_WINDOWSMEDIAFOUNDATION
  case vcVideoExportFormat_MP4_H264:
  {
    UD_ERROR_IF(FAILED(WriteFrame(pExport, pFrameBufferPixels)), udR_Failure_);
    pExport->rtStart += pExport->frameDuration;
    break;
  }
#endif // VC_HAS_WINDOWSMEDIAFOUNDATION
  case vcVideoExportFormat_Count:
  {
    UD_ERROR_SET(udR_Unsupported);
    break;
  }
  }

epilogue:
  udFile_Close(&callbackResult.pFile);

  ++pExport->totalFrames;
  udFree(pFrameBufferPixels);

  return udR_Success;
}
