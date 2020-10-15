#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "udStringUtil.h"
#include "udPlatformUtil.h"
#include "udContext.h"
#include "udConvertCustom.h"
#include "udMath.h"
#include "tiffio.h"

// There could be an arbitrary number of samples, but currently we only support 4: red, green, blue, alpha
#define MAX_SAMPLES 4

enum vcTiffPhotometric
{
  vcTP_GreyScale_WhiteIsZero = 0,
  vcTP_GreyScale_BlackIsZero = 1,
  vcTP_RGB                   = 2,
  vcTP_ColourPalatte         = 3,
};

struct vcTiffSampleData
{
  union
  {
    int64_t   t_int64[MAX_SAMPLES];  // Used to store 1, 2, 4, 8, 16, 32 and 64-bit int samples
    uint64_t  t_uint64[MAX_SAMPLES]; // Used to store 1, 2, 4, 8, 16, 32 and 64-bit uint samples
    double    t_double[MAX_SAMPLES]; // float32 values are promoted to doubles
  };
};

struct vcTiffPixelData
{
  udDouble3 point;
  vcTiffSampleData sample;
};

struct vcTiffImageFormat
{
  int32 imageDepth;
  uint16 bitsPerSample;
  uint16 sampleFormat;
  uint16 samplesPerPixel;
  uint16 photometric;
  uint16 planarconfig;
  uint16 **ppColourPalette; // Stored RRR...GGG...BBB..., number of colours == 2 ^ bitsPerSample, also samplesPerPixel MUST = 1
};

struct vcTiffConvertData;

typedef void(*vcTiffImageReaderDestroyFn)(vcTiffConvertData *pData);
typedef udError(*vcTiffImageReaderReadPixelFn)(vcTiffConvertData *pData, vcTiffPixelData *pPixelData);

struct TileData
{
  uint8_t *pRaster;
  uint64_t currentPixel;
  udVector2<uint32> tileDimensions;
  udVector2<uint32_t> currentTileCoords;
  uint64_t nTilePixels; // Can be less then the size of a tile
};

struct ScanLineData
{
  uint8_t *pRaster;
  uint64_t currentPixel;
  uint32 row;
};

struct vcTiffConvertData
{
  TIFF *pTif;
  vcTiffImageReaderDestroyFn pDestroy;
  vcTiffImageReaderReadPixelFn pRead;

  vcTiffImageFormat format;
  udVector2<uint32> imageDimensions;

  union
  {
    TileData tile;
    ScanLineData scanLine;
  };

  uint32_t pointsWritten;
  uint32_t everyNth;
  double pointResolution;
  udDouble3 origin;
  udConvertCustomItemFlags convertFlags;

  uint32_t currentDirectory;
  uint32_t directoryCount;
  uint64_t pointsProcessed;
};

static void vcTiff_PackSamples(uint8_t *pBuf, uint8_t *pSampleBuf, uint32_t sampleCount, uint32_t sampleBitWidth, uint32_t samplesPerPixel, uint32_t sample)
{
  if (sampleBitWidth < 8)
  {
    UDASSERT(false, "TODO!");
  }
  else
  {
    for (size_t i = 0; i < sampleCount; ++i)
    {
      uint8_t *pDest = pBuf + sample + i * ((size_t)samplesPerPixel + sampleBitWidth / CHAR_BIT);
      uint8_t *pSrc = pSampleBuf + i * sampleBitWidth / CHAR_BIT;
      memcpy(pDest, pSrc, sampleBitWidth);
    }
  }
}

static uint32_t vcTiff_To8BitSample(double x)
{
  return uint32_t(udClamp(x, 0.0, 1.0) * 255.);
}

static uint32_t vcTiff_To8BitSample(uint64_t value, uint32_t nBits)
{
  if (nBits < 8)
    return uint32_t(255ull * value / ((1ull << nBits) - 1));
  if (nBits > 8)
    return uint32_t(value >> (nBits - 8));
  return (uint32_t)value;
}

static uint32_t vcTiff_To8BitSamplei(int64_t value, int32_t nBits)
{
  if (value < 0)
    return 0;

  if (nBits < 8)
    return uint32_t(255ll * value / ((1ll << nBits) - 1));
  if (nBits > 8)
    return uint32_t(value >> (nBits - 8));
  return (uint32_t)value;
}

