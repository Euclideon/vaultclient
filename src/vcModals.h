#ifndef vcModals_h__
#define vcModals_h__

enum vcModalTypes
{
  // These are handled by DrawModals
  vcMT_LoggedOut,
  vcMT_ReleaseNotes,
  vcMT_About,
  vcMT_NewVersionAvailable,
  vcMT_TileServer,
  vcMT_AddUDS,
  vcMT_NotYetImplemented,

  // These ones are unhandled by the DrawModals
  vcMT_ModelProperties,

  vcMT_Count
};

struct vcState;

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type);
bool vcModals_IsOpening(vcState *pProgramState, vcModalTypes type);

void vcModals_DrawTileServer(vcState *pProgramState);

void vcModals_DrawModals(vcState *pProgramState);

#endif //vcModals_h__
