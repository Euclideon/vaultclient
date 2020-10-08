#include "vcTesting.h"
#include "tiffio.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udFile.h"
#include "vcTiff.h"

#define TEST_FILE_1_NAME "rgba_3x3.tiff"
static uint32 const gTEST_FILE_1_PIXELS[9] =
{
  0xFF000000, 0xFF0000FF, 0xFF00FF00,
  0xFFFF0000, 0xFF00FFFF, 0xFFFF00FF,
  0xFFFFFF00, 0xFF808080, 0xFFFFFFFF,
};

static const char *gPossibleFileLocations[] =
{
  "./vcTesting/resources/",
  "../../vcTesting/resources/"
};

static int vcTesting_TiffDirectoryCount(TIFF *pTif)
{
  if (pTif == nullptr)
    return -1;

  int dirCount = 0;
  do
  {
    dirCount++;
  } while (TIFFReadDirectory(pTif));
  return dirCount;
}

static TIFF *vcTesting_LoadTestFileAsTiff(char const *pFileName)
{
  for (size_t i = 0; i < udLengthOf(gPossibleFileLocations); ++i)
  {
    char buf[64] = {};
    udSprintf(buf, "%s%s", gPossibleFileLocations[i], pFileName);
    TIFF *pTif = TIFFOpen(buf, "r");
    if (pTif != nullptr)
      return pTif;
  }

  return nullptr;
}

static bool vcTesting_LoadTestFile(char const *pFileName, uint8_t **ppData, int64_t *pSize)
{
  for (size_t i = 0; i < udLengthOf(gPossibleFileLocations); ++i)
  {
    char buf[64] = {};
    udSprintf(buf, "%s%s", gPossibleFileLocations[i], pFileName);
    if (udFile_Load(buf, ppData, pSize) == udR_Success)
      return true;
  }

  return false;
}

TEST(vcTiff, LoadTiffFromMemory_direct)
{
  uint8_t *pFileContents = nullptr;
  uint8_t *pPixelData = nullptr;
  int64_t fileSize = 0;
  uint32_t w = 0;
  uint32_t h = 0;
  udResult result;

  do
  {
    bool loaded = vcTesting_LoadTestFile(TEST_FILE_1_NAME, &pFileContents, &fileSize);
    EXPECT_TRUE(loaded);
    if (!loaded)
      break;

    result = vcTiff_LoadFromMemory(pFileContents, fileSize, &w, &h, &pPixelData);
    EXPECT_EQ(result, udR_Success);
    if (result != udR_Success)
      break;

    EXPECT_EQ(w, 3ul);
    if (w != 3ul)
      break;

    EXPECT_EQ(h, 3ul);
    if (h != 3ul)
      break;

    EXPECT_EQ(memcmp(gTEST_FILE_1_PIXELS, pPixelData, w * h * 4), 0);
  } while (false);

  udFree(pFileContents);
  udFree(pPixelData);
}

TEST(vcTiff, LoadTiffFromMemory)
{
  uint8_t *pData = nullptr;
  int64_t fileSize = 0;
  vcMemoryInStream *pStream = nullptr;
  TIFF *pTif = nullptr;
  uint32 w = 0;
  uint32 h = 0;
  size_t npixels = 0;
  int directoryCount = 0;
  uint32 *raster = nullptr;
  udResult result;
  int intRes;

  do
  {
    bool loaded  = vcTesting_LoadTestFile(TEST_FILE_1_NAME, &pData, &fileSize);
    EXPECT_TRUE(loaded);
    if (!loaded)
      break;

    result = vcTiff_CreateMemoryStream(pData, size_t(fileSize), &pStream);
    EXPECT_EQ(result, udR_Success);
    if (result != udR_Success)
      break;

    result= vcTiff_CreateTiff(&pTif, pStream);
    EXPECT_EQ(result, udR_Success);
    if (result != udR_Success)
      break;

    // Directories
    directoryCount = vcTesting_TiffDirectoryCount(pTif);
    EXPECT_EQ(directoryCount, 1);
    if (directoryCount != 1)
      break;

    // Colour data
    intRes = TIFFGetField(pTif, TIFFTAG_IMAGEWIDTH, &w);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(w, 3ul);
    if (w != 3ul)
      break;

    intRes = TIFFGetField(pTif, TIFFTAG_IMAGELENGTH, &h);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(h, 3ul);
    if (h != 3ul)
      break;

    npixels = size_t(w) * h;

    raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
    EXPECT_NE(raster, nullptr);
    if (raster == nullptr)
      break;

    intRes = TIFFReadRGBAImageOriented(pTif, w, h, raster, 0, ORIENTATION_TOPLEFT);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(memcmp(gTEST_FILE_1_PIXELS, raster, npixels * sizeof(uint32)), 0);
  } while (false);

  _TIFFfree(raster);
  TIFFClose(pTif);
  vcTiff_DestroyMemoryStream(&pStream);
  udFree(pData);
}

TEST(vcTiff, LoadTiffFile)
{
  TIFF *pTif = nullptr;
  uint32 w = 0;
  uint32 h = 0;
  size_t npixels = 0;
  int directoryCount = 0;
  uint32 *raster = nullptr;
  int intRes;

  do
  {
    pTif = vcTesting_LoadTestFileAsTiff(TEST_FILE_1_NAME);
    EXPECT_NE(pTif, nullptr);
    if (pTif == nullptr)
      break;

    // Directories
    directoryCount = vcTesting_TiffDirectoryCount(pTif);
    EXPECT_EQ(directoryCount, 1);
    if (directoryCount != 1)
      break;

    // Colour data
    intRes = TIFFGetField(pTif, TIFFTAG_IMAGEWIDTH, &w);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(w, 3ul);
    if (w != 3ul)
      break;

    intRes = TIFFGetField(pTif, TIFFTAG_IMAGELENGTH, &h);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(h, 3ul);
    if (h != 3ul)
      break;

    npixels = size_t(w) * h;

    raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
    EXPECT_NE(raster, nullptr);
    if (raster == nullptr)
      break;

    intRes = TIFFReadRGBAImageOriented(pTif, w, h, raster, 0, ORIENTATION_TOPLEFT);
    EXPECT_EQ(intRes, 1);
    if (intRes != 1)
      break;

    EXPECT_EQ(memcmp(gTEST_FILE_1_PIXELS, raster, npixels * sizeof(uint32)), 0);
  } while (false);

  _TIFFfree(raster);
  TIFFClose(pTif);
}
