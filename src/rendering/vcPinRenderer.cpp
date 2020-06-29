#include "vcPinRenderer.h"
#include "udFile.h"
#include "udChunkedArray.h"
#include "udStringUtil.h"
#include "udThread.h"

#include "stb_image.h"

// higher == more aggressively group
#define COLLAPSE_DISTANCE_RATIO 0.25

struct vcPinIcon
{
  enum vcLoadState
  {
    vcLoadState_InQueue,
    vcLoadState_Downloaded,
    vcLoadState_Loaded,
    vcLoadState_Failed
  } volatile loadState; // todo: proper thread safety
  const char *pIconURL;

  vcTexture *pTexture;
  void *pTextureData;
  udInt2 size;
};

struct vcPinBin
{
  int count;
  int realPins;
  udDouble3 position; // average of group
  vcPinIcon *pIcon;

  char numbers[8];
  vcLabelInfo label;
};

struct vcPinRenderer
{
  udChunkedArray<vcPinBin> pinBins;
  udChunkedArray<vcPinIcon> pinIcons;

  udMutex *pMutex;
};

void vcPinRenderer_LoadIcon(void *pUserData)
{
  vcPinRenderer *pPinRenderer = (vcPinRenderer*)pUserData;
  vcPinIcon *pPinIcon = nullptr;

  udLockMutex(pPinRenderer->pMutex);
  for (size_t i = 0; i < pPinRenderer->pinIcons.length; ++i)
  {
    if (pPinRenderer->pinIcons[i].loadState == vcPinIcon::vcLoadState_InQueue)
    {
      pPinIcon = &pPinRenderer->pinIcons[i];
      pPinIcon->loadState = vcPinIcon::vcLoadState_Failed; // needs to happen in the loop
      break;
    }
  }

  udReleaseMutex(pPinRenderer->pMutex);

  if (pPinIcon == nullptr) // can't happen
    return;

  void *pFileData = nullptr;
  int64_t fileLen = -1;
  if (udFile_Load(pPinIcon->pIconURL, &pFileData, &fileLen) == udR_Success)
  {
    int width = 0;
    int height = 0;
    int channelCount = 0;
    uint8_t *pData = nullptr;
    pData = stbi_load_from_memory((stbi_uc*)pFileData, (int)fileLen, (int*)&width, (int*)&height, (int*)&channelCount, 4);
    if (pData != nullptr)
    {
      pPinIcon->size.x = width;
      pPinIcon->size.y = height;
      pPinIcon->pTextureData = udMemDup(pData, sizeof(uint32_t) * width * height, 0, udAF_None);

      pPinIcon->loadState = vcPinIcon::vcLoadState_Downloaded;

      stbi_image_free(pData);
    }

    udFree(pFileData);
  }
}

udResult vcPinRenderer_Create(vcPinRenderer **ppPinRenderer)
{
  vcPinRenderer *pPinRenderer = udAllocType(vcPinRenderer, 1, udAF_Zero);

  pPinRenderer->pinBins.Init(128);
  pPinRenderer->pinIcons.Init(32);
  pPinRenderer->pMutex = udCreateMutex();

  *ppPinRenderer = pPinRenderer;
  return udR_Success;
}

udResult vcPinRenderer_Destroy(vcPinRenderer **ppPinRenderer)
{
  if (ppPinRenderer == nullptr || *ppPinRenderer == nullptr)
    return udR_NothingToDo;
  
  vcPinRenderer *pPinRenderer = (*ppPinRenderer);
  *ppPinRenderer = nullptr;

  for (size_t i = 0; i < pPinRenderer->pinIcons.length; ++i)
  {
    while (pPinRenderer->pinIcons[i].loadState == vcPinIcon::vcLoadState_InQueue)
      udYield();

    vcTexture_Destroy(&pPinRenderer->pinIcons[i].pTexture);
    udFree(pPinRenderer->pinIcons[i].pTextureData);
  }

  udDestroyMutex(&pPinRenderer->pMutex);
  pPinRenderer->pinBins.Deinit();
  pPinRenderer->pinIcons.Deinit();
  udFree(pPinRenderer);

  return udR_Success;
}

