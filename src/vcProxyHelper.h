#ifndef vcProxyHelper_h__
#define vcProxyHelper_h__

#include "udError.h"

struct vcState;

udError vcProxyHelper_AutoDetectProxy(vcState *pProgramState);

udError vcProxyHelper_TestProxy(vcState *pProgramState);

udError vcProxyHelper_SetUserAndPass(vcState *pProgramState, const char *pProxyUsername, const char *pProxyPassword);

#endif
