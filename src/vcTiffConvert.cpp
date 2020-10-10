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

// A class to read image data from a particular directory within a tiff file.
class TiffPixelReader
{
public:

  TiffPixelReader()
    : m_pTiff(nullptr)
    , m_pointResolution(0.01)
    , m_origin{}
    , m_convertFlags(udCCIF_None)
  {

  }

  virtual ~TiffPixelReader()
  {

  }

  udError Init(TIFF *pTiff)
  {
    m_pTiff = pTiff;
    return OnInit();
  }

  virtual void ShutDown()
  {
    OnShutDown();
  }

  // Returns:
  //    udE_Success: next pixel set
  //    udE_NotFound: no more pixels to be found
  //    udE_*: An error has occured
  virtual udError GetNextPixel(udDouble3 *pCoords, uint32_t *pPixel) = 0;

  void SetConvertDefaults(udDouble3 const & origin, double pointResolution, enum udConvertCustomItemFlags flags)
  {
    m_origin = origin;
    m_pointResolution = pointResolution;
    m_convertFlags = flags;
  }

protected:

  virtual udError OnInit() = 0;
  virtual void OnShutDown() = 0;

  TIFF *m_pTiff;
  double m_pointResolution;
  udDouble3 m_origin;
  udConvertCustomItemFlags m_convertFlags;
};

// TODO add TiffScanLineReader, TiffStripReader, TiffTIleReader.
// And perhaps within these, objects to read the different types of data formats.

class TiffFullImageReader : public TiffPixelReader
{
public:

  TiffFullImageReader()
    : TiffPixelReader()
    , m_pRaster(nullptr)
    , m_currentPixel(0)
    , m_imageDimensions{}
  {

  }

  ~TiffFullImageReader()
  {
    OnShutDown();
  }

  udError GetNextPixel(udDouble3 *pCoords, uint32_t *pPixel) override
  {
    udError result;
    size_t pixelCount = (size_t)m_imageDimensions.x * m_imageDimensions.y;
    uint64_t x;
    uint64_t y;

    if (pCoords == nullptr || pPixel == nullptr)
      UD_ERROR_SET(udE_InvalidParameter);

    ++m_currentPixel;

    UD_ERROR_IF(m_currentPixel == pixelCount, udE_NotFound);

    x = m_currentPixel % m_imageDimensions.x;
    y = m_currentPixel / m_imageDimensions.x;

    *pCoords = m_origin + udDouble3::create((double)x * m_pointResolution, (double)y * m_pointResolution, 0.0);
    *pPixel = m_pRaster[m_currentPixel];

    result = udE_Success;
  epilogue:
    return result;
  }

private:


  udError OnInit() override
  {
    udError result;
    size_t pixelCount;

    m_currentPixel = 0;

    UD_ERROR_IF(TIFFGetField(m_pTiff, TIFFTAG_IMAGEWIDTH, &m_imageDimensions.x) != 1, udE_ReadFailure);
    UD_ERROR_IF(TIFFGetField(m_pTiff, TIFFTAG_IMAGELENGTH, &m_imageDimensions.y) != 1, udE_ReadFailure);

    pixelCount = (size_t)m_imageDimensions.x * m_imageDimensions.y;
    m_pRaster = (uint32 *)_TIFFmalloc(pixelCount * sizeof(uint32));
    UD_ERROR_NULL(m_pRaster, udE_MemoryAllocationFailure);

    UD_ERROR_IF(TIFFReadRGBAImageOriented(m_pTiff, m_imageDimensions.x, m_imageDimensions.y, m_pRaster, ORIENTATION_BOTLEFT, 0) == 0, udE_ReadFailure);

    result = udE_Success;
  epilogue:
    return result;
  }

  void OnShutDown() override
  {
    _TIFFfree(m_pRaster);
    m_pRaster = nullptr;
    m_currentPixel = 0;
  }

  uint32 *m_pRaster;
  uint64_t m_currentPixel;
  udVector2<uint32> m_imageDimensions;
};

struct vcTiffConvertData
{
  TIFF *pTiff;
  TiffPixelReader *pReader;

  uint32_t pointsWritten;
  uint32_t everyNth;
  double pointResolution;
  udDouble3 origin;
  udConvertCustomItemFlags convertFlags;

  uint32_t currentDirectory;
  uint32_t directoryCount;
  uint64_t pointsProcessed;
};

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

  delete pData->pReader;
  pData->pReader = nullptr;

  // TODO Has Image?

  // TODO Is tile image?

  // TODO Is scanline image?

  // TODO Is strip image?

  // Otherwise just try to load the whole image.
  pData->pReader = new TiffFullImageReader();
  pData->pReader->SetConvertDefaults(pData->origin, pData->pointResolution, pData->convertFlags);
  UD_ERROR_CHECK(pData->pReader->Init(pData->pTiff));

  result = udE_Success;
