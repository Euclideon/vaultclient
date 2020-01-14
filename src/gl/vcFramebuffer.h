#ifndef vcFramebuffer_h__
#define vcFramebuffer_h__

#include "udMath.h"
#include "vcTexture.h"

struct vcFramebuffer;

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth = nullptr, int level = 0);
void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer);

bool vcFramebuffer_Bind(vcFramebuffer *pFramebuffer);
bool vcFramebuffer_Clear(vcFramebuffer *pFramebuffer, uint32_t colour);

// If pAttachment is configured with vcTCF_AsynchronousRead flag, get results later with vcFramebuffer_ReadPreviousPixels();
// Otherwise will return results immediately.
bool vcFramebuffer_BeginReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels);
bool vcFramebuffer_EndReadPixels(vcFramebuffer *pFramebuffer, vcTexture *pAttachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void *pPixels);

#endif //vcFramebuffer_h__
