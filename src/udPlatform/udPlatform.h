#ifndef UDPLATFORM_H
#define UDPLATFORM_H

#include "udResult.h"

// An abstraction layer for common functions that differ on various platforms
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN64) || defined(__amd64__) || defined (__arm64__) || defined (__aarch64__)
  //64-bit code
# define UD_64BIT (1)
# define UD_32BIT (0)
# define UD_WORD_SHIFT  6   // 6 bits for 64 bit pointer
# define UD_WORD_BITS   64
# define UD_WORD_BYTES  8
# define UD_WORD_MAX    0x7fffffffffffffffLL
  typedef signed long long udIWord;
  typedef unsigned long long udUWord;
#elif defined(_WIN32) || defined(__i386__)  || defined(__arm__) || defined(__native_client__)
   //32-bit code
# define UD_64BIT (0)
# define UD_32BIT (1)
# define UD_WORD_SHIFT  5   // 5 bits for 32 bit pointer
# define UD_WORD_BITS   32
# define UD_WORD_BYTES  4
# define UD_WORD_MAX    0x7fffffffL
  typedef signed long udIWord;
  typedef unsigned long udUWord;
#else
# error "Unknown architecture (32/64 bit)"
#endif

#if defined(__native_client__)
# include <string.h>
# include <limits.h>
# define UDPLATFORM_NACL 1
# define USE_GLES
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <memory.h>
# define UDPLATFORM_WINDOWS 1
#elif defined(__ANDROID__)
# include <stddef.h>
# include <limits.h>
# include <memory.h>
# define UDPLATFORM_ANDROID 1
#elif defined(__linux__) // TODO: Work out best tag to detect linux here
# include <stddef.h>
# include <limits.h>
# include <memory.h>
# define UDPLATFORM_LINUX 1
#elif defined(__APPLE__)
# include <stddef.h>
# include <limits.h>
# include <memory.h>
# include "TargetConditionals.h"
# if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#  define UDPLATFORM_IOS_SIMULATOR 1
# elif TARGET_OS_IPHONE
#  define UDPLATFORM_IOS 1
# else
#  define UDPLATFORM_OSX 1
# endif
#else
# error "Unknown platform"
#endif

#ifndef UDPLATFORM_WINDOWS
# define UDPLATFORM_WINDOWS 0
#endif

#ifndef UDPLATFORM_ANDROID
# define UDPLATFORM_ANDROID 0
#endif

#ifndef UDPLATFORM_LINUX
# define UDPLATFORM_LINUX 0
#endif

#ifndef UDPLATFORM_OSX
# define UDPLATFORM_OSX 0
#endif

#ifndef UDPLATFORM_IOS_SIMULATOR
# define UDPLATFORM_IOS_SIMULATOR 0
#endif

#ifndef UDPLATFORM_IOS
# define UDPLATFORM_IOS 0
#endif

#ifndef UDPLATFORM_NACL
# define UDPLATFORM_NACL 0
#endif

#if defined(_DEBUG)
# define UD_DEBUG   1
# define UD_RELEASE 0
#else
# define UD_DEBUG   0
# define UD_RELEASE 1
#endif

#if UDPLATFORM_WINDOWS
# define udU64L(x) x##ULL
# define udI64L(x) x##LL
# define UDFORCE_INLINE __forceinline
#elif UDPLATFORM_NACL
# define udU64L(x) x##ULL
# define udI64L(x) x##LL
# define UDFORCE_INLINE inline
#else
# define udU64L(x) x##UL
# define udI64L(x) x##L
# define UDFORCE_INLINE inline
#endif


