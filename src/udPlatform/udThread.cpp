#include "udThread.h"

#if UDPLATFORM_WINDOWS
//
// SetThreadName function taken from https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; // Must be 0x1000.
  LPCSTR szName; // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
static void SetThreadName(DWORD dwThreadID, const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}
#else
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

void udThread_MsToTimespec(struct timespec *pTimespec, int waitMs)
{
  if (pTimespec == nullptr)
    return;

  clock_gettime(CLOCK_REALTIME, pTimespec);
  pTimespec->tv_sec += waitMs / 1000;
  pTimespec->tv_nsec += long(waitMs % 1000) * 1000000L;

  pTimespec->tv_sec += (pTimespec->tv_nsec / 1000000000L);
  pTimespec->tv_nsec %= 1000000000L;
}
#endif

#define DEBUG_CACHE 0
#define MAX_CACHED_THREADS 16
#define CACHE_WAIT_SECONDS 30
static volatile udThread *s_pCachedThreads[MAX_CACHED_THREADS ? MAX_CACHED_THREADS : 1];

struct udThread
{
  // First element of structure is GUARANTEED to be the operating system thread handle
# if UDPLATFORM_WINDOWS
  HANDLE handle;
# else
  pthread_t t;
# endif
  udThreadStart *pThreadStarter;
  void *pThreadData;
  udSemaphore *pCacheSemaphore; // Semaphore is non-null only while on the cached thread list
  volatile int32_t refCount;
};

// ----------------------------------------------------------------------------
static uint32_t udThread_Bootstrap(udThread *pThread)
{
  uint32_t threadReturnValue;
  bool reclaimed = false; // Set to true when the thread is reclaimed by the cache system
  UDASSERT(pThread->pThreadStarter, "No starter function");
  do
  {
#if DEBUG_CACHE
    if (reclaimed)
      udDebugPrintf("Successfully reclaimed thread %p\n", pThread);
#endif
    threadReturnValue = pThread->pThreadStarter ? pThread->pThreadStarter(pThread->pThreadData) : 0;

    pThread->pThreadStarter = nullptr;
    pThread->pThreadData = nullptr;
    reclaimed = false;

    if (pThread->refCount == 1)
    {
      // Instead of letting this thread go to waste, see if we can cache it to be recycled
      UDASSERT(pThread->pCacheSemaphore == nullptr, "pCachedSemaphore non-null");
      pThread->pCacheSemaphore = udCreateSemaphore(); // Only create a semaphore if a slot is available
      if (pThread->pCacheSemaphore)
      {
        for (int slotIndex = 0; slotIndex < MAX_CACHED_THREADS; ++slotIndex)
        {
          if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], pThread, nullptr) == nullptr)
          {
#if DEBUG_CACHE
            udDebugPrintf("Making thread %p available for cache (slot %d)\n", pThread, slotIndex);
#endif
            // Successfully added to the cache, now wait to see if anyone wants to dance
            udWaitSemaphore(pThread->pCacheSemaphore, CACHE_WAIT_SECONDS * 1000);
            if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], nullptr, pThread) == pThread)
            {
#if DEBUG_CACHE
              udDebugPrintf("Allowing thread %p to die\n", pThread);
#endif
            }
            else
            {
#if DEBUG_CACHE
              udDebugPrintf("Reclaiming thread %p\n", pThread);
#endif
              reclaimed = true;
            }
            break;
          }
        }
        udDestroySemaphore(&pThread->pCacheSemaphore);
      }
    }
  } while (reclaimed);

  // Call to destroy here will decrement reference count, and only destroy if
  // the original creator of the thread didn't take a reference themselves
  if (pThread)
    udThread_Destroy(&pThread);

  return threadReturnValue;
}


