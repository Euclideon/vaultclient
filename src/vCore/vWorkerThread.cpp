#include "vWorkerThread.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udPlatformUtil.h"
#include "udPlatform/udThread.h"
#include "vSafeDeque.h"
#include "udPlatform/udMath.h"

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

    if (vSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) != udR_Success)
      continue;

    pThreadData->isActive = true;
    currentTask.pFunction(currentTask.pDataBlock);

    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);

    pThreadData->isActive = false;
  }

  udInterlockedPreDecrement(&pPool->runningThreads);

  return 0;
}

void vWorkerThread_StartThreads(vWorkerThreadPool **ppPool, uint8_t maxWorkers)
{
  if (ppPool == nullptr || *ppPool != nullptr || maxWorkers == 0)
    return;

  vWorkerThreadPool *pPool = udAllocType(vWorkerThreadPool, 1, udAF_Zero);

  pPool->pSemaphore = udCreateSemaphore();

  vSafeDeque_Create(&pPool->pQueuedTasks, 32);

  pPool->isRunning = true;
  pPool->totalThreads = udMax(uint8_t(1), maxWorkers);
  pPool->pThreadData = udAllocType(vWorkerThreadData, pPool->totalThreads, udAF_Zero);

  for (int i = 0; i < pPool->totalThreads; ++i)
  {
    pPool->pThreadData[i].pPool = pPool;
    udThread_Create(&pPool->pThreadData[i].pThread, vWorkerThread_DoWork, &pPool->pThreadData[i]);
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
    udThread_Destroy(&pPool->pThreadData[i].pThread);

  vWorkerThreadTask currentTask;
  while (vSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) == udR_Success)
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }

  vSafeDeque_Destroy(&pPool->pQueuedTasks);
  udDestroySemaphore(&pPool->pSemaphore);

  udFree(pPool->pThreadData);
  udFree(*ppPool);
}

void vWorkerThread_AddTask(vWorkerThreadPool *pPool, vWorkerThreadCallback *pFunc, void *pUserData /*= nullptr*/, bool clearMemory /*= true*/)
{
  if (pPool == nullptr || pPool->pQueuedTasks == nullptr || pPool->pSemaphore == nullptr)
    return;

  vWorkerThreadTask tempTask;

  tempTask.pFunction = pFunc;
  tempTask.pDataBlock = pUserData;
  tempTask.freeDataBlock = clearMemory;

  vSafeDeque_PushBack(pPool->pQueuedTasks, tempTask);

  udIncrementSemaphore(pPool->pSemaphore);
}