static udError vcTiff_SampleDataToColour(vcTiffImageFormat const & format, vcTiffSampleData const & data, uint32_t *pColour)
{
  udError result;

  if (pColour == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  // We photometric is defined as colour map, we must have a colourmap loaded!
  UD_ERROR_IF((format.ppColourPalette != nullptr) == (format.photometric == 2), udE_InvalidConfiguration);

  switch (format.sampleFormat)
  {
    case SAMPLEFORMAT_INT:
    {
      if (format.photometric == vcTP_GreyScale_WhiteIsZero) // Assume data.samplesPerPixel == 1
      {
        uint32_t grey = 255 - vcTiff_To8BitSamplei(data.t_int64[0], format.bitsPerSample);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_GreyScale_BlackIsZero) // Assume data.samplesPerPixel == 1
      {
        uint32_t grey = vcTiff_To8BitSamplei(data.t_int64[0], format.bitsPerSample);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_RGB) // Assume data.samplesPerPixel == 3
      {
        uint32_t red = vcTiff_To8BitSamplei(data.t_int64[0], format.bitsPerSample);
        uint32_t green = vcTiff_To8BitSamplei(data.t_int64[1], format.bitsPerSample);
        uint32_t blue = vcTiff_To8BitSamplei(data.t_int64[2], format.bitsPerSample);
        *pColour = 0xFF000000 | (red) | (green << 8) | (blue << 16);
      }
      else if (format.photometric == vcTP_ColourPalatte) // Assume data.samplesPerPixel == 1
      {
        uint32_t nColours = (1ul << format.bitsPerSample);
        uint32_t index = uint32_t(data.t_int64[0]);
        uint32_t red = (*format.ppColourPalette[index] >> 8);
        uint32_t green = (*format.ppColourPalette[index] >> 8) + sizeof(uint16) * nColours;
        uint32_t blue = (*format.ppColourPalette[index] >> 8) + sizeof(uint16) * nColours * 2;
        *pColour = 0xFF000000 | (red) | (green << 8) | (blue << 16);
      }
      else if (format.samplesPerPixel == 4) // rgba
      {
        uint32_t red = vcTiff_To8BitSamplei(data.t_int64[0], format.bitsPerSample);
        uint32_t green = vcTiff_To8BitSamplei(data.t_int64[1], format.bitsPerSample);
        uint32_t blue = vcTiff_To8BitSamplei(data.t_int64[2], format.bitsPerSample);
        uint32_t alpha = vcTiff_To8BitSamplei(data.t_int64[3], format.bitsPerSample);
        *pColour = (red) | (green << 8) | (blue << 16) | (alpha << 24);
      }
      else
      {
        UD_ERROR_SET(udE_InvalidConfiguration);
      }
      break;
    }
    case SAMPLEFORMAT_UINT:
    {
      if (format.photometric == vcTP_GreyScale_WhiteIsZero) // Assume format.samplesPerPixel == 1
      {
        uint32_t grey = 255 - vcTiff_To8BitSample(data.t_uint64[0], format.bitsPerSample);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_GreyScale_BlackIsZero) // Assume format.samplesPerPixel == 1
      {
        uint32_t grey = vcTiff_To8BitSample(data.t_uint64[0], format.bitsPerSample);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_RGB) // Assume format.samplesPerPixel == 3
      {
        uint32_t red = vcTiff_To8BitSample(data.t_uint64[0], format.bitsPerSample);
        uint32_t green = vcTiff_To8BitSample(data.t_uint64[1], format.bitsPerSample);
        uint32_t blue = vcTiff_To8BitSample(data.t_uint64[2], format.bitsPerSample);
        *pColour = 0xFF000000 | (red) | (green << 8) | (blue << 16);
      }
      else if (format.photometric == vcTP_ColourPalatte) // Assume format.samplesPerPixel == 1
      {
        uint32_t nColours = (1ul << format.bitsPerSample);
        uint32_t index = uint32_t(data.t_uint64[0]);
        uint32_t red = (*format.ppColourPalette[index] >> 8);
        uint32_t green = (*format.ppColourPalette[index] >> 8) + sizeof(uint16) * nColours;
        uint32_t blue = (*format.ppColourPalette[index] >> 8) + sizeof(uint16) * nColours * 2;
        *pColour = 0xFF000000 | (red) | (green << 8) | (blue << 16);
      }
      else if (format.samplesPerPixel == 4) // rgba
      {
        uint32_t red = vcTiff_To8BitSample(data.t_uint64[0], format.bitsPerSample);
        uint32_t green = vcTiff_To8BitSample(data.t_uint64[1], format.bitsPerSample);
        uint32_t blue = vcTiff_To8BitSample(data.t_uint64[2], format.bitsPerSample);
        uint32_t alpha = vcTiff_To8BitSample(data.t_uint64[3], format.bitsPerSample);
        *pColour = (red) | (green << 8) | (blue << 16) | (alpha << 24);
      }
      else
      {
        UD_ERROR_SET(udE_InvalidConfiguration);
      }
      break;
    }
    case SAMPLEFORMAT_IEEEFP:
    {
      if (format.photometric == vcTP_GreyScale_WhiteIsZero) // Assume data.samplesPerPixel == 1
      {
        uint32_t grey = 255 - vcTiff_To8BitSample(data.t_double[0]);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_GreyScale_BlackIsZero) // Assume data.samplesPerPixel == 1
      {
        uint32_t grey = vcTiff_To8BitSample(data.t_double[0]);
        *pColour = 0xFF000000 | (grey) | (grey << 8) | (grey << 16);
      }
      else if (format.photometric == vcTP_RGB) // Assume data.samplesPerPixel == 3
      {
        uint32_t red = vcTiff_To8BitSample(data.t_double[0]);
        uint32_t green = vcTiff_To8BitSample(data.t_double[1]);
        uint32_t blue = vcTiff_To8BitSample(data.t_double[2]);
        *pColour = 0xFF000000 | (red) | (green << 8) | (blue << 16);
      }
      else if (format.photometric == vcTP_ColourPalatte) // Assume data.samplesPerPixel == 1
      {
        // Makes no sense to have a colour palette indexed by a double
        UD_ERROR_SET(udE_InvalidConfiguration);
      }
      else if (format.samplesPerPixel == 4) // rgba
      {
        uint32_t red = vcTiff_To8BitSample(data.t_double[0]);
        uint32_t green = vcTiff_To8BitSample(data.t_double[1]);
        uint32_t blue = vcTiff_To8BitSample(data.t_double[2]);
        uint32_t alpha = vcTiff_To8BitSample(data.t_double[3]);
        *pColour = (red) | (green << 8) | (blue << 16) | (alpha << 24);
      }
      else
      {
        UD_ERROR_SET(udE_InvalidConfiguration);
      }
      break;
    }
  }

  result = udE_Success;
epilogue:
  return result;
}

static udError vcTiff_DecodeSample(void * pBuf, uint64_t pixelIndex, vcTiffImageFormat const & format, uint32_t sampleIndex, vcTiffSampleData *pSample)
{
  udError result;

  if (pBuf == nullptr || pSample == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  if (format.bitsPerSample == 1)
  {
    // A 1-bit int or float makes no sense
    UD_ERROR_IF(format.sampleFormat != SAMPLEFORMAT_UINT, udE_InvalidConfiguration);
    uint64_t bitIndex = pixelIndex *format.samplesPerPixel + sampleIndex;
    uint8_t byte = ((uint8_t *)pBuf)[bitIndex / 8];
    uint64_t value = uint64_t(byte & (1 << (bitIndex % 8))) == 0 ? 0 : 1;
    pSample->t_uint64[sampleIndex] = value;
  }
  else if (format.bitsPerSample == 2)
  {
    // Currently samples less than 8 bits must be uint
    UD_ERROR_IF(format.sampleFormat != SAMPLEFORMAT_UINT, udE_InvalidConfiguration);
    uint64_t bitIndex = pixelIndex * format.samplesPerPixel + sampleIndex;
    uint8_t byte = ((uint8_t *)pBuf)[bitIndex / 4];
    uint32_t shft = (bitIndex % 4) * 2;
    uint64_t value = uint64_t((byte >> shft) & 0x3);
    pSample->t_uint64[sampleIndex] = value;
  }
  else if (format.bitsPerSample == 4)
  {
    // Currently samples less than 8 bits must be uint
    UD_ERROR_IF(format.sampleFormat != SAMPLEFORMAT_UINT, udE_InvalidConfiguration);
    uint64_t bitIndex = pixelIndex * format.samplesPerPixel + sampleIndex;
    uint8_t byte = ((uint8_t *)pBuf)[bitIndex / 2];
    uint32_t shft = (bitIndex % 2) * 4;
    uint64_t value = uint64_t((byte >> shft) & 0xF);
    pSample->t_uint64[sampleIndex] = value;
  }
  else if (format.bitsPerSample == 8)
  {
    // Currently do not support 8-bit floats
    uint64_t ind = pixelIndex * format.samplesPerPixel + sampleIndex;
    if (format.sampleFormat == SAMPLEFORMAT_INT)
      pSample->t_int64[sampleIndex] = ((int8_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_UINT)
      pSample->t_uint64[sampleIndex] = ((uint8_t *)(pBuf))[ind];
    else
      UD_ERROR_SET(udE_InvalidConfiguration);
  }
  else if (format.bitsPerSample == 16)
  {
    // Currently do not support 16-bit floats
    uint64_t ind = pixelIndex * format.samplesPerPixel + sampleIndex;
    if (format.sampleFormat == SAMPLEFORMAT_INT)
      pSample->t_int64[sampleIndex] = ((int16_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_UINT)
      pSample->t_uint64[sampleIndex] = ((uint16_t *)(pBuf))[ind];
    else
      UD_ERROR_SET(udE_InvalidConfiguration);
  }
  else if (format.bitsPerSample == 32)
  {
    uint64_t ind = pixelIndex * format.samplesPerPixel + sampleIndex;
    if (format.sampleFormat == SAMPLEFORMAT_INT)
      pSample->t_int64[sampleIndex] = ((int32_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_UINT)
      pSample->t_uint64[sampleIndex] = ((uint32_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_IEEEFP)
      pSample->t_double[sampleIndex] = (double)((float *)(pBuf))[ind];
    else
      UD_ERROR_SET(udE_InvalidConfiguration);
  }
  else if (format.bitsPerSample == 64)
  {
    uint64_t ind = pixelIndex * format.samplesPerPixel + sampleIndex;
    if (format.sampleFormat == SAMPLEFORMAT_INT)
      pSample->t_int64[sampleIndex] = ((int64_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_UINT)
      pSample->t_uint64[sampleIndex] = ((uint64_t *)(pBuf))[ind];
    else if (format.sampleFormat == SAMPLEFORMAT_IEEEFP)
      pSample->t_double[sampleIndex] = ((double *)(pBuf))[ind];
    else
      UD_ERROR_SET(udE_InvalidConfiguration);
  }

  result = udE_Success;
epilogue:
  return result;
}

static udError vcTiff_LoadTile(vcTiffConvertData *pData, uint32_t x, uint32_t y)
{
  udError result;
  uint8_t *pBuf = nullptr;
  uint32_t px, py;

  UD_ERROR_NULL(pData, udE_InvalidParameter);

  px = x * pData->tile.tileDimensions.x;
  py = y * pData->tile.tileDimensions.y;

  pData->tile.currentPixel = 0;
  pData->tile.currentTileCoords = {x, y};

  if (pData->format.planarconfig == PLANARCONFIG_CONTIG)
  {
    if (pData->tile.pRaster == nullptr)
    {
      pData->tile.pRaster = (uint8_t *)_TIFFmalloc(TIFFTileSize(pData->pTif));
      UD_ERROR_NULL(pData->tile.pRaster, udE_MemoryAllocationFailure);
    }
    UD_ERROR_IF(TIFFReadTile(pData->pTif, pData->tile.pRaster, px, py, 0, 0) == 0, udE_ReadFailure);
  }
  else
  {
    if (pData->tile.pRaster == nullptr)
    {
      pData->tile.pRaster = (uint8_t *)_TIFFmalloc(TIFFTileSize(pData->pTif) * udMin(pData->format.samplesPerPixel, MAX_SAMPLES));
      UD_ERROR_NULL(pData->tile.pRaster, udE_MemoryAllocationFailure);
    }

    size_t pixelCount = (size_t)pData->tile.tileDimensions.x * pData->tile.tileDimensions.y;
    pBuf = (uint8 *)_TIFFmalloc(TIFFTileSize(pData->pTif)); // TODO could allocated this once per image
    UD_ERROR_NULL(pBuf, udE_MemoryAllocationFailure);

    for (uint16 i = 0; i < udMin(pData->format.samplesPerPixel, MAX_SAMPLES); ++i)
    {
      UD_ERROR_IF(TIFFReadTile(pData->pTif, pBuf, px, py, 0, i) == 0, udE_ReadFailure);
      vcTiff_PackSamples(pData->tile.pRaster, pBuf, (uint32_t)pixelCount, pData->format.bitsPerSample, pData->format.samplesPerPixel, i);
    }
  }

  result = udE_Success;
epilogue:
  _TIFFfree(pBuf);
  return result;
}

static udError vcTiffImageReaderInit_Tile(vcTiffConvertData *pData)
{
  udError result;

  if (pData == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  pData->tile = {};

  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_IMAGEWIDTH, &pData->imageDimensions.x) != 1, udE_ReadFailure);
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_IMAGELENGTH, &pData->imageDimensions.y) != 1, udE_ReadFailure);
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_TILEWIDTH, &pData->tile.tileDimensions.x) != 1, udE_ReadFailure);
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_TILELENGTH, &pData->tile.tileDimensions.y) != 1, udE_ReadFailure);

  UD_ERROR_CHECK(vcTiff_LoadTile(pData, 0, 0));

  result = udE_Success;
epilogue:
  return result;
}