#if UDPLATFORM_WINDOWS
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#include <Intrin.h>
inline int32_t udInterlockedPreIncrement(volatile int32_t *p)  { return (int32_t)_InterlockedIncrement((long*)p); }
inline int32_t udInterlockedPostIncrement(volatile int32_t *p) { return (int32_t)_InterlockedIncrement((long*)p) - 1; }
inline int32_t udInterlockedPreDecrement(volatile int32_t *p) { return (int32_t)_InterlockedDecrement((long*)p); }
inline int32_t udInterlockedPostDecrement(volatile int32_t *p) { return (int32_t)_InterlockedDecrement((long*)p) + 1; }
inline int32_t udInterlockedExchange(volatile int32_t *dest, int32_t exchange) { return (int32_t)_InterlockedExchange((volatile long*)dest, exchange); }
inline int32_t udInterlockedCompareExchange(volatile int32_t *dest, int32_t exchange, int32_t comparand) { return (int32_t)_InterlockedCompareExchange((volatile long*)dest, exchange, comparand); }
# if UD_32BIT
template <typename T>
inline void *udInterlockedExchangePointer(T * volatile* dest, void *exchange) { return (void*)_InterlockedExchange((volatile long*)dest, (long)exchange); }
template <typename T>
inline void *udInterlockedCompareExchangePointer(T * volatile* dest, void *exchange, void *comparand) { return (void*)_InterlockedCompareExchange((volatile long *)dest, (long)exchange, (long)comparand); }
# else // UD_32BIT
template <typename T>
inline void *udInterlockedExchangePointer(T * volatile* dest, void *exchange) { return _InterlockedExchangePointer((volatile PVOID*)dest, exchange); }
template <typename T>
inline void *udInterlockedCompareExchangePointer(T * volatile* dest, void *exchange, void *comparand) { return _InterlockedCompareExchangePointer((volatile PVOID*)dest, exchange, comparand); }
# endif // UD_32BIT
# define udSleep(x) Sleep(x)
# define udYield() SwitchToThread()
# define UDTHREADLOCAL __declspec(thread)

#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
#include <unistd.h>
#include <sched.h>
inline int32_t udInterlockedPreIncrement(volatile int32_t *p)  { return __sync_add_and_fetch(p, 1); }
inline int32_t udInterlockedPostIncrement(volatile int32_t *p) { return __sync_fetch_and_add(p, 1); }
inline int32_t udInterlockedPreDecrement(volatile int32_t *p)  { return __sync_add_and_fetch(p, -1); }
inline int32_t udInterlockedPostDecrement(volatile int32_t *p) { return __sync_fetch_and_add(p, -1); }
inline int32_t udInterlockedExchange(volatile int32_t *dest, int32_t exchange) { return __sync_lock_test_and_set(dest, exchange); }
inline int32_t udInterlockedCompareExchange(volatile int32_t *dest, int32_t exchange, int32_t comparand) { return __sync_val_compare_and_swap(dest, comparand, exchange); }
template <typename T>
inline void *udInterlockedExchangePointer(T * volatile* dest, void *exchange) { return __sync_lock_test_and_set((void * volatile*)dest, exchange); }
template <typename T>
inline void *udInterlockedCompareExchangePointer(T * volatile* dest, void *exchange, void *comparand) { return __sync_val_compare_and_swap((void * volatile*)dest, comparand, exchange); }
# define udSleep(x) usleep((x)*1000)
# define udYield(x) sched_yield()
# if defined(__INTELLISENSE__)
#   define UDTHREADLOCAL
# else
#   define UDTHREADLOCAL __thread
# endif

#else
#error Unknown platform
#endif
// Helpers to perform various interlocked functions based on the platform-wrapped primitives
inline long udInterlockedAdd(volatile int32_t *p, int32_t amount) { int32_t prev, after; do { prev = *p; after = prev + amount; } while (udInterlockedCompareExchange(p, after, prev) != prev); return after; }
inline ptrdiff_t udInterlockedAddPtrDiff(volatile ptrdiff_t *p, ptrdiff_t amount) { ptrdiff_t prev, after; do { prev = *p; after = prev + amount; } while (udInterlockedCompareExchangePointer((void*volatile*)p, (void*)after, (void*)prev) != (void*)prev); return after; }
inline long udInterlockedMin(volatile int32_t *dest, int32_t newValue) { for (;;) { int32_t oldValue = *dest; if (oldValue < newValue) return oldValue; if (udInterlockedCompareExchange(dest, newValue, oldValue) == oldValue) return newValue; } }
inline long udInterlockedMax(volatile int32_t *dest, int32_t newValue) { for (;;) { int32_t oldValue = *dest; if (oldValue > newValue) return oldValue; if (udInterlockedCompareExchange(dest, newValue, oldValue) == oldValue) return newValue; } }

