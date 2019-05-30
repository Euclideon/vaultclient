#ifndef vdkConfig_h__
#define vdkConfig_h__

#include "vdkDLLExport.h"
#include "vdkError.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

VDKDLL_API enum vdkError vdkConfig_ForceProxy(const char *pProxyAddress);
VDKDLL_API enum vdkError vdkConfig_SetProxyAuth(const char *pProxyUsername, const char *pProxyPassword);
VDKDLL_API enum vdkError vdkConfig_IgnoreCertificateVerification(bool ignore);

#ifdef __cplusplus
}
#endif

#endif // vdkConfig_h__
