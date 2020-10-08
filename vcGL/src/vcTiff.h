#ifndef vcTiff_h__
#define vcTiff_h__

#include <stdint.h>
#include "tiffio.h"
#include "udResult.h"

struct vcMemoryInStream;

// Will load in RGBA8 format
udResult vcTiff_LoadFromMemory(const uint8_t *pBuffer, size_t bufSize, uint32_t *pW, uint32_t *pH, uint8_t **ppPixelData);

// Does not duplicate pData. Ensure you do not delete pData before you destroy the memory stream!
udResult vcTiff_CreateMemoryStream(const uint8_t *pData, size_t bufSize, vcMemoryInStream **ppStream);
void vcTiff_DestroyMemoryStream(vcMemoryInStream **ppStream);

udResult vcTiff_CreateTiff(TIFF **ppTiff, vcMemoryInStream *pStream);

#endif
