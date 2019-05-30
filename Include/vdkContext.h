#ifndef vdkContext_h__
#define vdkContext_h__

#include "vdkDLLExport.h"
#include "vdkError.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vdkContext;

enum vdkLicenseType
{
  vdkLT_Render, // Allows access to the rendering and model modules
  vdkLT_Convert, // Allows access to convert models to a UD model

  vdkLT_Count // Total number of license types
};

struct vdkLicenseInfo
{
  int64_t queuePosition; // -1 = Have License, 0 = Not In Queue, >0 is Queue Position

  // Have License
  char licenseKey[37]; // 36 Characters + null terminator
  uint64_t expiresTimestamp;
};

VDKDLL_API enum vdkError vdkContext_Connect(struct vdkContext **ppContext, const char *pURL, const char *pApplicationName, const char *pUsername, const char *pPassword);
VDKDLL_API enum vdkError vdkContext_Disconnect(struct vdkContext **ppContext);

VDKDLL_API enum vdkError vdkContext_RequestLicense(struct vdkContext *pContext, enum vdkLicenseType licenseType);
VDKDLL_API enum vdkError vdkContext_CheckLicense(struct vdkContext *pContext, enum vdkLicenseType licenseType);
VDKDLL_API enum vdkError vdkContext_GetLicenseInfo(struct vdkContext *pContext, enum vdkLicenseType licenseType, struct vdkLicenseInfo *pInfo);

VDKDLL_API enum vdkError vdkContext_KeepAlive(struct vdkContext *pContext);

#ifdef __cplusplus
}
#endif

#endif // vdkContext_h__
