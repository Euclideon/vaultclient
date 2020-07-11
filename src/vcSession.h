#ifndef vcSession_h__
#define vcSession_h__

#include "udMath.h"

struct vcState;

void vcSession_Login(void *pProgramStatePtr);
void vcSession_Logout(vcState *pProgramState);

void vcSession_Resume(vcState *pProgramState);

void vcSession_UpdateInfo(void *pProgramStatePtr);

void vcSession_CleanupSession(vcState *pProgramState);

#endif // vcSession_h__
