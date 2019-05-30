#ifndef vdkVersion_h__
#define vdkVersion_h__

#include <stdint.h>
#include "vdkDLLExport.h"
#include "vdkError.h"

#ifdef __cplusplus
extern "C" {
#endif

# define VDK_MAJOR_VERSION 0
# define VDK_MINOR_VERSION 2
# define VDK_PATCH_VERSION 0

  struct vdkVersion
  {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
  };

  VDKDLL_API enum vdkError vdkVersion_GetVersion(struct vdkVersion *pVersion);

#ifdef __cplusplus
}
#endif

#endif // vdkVersion_h__