// ****************************************************************************
udResult udThread_Create(udThread **ppThread, udThreadStart *pThreadStarter, void *pThreadData, udThreadCreateFlags /*flags*/, const char *pThreadName)
{
  udResult result;
  udThread *pThread = nullptr;
  int slotIndex;
  udUnused(pThreadName);

  UD_ERROR_NULL(pThreadStarter, udR_InvalidParameter_);
  for (slotIndex = 0; pThread == nullptr && slotIndex < MAX_CACHED_THREADS; ++slotIndex)
  {
    pThread = const_cast<udThread*>(s_pCachedThreads[slotIndex]);
    if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], nullptr, pThread) != pThread)
      pThread = nullptr;
  }
  if (pThread)
  {
    pThread->pThreadStarter = pThreadStarter;
    pThread->pThreadData = pThreadData;
    udIncrementSemaphore(pThread->pCacheSemaphore);
  }
  else
  {
    pThread = udAllocType(udThread, 1, udAF_Zero);
    UD_ERROR_NULL(pThread, udR_MemoryAllocationFailure);
#if DEBUG_CACHE
    udDebugPrintf("Creating udThread %p\n", pThread);
#endif
    pThread->pThreadStarter = pThreadStarter;
    pThread->pThreadData = pThreadData;
    udInterlockedExchange(&pThread->refCount, 1);
# if UDPLATFORM_WINDOWS
    pThread->handle = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)udThread_Bootstrap, pThread, 0, NULL);
#else
    typedef void *(*PTHREAD_START_ROUTINE)(void *);
    pthread_create(&pThread->t, NULL, (PTHREAD_START_ROUTINE)udThread_Bootstrap, pThread);
#endif
  }
#if UDPLATFORM_WINDOWS
  if (pThreadName)
    SetThreadName(GetThreadId(pThread->handle), pThreadName);
#elif (UDPLATFORM_LINUX || UDPLATFORM_ANDROID)
  if (pThreadName)
    pthread_setname_np(pThread->t, pThreadName);
#endif

  if (ppThread)
  {
    // Since we're returning a handle, increment the ref count because the caller is now expected to destroy it
    *ppThread = pThread;
    udInterlockedPreIncrement(&pThread->refCount);
  }
  result = udR_Success;

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2014
void udThread_SetPriority(udThread *pThread, udThreadPriority priority)
{
  if (pThread)
  {
#if UDPLATFORM_WINDOWS
    switch (priority)
    {
    case udTP_Lowest:   SetThreadPriority(pThread->handle, THREAD_PRIORITY_LOWEST); break;
    case udTP_Low:      SetThreadPriority(pThread->handle, THREAD_PRIORITY_BELOW_NORMAL); break;
    case udTP_Normal:   SetThreadPriority(pThread->handle, THREAD_PRIORITY_NORMAL); break;
    case udTP_High:     SetThreadPriority(pThread->handle, THREAD_PRIORITY_ABOVE_NORMAL); break;
    case udTP_Highest:  SetThreadPriority(pThread->handle, THREAD_PRIORITY_HIGHEST); break;
    }
#elif UDPLATFORM_LINUX
    int policy = sched_getscheduler(0);
    int lowest = sched_get_priority_min(policy);
    int highest = sched_get_priority_max(policy);
    int pthreadPrio = (priority * (highest - lowest) / udTP_Highest) + lowest;
    pthread_setschedprio(pThread->t, pthreadPrio);
#else
    udUnused(priority);
#endif
  }
}

// ****************************************************************************
void udThread_Destroy(udThread **ppThread)
{
  if (ppThread)
  {
    udThread *pThread = *ppThread;
    *ppThread = nullptr;
    if (pThread && udInterlockedPreDecrement(&pThread->refCount) == 0)
    {
      UDASSERT(pThread->pCacheSemaphore == nullptr, "pCachedSemaphore non-null");
#if UDPLATFORM_WINDOWS
      CloseHandle(pThread->handle);
#endif
      udFree(pThread);
    }
  }
}

