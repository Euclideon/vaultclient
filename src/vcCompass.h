#ifndef vcCompass_h__
#define vcCompass_h__

#include "udPlatform/udResult.h"
#include "udPlatform/udMath.h"

struct vcCompass;

udResult vcCompass_Create(vcCompass **ppCompass);
udResult vcCompass_Destroy(vcCompass **ppCompass);

udResult vcCompass_Render(vcCompass *pCompass, udDouble4x4 worldViewProj);

#endif //vcCompass_h__
