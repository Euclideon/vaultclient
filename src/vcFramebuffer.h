#ifndef vcFramebuffer_h__
#define vcFramebuffer_h__

#include "udPlatform/udMath.h"
#include "vcTexture.h"

struct vcFramebuffer
{
  GLuint id;

  vcTexture *pAttachments[1];
  vcTexture *pDepth; // optional
};

bool vcFramebuffer_Create(vcFramebuffer **ppFramebuffer, vcTexture *pTexture, vcTexture *pDepth = nullptr, int level = 0);
void vcFramebuffer_Destroy(vcFramebuffer **ppFramebuffer);


#endif //vcFramebuffer_h__
