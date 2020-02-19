#ifndef vcModals_h__
#define vcModals_h__

enum vcModalTypes
{
  // These are handled by DrawModals
  vcMT_LoggedOut,
  vcMT_ProxyAuth,
  vcMT_TileServer,
  vcMT_AddSceneItem,
  vcMT_ExportProject,
  vcMT_ImageViewer,
  vcMT_ProjectChange,
  vcMT_ProjectReadOnly,
  vcMT_UnsupportedFile,
  vcMT_Profile,
  vcMT_Convert,

  vcMT_Count
};

struct vcState;

void vcModals_OpenModal(vcState *pProgramState, vcModalTypes type);
void vcModals_DrawModals(vcState *pProgramState);

// Returns true if its safe to write- if exists the user is asked if it can be overriden
bool vcModals_OverwriteExistingFile(const char *pFilename);

#endif //vcModals_h__
