#ifndef vcFeatures_h__
#define vcFeatures_h__

#include "udPlatform.h"

#ifndef VC_SIMPLEUI
# if UDPLATFORM_EMSCRIPTEN || UDPLATFORM_ANDROID || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#   define VC_SIMPLEUI 1
# else
#   define VC_SIMPLEUI 0
# endif
#endif

#define VC_HASCONVERT (!VC_SIMPLEUI)
#define VC_HASNATIVEFILEPICKER UDPLATFORM_WINDOWS

#define VC_HAS_WINDOWSMEDIAFOUNDATION UDPLATFORM_WINDOWS

#endif //vcFeatures_h__
