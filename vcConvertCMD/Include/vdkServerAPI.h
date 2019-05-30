#ifndef vdkServerAPI_h__
#define vdkServerAPI_h__

#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vdkContext;

VDKDLL_API enum vdkError vdkServerAPI_Query(struct vdkContext *pContext, const char *pAPIAddress, const char *pJSON, const char **ppResult);
VDKDLL_API enum vdkError vdkServerAPI_ReleaseResult(struct vdkContext *pContext, const char **ppResult);

#ifdef __cplusplus
}
#endif

#endif // vaultServerAPI_h__
