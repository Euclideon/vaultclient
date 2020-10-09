#include "vcError.h"
#include "vcState.h"
#include "vcModals.h"

void vcError_AddError(vcState *pProgramState, const ErrorItem &error)
{
  pProgramState->errorItems.PushBack(error);
  vcModals_OpenModal(pProgramState, vcMT_ErrorInformation);
}