class udInterlockedInt32
{
public:
  // Get the value
  int32_t Get()                 { return m_value; }
  // Set a new value, returning the previous value
  int32_t Set(int32_t v)        { return udInterlockedExchange(&m_value, v); }
  // Compare exchange to a new value, returning true if set was successful
  bool TestAndSet(int32_t v, int32_t expected) { return udInterlockedCompareExchange(&m_value, v, expected) == expected; }
  // Set to the minimum of the existing or new value
  void SetMin(int32_t v)        { udInterlockedMin(&m_value, v); }
  // Set to the maximum of the existing or new value
  void SetMax(int32_t v)        { udInterlockedMax(&m_value, v); }
  // Add an integer
  void Add(int32_t v)           { udInterlockedAdd(&m_value, v); }
  // Increment operators
  int32_t operator++()          { return udInterlockedPreIncrement(&m_value);       }
  int32_t operator++(int)       { return udInterlockedPostIncrement(&m_value);      }
  int32_t operator--()          { return udInterlockedPreDecrement(&m_value);       }
  int32_t operator--(int)       { return udInterlockedPostDecrement(&m_value);      }
protected:
  volatile int32_t m_value;
};


#define UDALIGN_POWEROF2(x,b) (((x)+(b)-1) & -(b))

#define __MEMORY_DEBUG__  0
#define __BREAK_ON_MEMORY_ALLOCATION_FAILURE 0

#if __MEMORY_DEBUG__
# define IF_MEMORY_DEBUG(x,y) ,x,y
#else
# define IF_MEMORY_DEBUG(x,y)
#endif //  __MEMORY_DEBUG__


#if __cplusplus >= 201103L || _MSC_VER >= 1700
# define UDCPP11 1
#else
# define UDCPP11 0
#endif

#if defined(__clang__) || defined(__GNUC__)
# if !UDCPP11 && !defined(nullptr)
#   define nullptr NULL
# endif // !UDCPP11 && !defined(nullptr)
#endif // defined(__clang__) || defined(__GNUC__)

#if UDCPP11 && !defined(_MSC_VER)
# include <cstddef>
  using std::nullptr_t;
#endif //!defined(_MSC_VER)


#if UDPLATFORM_LINUX || UDPLATFORM_NACL
#include <alloca.h>
#endif

#if defined(_MSC_VER)
  UDFORCE_INLINE void udMemset32(void *pDest, uint32_t val, size_t size) { __stosd((unsigned long*)pDest, val, size); }
#else
  UDFORCE_INLINE void udMemset32(void *pDest, uint32_t val, size_t size) { uint32_t *p = (uint32_t*)pDest; while (size--) *p++ = val; }
#endif

UDFORCE_INLINE void *udSetZero(void *pMemory, size_t size) { memset(pMemory, 0, size); return pMemory; }

enum udAllocationFlags
{
  udAF_None = 0,
  udAF_Zero = 1
};

// Inline of operator to allow flags to be combined and retain type-safety
inline udAllocationFlags operator|(udAllocationFlags a, udAllocationFlags b) { return (udAllocationFlags)(int(a) | int(b)); }

void *_udMemDup(const void *pMemory, size_t size, size_t additionalBytes, udAllocationFlags flags IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__));
#define udMemDup(pMemory, size, additionalBytes, flags) _udMemDup(pMemory, size, additionalBytes, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udAlloc(size_t size, udAllocationFlags flags = udAF_None IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__));
#define udAlloc(size) _udAlloc(size, udAF_None IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__));
#define udAllocAligned(size, alignment, flags) _udAllocAligned(size, alignment, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))

#define udAllocFlags(size, flags) _udAlloc(size, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))
#define udAllocType(type, count, flags) (type*)_udAlloc(sizeof(type) * (count), flags IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udRealloc(void *pMemory, size_t size IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__));
#define udRealloc(pMemory, size) _udRealloc(pMemory, size IF_MEMORY_DEBUG(__FILE__, __LINE__))
#define udReallocType(pMemory, type, count) (type*)_udRealloc(pMemory, sizeof(type) * (count) IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udReallocAligned(void *pMemory, size_t size, size_t alignment IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__));
#define udReallocAligned(pMemory, size, alignment) _udReallocAligned(pMemory, size, alignment IF_MEMORY_DEBUG(__FILE__, __LINE__))

