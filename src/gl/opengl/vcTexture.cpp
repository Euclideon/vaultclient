#include "vcOpenGL.h"

#include "gl/vcTexture.h"

#include "vcSettings.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"
#include "stb_image.h"

udResult vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags /*flags = vcTCF_None*/, int32_t aniFilter /* = 0 */)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);

  glGenTextures(1, &pTexture->id);
  glBindTexture(GL_TEXTURE_2D, pTexture->id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, vcTFMToGL[filterMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : vcTFMToGL[filterMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vcTWMToGL[wrapMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vcTWMToGL[wrapMode]);

  if (aniFilter > 0)
  {
    int32_t realAniso = vcGLState_GetMaxAnisotropy(aniFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, realAniso);
  }

  GLint internalFormat = GL_INVALID_ENUM;
  GLenum type = GL_INVALID_ENUM;
  GLint glFormat = GL_INVALID_ENUM;

  switch (format)
  {
  case vcTextureFormat_RGBA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_BYTE;
    glFormat = GL_RGBA;
    break;
  case vcTextureFormat_BGRA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_BYTE;
    glFormat = GL_BGRA;
    break;
  case vcTextureFormat_D32F:
    internalFormat = GL_DEPTH_COMPONENT32F;
    type = GL_FLOAT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
  case vcTextureFormat_D24S8:
    internalFormat = GL_DEPTH24_STENCIL8;
    type = GL_UNSIGNED_INT_24_8;
    glFormat = GL_DEPTH_STENCIL;
    break;
  default:
    UD_ERROR_SET(udR_InvalidParameter_);
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, glFormat, type, pPixels);

  if (hasMipmaps)
    glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
  VERIFY_GL();

  pTexture->format = format;
  pTexture->width = width;
  pTexture->height = height;

  *ppTexture = pTexture;

  pTexture = nullptr;

epilogue:
  if (pTexture != nullptr)
    vcTexture_Destroy(&pTexture);

  return result;

}

bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/, int32_t aniFilter /*= 0*/)
{
  if (ppTexture == nullptr || pFilename == nullptr)
    return false;

  uint32_t width, height, channelCount;
  vcTexture *pTexture = nullptr;

  void *pFileData;
  int64_t fileLen;

  if (udFile_Load(pFilename, &pFileData, &fileLen) != udR_Success)
    return false;

  uint8_t *pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
  udFree(pFileData);

  if (pData)
    vcTexture_Create(&pTexture, width, height, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags, aniFilter);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  *ppTexture = pTexture;

  return (pTexture != nullptr);
}

udResult vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height)
{
  if (pTexture == nullptr || pPixels == nullptr || width == 0 || height == 0)
    return udR_InvalidParameter_;

  udResult result = udR_Success;


  GLint internalFormat = GL_INVALID_ENUM;
  GLenum type = GL_INVALID_ENUM;
  GLint glFormat = GL_INVALID_ENUM;

  switch (pTexture->format)
  {
  case vcTextureFormat_RGBA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_BYTE;
    glFormat = GL_RGBA;
    break;
  case vcTextureFormat_BGRA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_BYTE;
    glFormat = GL_BGRA;
    break;
  case vcTextureFormat_D32F:
    internalFormat = GL_DEPTH_COMPONENT32F;
    type = GL_FLOAT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
  case vcTextureFormat_D24S8:
    internalFormat = GL_DEPTH24_STENCIL8;
    type = GL_UNSIGNED_INT_24_8;
    glFormat = GL_DEPTH_STENCIL;
    break;
  default:
    UD_ERROR_SET(udR_InvalidParameter_);
  }

  pTexture->width = width;
  pTexture->height = height;

  glBindTexture(GL_TEXTURE_2D, pTexture->id);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, pTexture->width, pTexture->height, 0, glFormat, type, pPixels);
  glBindTexture(GL_TEXTURE_2D, 0);

epilogue:
  return result;
}

void vcTexture_Destroy(vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr)
    return;

  glDeleteTextures(1, &(*ppTexture)->id);
  udFree(*ppTexture);
  *ppTexture = nullptr;
}

bool vcTexture_LoadCubemap(vcTexture **ppTexture, const char *pFilename)
{
  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);
  udFilename fileName(pFilename);

  pTexture->format = vcTextureFormat_Cubemap;

  glGenTextures(1, &pTexture->id);
  glBindTexture(GL_TEXTURE_CUBE_MAP, pTexture->id);

  const char* names[] = { "_LF", "_RT", "_FR", "_BK", "_UP", "_DN" };
  const GLenum types[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    int width, height, depth;

    char fileNameNoExt[256] = "";
    fileName.ExtractFilenameOnly(fileNameNoExt, (int)udLengthOf(fileNameNoExt));
    uint8_t* data = stbi_load(vcSettings_GetAssetPath(udTempStr("assets/skyboxes/%s%s%s", fileNameNoExt, names[i], fileName.GetExt())), &width, &height, &depth, 0);

    pTexture->height = height;
    pTexture->width = width;

    if (data)
    {
      if (depth == 3)
        glTexImage2D(types[i], 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      else
        glTexImage2D(types[i], 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

      stbi_image_free(data);
    }
    VERIFY_GL();
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  VERIFY_GL();

  if (pTexture->id == GL_INVALID_INDEX)
  {
    udFree(pTexture);
    return false;
  }

  *ppTexture = pTexture;
  return true;
}

udResult vcTexture_GetSize(vcTexture *pTexture, int *pWidth, int *pHeight)
{
  if (pTexture == nullptr)
    return udR_InvalidParameter_;

  if (pWidth != nullptr)
    *pWidth = (int)pTexture->width;

  if (pHeight != nullptr)
    *pHeight = (int)pTexture->height;

  return udR_Success;
}