// ****************************************************************************
// Author: Dave Pevreal, July 2018
void udThread_DestroyCached()
{
  while (1)
  {
    bool anyThreadsLeft = false;
    for (int slotIndex = 0; slotIndex < MAX_CACHED_THREADS; ++slotIndex)
    {
      volatile udThread *pThread = s_pCachedThreads[slotIndex];
      if (pThread && pThread->pCacheSemaphore)
      {
        udIncrementSemaphore(pThread->pCacheSemaphore);
        anyThreadsLeft = true;
      }
    }
    if (anyThreadsLeft)
      udYield();
    else
      break;
  }
}

// ****************************************************************************
udResult udThread_Join(udThread *pThread, int waitMs)
{
  if (!pThread)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  UDCOMPILEASSERT(INFINITE == UDTHREAD_WAIT_INFINITE, "Infinite constants don't match");

  DWORD result = WaitForSingleObject(pThread->handle, (DWORD)waitMs);
  if (result)
  {
    if (result == WAIT_TIMEOUT)
      return udR_Timeout;

    return udR_Failure_;
  }
#elif UDPLATFORM_LINUX
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    int result = pthread_join(pThread->t, nullptr);
    if (result)
    {
      if (result == EINVAL)
        return udR_InvalidParameter_;

      return udR_Failure_;
    }
  }
  else
  {
    timespec ts;
    udThread_MsToTimespec(&ts, waitMs);

    int result = pthread_timedjoin_np(pThread->t, nullptr, &ts);
    if (result)
    {
      if (result == ETIMEDOUT)
        return udR_Timeout;

      if (result == EINVAL)
        return udR_InvalidParameter_;

      return udR_Failure_;
    }
  }
#else
  udUnused(waitMs);
  int result = pthread_join(pThread->t, nullptr);
  if (result)
  {
    if (result == EINVAL)
      return udR_InvalidParameter_;

    return udR_Failure_;
  }
#endif

  return udR_Success;
}

struct udSemaphore
{
#if UDPLATFORM_WINDOWS
  CRITICAL_SECTION criticalSection;
  CONDITION_VARIABLE condition;
#else
  pthread_mutex_t mutex;
  pthread_cond_t condition;
#endif

  volatile int count;
  volatile int refCount;
};

