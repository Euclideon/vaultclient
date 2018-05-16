#include "udPlatform.h"
#include "udThread.h"
#include <stdlib.h>
#include <string.h>
#if __MEMORY_DEBUG__
# if defined(_MSC_VER)
#   pragma warning(disable:4530) //  C++ exception handler used, but unwind semantics are not enabled.
# endif
#include <map>

size_t gAddressToBreakOnAllocation = (size_t)-1;
size_t gAllocationCount = 0;
size_t gAllocationCountToBreakOn = (size_t)-1;
size_t gAddressToBreakOnFree = (size_t)-1;

struct MemTrack
{
  void *pMemory;
  size_t size;
  const char *pFile;
  int line;
  size_t allocationNumber;
};

typedef std::map<size_t, MemTrack> MemTrackMap;

static MemTrackMap *pMemoryTrackingMap;

static udMutex *pMemoryTrackingMutex;

// ----------------------------------------------------------------------------
// Author: David Ely
void udValidateHeap()
{
#if UDPLATFORM_WINDOWS
  UDASSERT(_heapchk() == _HEAPOK, "Heap not valid");
#endif // UDPLATFORM_WINDOWS
}

// ----------------------------------------------------------------------------
// Author: David Ely
void udMemoryDebugTrackingInit()
{
  if (pMemoryTrackingMutex)
  {
    return;
  }
#if UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX
  {
    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (mutex)
    {
      pthread_mutex_init(mutex, NULL);
    }
    pMemoryTrackingMutex = (udMutex*)mutex;
  }
#else
  pMemoryTrackingMutex = udCreateMutex();
#endif
  if (!pMemoryTrackingMutex)
  {
    PRINT_ERROR_STRING("Failed to create memory tracking mutex\n");
  }

  if (!pMemoryTrackingMap)
  {
    pMemoryTrackingMap = new MemTrackMap;
  }
}

// ----------------------------------------------------------------------------
// Author: David Ely
void udMemoryDebugTrackingDeinit()
{
  UDASSERT(pMemoryTrackingMap, "pMemoryTrackingMap is NULL");

  udLockMutex(pMemoryTrackingMutex);

  delete pMemoryTrackingMap;
  pMemoryTrackingMap = NULL;

  udReleaseMutex(pMemoryTrackingMutex);

#if UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX
  pthread_mutex_t *mutex = (pthread_mutex_t *)pMemoryTrackingMutex;
  pthread_mutex_destroy(mutex);
  free(pMemoryTrackingMutex);
  pMemoryTrackingMutex = nullptr;
#else
  udDestroyMutex(&pMemoryTrackingMutex);
#endif
}

// ----------------------------------------------------------------------------
// Author: David Ely
void udMemoryOutputLeaks()
{
  if (pMemoryTrackingMap)
  {
    udScopeLock scopeLock(pMemoryTrackingMutex);

    if (pMemoryTrackingMap->size() > 0)
    {
      udDebugPrintf("%d Allocations\n", uint32_t(gAllocationCount));

      udDebugPrintf("%d Memory leaks detected\n", pMemoryTrackingMap->size());
      for (MemTrackMap::iterator memIt = pMemoryTrackingMap->begin(); memIt != pMemoryTrackingMap->end(); ++memIt)
      {
        const MemTrack &track = memIt->second;
        udDebugPrintf("%s(%d): Allocation %d Address 0x%p, size %u\n", track.pFile, track.line, (void*)track.allocationNumber, track.pMemory, track.size);
      }
    }
    else
    {
      udDebugPrintf("All tracked allocations freed\n");
    }
  }
}

// ----------------------------------------------------------------------------
// Author: David Ely
void udMemoryOutputAllocInfo(void *pAlloc)
{
  udScopeLock scopeLock(pMemoryTrackingMutex);

  const MemTrack &track = (*pMemoryTrackingMap)[size_t(pAlloc)];
  udDebugPrintf("%s(%d): Allocation 0x%p Address 0x%p, size %u\n", track.pFile, track.line, (void*)track.allocationNumber, track.pMemory, track.size);
}

