#ifndef vcProxyHelper_h__
#define vcProxyHelper_h__

struct vcState;

#include "vdkError.h"

vdkError vcProxyHelper_AutoDetectProxy(vcState *pProgramState);

vdkError vcProxyHelper_TestProxy(vcState *pProgramState);

vdkError vcProxyHelper_SetUserAndPass(vcState *pProgramState, const char *pProxyUsername, const char *pProxyPassword);

#endif
