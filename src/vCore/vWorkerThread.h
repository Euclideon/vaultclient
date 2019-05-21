#ifndef vWorkerThread_h__
#define vWorkerThread_h__

#include <stdint.h>
#include "udMath.h"
#include "vSafeDeque.h"

// A basic system to queue tasks in the background
// The worker thread system does not rely on having a udCDKProject*

typedef void (vWorkerThreadCallback)(void *pUserData);
struct vWorkerThreadPool;
struct udThread;

struct vWorkerThreadTask
{
  vWorkerThreadCallback *pFunction;
  vWorkerThreadCallback *pPostFunction; // run on main thread
  void *pDataBlock;
  bool freeDataBlock;
};

struct vWorkerThreadData
{
  vWorkerThreadPool *pPool;
  udThread *pThread;

  bool isActive;
};

struct vWorkerThreadPool
{
  vSafeDeque<vWorkerThreadTask> *pQueuedTasks;
  vSafeDeque<vWorkerThreadTask> *pQueuedPostTasks;

  udSemaphore *pSemaphore;
  volatile int32_t runningThreads;

  int32_t totalThreads;
  vWorkerThreadData *pThreadData;

  bool isRunning;
};

// The theory with starting threads is to keep starting for each system that needs it
//   it internally increments and onces all started are shutdown it cleans up
void vWorkerThread_StartThreads(vWorkerThreadPool **ppPool, uint8_t maxThreads = 4);
void vWorkerThread_Shutdown(vWorkerThreadPool **ppPool, bool waitForCompletion = true);

void vWorkerThread_DoPostWork(vWorkerThreadPool *pPool);

// Adds a function to run on a background thread, optionally with userdata. If clearMemory is true, it will call udFree on pUserData after running
void vWorkerThread_AddTask(vWorkerThreadPool *pPool, vWorkerThreadCallback *pFunc, void *pUserData = nullptr, bool clearMemory = true, vWorkerThreadCallback *pPostFunction = nullptr);

#endif // vWorkerThread_h__
