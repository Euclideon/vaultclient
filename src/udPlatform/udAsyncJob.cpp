#include "udAsyncJob.h"
#include "udThread.h"

#define RESULT_SENTINAL ((udResult)-1) // A sentinal value used to determine when valid result has been written
#define RESULT_PENDING ((udResult)-2) // A sentinal value used to determine when async call has been made and not returned

struct udAsyncJob
{
  udSemaphore *pSemaphore;
  volatile udResult returnResult;
  volatile bool pending;
};

// ****************************************************************************
// Author: Dave Pevreal, March 2018
udResult udAsyncJob_Create(udAsyncJob **ppJobHandle)
{
  udResult result;
  udAsyncJob *pJob = nullptr;

  pJob = udAllocType(udAsyncJob, 1, udAF_Zero);
  UD_ERROR_NULL(pJob, udR_MemoryAllocationFailure);
  pJob->pSemaphore = udCreateSemaphore();
  UD_ERROR_NULL(pJob->pSemaphore, udR_MemoryAllocationFailure);
  pJob->returnResult = RESULT_SENTINAL;
  pJob->pending = false;
  *ppJobHandle = pJob;
  pJob = nullptr;
  result = udR_Success;

epilogue:
  if (pJob)
    udAsyncJob_Destroy(&pJob);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
void udAsyncJob_SetResult(udAsyncJob *pJobHandle, udResult returnResult)
{
  if (pJobHandle)
  {
    pJobHandle->returnResult = returnResult;
    udIncrementSemaphore(pJobHandle->pSemaphore);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
udResult udAsyncJob_GetResult(udAsyncJob *pJobHandle)
{
  if (pJobHandle)
  {
    if (pJobHandle->pending)
    {
      udWaitSemaphore(pJobHandle->pSemaphore);
      udResult result = pJobHandle->returnResult;
      pJobHandle->returnResult = RESULT_SENTINAL;
      pJobHandle->pending = false;
      return result;
    }
    else
    {
      return udR_NothingToDo;
    }
  }
  return udR_InvalidParameter_;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
bool udAsyncJob_GetResultTimeout(udAsyncJob *pJobHandle, udResult *pResult, int timeoutMs)
{
  if (pJobHandle)
  {
    udWaitSemaphore(pJobHandle->pSemaphore, timeoutMs);
    if (pJobHandle->returnResult != RESULT_SENTINAL)
    {
      *pResult = pJobHandle->returnResult;
      pJobHandle->returnResult = RESULT_SENTINAL;
      pJobHandle->pending = false;
      return true;
    }
  }
  return false;
}

// ****************************************************************************
// Author: Dave Pevreal, June 2018
void udAsyncJob_SetPending(udAsyncJob *pJobHandle)
{
  if (pJobHandle)
    pJobHandle->pending = true;
}

// ****************************************************************************
// Author: Dave Pevreal, June 2018
bool udAsyncJob_IsPending(udAsyncJob *pJobHandle)
{
  return (pJobHandle) ? pJobHandle->pending : false;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
void udAsyncJob_Destroy(udAsyncJob **ppJobHandle)
{
  if (ppJobHandle && *ppJobHandle)
  {
    udDestroySemaphore(&(*ppJobHandle)->pSemaphore);
    udFree(*ppJobHandle);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
void udAsyncPause_RequestPause(udAsyncPause *pPause)
{
  if (pPause && !pPause->pSema)
  {
    // Only attempt to pause if a pause is not already in progress
    udSemaphore *pSema = udCreateSemaphore();
    if (udInterlockedCompareExchangePointer(&pPause->pSema, pSema, nullptr) != nullptr)
    {
      // CompareExchange failed, so another thread initiated a pause, just destroy
      udDestroySemaphore(&pSema);
    }
  }
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
void udAsyncPause_Resume(udAsyncPause *pPause)
{
  if (pPause && pPause->pSema)
  {
    // Always clear an error causing the pause upon resume, it is expected to retry
    pPause->errorCausingPause = udR_Success;
    pPause->errorContext = udAsyncPause::EC_None;
    udIncrementSemaphore(pPause->pSema);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
void udAsyncPause_HandlePause(udAsyncPause *pPause)
{
  udSemaphore *pSema = pPause->pSema;
  if (pSema)
  {
    pPause->isPaused = true;
    udWaitSemaphore(pSema);
    // Once the wait is over the pause has been released, so destroy the semaphore
    pPause->isPaused = false;
    udInterlockedExchangePointer(&pPause->pSema, nullptr);
    udDestroySemaphore(&pSema);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, March 2019
const char * udAsyncPause_GetErrorContextString(udAsyncPause::Context errorContext)
{
  switch (errorContext)
  {
    case udAsyncPause::EC_None:                 return "No error context";
    case udAsyncPause::EC_WritingOutputFile:    return "Writing output file, disk full?";
    case udAsyncPause::EC_WritingTemporaryFile: return "Writing temporary file, disk full?";
    default:                                    return "(unknown error context)";
  }
}
