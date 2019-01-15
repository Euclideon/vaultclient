#include "udPlatform.h"
#include "udThread.h"
#include <stdlib.h>
#include <string.h>

#if UDPLATFORM_WINDOWS
# include <crtdbg.h>
#endif

#define __BREAK_ON_MEMORY_ALLOCATION_FAILURE 0
#define DebugTrackMemoryAlloc(pMemory, size, pFile, line) udUnused(pMemory); udUnused(size); udUnused(pFile); udUnused(line);
#define DebugTrackMemoryFree(pMemory, pFile, line) udUnused(pMemory); udUnused(pFile); udUnused(line);


// ----------------------------------------------------------------------------
// Author: Bryce Kiefer, March 2017
void *_udMemDup(const void *pMemory, size_t size, size_t additionalBytes, udAllocationFlags flags, const char *pFile, int line)
{
  void *pDuplicated = _udAlloc(size + additionalBytes, udAF_None, pFile, line);
  memcpy(pDuplicated, pMemory, size);

  if (flags & udAF_Zero)
    memset((char*)pDuplicated + size, 0, additionalBytes);

  return pDuplicated;
}

#define UD_DEFAULT_ALIGNMENT (8)
// ----------------------------------------------------------------------------
// Author: David Ely
void *_udAlloc(size_t size, udAllocationFlags flags, const char *pFile, int line)
{
#if defined(_MSC_VER)
  void *pMemory = (flags & udAF_Zero) ? _aligned_recalloc_dbg(nullptr, size, 1, UD_DEFAULT_ALIGNMENT, pFile, line) : _aligned_malloc_dbg(size, UD_DEFAULT_ALIGNMENT, pFile, line);
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
void *_udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags, const char *pFile, int line)
{
#if defined(_MSC_VER)
  void *pMemory =  (flags & udAF_Zero) ? _aligned_recalloc_dbg(nullptr, size, 1, alignment, pFile, line) : _aligned_malloc_dbg(size, alignment, pFile, line);

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
#else
# error "Unsupported platform!"
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *_udRealloc(void *pMemory, size_t size, const char *pFile, int line)
{
  DebugTrackMemoryFree(pMemory, pFile, line);
#if defined(_MSC_VER)
  pMemory =  _aligned_realloc_dbg(pMemory, size, UD_DEFAULT_ALIGNMENT, pFile, line);
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
void *_udReallocAligned(void *pMemory, size_t size, size_t alignment, const char *pFile, int line)
{
  DebugTrackMemoryFree(pMemory, pFile, line);
#if defined(_MSC_VER)
  pMemory = _aligned_realloc_dbg(pMemory, size, alignment, pFile, line);
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
    pMemory = _udAllocAligned(size, alignment, udAF_None, pFile, line);
  }
  else
  {
    void *pNewMem = _udAllocAligned(size, alignment, udAF_None, pFile, line);

    size_t *pSize = (size_t*)((uint8_t*)pMemory - sizeof(size_t));
    memcpy(pNewMem, pMemory, *pSize);
    _udFree(pMemory, pFile, line);

    return pNewMem;
  }
#else
# error "Unsupported platform!"
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);


  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void _udFreeInternal(void * pMemory, const char *pFile, int line)
{
  DebugTrackMemoryFree(pMemory, pFile, line);
#if defined(_MSC_VER)
  _aligned_free_dbg(pMemory);
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
