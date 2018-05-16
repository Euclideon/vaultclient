#ifndef UDDEBUG_H
#define UDDEBUG_H

#include "udPlatform.h"
#include "udResult.h"

// Outputs a string to debug console
void udDebugPrintf(const char *format, ...);

// Optional, set this pointer to redirect debug printfs
extern void (*gpudDebugPrintfOutputCallback)(const char *pString);

class udTrace
{
public:
  udTrace(const char *, int traceLevel);
  ~udTrace();
  static void Message(const char *pFormat, ...); // Print a message at the indentation level of the current trace
  static void ShowCallstack();

  const char *functionName;
  udTrace *next;
  bool entryPrinted;
  static UDTHREADLOCAL udTrace *head;
  static UDTHREADLOCAL int depth;
  static int GetThreadId();

private:
  static UDTHREADLOCAL int threadId;
};

template <typename T>
inline void udTrace_Variable(const char *pName, const T *value, int line)     { udTrace::Message("%s = %p (line#=%d)", pName, value, line); }
inline void udTrace_Variable(const char *pName, udResult value, int line)     { udTrace::Message("%s = udR_%s (line#=%d)", pName, udResultAsString(value), line); }
inline void udTrace_Variable(const char *pName, const char *value, int line)  { udTrace::Message("%s = '%s' (line#=%d)", pName, value, line); }
inline void udTrace_Variable(const char *pName, int value, int line)          { udTrace::Message("%s = %d/0x%x (int, line#=%d)", pName, value, value, line); }
inline void udTrace_Variable(const char *pName, int64_t value, int line)      { udTrace::Message("%s = %lld/0x%llx (int64, line#=%d)", pName, value, value, line); }
inline void udTrace_Variable(const char *pName, unsigned value, int line)     { udTrace::Message("%s = %u/0x%x (int, line#=%d)", pName, value, value, line); }
inline void udTrace_Variable(const char *pName, uint64_t value, int line)     { udTrace::Message("%s = %llu/0x%llx (int64, line#=%d)", pName, value, value, line); }
inline void udTrace_Variable(const char *pName, float value, int line)        { udTrace::Message("%s = %f (float, line#=%d)", pName, value, line); }
inline void udTrace_Variable(const char *pName, double value, int line)       { udTrace::Message("%s = %lf (double line#=%d)", pName, value, line); }
inline void udTrace_Variable(const char *pName, bool value, int line)         { udTrace::Message("%s = %s (line#=%d)", pName, value ? "true" : "false", line); }
void udTrace_Memory(const char *pName, const void *pMem, int length, int line = 0);


#if UD_DEBUG

# define UDTRACE_ON     0     // Set to 1 to enable, set to 2 for printf on entry/exit of every function
# define UDASSERT_ON    1
# define UDRELASSERT_ON 1

#elif UD_RELEASE

#if !defined(UDTRACE_ON)
# define UDTRACE_ON     0
# endif
# define UDASSERT_ON    0
# define UDRELASSERT_ON 1

#endif

#if defined(__GNUC__)
# if UD_DEBUG
#   include <signal.h>
#   if !defined(__debugbreak)
#     define __debugbreak() raise(SIGTRAP)
#   endif // !defined(__debugbreak)
#   if !defined(DebugBreak)
#     define DebugBreak() raise(SIGTRAP)
#   endif // !defined(DebugBreak)
# else
#   if !defined(__debugbreak)
#     define __debugbreak() {}
#   endif // !defined(__debugbreak)
#   if !defined(DebugBreak)
#     define DebugBreak() {}
#   endif // !defined(DebugBreak)
# endif
#endif


#if UDTRACE_ON
# define UDTRACE() udTrace __udtrace##__LINE__(__FUNCTION__, UDTRACE_ON)
# define UDTRACE_LINE() udTrace::Message("Line %d\n", __LINE__)
# define UDTRACE_SCOPE(id) udTrace __udtrace##__LINE__(id, UDTRACE_ON)
# define UDTRACE_MESSAGE(format,...) udTrace::Message(format,__VA_ARGS__)
# define UDTRACE_VARIABLE(var) udTrace_Variable(#var, var, __LINE__)
# define UDTRACE_MEMORY(var,length) udTrace_Memory(#var, var, length, __LINE__)
#else
# define UDTRACE()
# define UDTRACE_LINE()
# define UDTRACE_SCOPE(id)
# define UDTRACE_MESSAGE(format,...)
# define UDTRACE_VARIABLE(var)
# define UDTRACE_MEMORY(var,length)
#endif // UDTRACE_ON

// TODO: Make assertion system handle pop-up window where possible
#if UDASSERT_ON
# define UDASSERT(condition, ...) do { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(__VA_ARGS__); DebugBreak(); udDebugPrintf("\n"); } } while (0)
# define IF_UDASSERT(x) x
#else
# define UDASSERT(condition, ...) do { } while (0) // TODO: Make platform-specific __assume(condition)
# define IF_UDASSERT(x)
#endif // UDASSERT_ON

#if UDRELASSERT_ON
# define UDRELASSERT(condition, ...) do { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(__VA_ARGS__); DebugBreak(); udDebugPrintf("\n"); } } while(0)
# define IF_UDRELASSERT(x) x
#else
# define UDRELASSERT(condition, ...) do { } while (0) // TODO: Make platform-specific __assume(condition)
# define IF_UDRELASSERT(x)
#endif //UDRELASSERT_ON

#if UDCPP11
# define UDCOMPILEASSERT(a_condition, a_error) static_assert(a_condition, a_error)
#else
# if UDPLATFORM_WINDOWS
#   define UDCOMPILEASSERT(a_condition, a_error) typedef char UDCOMPILEASSERT##__LINE__[(a_condition)?1:-1]
# else
#   define UDCOMPILEASSERT(a_condition, a_error) typedef char UDCOMPILEASSERT##__LINE__[(a_condition)?1:-1] __attribute__ ((unused))
# endif
#endif
#define UDUNREACHABLE() UDASSERT(false, "Unreachable!")

#if UD_DEBUG
# define OUTPUT_ERROR_STRINGS (1)
#else
# define OUTPUT_ERROR_STRINGS (0)
#endif

#define UD_BREAK_ON_ERROR_STRING (0)

#if OUTPUT_ERROR_STRINGS
# if UD_BREAK_ON_ERROR_STRING
#   define __breakOnErrorString() __debugbreak()
# else
#   define __breakOnErrorString()
# endif
# define PRINT_ERROR_STRING(...) do {udDebugPrintf("%s : ", __FUNCTION__); udDebugPrintf(__VA_ARGS__); __breakOnErrorString(); } while (false)

#else
# define PRINT_ERROR_STRING(...)
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define UDCHECKRESULT __attribute__ ((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define UDCHECKRESULT _Check_return_
#else
#define UDCHECKRESULT
#endif

#endif // UDDEBUG_H
