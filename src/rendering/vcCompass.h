#ifndef vcCompass_h__
#define vcCompass_h__

#include "udResult.h"
#include "udMath.h"

#include "vcSettings.h"

struct vcAnchor;

udResult vcCompass_Create(vcAnchor **ppCompass);
udResult vcCompass_Destroy(vcAnchor **ppCompass);

bool vcCompass_Render(vcAnchor *pCompass, vcAnchorStyle anchorStyle, const udDouble4x4 &worldViewProj, const udDouble4 &colour = udDouble4::create(1.0, 0.8431, 0.0, 1.0));

#endif //vcCompass_h__
