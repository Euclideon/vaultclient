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