static void DebugTrackMemoryAlloc(void *pMemory, size_t size, const char * pFile, int line)
{
  if (gAddressToBreakOnAllocation == (uint64_t)pMemory || gAllocationCount == gAllocationCountToBreakOn)
  {
    udDebugPrintf("Allocation 0x%p address 0x%p, at File %s, line %d", (void*)gAllocationCount, pMemory, pFile, line);
    __debugbreak();
  }

  if (pMemoryTrackingMutex)
  {
    udScopeLock scopeLock(pMemoryTrackingMutex);
    MemTrack track = { pMemory, size, pFile, line, gAllocationCount };

    if (pMemoryTrackingMap->find(size_t(pMemory)) != pMemoryTrackingMap->end())
    {
      udDebugPrintf("Tracked allocation already exists %p at File %s, line %d", pMemory, pFile, line);
      __debugbreak();
    }

    (*pMemoryTrackingMap)[size_t(pMemory)] = track;
    ++gAllocationCount;
  }
}

// ----------------------------------------------------------------------------
// Author: David Ely
static void DebugTrackMemoryFree(void *pMemory, const char * pFile, int line)
{
  if (gAddressToBreakOnFree == (uint64_t)pMemory)
  {
    udDebugPrintf("Allocation 0x%p address 0x%p, at File %s, line %d", (void*)gAllocationCount, pMemory, pFile, line);
    __debugbreak();
  }

  if (pMemoryTrackingMutex)
  {
    udScopeLock scopelock(pMemoryTrackingMutex);

    MemTrackMap::iterator it = pMemoryTrackingMap->find(size_t(pMemory));
    if (it == pMemoryTrackingMap->end())
    {
      udDebugPrintf("Error freeing address %p at File %s, line %d, did not find a matching allocation", pMemory, pFile, line);
      //__debugbreak();
      goto epilogue;
    }
    UDASSERT(it->second.pMemory == (pMemory), "Pointers didn't match");
    pMemoryTrackingMap->erase(it);
  }

epilogue:
  return;
}

void udMemoryDebugLogMemoryStats()
{
  udDebugPrintf("Memory Stats\n");

  size_t totalMemory = 0;
  for (MemTrackMap::iterator memIt = pMemoryTrackingMap->begin(); memIt != pMemoryTrackingMap->end(); ++memIt)
  {
    const MemTrack &track = memIt->second;
    totalMemory += track.size;
  }

  udDebugPrintf("Total allocated Memory %llu\n", totalMemory);
}

#else
# define DebugTrackMemoryAlloc(pMemory, size, pFile, line)
# define DebugTrackMemoryFree(pMemory, pFile, line)
#endif // __MEMORY_DEBUG__


// ----------------------------------------------------------------------------
// Author: Bryce Kiefer, March 2017
void *_udMemDup(const void *pMemory, size_t size, size_t additionalBytes, udAllocationFlags flags IF_MEMORY_DEBUG(const char * pFile, int line))
{
  void *pDuplicated = _udAlloc(size + additionalBytes, udAF_None IF_MEMORY_DEBUG(pFile, line));
  memcpy(pDuplicated, pMemory, size);

  if (flags & udAF_Zero)
    memset((char*)pMemory + size, 0, additionalBytes);

  return pDuplicated;
}

