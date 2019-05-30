#ifndef vdkError_h__
#define vdkError_h__

#ifdef __cplusplus
extern "C" {
#endif

enum vdkError
{
  vE_Success,

  vE_Failure,
  vE_InvalidParameter,
  vE_InvalidLicense,
  vE_SessionExpired,

  vE_NotAllowed,
  vE_NotSupported,
  vE_NotFound,

  vE_ConnectionFailure,
  vE_MemoryAllocationFailure,
  vE_ServerFailure,
  vE_AuthFailure,
  vE_SecurityFailure,
  vE_ProxyError,
  vE_ProxyAuthRequired,

  vE_OpenFailure,
  vE_ReadFailure,
  vE_WriteFailure,

  vE_OutOfSync,
  vE_ParseError,
  vE_ImageParseError,

  vE_Pending,
  vE_TooManyRequests,
  vE_Cancelled,

  vE_Count
};

#ifdef __cplusplus
}
#endif

#endif // vdkError_h__
