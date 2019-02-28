#ifndef UDRESULT_H
#define UDRESULT_H

extern bool g_udBreakOnError;      // Set to true normally, unset and reset around sections of code (eg tests) that aren't unexpected
extern const char *g_udLastErrorFilename;
extern int g_udLastErrorLine;

#if !defined(UD_ERROR_BREAK_ON_ERROR)
# define UD_ERROR_BREAK_ON_ERROR 0  // Set to 1 to have the debugger break on error
#endif

// Some helper macros that assume an exit path label "epilogue" and a local variable "result"

#define UD_ERROR_IF(cond, code)     do { if (cond)                      { result = code; if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_NULL(ptr, code)    do { if (ptr == nullptr)            { result = code; if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_CHECK(funcCall)    do { result = funcCall;                              if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); }   goto epilogue; } } while(0)
#define UD_ERROR_HANDLE()           do {                                                 if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); }   goto epilogue; } } while(0)
#define UD_ERROR_SET(code)          do { result = code;                                  if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue;   } while(0)
#define UD_ERROR_SET_NO_BREAK(code) do { result = code;                                  if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__;                                                                      } goto epilogue;   } while(0)


enum udResult
{
  udR_Success,
  udR_Failure_,
  udR_NothingToDo,
  udR_InternalError,
  udR_NotInitialized_,
  udR_InvalidConfiguration,
  udR_InvalidParameter_,
  udR_OutstandingReferences,
  udR_MemoryAllocationFailure,
  udR_CountExceeded,
  udR_ObjectNotFound,
  udR_ParentNotFound,
  udR_RenderAlreadyInProgress,
  udR_BufferTooSmall,
  udR_VersionMismatch,
  udR_FormatVariationNotSupported,
  udR_ObjectTypeMismatch,
  udR_NodeLimitExceeded,
  udR_BlockLimitExceeded,
  udR_CorruptData,
  udR_InputExhausted,
  udR_OutputExhausted,
  udR_CompressionError,
  udR_Unsupported,
  udR_Timeout,
  udR_AlignmentRequirement,
  udR_DecryptionKeyRequired,
  udR_DecryptionKeyMismatch,
  udR_SignatureMismatch,
  udR_Expired,
  udR_ParseError,
  udR_InternalCryptoError,
  udR_OutOfOrder,
  udR_OutOfRange,
  udR_CalledMoreThanOnce,
  udR_ImageLoadFailure,

  udR_OpenFailure,
  udR_CloseFailure,
  udR_ReadFailure,
  udR_WriteFailure,
  udR_SocketError,

  udR_EventNotHandled,
  udR_DatabaseError,
  udR_ServerError,
  udR_NotAllowed,
  udR_InvalidLicense,
  udR_Pending,
  udR_Cancelled,
  udR_OutOfSync,
  udR_SessionExpired,
  udR_ProxyError,

  udR_Count,

  udR_ForceInt = 0x7FFFFFFF
};

// Return a human-friendly string for a given result code
const char *udResultAsString(udResult result);

#endif // UDRESULT_H
