#include "vcTesting.h"
#include "tiffio.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#define EXPECT_EQ_BREAK(a, b) EXPECT_EQ(a, b); if (a != b) break
#define EXPECT_NE_BREAK(a, b) EXPECT_NE(a, b); if (a == b) break

#define TEST_FILE_1_NAME "rgba_3x3.tiff"
static uint32 const gTEST_FILE_1_PIXELS[9] =
{
  0xFF000000, 0xFF0000FF, 0xFF00FF00,
  0xFFFF0000, 0xFF00FFFF, 0xFFFF00FF,
  0xFFFFFF00, 0xFF808080, 0xFFFFFFFF,
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

static TIFF *vcTesting_LoadTiffTestFile(char const *pFileName)
{
  static const char *pPossibleLocations[] =
  {
    "./vcTesting/resources/",
    "../../vcTesting/resources/"
  };

  for (size_t i = 0; i < udLengthOf(pPossibleLocations); ++i)
  {
    char buf[64] = {};
    udSprintf(buf, "%s%s", pPossibleLocations[i], pFileName);
    TIFF *pTif = TIFFOpen(buf, "r");
    if (pTif != nullptr)
      return pTif;
  }

  return nullptr;
}

TEST(vcTiff, LoadTiffFile)
{
  TIFF *pTif = nullptr;
  uint32 w = 0;
  uint32 h = 0;
  size_t npixels = 0;
  int directoryCount = 0;
  uint32 *raster = nullptr;

  do
  {
    pTif = vcTesting_LoadTiffTestFile(TEST_FILE_1_NAME);

    EXPECT_NE_BREAK(pTif, nullptr);

    // Directories
    directoryCount = vcTesting_TiffDirectoryCount(pTif);
    EXPECT_EQ_BREAK(directoryCount, 1);

    // Colour data
    EXPECT_EQ_BREAK(TIFFGetField(pTif, TIFFTAG_IMAGEWIDTH, &w), 1);
    EXPECT_EQ_BREAK(w, 3ul);
    EXPECT_EQ_BREAK(TIFFGetField(pTif, TIFFTAG_IMAGELENGTH, &h), 1);
    EXPECT_EQ_BREAK(h, 3ul);

    npixels = size_t(w) * h;

    raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
    EXPECT_NE_BREAK(raster, nullptr);

    int readSuccess = TIFFReadRGBAImageOriented(pTif, w, h, raster, 0, ORIENTATION_TOPLEFT);
    EXPECT_EQ_BREAK(readSuccess, 1);

    EXPECT_EQ(memcmp(gTEST_FILE_1_PIXELS, raster, npixels * sizeof(uint32)), 0);
  } while (false);

  _TIFFfree(raster);
  TIFFClose(pTif);
}
