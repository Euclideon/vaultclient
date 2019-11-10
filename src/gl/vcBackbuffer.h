#ifndef vcBackbuffer_h__
#define vcBackbuffer_h__

#include "udMath.h"
#include "gl/vcShader.h"
#include "gl/vcFramebuffer.h"

struct vcBackbuffer;
struct SDL_Window;

udResult vcBackbuffer_Create(vcBackbuffer **ppWindow, SDL_Window *pHostWindow, int width, int height);
void vcBackbuffer_Destroy(vcBackbuffer **ppWindow);

udResult vcBackbuffer_WindowResized(vcBackbuffer *pWindow, int width, int height);

udResult vcBackbuffer_Bind(vcBackbuffer *pWindow, vcFramebufferClearOperation clearOperation = vcFramebufferClearOperation_All, uint32_t clearColour = 0x0);
udResult vcBackbuffer_Present(vcBackbuffer *pWindow);

#endif // vcBackbuffer_h__
