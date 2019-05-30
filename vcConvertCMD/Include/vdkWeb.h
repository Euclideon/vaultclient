#ifndef vdkWeb_h__
#define vdkWeb_h__

#include <stdint.h>
#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

  enum vdkWebMethod
  {
    vdkWM_HEAD = 0x0,
    vdkWM_GET = 0x1,
    vdkWM_POST = 0x2,
  };

  struct vdkWebOptions
  {
    enum vdkWebMethod method;
    const uint8_t *pPostData;
    uint64_t postLength;
    uint64_t rangeBegin;
    uint64_t rangeEnd;
    const char *pAuthUsername;
    const char *pAuthPassword;
  };

  VDKDLL_API enum vdkError vdkWeb_Request(const char *pURL, const char **ppResponse, uint64_t *pResponseLength, int *pHTTPResponseCode);
  VDKDLL_API enum vdkError vdkWeb_RequestAdv(const char *pURL, struct vdkWebOptions options, const char **ppResponse, uint64_t *pResponseLength, int *pHTTPResponseCode);

  VDKDLL_API enum vdkError vdkWeb_ReleaseResponse(const char **ppResponse);

#ifdef __cplusplus
}
#endif

#endif // vdkWeb_h__