void _udFreeInternal(void *pMemory IF_MEMORY_DEBUG(const char * pFile, int line));
template <typename T>
void _udFree(T *&pMemory IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__))
{
  void *pActualPtr = (void*)pMemory;
  if (pActualPtr && udInterlockedCompareExchangePointer((void**)&pMemory, NULL, pActualPtr) == pActualPtr)
  {
    _udFreeInternal((void*)pActualPtr IF_MEMORY_DEBUG(pFile, line));
  }
}
#define udFree(pMemory) _udFree(pMemory IF_MEMORY_DEBUG(__FILE__, __LINE__))

// A secure free will overwrite the memory (to the size specified) with a random 32-bit
// constant. For example cryptographic functions use this to overwrite key data before freeing
template <typename T>
void _udFreeSecure(T *&pMemory, size_t size IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__))
{
  void *pActualPtr = (void*)pMemory;
  if (pActualPtr && udInterlockedCompareExchangePointer((void**)&pMemory, NULL, pActualPtr) == pActualPtr)
  {
    // Use a simple random value just to avoid filling sensitive memory with a constant
    // that can be used to identify the locations in memory where sensitive data was stored
    int randVal = rand();
    size_t i = size & ~3;
    if (i)
      udMemset32((void*)pActualPtr, randVal, i >> 2); // Fill 32-bits at a time first for performance reasons
    // Fill the last odd bytes
    while (i < size)
      ((uint8_t*)pActualPtr)[i++] = (uint8_t)randVal;
    _udFreeInternal((void*)pActualPtr IF_MEMORY_DEBUG(pFile, line));
  }
}
#define udFreeSecure(pMemory, size) _udFreeSecure(pMemory, size IF_MEMORY_DEBUG(__FILE__, __LINE__))

// Wrapper for alloca with flags. Note flags is OR'd with udAF_None to avoid a cppcat warning
#define udAllocStack(type, count, flags)   ((flags & udAF_Zero) ? (type*)udSetZero(alloca(sizeof(type) * count), sizeof(type) * count) : (type*)alloca(sizeof(type) * count))
#define udFreeStack(pMemory)

#if __MEMORY_DEBUG__
void udMemoryDebugTrackingInit();
void udMemoryOutputLeaks();
void udMemoryOutputAllocInfo(void *pAlloc);
void udMemoryDebugTrackingDeinit();
void udMemoryDebugLogMemoryStats();
void udValidateHeap();
#else
# define udMemoryDebugTrackingInit()
# define udMemoryOutputLeaks()
#define udMemoryOutputAllocInfo(pAlloc)
#define udMemoryDebugTrackingDeinit()
#define udMemoryDebugLogMemoryStats()
#define udValidateHeap()
#endif // __MEMORY_DEBUG__

#if UDPLATFORM_WINDOWS
# define udMemoryBarrier() MemoryBarrier()
#else
# define udMemoryBarrier() __sync_synchronize()
#endif

#define udUnused(x) (void)x

#if defined(_MSC_VER)
# define __FUNC_NAME__ __FUNCTION__
#elif defined(__GNUC__)
# define __FUNC_NAME__ __PRETTY_FUNCTION__
#else
#pragma message ("This platform hasn't setup up __FUNC_NAME__")
# define __FUNC_NAME__ "unknown"
#endif


// Disabled Warnings
#if defined(_MSC_VER)
#pragma warning(disable:4127) // conditional expression is constant
#endif //defined(_MSC_VER)

#if defined(__GNUC__)
# define UD_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100  + __GNUC_PATCHLEVEL__)
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif


#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#define MAKE_FOURCC(a, b, c, d) (  (((uint32_t)(a)) << 0) | (((uint32_t)(b)) << 8) | (((uint32_t)(c)) << 16) | (((uint32_t)(d)) << 24) )
#define UDARRAYSIZE(_array) ( sizeof(_array) / sizeof((_array)[0]) )

udResult udGetTotalPhysicalMemory(uint64_t *pTotalMemory);

#include "udDebug.h"

#endif // UDPLATFORM_H
