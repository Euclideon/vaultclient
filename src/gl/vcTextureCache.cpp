#include "vcTextureCache.h"

#include "udChunkedArray.h"
#include "udThread.h"
#include "udStringUtil.h"
#include "udPlatformUtil.h"

struct vcCachedTexture
{
  const char *pAssetName;
  vcTexture *pTexture;

  vcTextureFilterMode filterMode;
  vcTextureWrapMode wrapMode;

  double timeLastUsed; // This is set when refCount hits zero
  udInterlockedInt32 refCount;
};

udChunkedArray<vcCachedTexture> g_textures = {};
udWorkerPool *g_pWorkerPool = nullptr;
udRWLock *g_pRWLock = nullptr;

vcTexture *g_pLoadingTexture = nullptr;

void vcTextureCache_InitGlobal(udWorkerPool *pWorkerPool)
{
  g_pRWLock = udCreateRWLock();
  g_textures.Init(32);
  g_pWorkerPool = pWorkerPool;

  const int WhitePixel = 0xFFFFFFFF;
  vcTexture_Create(&g_pLoadingTexture, 1, 1, &WhitePixel);
}

void vcTextureCache_DeinitGlobal()
{
  udDestroyRWLock(&g_pRWLock);
  g_pWorkerPool = nullptr;

  for (size_t i = 0; i < g_textures.length; ++i)
  {
    udFree(g_textures[i].pAssetName);
    vcTexture_Destroy(&g_textures[i].pTexture);
  }

  g_textures.Deinit();

  vcTexture_Destroy(&g_pLoadingTexture);
}

int vcTextureCache_IndexOf(const char *pAssetName, vcTextureFilterMode filterMode, vcTextureWrapMode wrapMode)
{
  int left = 0;
  int right = (int)g_textures.length - 1;
  int m = (left + right) / 2;
  int compare = 0;

  while (left <= right)
  {
    m = (left + right) / 2;
    compare = udStrcmp(g_textures[m].pAssetName, pAssetName);

    if (compare == 0)
    {
      compare = ((int)g_textures[m].wrapMode - (int)wrapMode);

      if (compare == 0)
        compare = ((int)g_textures[m].filterMode - (int)filterMode);
    }

    if (compare < 0)
      left = m + 1;
    else if (compare > 0)
      right = m - 1;
    else
      return m;
  }

  if (compare < 0)
    return ~(m + 1);

  return ~m;
}

vcTexture* vcTextureCache_Get(const char *pAssetName, vcTextureFilterMode filterMode /*= vcTFM_Nearest*/, bool hasMipmaps /*= false*/, vcTextureWrapMode wrapMode /*= vcTWM_Repeat*/)
{
  vcTexture *pTexture = nullptr;
  int index = -1;

  udReadLockRWLock(g_pRWLock);

  index = vcTextureCache_IndexOf(pAssetName, filterMode, wrapMode);

  if (index >= 0)
  {
    pTexture = g_textures[index].pTexture;
    ++g_textures[index].refCount;

    UDASSERT(g_textures[index].filterMode == filterMode, "BAD!");
    UDASSERT(g_textures[index].wrapMode == wrapMode, "BAD!");
  }

  udReadUnlockRWLock(g_pRWLock);

  if (pTexture != nullptr)
    return pTexture;

  udWriteLockRWLock(g_pRWLock);

  index = vcTextureCache_IndexOf(pAssetName, filterMode, wrapMode);

  // Another thread added it while waiting for the write lock
  if (index >= 0)
  {
    pTexture = g_textures[index].pTexture;
    ++g_textures[index].refCount;
  }
  else
  {
    vcCachedTexture newTexture = {};

    newTexture.pAssetName = udStrdup(pAssetName);
    newTexture.refCount.Set(1);
    newTexture.filterMode = filterMode;
    newTexture.wrapMode = wrapMode;
    
    vcTexture_Clone(&newTexture.pTexture, g_pLoadingTexture);
    vcTexture_AsyncLoadFromFilename(newTexture.pTexture, g_pWorkerPool, pAssetName, filterMode, hasMipmaps, wrapMode);

    g_textures.Insert(~index, &newTexture);

    pTexture = newTexture.pTexture;
  }

  udWriteUnlockRWLock(g_pRWLock);

  return pTexture;
}

void vcTextureCache_CleanupTexture(vcTexture **ppTexture)
{
  if (*ppTexture == nullptr)
    return;
  
  udWorkerPool_AddTask(g_pWorkerPool, nullptr, *ppTexture, false, [](void *pPtr) {
    vcTexture *pTexture = (vcTexture*)pPtr;
    vcTexture_Destroy(&pTexture);
    });

  *ppTexture = nullptr;
}

void vcTextureCache_Release(vcTexture **ppTexture)
{
  if (ppTexture == nullptr || *ppTexture == nullptr)
    return;

  udReadLockRWLock(g_pRWLock);

  for (vcCachedTexture &texture : g_textures)
  {
    if (texture.pTexture == *ppTexture)
    {
      --texture.refCount;

      if (texture.refCount.Get() == 0)
        texture.timeLastUsed = udGetEpochSecsUTCf();

      *ppTexture = nullptr;

      break;
    }
  }

  udReadUnlockRWLock(g_pRWLock);

  vcTextureCache_CleanupTexture(ppTexture);
}

void vcTextureCache_ConditionallyPruneGlobal()
{
  static const double PruneFrequencySec = 3.0f;
  static const double TextureTTLSec = 10.0f;

  static double lastUpdateTime = 0;
  double currentTime = udGetEpochSecsUTCf();

  if (currentTime - lastUpdateTime >= PruneFrequencySec)
  {
    udWriteLockRWLock(g_pRWLock);

    for (size_t i = 0; i < g_textures.length; ++i)
    {
      vcCachedTexture *pCachedTexture = &g_textures[i];
      if ((pCachedTexture->refCount.Get() == 0) && (currentTime - pCachedTexture->timeLastUsed >= TextureTTLSec))
      {
        udFree(pCachedTexture->pAssetName);
        vcTextureCache_CleanupTexture(&pCachedTexture->pTexture);

        g_textures.RemoveSwapLast(i);
        --i;
      }
    }

    lastUpdateTime = currentTime;

    udWriteUnlockRWLock(g_pRWLock);
  }
}
