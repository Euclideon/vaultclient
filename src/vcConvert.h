#ifndef vcConvert_h__
#define vcConvert_h__

struct vcState;
struct vcConvertContext;

void vcConvert_Init(vcState *pProgramState);
void vcConvert_Deinit(vcState *pProgramState);

void vcConvert_ShowUI(vcState *pProgramState);

bool vcConvert_AddFile(vcState *pProgramState, const char *pFilename);

#endif // !vcConvert_h__