static void vcTiffImageReaderDestroy_Tile(vcTiffConvertData *pData)
{
  if (pData == nullptr)
    return;

  _TIFFfree(pData->tile.pRaster);
  pData->tile.pRaster = nullptr;
}

static udError vcTiffImageReaderReadPixel_Tile(vcTiffConvertData *pConvertData, vcTiffPixelData *pPixelData)
{
  udError result;
  udDouble3 tileOrigin;
  udDouble3 pixelCoords;
  uint64_t tilePixelCount = (uint64_t)pConvertData->tile.tileDimensions.x * pConvertData->tile.tileDimensions.y;

  if (pConvertData == nullptr || pPixelData == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  if (pConvertData->tile.currentPixel >= tilePixelCount) // finished this tile
  {
    pConvertData->tile.currentPixel = 0;
    pConvertData->tile.currentTileCoords.x++;
    if (pConvertData->tile.currentTileCoords.x * pConvertData->tile.tileDimensions.x >= pConvertData->imageDimensions.x)
    {
      pConvertData->tile.currentTileCoords.x = 0;
      pConvertData->tile.currentTileCoords.y++;

      if (pConvertData->tile.currentTileCoords.y * pConvertData->tile.tileDimensions.y >= pConvertData->imageDimensions.y)
        UD_ERROR_SET(udE_NotFound);
    }
    UD_ERROR_CHECK(vcTiff_LoadTile(pConvertData, pConvertData->tile.currentTileCoords.x, pConvertData->tile.currentTileCoords.y));
  }
  
  pixelCoords.x = double(pConvertData->tile.currentPixel % pConvertData->tile.tileDimensions.x) * pConvertData->pointResolution;
  pixelCoords.y = double(pConvertData->tile.currentPixel / pConvertData->tile.tileDimensions.x) * pConvertData->pointResolution;
  pixelCoords.z = 0.0;
  tileOrigin = {(double)pConvertData->tile.currentTileCoords.x * pConvertData->tile.tileDimensions.x, (double)pConvertData->tile.currentTileCoords.y * pConvertData->tile.tileDimensions.y, 0.0};
  tileOrigin *= pConvertData->pointResolution;

  pPixelData->point = pConvertData->origin + tileOrigin + pixelCoords;

  for (uint32_t i = 0; i < udMin(pConvertData->format.samplesPerPixel, MAX_SAMPLES); ++i)
    UD_ERROR_CHECK(vcTiff_DecodeSample(pConvertData->tile.pRaster, pConvertData->tile.currentPixel, pConvertData->format, i, &pPixelData->sample));

  ++pConvertData->tile.currentPixel;

  result = udE_Success;
epilogue:
  return result;
}

static udError vcTiff_LoadScanLine(vcTiffConvertData *pData, uint32_t row)
{
  udError result;
  uint8_t *pBuf = nullptr;

  UD_ERROR_NULL(pData, udE_InvalidParameter);

  pData->scanLine.currentPixel = 0;
  pData->scanLine.row = row;

  UD_ERROR_IF(pData->scanLine.row == pData->imageDimensions.y, udE_NotFound);

  if (pData->format.planarconfig == PLANARCONFIG_CONTIG)
  {
    if (pData->scanLine.pRaster == nullptr)
    {
      pData->scanLine.pRaster = (uint8 *)_TIFFmalloc(TIFFScanlineSize(pData->pTif));
      UD_ERROR_NULL(pData->scanLine.pRaster, udE_MemoryAllocationFailure);
    }

    UD_ERROR_IF(TIFFReadScanline(pData->pTif, pData->scanLine.pRaster, pData->scanLine.row) == 0, udE_ReadFailure);
  }
  else
  {
    if (pData->scanLine.pRaster == nullptr)
    {
      pData->scanLine.pRaster = (uint8 *)_TIFFmalloc(TIFFScanlineSize(pData->pTif) * udMin(pData->format.samplesPerPixel, MAX_SAMPLES));
      UD_ERROR_NULL(pData->scanLine.pRaster, udE_MemoryAllocationFailure);
    }

    pBuf = (uint8 *)_TIFFmalloc(TIFFScanlineSize(pData->pTif));
    UD_ERROR_NULL(pBuf, udE_MemoryAllocationFailure);

    for (uint16 i = 0; i < pData->format.samplesPerPixel; ++i)
    {
      UD_ERROR_IF(TIFFReadScanline(pData->pTif, pBuf, pData->scanLine.row, i) == 0, udE_ReadFailure);
      vcTiff_PackSamples(pData->scanLine.pRaster, pBuf, pData->imageDimensions.x, pData->format.bitsPerSample, pData->format.samplesPerPixel, i);
    }
  }

  result = udE_Success;
epilogue:
  _TIFFfree(pBuf);
  return result;
}

static udError vcTiffImageReaderInit_ScanLine(vcTiffConvertData *pData)
{
  udError result;
  
  UD_ERROR_NULL(pData, udE_InvalidParameter);

  pData->scanLine.currentPixel = 0;

  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_IMAGEWIDTH, &pData->imageDimensions.x) != 1, udE_ReadFailure);
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_IMAGELENGTH, &pData->imageDimensions.y) != 1, udE_ReadFailure);

  UD_ERROR_CHECK(vcTiff_LoadScanLine(pData, 0));

  result = udE_Success;
