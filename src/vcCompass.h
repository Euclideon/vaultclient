#ifndef vcCompass_h__
#define vcCompass_h__

#include "udPlatform/udResult.h"
#include "udPlatform/udMath.h"

struct vcCompass;

udResult vcCompass_Create(vcCompass **ppCompass);
udResult vcCompass_Destroy(vcCompass **ppCompass);

udResult vcCompass_Render(vcCompass *pCompass, const udDouble4x4 &worldViewProj, const udDouble4 &colour = udDouble4::create(1.0, 0.8431, 0.0, 1.0));

#endif //vcCompass_h__
