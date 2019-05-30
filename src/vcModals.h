#ifndef vcModals_h__
#define vcModals_h__

enum vcModalTypes
{
  // These are handled by DrawModals
  vcMT_LoggedOut,
  vcMT_ProxyAuth,
  vcMT_ReleaseNotes,
  vcMT_About,
  vcMT_NewVersionAvailable,
  vcMT_TileServer,
  vcMT_AddUDS,
  vcMT_ImportProject,
  vcMT_ExportProject,
  vcMT_LoadWatermark,
  vcMT_ImageViewer,
  vcMT_ProjectChangeFailed,
  vcMT_ProjectChangeSucceeded,
  vcMT_ProjectReadOnly,
  vcMT_UnsupportedFile,

  vcMT_Count
};

struct vcState;

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type);
void vcModals_DrawModals(vcState *pProgramState);

#endif //vcModals_h__