epilogue:
  /*if (result != udE_Success && pData != nullptr)
  {
    _TIFFfree(pData->scanLine.pRaster);
    pData->scanLine.pRaster = nullptr;
  }*/

  return result;
}

static void vcTiffImageReaderDestroy_ScanLine(vcTiffConvertData *pData)
{
  if (pData == nullptr)
    return;

  _TIFFfree(pData->scanLine.pRaster);
  pData->scanLine.pRaster = nullptr;
}

static udError vcTiffImageReaderReadPixel_ScanLine(vcTiffConvertData *pConvertData, vcTiffPixelData *pPixelData)
{
  udError result;
  uint64_t x;
  uint64_t y;

  if (pConvertData == nullptr || pPixelData == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  if (pConvertData->scanLine.currentPixel == pConvertData->imageDimensions.x)
  {
    UD_ERROR_IF(pConvertData->scanLine.row == pConvertData->imageDimensions.y, udE_NotFound);
    UD_ERROR_CHECK(vcTiff_LoadScanLine(pConvertData, pConvertData->scanLine.row + 1));
  }

  x = pConvertData->scanLine.currentPixel;
  y = pConvertData->scanLine.row;

  pPixelData->point = pConvertData->origin + udDouble3::create((double)x * pConvertData->pointResolution, (double)y * pConvertData->pointResolution, 0.0);

  for (uint32_t i = 0; i < udMin(pConvertData->format.samplesPerPixel, MAX_SAMPLES); ++i)
    UD_ERROR_CHECK(vcTiff_DecodeSample(pConvertData->scanLine.pRaster, pConvertData->scanLine.currentPixel, pConvertData->format, i, &pPixelData->sample));

  ++pConvertData->scanLine.currentPixel;

  result = udE_Success;
epilogue:
  return result;
}

udError vcTiff_GetDirctoryCount(const char *pFileName, uint32_t *pDirectoryCount)
{
  udError result;
  TIFF *pTiff = nullptr;

  if (pFileName == nullptr || pDirectoryCount == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  pTiff = TIFFOpen(pFileName, "r");
  UD_ERROR_NULL(pTiff, udE_OpenFailure);

  *pDirectoryCount = 0;
  do
  {
    (*pDirectoryCount)++;
  } while (TIFFReadDirectory(pTiff));

  result = udE_Success;
epilogue:
  TIFFClose(pTiff);
  return result;
}

static udError vcTiff_InitReader(vcTiffConvertData *pData)
{
  udError result;

  UD_ERROR_NULL(pData, udE_InvalidParameter);

  pData->pDestroy = nullptr;
  pData->pRead = nullptr;

  // Is tile image?
  if (vcTiffImageReaderInit_Tile(pData) == udE_Success)
  {
    pData->pDestroy = vcTiffImageReaderDestroy_Tile;
    pData->pRead = vcTiffImageReaderReadPixel_Tile;
  }
  else
  {
    UD_ERROR_CHECK(vcTiffImageReaderInit_ScanLine(pData));
    pData->pDestroy = vcTiffImageReaderDestroy_ScanLine;
    pData->pRead = vcTiffImageReaderReadPixel_ScanLine;
  }

  // TODO Strip image (but currently should load as scanline)

  result = udE_Success;
epilogue:
  return result;
}

static udError vcTiff_LoadNextDirectory(vcTiffConvertData *pData)
{
  udError result;

  if (pData == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  if (pData->pDestroy != nullptr)
    pData->pDestroy(pData);

  pData->pDestroy = nullptr;
  pData->pRead = nullptr;
  ++pData->currentDirectory;

  UD_ERROR_IF(TIFFReadDirectory(pData->pTif) == 0, udE_NotFound);
  UD_ERROR_CHECK(vcTiff_InitReader(pData));

  result = udE_Success;
epilogue:
  return result;
}

// TODO we should be able to add the point estimate here...
udError TiffConvert_Open(struct udConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, enum udConvertCustomItemFlags flags)
{
  udError result = udE_Failure;
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;
  uint32 imageDepth;

  UD_ERROR_CHECK(vcTiff_GetDirctoryCount(pConvertInput->pName, &pData->directoryCount));

  pData->pTif = TIFFOpen(pConvertInput->pName, "r");
  UD_ERROR_NULL(pData->pTif, udE_OpenFailure);

  pData->pointResolution = pointResolution;
  pData->origin = {origin[0], origin[1], origin[2]};
  pData->everyNth = everyNth;
  pData->convertFlags = flags;

  // TODO check for and deal with image depth; saved as (format.imageDepth)
  // TODO retrieve colour map
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_BITSPERSAMPLE, &pData->format.bitsPerSample) != 1, udE_ReadFailure);
  if (TIFFGetField(pData->pTif, TIFFTAG_SAMPLEFORMAT, &pData->format.sampleFormat) != 1)
    pData->format.sampleFormat = SAMPLEFORMAT_UINT; // Just assume unsigned int

  if (TIFFGetField(pData->pTif, TIFFTAG_IMAGEDEPTH, &imageDepth) != 1)
    pData->format.imageDepth = uint32(imageDepth);
  else
    pData->format.imageDepth = -1;

  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_SAMPLESPERPIXEL, &pData->format.samplesPerPixel) != 1, udE_ReadFailure);
  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_PHOTOMETRIC, &pData->format.photometric) != 1, udE_ReadFailure);    // Will a tif always have this tag?

  if (pData->format.photometric == PHOTOMETRIC_PALETTE)
    UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_COLORMAP, &pData->format.ppColourPalette) != 1, udE_InvalidConfiguration);

  UD_ERROR_IF(TIFFGetField(pData->pTif, TIFFTAG_PLANARCONFIG, &pData->format.planarconfig) != 1, udE_ReadFailure);  // Will a tif always have this tag?

  UD_ERROR_CHECK(vcTiff_InitReader(pData));

  result = udE_Success;
