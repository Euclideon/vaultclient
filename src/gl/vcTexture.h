#ifndef vcTexture_h__
#define vcTexture_h__

#include "udPlatform/udPlatform.h"
#include "udPlatform/udMath.h"

enum vcTextureFormat
{
  vcTextureFormat_Unknown,

  vcTextureFormat_RGBA8,
  vcTextureFormat_D24,
  vcTextureFormat_D32F,

  vcTextureFormat_Cubemap,

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

struct vcTexture;

bool vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format = vcTextureFormat_RGBA8, vcTextureFilterMode filterMode = vcTFM_Nearest, bool hasMipmaps = false, int32_t aniFilter = 0, vcTextureWrapMode wrapMode = vcTWM_Repeat);
bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth = nullptr, uint32_t *pHeight = nullptr, vcTextureFilterMode filterMode = vcTFM_Linear, bool hasMipmaps = false, int32_t aniFilter = 0, vcTextureWrapMode wrapMode = vcTWM_Repeat);
bool vcTexture_LoadCubemap(vcTexture **ppTexture, const char *pFilename);

void vcTexture_Destroy(vcTexture **ppTexture);

void vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height);

#endif//vcTexture_h__
