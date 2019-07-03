#include "vWorkerThread.h"

#include "udChunkedArray.h"
#include "udSafeDeque.h"
#include "udPlatformUtil.h"
#include "udThread.h"
#include "udMath.h"
#include "udStringUtil.h"

uint32_t vWorkerThread_DoWork(void *pPoolPtr)
{
  UDRELASSERT(pPoolPtr != nullptr, "Bad Pool Ptr!");

  vWorkerThreadData *pThreadData = (vWorkerThreadData*)pPoolPtr;
  vWorkerThreadPool *pPool = pThreadData->pPool;

  vWorkerThreadTask currentTask;
  int waitValue;

  udInterlockedPreIncrement(&pPool->runningThreads);

  while (pPool->isRunning)
  {
    waitValue = udWaitSemaphore(pPool->pSemaphore, 100);

    if (waitValue != 0)
      continue;

    if (udSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) != udR_Success)
      continue;

    pThreadData->isActive = true;
    if (currentTask.pFunction)
      currentTask.pFunction(currentTask.pDataBlock);

    if (currentTask.pPostFunction)
      udSafeDeque_PushBack(pPool->pQueuedPostTasks, currentTask);
    else if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);

    pThreadData->isActive = false;
  }

  udInterlockedPreDecrement(&pPool->runningThreads);

  return 0;
}

void vWorkerThread_StartThreads(vWorkerThreadPool **ppPool, uint8_t maxWorkers /*= 4*/, const char *pThreadPrefix /*= "vWorkerThread"*/)
{
  if (ppPool == nullptr || *ppPool != nullptr || maxWorkers == 0)
    return;

  vWorkerThreadPool *pPool = udAllocType(vWorkerThreadPool, 1, udAF_Zero);

  pPool->pSemaphore = udCreateSemaphore();

  udSafeDeque_Create(&pPool->pQueuedTasks, 32);
  udSafeDeque_Create(&pPool->pQueuedPostTasks, 32);

  pPool->isRunning = true;
  pPool->totalThreads = udMax(uint8_t(1), maxWorkers);
  pPool->pThreadData = udAllocType(vWorkerThreadData, pPool->totalThreads, udAF_Zero);

  for (int i = 0; i < pPool->totalThreads; ++i)
  {
    pPool->pThreadData[i].pPool = pPool;
    udThread_Create(&pPool->pThreadData[i].pThread, vWorkerThread_DoWork, &pPool->pThreadData[i], udTCF_None, udTempStr("%s%d", pThreadPrefix, i));
  }

  *ppPool = pPool;
}

void vWorkerThread_Shutdown(vWorkerThreadPool **ppPool, bool waitForCompletion)
{
  if (ppPool == nullptr || *ppPool == nullptr)
    return;

  vWorkerThreadPool *pPool = *ppPool;

  pPool->isRunning = false;

  while (pPool->runningThreads > 0 && waitForCompletion)
    udYield(); //Spin until all threads complete

  for (int i = 0; i < pPool->totalThreads; i++)
  {
    udThread_Join(pPool->pThreadData[i].pThread);
    udThread_Destroy(&pPool->pThreadData[i].pThread);
  }

  vWorkerThreadTask currentTask;
  while (udSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) == udR_Success)
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }

  while (udSafeDeque_PopFront(pPool->pQueuedPostTasks, &currentTask) == udR_Success)
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }

  udSafeDeque_Destroy(&pPool->pQueuedTasks);
  udSafeDeque_Destroy(&pPool->pQueuedPostTasks);
  udDestroySemaphore(&pPool->pSemaphore);

  udFree(pPool->pThreadData);
  udFree(*ppPool);
}

void vWorkerThread_DoPostWork(vWorkerThreadPool *pPool)
{
  vWorkerThreadTask currentTask;
  while (udSafeDeque_PopFront(pPool->pQueuedPostTasks, &currentTask) == udR_Success)
  {
    currentTask.pPostFunction(currentTask.pDataBlock);

    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }
}

void vWorkerThread_AddTask(vWorkerThreadPool *pPool, vWorkerThreadCallback *pFunc, void *pUserData /*= nullptr*/, bool clearMemory /*= true*/, vWorkerThreadCallback *pPostFunc /*= nullptr*/)
{
  if (pPool == nullptr || pPool->pQueuedTasks == nullptr || pPool->pSemaphore == nullptr)
    return;

  vWorkerThreadTask tempTask;

  tempTask.pFunction = pFunc;
  tempTask.pPostFunction = pPostFunc;
  tempTask.pDataBlock = pUserData;
  tempTask.freeDataBlock = clearMemory;

  udSafeDeque_PushBack(pPool->pQueuedTasks, tempTask);

  udIncrementSemaphore(pPool->pSemaphore);
}