epilogue:
  return result;
}

udError TiffConvert_ReadFloat(struct udConvertCustomItem *pConvertInput, struct udPointBufferF64 *pBuffer)
{
  udError result;
  udError pixReadRes;
  vcTiffPixelData pixelData = {};
  uint32_t colour = 0xFFFF00FF;
  vcTiffConvertData *pData = nullptr;

  if (pConvertInput == nullptr || pBuffer == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  pData = (vcTiffConvertData *)pConvertInput->pData;
  pBuffer->pointCount = 0;

  UD_ERROR_NULL(pData->pRead, udE_Success); // No reader = all done

  while (true)
  {
    if (pData->pRead == nullptr)
      pixReadRes = udE_NotFound;

    if (pBuffer->pointCount == pBuffer->pointsAllocated)
      break;

    pixReadRes = pData->pRead(pData, &pixelData);

    if (pixReadRes == udE_Success)
    {
      ++pData->pointsProcessed;

      if (pData->everyNth != 0 && pData->pointsProcessed % pData->everyNth != 0)
        continue;

      UD_ERROR_CHECK(vcTiff_SampleDataToColour(pData->format, pixelData.sample, &colour));

      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 0] = uint8_t((colour >> 16) & 0xFF);
      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 1] = uint8_t((colour >> 8)  & 0xFF);
      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 2] = uint8_t((colour >> 0)  & 0xFF);

      pBuffer->pPositions[pBuffer->pointCount * 3 + 0] = pixelData.point[0];
      pBuffer->pPositions[pBuffer->pointCount * 3 + 1] = pixelData.point[1];
      pBuffer->pPositions[pBuffer->pointCount * 3 + 2] = pixelData.point[2];

      ++pBuffer->pointCount;
    }
    else if (pixReadRes == udE_NotFound)
    {
      // Keep loading directories until we find one we can read
      bool newDirFound = false;
      while (pData->currentDirectory < pData->directoryCount)
      {
        if (vcTiff_LoadNextDirectory(pData) == udE_Success)
        {
          newDirFound = true;
          break;
        }
      }

      if (!newDirFound)
        break;
    }
    else
    {
      if ((pData->convertFlags & udCCIF_SkipErrorsWherePossible) == 0)
        UD_ERROR_SET(pixReadRes);
    }
  }

  result = udE_Success;
