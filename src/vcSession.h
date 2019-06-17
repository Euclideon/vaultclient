#ifndef vcSession_h__
#define vcSession_h__

struct vcState;

void vcSession_Login(void *pProgramStatePtr);
void vcSession_Logout(vcState *pProgramState);

void vcSession_UpdateInfo(void *pProgramStatePtr);

#endif // vcSession_h__
