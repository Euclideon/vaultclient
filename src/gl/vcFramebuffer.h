#ifndef vcFramebuffer_h__
#define vcFramebuffer_h__

#include "udMath.h"
#include "vcTexture.h"

struct vcFramebuffer;

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth = nullptr, int level = 0);
void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer);

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer);
bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour);

bool vcFramebuffer_ReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, void *pPixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#endif //vcFramebuffer_h__
