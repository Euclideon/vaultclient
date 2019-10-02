#ifndef vcTexture_h__
#define vcTexture_h__

#include "udPlatform.h"
#include "udMath.h"

enum vcTextureFormat
{
  vcTextureFormat_Unknown,

  vcTextureFormat_RGBA8,
  vcTextureFormat_BGRA8,
  vcTextureFormat_D24S8,
  vcTextureFormat_D32F,

  vcTextureFormat_Count
};

enum vcTextureFilterMode
{
  vcTFM_Nearest,
  vcTFM_Linear,

  vcTFM_Total
};

enum vcTextureWrapMode
{
  vcTWM_Repeat,
  vcTWM_Clamp,

  vcTWM_Total
};

enum vcTextureCreationFlags
{
  vcTCF_None,
  vcTCF_Dynamic = 1, // Texture gets reuploaded every frame
  vcTCF_RenderTarget = 2, // This texture needs to be able to be turned into a render target
  vcTCF_AsynchronousRead = 4, // Read operations are asynchronous. TODO (EVC-765) Ignored in metal
};

inline vcTextureCreationFlags operator|(const vcTextureCreationFlags &a, const vcTextureCreationFlags &b) { return (vcTextureCreationFlags)(int(a) | int(b)); }

struct vcTexture;
struct vcFramebuffer;
struct udWorkerPool;

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format = vcTextureFormat_RGBA8, vcTextureFilterMode filterMode = vcTFM_Nearest, bool hasMipmaps = false, vcTextureWrapMode wrapMode = vcTWM_Repeat, vcTextureCreationFlags flags = vcTCF_None, int32_t aniFilter = 0);
bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth = nullptr, uint32_t *pHeight = nullptr, vcTextureFilterMode filterMode = vcTFM_Linear, bool hasMipmaps = false, vcTextureWrapMode wrapMode = vcTWM_Repeat, vcTextureCreationFlags flags = vcTCF_None, int32_t aniFilter = 0);
bool vcTexture_CreateFromMemory(vcTexture **ppTexture, void *pFileData, size_t fileLength, uint32_t *pWidth = nullptr, uint32_t *pHeight = nullptr, vcTextureFilterMode filterMode = vcTFM_Linear, bool hasMipmaps = false, vcTextureWrapMode wrapMode = vcTWM_Repeat, vcTextureCreationFlags flags = vcTCF_None, int32_t aniFilter = 0);

void vcTexture_AsyncCreateFromFilename(vcTexture **ppTexture, udWorkerPool *pPool, const char *pFilename, vcTextureFilterMode filterMode = vcTFM_Linear, bool hasMipmaps = false, vcTextureWrapMode wrapMode = vcTWM_Repeat);

void vcTexture_Destroy(vcTexture **ppTexture);

udResult vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height);
udResult vcTexture_GetSize(vcTexture *pTexture, int *pWidth, int *pHeight);

// If 'pTexture' is created with `AsynchronousRead` flag, BeginReadPixels() / EndReadPixels() will perform
// asynchronous transfer. It is recommended to call  BeginReadPixels(), then wait an appropriate amount of
// time (e.g. next frame), then get results later with ReadPreviousPixels().
// Otherwise if 'pTexture' is created without the `AsynchronousRead` flag, BeginReadPixels() will perform a
// synchronous transfer, and there is no need for a corresponding EndReadPixels() call.
// In both cases, 'pTexture' must be an attachment of 'pFramebuffer'.
bool vcTexture_BeginReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels, vcFramebuffer *pFramebuffer);
bool vcTexture_EndReadPixels(vcTexture *pTexture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels);

#endif//vcTexture_h__
