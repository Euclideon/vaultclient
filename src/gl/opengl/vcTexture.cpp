#include "vcOpenGL.h"

#include "gl/vcTexture.h"

#include "vcSettings.h"
#include "udPlatform/udFile.h"
#include "udPlatform/udPlatformUtil.h"
#include "stb_image.h"

bool vcTexture_Create(vcTexture **ppTexture, uint32_t width, uint32_t height, const void *pPixels, vcTextureFormat format /*= vcTextureFormat_RGBA8*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags /*flags = vcTCF_None*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return false;

  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);

  glGenTextures(1, &pTexture->id);
  glBindTexture(GL_TEXTURE_2D, pTexture->id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, vcTFMToGL[filterMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : vcTFMToGL[filterMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vcTWMToGL[wrapMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vcTWMToGL[wrapMode]);

  GLenum internalFormat;
  GLenum pixelFormat, pixelType;
  GLenum type;
  GLint glFormat;

  switch (format)
  {
  case vcTextureFormat_RGBA8:
    internalFormat = GL_RGBA;
    pixelFormat = GL_RGBA;
    pixelType = GL_UNSIGNED_BYTE;
    break;
  case vcTextureFormat_BGRA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_INT_8_8_8_8_REV;
    glFormat = GL_BGRA;
    break;
  case vcTextureFormat_D24:
    internalFormat = GL_DEPTH_COMPONENT24;
    pixelFormat = GL_DEPTH_COMPONENT;
    pixelType = GL_FLOAT;
    break;
  case vcTextureFormat_D32F:
    internalFormat = GL_DEPTH_COMPONENT32F;
    pixelFormat = GL_DEPTH_COMPONENT;
    pixelType = GL_FLOAT;
    break;
    // unknown texture format, do nothing
    return false;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelFormat, pixelType, pPixels);

  if (hasMipmaps)
    glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
  VERIFY_GL();

  pTexture->format = format;
  pTexture->width = width;
  pTexture->height = height;

  *ppTexture = pTexture;
  return true;
}

bool vcTexture_CreateDepth(vcTexture **ppTexture, uint32_t width, uint32_t height, vcTextureFormat format /*= vcTextureFormat_D24*/, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/)
{
  if (ppTexture == nullptr || width == 0 || height == 0)
    return false;

  vcTexture *pTexture = udAllocType(vcTexture, 1, udAF_Zero);

  glGenTextures(1, &pTexture->id);
  glBindTexture(GL_TEXTURE_2D, pTexture->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, vcTFMToGL[filterMode]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, vcTFMToGL[filterMode]);
  VERIFY_GL();

  GLint internalFormat;
  GLenum type;
  switch (format)
  {
  case vcTextureFormat_D32F:
    internalFormat = GL_DEPTH_COMPONENT32F;
    type = GL_FLOAT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
  case vcTextureFormat_D24: // fall through
    internalFormat = GL_DEPTH_COMPONENT24;
    type = GL_UNSIGNED_INT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
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
  return true;
}

bool vcTexture_CreateFromFilename(vcTexture **ppTexture, const char *pFilename, uint32_t *pWidth /*= nullptr*/, uint32_t *pHeight /*= nullptr*/, vcTextureFilterMode filterMode /*= vcTFM_Linear*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/, vcTextureCreationFlags flags /*= vcTCF_None*/)
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
    vcTexture_Create(&pTexture, width, height, pData, vcTextureFormat_RGBA8, filterMode, hasMipmaps, wrapMode, flags);

  stbi_image_free(pData);

  if (pWidth != nullptr)
    *pWidth = width;

  if (pHeight != nullptr)
    *pHeight = height;

  *ppTexture = pTexture;

  return (pTexture != nullptr);
}

void vcTexture_UploadPixels(vcTexture *pTexture, const void *pPixels, int width, int height)
{
  GLint internalFormat = GL_INVALID_ENUM;
  GLenum type = GL_INVALID_ENUM;
  GLint glFormat = GL_INVALID_ENUM;

  switch (pTexture->format)
  {
  case vcTextureFormat_RGBA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_INT_8_8_8_8_REV;
    glFormat = GL_RGBA;
    break;
  case vcTextureFormat_BGRA8:
    internalFormat = GL_RGBA8;
    type = GL_UNSIGNED_INT_8_8_8_8_REV;
    glFormat = GL_BGRA;
    break;
  case vcTextureFormat_D32F:
    internalFormat = GL_DEPTH_COMPONENT32F;
    type = GL_FLOAT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
  case vcTextureFormat_D24: // fall through
    internalFormat = GL_DEPTH_COMPONENT24;
    type = GL_UNSIGNED_INT;
    glFormat = GL_DEPTH_COMPONENT;
    break;
  }

  pTexture->width = width;
  pTexture->height = height;

  glBindTexture(GL_TEXTURE_2D, pTexture->id);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, pTexture->width, pTexture->height, 0, glFormat, type, pPixels);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void vcTexture_Destroy(vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr)
    return;

  glDeleteTextures(1, &(*ppTexture)->id);
  udFree(*ppTexture);
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

  size_t filenameLen = udStrlen(pFilename);
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  const char skyboxPath[] = ASSETDIR;
#elif UDPLATFORM_OSX

  char *pSkyboxPath;
  char *pBaseDir = SDL_GetBasePath();
  if (pBaseDir)
    pSkyboxPath = pBaseDir;
  else
    pSkyboxPath = SDL_strdup("./");
  char skyboxPath[1024] = "";
  udSprintf(skyboxPath, 1024, "%s", pSkyboxPath);
#else
  const char skyboxPath[] = ASSETDIR "skyboxes/";
#endif
  size_t pathLen = udStrlen(skyboxPath);
  char *pFilePath = udStrdup(skyboxPath, filenameLen + 5);

  for (int i = 0; i < 6; i++) // for each face of the cube map
  {
    int width, height, depth;

    char fileNameNoExt[256] = "";
    fileName.ExtractFilenameOnly(fileNameNoExt,UDARRAYSIZE(fileNameNoExt));
    udSprintf(pFilePath, filenameLen + 5 + pathLen, "%s%s%s%s", skyboxPath, fileNameNoExt, names[i], fileName.GetExt());
    uint8_t* data = stbi_load(pFilePath, &width, &height, &depth, 0);

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

  udFree(pFilePath);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  VERIFY_GL();

#if UDPLATFORM_OSX
  SDL_free(pBaseDir);
#endif

  if (pTexture->id == GL_INVALID_INDEX)
  {
    udFree(pTexture);
    return false;
  }

  *ppTexture = pTexture;
  return true;
}