epilogue:
  return result;
}

void TiffConvert_Destroy(struct udConvertCustomItem *pConvertInput)
{
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;
  udAttributeSet_Destroy(&pConvertInput->attributes);
  udFree(pConvertInput->pName);

  if (pData->pDestroy)
    pData->pDestroy(pData);

  if (pData->pTif != nullptr)
    TIFFClose(pData->pTif);

  udFree(pData);
}

void TiffConvert_Close(struct udConvertCustomItem *pConvertInput)
{
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;

  if (pData->pDestroy)
    pData->pDestroy(pData);

  if (pData->pTif != nullptr)
    TIFFClose(pData->pTif);

  memset(pData, 0, sizeof(vcTiffConvertData));
}

udError vcTiff_AddItem(udConvertContext *pConvertContext, const char *pFilename)
{
  udConvertCustomItem item = {};
  vcTiffConvertData *pData = udAllocType(vcTiffConvertData, 1, udAF_Zero);

  item.pOpen = TiffConvert_Open;
  item.pReadPointsFloat = TiffConvert_ReadFloat;
  item.pDestroy = TiffConvert_Destroy;
  item.pClose = TiffConvert_Close;

  item.pData = pData;

  item.pName = udStrdup(pFilename);
  item.boundsKnown = false;
  item.pointCount = -1;
  item.pointCountIsEstimate = false;

  item.sourceResolution = 0;
  udAttributeSet_Create(&item.attributes, udSAC_ARGB, 0);

  // Do the actual conversion
  return udConvert_AddCustomItem(pConvertContext, &item);
}
