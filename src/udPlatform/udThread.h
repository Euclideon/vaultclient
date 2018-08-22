#ifndef UDTHREAD_H
#define UDTHREAD_H
#include "udPlatform.h"

// Minimalist MOST BASIC cross-platform thread support
struct udSemaphore;
struct udConditionVariable;
struct udMutex;
struct udThread;
enum udThreadPriority { udTP_Lowest, udTP_Low, udTP_Normal, udTP_High, udTP_Highest };
enum udThreadCreateFlags { udTCF_None }; // For future expansion
#define UDTHREAD_WAIT_INFINITE -1

typedef uint32_t(udThreadStart)(void *data);

// Create a thread object
udResult udThread_Create(udThread **ppThread, udThreadStart *pThreadStarter, void *pThreadData, udThreadCreateFlags flags = udTCF_None, const char *pThreadName = nullptr);

// Set the thread priority
void udThread_SetPriority(udThread *pThread, udThreadPriority priority);

// Destroy a thread, this should be called after the thread has exited (udThread_Join can be used to assist)
void udThread_Destroy(udThread **ppThreadHandle);

// Destroy cached threads
void udThread_DestroyCached();

// Wait for a thread to complete
udResult udThread_Join(udThread *pThread, int waitMs = UDTHREAD_WAIT_INFINITE);

udSemaphore *udCreateSemaphore();
void udDestroySemaphore(udSemaphore **ppSemaphore);
void udIncrementSemaphore(udSemaphore *pSemaphore, int count = 1);
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs = UDTHREAD_WAIT_INFINITE); // Returns zero on success

udConditionVariable *udCreateConditionVariable();
void udDestroyConditionVariable(udConditionVariable **ppConditionVariable);
void udSignalConditionVariable(udConditionVariable *pConditionVariable, int count = 1);
int udWaitConditionVariable(udConditionVariable *pConditionVariable, udMutex *pMutex, int waitMs = UDTHREAD_WAIT_INFINITE); // Returns zero on success

udMutex *udCreateMutex();
void udDestroyMutex(udMutex **ppMutex);
void udLockMutex(udMutex *pMutex);
void udReleaseMutex(udMutex *pMutex);

// A convenience class to lock and unlock based on scope of the variable
class udScopeLock
{
public:
  udScopeLock(udMutex *mutex) { m_mutex = mutex; udLockMutex(m_mutex); }
  ~udScopeLock() { udReleaseMutex(m_mutex); }
protected:
  udMutex *m_mutex;
};

#endif // UDTHREAD_H
