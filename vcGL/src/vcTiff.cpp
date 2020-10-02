#include "vcTiff.h"
#include "udPlatform.h"

struct vcMemoryInStream
{
  const uint8_t *pData;
  uint64_t size;
  uint64_t pos;
};

enum vcSeekDir
{
  vcSD_Beg,
  vcSD_Cur,
  vcSD_End
};

//-----------------------------------------------------------------
// Tiff client methods
//-----------------------------------------------------------------

static tsize_t vcTiff_MemoryInStream_Read(thandle_t fd, tdata_t buf, tsize_t size)
{
  vcMemoryInStream *pData = reinterpret_cast<vcMemoryInStream *>(fd);

  uint64_t remain = pData->size - pData->pos;
  uint64_t actual = remain < (uint64_t)size ? remain : (uint64_t)size;
  memcpy(buf, pData->pData + pData->pos, actual);
  pData->pos += actual;
  return actual;
}

// The in stream cannot write
static tsize_t vcTiff_MemoryInStream_Write(thandle_t /*fd*/, tdata_t /*buf*/, tsize_t /*size*/)
{
  return 0;
}

static toff_t vcTiff_MemoryInStream_Seek(thandle_t fd, toff_t offset, int origin)
{
  vcMemoryInStream *pData = reinterpret_cast<vcMemoryInStream *>(fd);

  uint64_t newPos = 0;
  switch (origin)
  {
    case vcSD_Beg:
    {
      newPos = offset;
      break;
    }
    case vcSD_Cur:
    {
      newPos = pData->pos + offset;
      break;
    }
    case vcSD_End:
    {
      newPos = pData->size + offset;
      break;
    }
    default:
    {
      // Error...
      return 0;
    }
  }

  toff_t result = 0;
  if (newPos < pData->size)
  {
    pData->pos = newPos;
    result = offset;
  }
  return result;
}

static toff_t vcTiff_MemoryInStream_Size(thandle_t fd)
{
  vcMemoryInStream *pData = reinterpret_cast<vcMemoryInStream *>(fd);
  return toff_t(pData->size);
}

static int vcTiff_MemoryInStream_Close(thandle_t /*fd*/)
{
  // Do nothing...
  return 0;
}

static int vcTiff_MemoryInStream_Map(thandle_t /*fd*/, tdata_t * /*phase*/, toff_t * /*psize*/)
{
  return 0;
}

static void vcTiff_MemoryInStream_Unmap(thandle_t /*fd*/, tdata_t /*base*/, toff_t /*size*/)
{

}


udResult vcTiff_CreateMemoryStream(const uint8_t *pData, size_t bufSize, vcMemoryInStream **ppStream)
{
  udResult result;

  UD_ERROR_IF(pData == nullptr, udR_InvalidParameter_);
  UD_ERROR_IF(ppStream == nullptr, udR_InvalidParameter_);
  *ppStream = udAllocType(vcMemoryInStream, 1, udAF_Zero);
  UD_ERROR_NULL(*ppStream, udR_MemoryAllocationFailure);

  (*ppStream)->pData = pData;
  (*ppStream)->pos = 0;
  (*ppStream)->size = bufSize;

  result = udR_Success;
epilogue:
  return result;
}

void vcTiff_DestroyMemoryStream(vcMemoryInStream **ppStream)
{
  if (ppStream != nullptr)
    udFree(*ppStream);
}

udResult vcTiff_CreateTiff(TIFF **ppTiff, vcMemoryInStream *pStream)
{
  udResult result;
  TIFF *pTiff = nullptr;

  UD_ERROR_NULL(ppTiff, udR_InvalidParameter_);
  UD_ERROR_NULL(pStream, udR_InvalidParameter_);

  pTiff = TIFFClientOpen("MEMTIFF", "r", (thandle_t)pStream,
                          vcTiff_MemoryInStream_Read,
                          vcTiff_MemoryInStream_Write,
                          vcTiff_MemoryInStream_Seek,
                          vcTiff_MemoryInStream_Close,
                          vcTiff_MemoryInStream_Size,
                          vcTiff_MemoryInStream_Map,
                          vcTiff_MemoryInStream_Unmap );

  UD_ERROR_NULL(pTiff, udR_OpenFailure);

  *ppTiff = pTiff;
  result = udR_Success;
epilogue:
  return result;
}

udResult vcTiff_LoadFromMemory(const uint8_t *pBuffer, size_t bufSize, uint32_t *pW, uint32_t *pH, uint8_t **ppPixelData)
{
  udResult result;
  vcMemoryInStream *pStream = nullptr;
  TIFF *pTif = nullptr;
  size_t npixels = 0;
  uint32 w, h;
  uint32 *pRaster = nullptr;

  UD_ERROR_NULL(pBuffer, udR_InvalidParameter_);
  UD_ERROR_NULL(pW, udR_InvalidParameter_);
  UD_ERROR_NULL(pH, udR_InvalidParameter_);
  UD_ERROR_NULL(ppPixelData, udR_InvalidParameter_);

  UD_ERROR_CHECK(vcTiff_CreateMemoryStream(pBuffer, size_t(bufSize), &pStream));
  UD_ERROR_CHECK(vcTiff_CreateTiff(&pTif, pStream));

  // Colour data
  UD_ERROR_IF(TIFFGetField(pTif, TIFFTAG_IMAGEWIDTH, &w) != 1, udR_InternalError);
  UD_ERROR_IF(TIFFGetField(pTif, TIFFTAG_IMAGELENGTH, &h) != 1, udR_InternalError);

  npixels = size_t(w) * h;

  pRaster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
  UD_ERROR_NULL(pRaster, udR_MemoryAllocationFailure);

  UD_ERROR_IF(TIFFReadRGBAImageOriented(pTif, w, h, pRaster, 0, ORIENTATION_TOPLEFT) != 1, udR_ReadFailure);

  *pW = uint32_t(w);
  *pH = uint32_t(h);

  // We need to duplicate the raster as pRaster is owned by the Tiff library.
  *ppPixelData = (uint8_t*)udMemDup(pRaster, npixels * sizeof(pRaster[0]), 0, udAF_None);

  result = udR_Success;
epilogue:
  _TIFFfree(pRaster);
  TIFFClose(pTif);
  vcTiff_DestroyMemoryStream(&pStream);
  return result;
}
