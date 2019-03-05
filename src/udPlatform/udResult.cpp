#include "udResult.h"
#include "udDebug.h"

#define RESULTINFO(x) { #x+4, x }
#define RESULTINFO_CUSTOM(x,str) { str, x }

const char *g_udLastErrorFilename = nullptr;
int g_udLastErrorLine = 0;
bool g_udBreakOnError = true;

struct udResultInfo
{
  const char *pString;
  udResult result; // Used to make sure the array is correctly filled out
};

udResultInfo udResultInfoArray[] =
{
  RESULTINFO(udR_Success),
  RESULTINFO(udR_Failure_),
  RESULTINFO(udR_NothingToDo),
  RESULTINFO(udR_InternalError),
  RESULTINFO(udR_NotInitialized_),
  RESULTINFO(udR_InvalidConfiguration),
  RESULTINFO(udR_InvalidParameter_),
  RESULTINFO(udR_OutstandingReferences),
  RESULTINFO(udR_MemoryAllocationFailure),
  RESULTINFO(udR_CountExceeded),
  RESULTINFO(udR_ObjectNotFound),
  RESULTINFO(udR_ParentNotFound),
  RESULTINFO(udR_RenderAlreadyInProgress),
  RESULTINFO(udR_BufferTooSmall),
  RESULTINFO(udR_VersionMismatch),
  RESULTINFO(udR_FormatVariationNotSupported),
  RESULTINFO(udR_ObjectTypeMismatch),
  RESULTINFO(udR_NodeLimitExceeded),
  RESULTINFO(udR_BlockLimitExceeded),
  RESULTINFO(udR_CorruptData),
  RESULTINFO(udR_InputExhausted),
  RESULTINFO(udR_OutputExhausted),
  RESULTINFO(udR_CompressionError),
  RESULTINFO(udR_Unsupported),
  RESULTINFO(udR_Timeout),
  RESULTINFO(udR_AlignmentRequirement),
  RESULTINFO(udR_DecryptionKeyRequired),
  RESULTINFO(udR_DecryptionKeyMismatch),
  RESULTINFO(udR_SignatureMismatch),
  RESULTINFO(udR_Expired),
  RESULTINFO(udR_ParseError),
  RESULTINFO(udR_InternalCryptoError),
  RESULTINFO(udR_OutOfOrder),
  RESULTINFO(udR_OutOfRange),
  RESULTINFO(udR_CalledMoreThanOnce),
  RESULTINFO(udR_ImageLoadFailure),
  RESULTINFO(udR_StreamerNotInitialised),

  RESULTINFO(udR_OpenFailure),
  RESULTINFO(udR_CloseFailure),
  RESULTINFO(udR_ReadFailure),
  RESULTINFO(udR_WriteFailure),
  RESULTINFO(udR_SocketError),

  RESULTINFO(udR_EventNotHandled),
  RESULTINFO(udR_DatabaseError),
  RESULTINFO(udR_ServerError),
  RESULTINFO(udR_NotAllowed),
  RESULTINFO(udR_InvalidLicense),
  RESULTINFO(udR_Pending),
  RESULTINFO(udR_Cancelled),
  RESULTINFO(udR_OutOfSync),
  RESULTINFO(udR_SessionExpired),
  RESULTINFO(udR_ProxyError),
};

UDCOMPILEASSERT(sizeof(udResultInfoArray) == (udR_Count*sizeof(udResultInfoArray[0])), "Result code not entered in strings table");


// ****************************************************************************
// Author: Dave Pevreal, March 2014
const char *udResultAsString(udResult result)
{
  if (result < 0  || result > udR_Count)
    return "Unknown error";
  if (result == udR_Count)
  {
    // Count is a special case to test the result string array
    for (int i = 0; i < udR_Count; ++i)
    {
      if (udResultInfoArray[i].result != i)
        return 0; // Signal that the array is out of sync
    }
    return ""; // Empty string signals array looks ok
  }
  return udResultInfoArray[result].pString;
}