#define UD_DEFAULT_ALIGNMENT (8)
// ----------------------------------------------------------------------------
// Author: David Ely
void *_udAlloc(size_t size, udAllocationFlags flags IF_MEMORY_DEBUG(const char * pFile, int line))
{
#if defined(_MSC_VER)
  void *pMemory = (flags & udAF_Zero) ? _aligned_recalloc(nullptr, size, 1, UD_DEFAULT_ALIGNMENT) : _aligned_malloc(size, UD_DEFAULT_ALIGNMENT);
#else
  void *pMemory = (flags & udAF_Zero) ? calloc(size, 1) : malloc(size);
#endif // defined(_MSC_VER)

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("udAlloc failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *_udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags IF_MEMORY_DEBUG(const char * pFile, int line))
{
#if defined(_MSC_VER)
  void *pMemory =  (flags & udAF_Zero) ? _aligned_recalloc(nullptr, size, 1, alignment) : _aligned_malloc(size, alignment);

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("udAllocAligned failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE

#elif UDPLATFORM_NACL
  void *pMemory =  (flags & udAF_Zero) ? calloc(size, 1) : malloc(size);

#elif defined(__GNUC__)
  if (alignment < sizeof(size_t))
  {
    alignment = sizeof(size_t);
  }
  void *pMemory;
  int err = posix_memalign(&pMemory, alignment, size + alignment);
  if (err != 0)
  {
	  return nullptr;
  }

  if (flags & udAF_Zero)
  {
    memset(pMemory, 0, size);
  }
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *_udRealloc(void *pMemory, size_t size IF_MEMORY_DEBUG(const char * pFile, int line))
{
#if __MEMORY_DEBUG__
  if (pMemory)
  {
    DebugTrackMemoryFree(pMemory, pFile, line);
  }
#endif
#if defined(_MSC_VER)
  pMemory =  _aligned_realloc(pMemory, size, UD_DEFAULT_ALIGNMENT);
#else
  pMemory = realloc(pMemory, size);
#endif // defined(_MSC_VER)

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("udRealloc failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);


  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *_udReallocAligned(void *pMemory, size_t size, size_t alignment IF_MEMORY_DEBUG(const char * pFile, int line))
{
#if __MEMORY_DEBUG__
  if (pMemory)
  {
    DebugTrackMemoryFree(pMemory, pFile, line);
  }
#endif

#if defined(_MSC_VER)
  pMemory = _aligned_realloc(pMemory, size, alignment);
#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("udReallocAligned failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
#elif UDPLATFORM_NACL
  pMemory = realloc(pMemory, size);
#elif defined(__GNUC__)
  if (!pMemory)
  {
    pMemory = udAllocAligned(size, alignment, udAF_None IF_MEMORY_DEBUG(pFile, line));
  }
  else
  {
    void *pNewMem = udAllocAligned(size, alignment, udAF_None IF_MEMORY_DEBUG(pFile, line));

    size_t *pSize = (size_t*)((uint8_t*)pMemory - sizeof(size_t));
    memcpy(pNewMem, pMemory, *pSize);
    udFree(pMemory IF_MEMORY_DEBUG(pFile, line));

    return pNewMem;
  }
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);


  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void _udFreeInternal(void * pMemory IF_MEMORY_DEBUG(const char * pFile, int line))
{
  DebugTrackMemoryFree(pMemory, pFile, line);
#if defined(_MSC_VER)
  _aligned_free(pMemory);
#else
  free(pMemory);
#endif // defined(_MSC_VER)
}

// ----------------------------------------------------------------------------
// Author: David Ely
udResult udGetTotalPhysicalMemory(uint64_t *pTotalMemory)
{
#if UDPLATFORM_WINDOWS
  MEMORYSTATUSEX memorySatusEx;
  memorySatusEx.dwLength = sizeof(memorySatusEx);
  BOOL result = GlobalMemoryStatusEx(&memorySatusEx);
  if (result)
  {
    *pTotalMemory = memorySatusEx.ullTotalPhys;
    return udR_Success;
  }

  *pTotalMemory = 0;
  return udR_Failure_;

#elif UDPLATFORM_LINUX

// see http://nadeausoftware.com/articles/2012/09/c_c_tip_how_get_physical_memory_size_system for
// explanation.

#if !defined(_SC_PHYS_PAGES)
#error "_SC_PHYS_PAGES is not defined"
#endif

#if !defined(_SC_PAGESIZE)
#error "_SC_PAGESIZE is not defined"
#endif

  *pTotalMemory = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGESIZE);
  return udR_Success;

#else
  *pTotalMemory = 0;
  return udR_Success;
#endif
}
