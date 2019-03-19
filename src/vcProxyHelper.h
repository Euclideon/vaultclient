#ifndef vcProxyHelper_h__
#define vcProxyHelper_h__

#include "vdkError.h"

struct vcState;

vdkError vcProxyHelper_AutoDetectProxy(vcState *pProgramState);

vdkError vcProxyHelper_TestProxy(vcState *pProgramState);

vdkError vcProxyHelper_SetUserAndPass(vcState *pProgramState, const char *pProxyUsername, const char *pProxyPassword);

#endif
