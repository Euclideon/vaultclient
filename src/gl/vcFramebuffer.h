#ifndef vcFramebuffer_h__
#define vcFramebuffer_h__

#include "udMath.h"
#include "vcTexture.h"

struct vcFramebuffer;

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth = nullptr, uint32_t level = 0);
void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer);

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer);
bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour);

#endif //vcFramebuffer_h__
