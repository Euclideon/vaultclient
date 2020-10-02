#include "vcFileError.h"
#include "vcModals.h"

void vcFileError_AddError(vcState *pProgramState, const vcState::ErrorItem &error)
{
  pProgramState->errorItems.PushBack(error);
  vcModals_OpenModal(pProgramState, vcMT_UnsupportedFile);
}