// ****************************************************************************
// Author: Samuel Surtees, August 2017
udSemaphore *udCreateSemaphore()
{
  udSemaphore *pSemaphore = udAllocType(udSemaphore, 1, udAF_None);

#if UDPLATFORM_WINDOWS
  InitializeCriticalSection(&pSemaphore->criticalSection);
  InitializeConditionVariable(&pSemaphore->condition);
#else
  pthread_mutex_init(&(pSemaphore->mutex), NULL);
  pthread_cond_init(&(pSemaphore->condition), NULL);
#endif

  pSemaphore->count = 0;
  pSemaphore->refCount = 1;

  return pSemaphore;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, August 2017
void udDestroySemaphore_Internal(udSemaphore *pSemaphore)
{
  if (pSemaphore == nullptr)
    return;

#if UDPLATFORM_WINDOWS
  DeleteCriticalSection(&pSemaphore->criticalSection);
  // CONDITION_VARIABLE doesn't have a delete/destroy function
#else
  pthread_mutex_destroy(&pSemaphore->mutex);
  pthread_cond_destroy(&pSemaphore->condition);
#endif

  udFree(pSemaphore);
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, August 2017
void udLockSemaphore_Internal(udSemaphore *pSemaphore)
{
#if UDPLATFORM_WINDOWS
  EnterCriticalSection(&pSemaphore->criticalSection);
#else
  pthread_mutex_lock(&pSemaphore->mutex);
#endif
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, August 2017
void udUnlockSemaphore_Internal(udSemaphore *pSemaphore)
{
#if UDPLATFORM_WINDOWS
  LeaveCriticalSection(&pSemaphore->criticalSection);
#else
  pthread_mutex_unlock(&pSemaphore->mutex);
#endif
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, August 2017
void udWakeSemaphore_Internal(udSemaphore *pSemaphore)
{
#if UDPLATFORM_WINDOWS
  WakeConditionVariable(&pSemaphore->condition);
#else
  pthread_cond_signal(&pSemaphore->condition);
#endif
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, August 2017
bool udSleepSemaphore_Internal(udSemaphore *pSemaphore, int waitMs)
{
#if UDPLATFORM_WINDOWS
  BOOL retVal = SleepConditionVariableCS(&pSemaphore->condition, &pSemaphore->criticalSection, (waitMs == UDTHREAD_WAIT_INFINITE ? INFINITE : waitMs));
  return (retVal == TRUE);
#else
  int retVal = 0;
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    retVal = pthread_cond_wait(&(pSemaphore->condition), &(pSemaphore->mutex));
  }
  else
  {
    struct timespec ts;
    udThread_MsToTimespec(&ts, waitMs);
    retVal = pthread_cond_timedwait(&(pSemaphore->condition), &(pSemaphore->mutex), &ts);
  }

  return (retVal == 0);
#endif
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
void udDestroySemaphore(udSemaphore **ppSemaphore)
{
  if (ppSemaphore == nullptr || *ppSemaphore == nullptr)
    return;

  udSemaphore *pSemaphore = (*(udSemaphore*volatile*)ppSemaphore);
  if (udInterlockedCompareExchangePointer(ppSemaphore, nullptr, pSemaphore) != pSemaphore)
    return;

  udLockSemaphore_Internal(pSemaphore);
  if (udInterlockedPreDecrement(&pSemaphore->refCount) == 0)
  {
    udDestroySemaphore_Internal(pSemaphore);
  }
  else
  {
    int refCount = pSemaphore->refCount;
    for (int i = 0; i < refCount; ++i)
    {
      ++(pSemaphore->count);
      udWakeSemaphore_Internal(pSemaphore);
    }
    udUnlockSemaphore_Internal(pSemaphore);
  }
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
void udIncrementSemaphore(udSemaphore *pSemaphore, int count)
{
  // Exit the function if the refCount is 0 - It's being destroyed!
  if (pSemaphore == nullptr || pSemaphore->refCount == 0)
    return;

  udInterlockedPreIncrement(&pSemaphore->refCount);
  while (count-- > 0)
  {
    udLockSemaphore_Internal(pSemaphore);
    ++(pSemaphore->count);
    udWakeSemaphore_Internal(pSemaphore);
    udUnlockSemaphore_Internal(pSemaphore);
  }
  udInterlockedPreDecrement(&pSemaphore->refCount);
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs)
{
  // Exit the function if the refCount is 0 - It's being destroyed!
  if (pSemaphore == nullptr || pSemaphore->refCount == 0)
    return -1;

  udInterlockedPreIncrement(&pSemaphore->refCount);
  udLockSemaphore_Internal(pSemaphore);
  bool retVal;
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    retVal = true;
    while (pSemaphore->count == 0)
    {
      retVal = udSleepSemaphore_Internal(pSemaphore, waitMs);

      // If something went wrong, exit the loop
      if (!retVal)
        break;
    }

    if (retVal)
      pSemaphore->count--;
  }
  else
  {
    retVal = udSleepSemaphore_Internal(pSemaphore, waitMs);

    if (retVal)
    {
      // Check for spurious wake-up
      if (pSemaphore->count > 0)
        pSemaphore->count--;
      else
        retVal = false;
    }
  }

  if (udInterlockedPreDecrement(&pSemaphore->refCount) == 0)
  {
    udDestroySemaphore_Internal(pSemaphore);
    return -1;
  }
  else
  {
    udUnlockSemaphore_Internal(pSemaphore);

    // 0 is success, not 0 is failure
    return !retVal;
  }
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
udConditionVariable *udCreateConditionVariable()
{
#if UDPLATFORM_WINDOWS
  CONDITION_VARIABLE *pCondition = udAllocType(CONDITION_VARIABLE, 1, udAF_None);
  InitializeConditionVariable(pCondition);
#else
  pthread_cond_t *pCondition = udAllocType(pthread_cond_t, 1, udAF_None);
  pthread_cond_init(pCondition, NULL);
#endif

  return (udConditionVariable*)pCondition;
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
void udDestroyConditionVariable(udConditionVariable **ppConditionVariable)
{
  udConditionVariable *pCondition = *ppConditionVariable;
  *ppConditionVariable = nullptr;

  // Windows doesn't have a clean-up function
#if !UDPLATFORM_WINDOWS
  pthread_cond_destroy((pthread_cond_t *)pCondition);
#endif

  udFree(pCondition);
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
void udSignalConditionVariable(udConditionVariable *pConditionVariable, int count)
{
  while (count-- > 0)
  {
#if UDPLATFORM_WINDOWS
    CONDITION_VARIABLE *pCondition = (CONDITION_VARIABLE*)pConditionVariable;
    WakeConditionVariable(pCondition);
#else
    pthread_cond_t *pCondition = (pthread_cond_t*)pConditionVariable;
    pthread_cond_signal(pCondition);
#endif
  }
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
int udWaitConditionVariable(udConditionVariable *pConditionVariable, udMutex *pMutex, int waitMs)
{
#if UDPLATFORM_WINDOWS
  CONDITION_VARIABLE *pCondition = (CONDITION_VARIABLE*)pConditionVariable;
  CRITICAL_SECTION *pCriticalSection = (CRITICAL_SECTION*)pMutex;
  BOOL retVal = SleepConditionVariableCS(pCondition, pCriticalSection, (waitMs == UDTHREAD_WAIT_INFINITE ? INFINITE : waitMs));
  return (retVal == TRUE ? 0 : 1); // This isn't (!retVal) for clarity.
#else
  pthread_cond_t *pCondition = (pthread_cond_t*)pConditionVariable;
  pthread_mutex_t *pMutexInternal = (pthread_mutex_t*)pMutex;
  int retVal = 0;
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    retVal = pthread_cond_wait(pCondition, pMutexInternal);
  }
  else
  {
    struct timespec ts;
    udThread_MsToTimespec(&ts, waitMs);
    retVal = pthread_cond_timedwait(pCondition, pMutexInternal, &ts);
  }

  return retVal;
#endif
}

// ****************************************************************************
udMutex *udCreateMutex()
{
#if UDPLATFORM_WINDOWS
  CRITICAL_SECTION *pCriticalSection = udAllocType(CRITICAL_SECTION, 1, udAF_None);
  if (pCriticalSection)
    InitializeCriticalSection(pCriticalSection);
  return (udMutex *)pCriticalSection;
#else
  pthread_mutex_t *mutex = (pthread_mutex_t *)udAlloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return (udMutex*)mutex;
#endif
}

// ****************************************************************************
void udDestroyMutex(udMutex **ppMutex)
{
  if (ppMutex && *ppMutex)
  {
#if UDPLATFORM_WINDOWS
    CRITICAL_SECTION *pCriticalSection = (CRITICAL_SECTION*)(*ppMutex);
    *ppMutex = NULL;
    DeleteCriticalSection(pCriticalSection);
    udFree(pCriticalSection);
#else
    pthread_mutex_t *mutex = (pthread_mutex_t *)(*ppMutex);
    pthread_mutex_destroy(mutex);
    udFree(mutex);
    *ppMutex = nullptr;
#endif
  }
}

// ****************************************************************************
void udLockMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    EnterCriticalSection((CRITICAL_SECTION*)pMutex);
#else
    pthread_mutex_lock((pthread_mutex_t *)pMutex);
#endif
  }
}

// ****************************************************************************
void udReleaseMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    LeaveCriticalSection((CRITICAL_SECTION*)pMutex);
#else
    pthread_mutex_unlock((pthread_mutex_t *)pMutex);
#endif
  }
}