bool vcPinRenderer_AddPin(vcPinRenderer *pPinRenderer, vcState *pProgramState, vcSceneItem *pSceneItem, const char *pPinIconURL, udDouble3 position, int count)
{
  if (pPinIconURL == nullptr)
    return false;

  count = udMax(count, 1);

  vcPinIcon *pIcon = nullptr;
  for (size_t i = 0; i < pPinRenderer->pinIcons.length; ++i)
  {
    if (udStrEqual(pPinRenderer->pinIcons[i].pIconURL, pPinIconURL)) // TODO: remove string compare for lookup (dictionary?)
    {
      pIcon = &pPinRenderer->pinIcons[i];
      break;
    }
  }

  if (pIcon == nullptr)
  {
    vcPinIcon pinIcon = {};
    pinIcon.loadState = vcPinIcon::vcLoadState_InQueue;
    pinIcon.pIconURL = udStrdup(pPinIconURL);

    udLockMutex(pPinRenderer->pMutex);
    pPinRenderer->pinIcons.PushBack(pinIcon);
    udWorkerPool_AddTask(pProgramState->pWorkerPool, vcPinRenderer_LoadIcon, pPinRenderer, false);
    udReleaseMutex(pPinRenderer->pMutex);

    pIcon = &pPinRenderer->pinIcons[pPinRenderer->pinIcons.length - 1];
  }

  // put in a bin
  size_t b = 0;
  for (; b < pPinRenderer->pinBins.length; ++b)
  {
    vcPinBin *pBin = &pPinRenderer->pinBins[b];
    double distToBinSqr = udMagSq3(position - pBin->position);
    double cameraDistToBinSqr = udMagSq3(pBin->position - pProgramState->camera.position);

    if (pBin->pIcon == pIcon && distToBinSqr <= (cameraDistToBinSqr * COLLAPSE_DISTANCE_RATIO * COLLAPSE_DISTANCE_RATIO))
    {
      // 'put' in this bin
      pBin->position = (pBin->position * pBin->realPins + position) / (pBin->realPins + 1);
      pBin->count += count;
      ++pBin->realPins;
      break;
    }
  }

  // add new bin if didnt find one
  if (b == pPinRenderer->pinBins.length)
  {
    vcPinBin *pBin = pPinRenderer->pinBins.PushBack();
    pBin->pIcon = pIcon;
    pBin->position = position;
    pBin->count = count;

    pBin->label.pText = pBin->numbers;
    pBin->label.textColourRGBA = 0xFF000000;
    pBin->label.backColourRGBA = 0xC0FFFFFF;
    pBin->label.pSceneItem = pSceneItem;
  }

  return true;
}

void vcPinRenderer_Reset(vcPinRenderer *pPinRenderer)
{
  pPinRenderer->pinBins.Clear();
}

void vcPinRenderer_UpdateTextures(vcPinRenderer *pPinRenderer)
{
  for (size_t i = 0; i < pPinRenderer->pinIcons.length; ++i)
  {
    vcPinIcon *pIcon = &pPinRenderer->pinIcons[i];;
    if (pIcon->loadState == vcPinIcon::vcLoadState_Downloaded)
    {
      vcTexture_Create(&pIcon->pTexture, pIcon->size.x, pIcon->size.y, pIcon->pTextureData, vcTextureFormat_RGBA8, vcTFM_Linear);
      udFree(pIcon->pTextureData);
      pIcon->loadState = vcPinIcon::vcLoadState_Loaded;
    }
  }
}

void vcPinRenderer_Render(vcPinRenderer *pPinRenderer, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize, udChunkedArray<vcLabelInfo*> &labels)
{
  vcPinRenderer_UpdateTextures(pPinRenderer);

  for (size_t b = 0; b < pPinRenderer->pinBins.length; ++b)
  {
    vcPinBin *pBin = &pPinRenderer->pinBins[b];

    if (pBin->pIcon->loadState != vcPinIcon::vcLoadState_Loaded)
      continue;

    vcImageRenderInfo info = {};
    info.pTexture = pBin->pIcon->pTexture;
    info.colour = udFloat4::create(1.0f, 1.0f, 1.0f, 1.0f);
    info.size = vcImageThumbnailSize::vcIS_Native;
    info.position = pBin->position;
    info.scale = 1.0;
    info.type = vcIT_ScreenPhoto;
    info.pSceneItem = pBin->label.pSceneItem;
    vcImageRenderer_Render(&info, 0.0f, viewProjectionMatrix, screenSize, 1.0f);

    if (pBin->count > 1)
    {
      pBin->label.worldPosition = pBin->position;
      udSprintf(pBin->numbers, "%d", pBin->count);
      labels.PushBack(&pBin->label);
    }
  }
}