epilogue:
  return result;
}

static udError vcTiff_LoadNextDirectory(vcTiffConvertData *pData)
{
  udError result;

  if (pData == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  delete pData->pReader;
  pData->pReader = nullptr;

  pData->currentDirectory++;

  UD_ERROR_IF(TIFFReadDirectory(pData->pTiff) == 0, udE_NotFound);
  UD_ERROR_CHECK(vcTiff_InitReader(pData));

  result = udE_Success;
epilogue:
  return result;
}

udError TiffConvert_Open(struct udConvertCustomItem *pConvertInput, uint32_t everyNth, const double origin[3], double pointResolution, enum udConvertCustomItemFlags flags)
{
  udError result = udE_Failure;
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;

  UD_ERROR_CHECK(vcTiff_GetDirctoryCount(pConvertInput->pName, &pData->directoryCount));

  pData->pTiff = TIFFOpen(pConvertInput->pName, "r");
  UD_ERROR_NULL(pData->pTiff, udE_OpenFailure);

  pData->pointResolution = pointResolution;
  pData->origin = {origin[0], origin[1], origin[2]};
  pData->everyNth = everyNth;
  pData->convertFlags = flags;

  UD_ERROR_CHECK(vcTiff_InitReader(pData));

  result = udE_Success;
epilogue:
  return result;
}

udError TiffConvert_ReadFloat(struct udConvertCustomItem *pConvertInput, struct udPointBufferF64 *pBuffer)
{
  udError result;
  udError pixReadRes;
  vcTiffConvertData *pData = nullptr;
  udDouble3 point = {};
  uint32_t colour = 0;

  if (pConvertInput == nullptr || pBuffer == nullptr)
    UD_ERROR_SET(udE_InvalidParameter);

  pData = (vcTiffConvertData *)pConvertInput->pData;
  pBuffer->pointCount = 0;

  UD_ERROR_NULL(pData->pReader, udE_Success); // No reader = all done

  while (true)
  {
    if (pData->pReader == nullptr)
      pixReadRes = udE_NotFound; // This would suggest 

    if (pBuffer->pointCount == pBuffer->pointsAllocated)
      break;

    pixReadRes = pData->pReader->GetNextPixel(&point, &colour);

    if (pixReadRes == udE_Success)
    {
      ++pData->pointsProcessed;

      if (pData->everyNth != 0 && pData->pointsProcessed % pData->everyNth != 0)
        continue;

      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 0] = (colour & 0xFF);
      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 1] = ((colour >> 8) & 0xFF);
      pBuffer->pAttributes[pBuffer->pointCount * pBuffer->attributeStride + 2] = ((colour >> 16) & 0xFF);

      pBuffer->pPositions[pBuffer->pointCount * 3 + 0] = point[0];
      pBuffer->pPositions[pBuffer->pointCount * 3 + 1] = point[1];
      pBuffer->pPositions[pBuffer->pointCount * 3 + 2] = point[2];

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
  return udE_Success;
}

void TiffConvert_Destroy(struct udConvertCustomItem *pConvertInput)
{
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;
  udAttributeSet_Destroy(&pConvertInput->attributes);
  udFree(pConvertInput->pName);

  delete pData->pReader;
  if (pData->pTiff != nullptr)
    TIFFClose(pData->pTiff);
  memset(pData, 0, sizeof(vcTiffConvertData));

  udFree(pData);
}

void TiffConvert_Close(struct udConvertCustomItem *pConvertInput)
{
  vcTiffConvertData *pData = (vcTiffConvertData *)pConvertInput->pData;

  delete pData->pReader;
  if (pData->pTiff != nullptr)
    TIFFClose(pData->pTiff);
  memset(pData, 0, sizeof(vcTiffConvertData));
}

udError vcTiff_AddItem(udConvertContext *pConvertContext, const char *pFilename)
{
  // Setup the custom item
  udConvertCustomItem *pItem = udAllocType(udConvertCustomItem, 1, udAF_Zero);
  vcTiffConvertData *pData = udAllocType(vcTiffConvertData, 1, udAF_Zero);

  pItem->pOpen = TiffConvert_Open;
  pItem->pReadPointsFloat = TiffConvert_ReadFloat;
  pItem->pDestroy = TiffConvert_Destroy;
  pItem->pClose = TiffConvert_Close;

  pItem->pData = pData;

  pItem->pName = udStrdup(pFilename);
  pItem->boundsKnown = false;
  pItem->pointCount = -1;
  pItem->pointCountIsEstimate = false;

  pItem->sourceResolution = 0;
  udAttributeSet_Create(&pItem->attributes, udSAC_ARGB, 0);

  // Do the actual conversion
  return udConvert_AddCustomItem(pConvertContext, pItem);
}
