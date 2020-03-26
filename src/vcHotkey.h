#ifndef vcHotkey_h__
#define vcHotkey_h__

#include "udResult.h"
#include "vcState.h"

enum modifierFlags
{
  vcMOD_Mask = 511,
  vcMOD_Shift = 512,
  vcMOD_Ctrl = 1024,
  vcMOD_Alt = 2048,
  vcMOD_Super = 4096
};

enum vcBind
{
  vcB_Forward,
  vcB_Backward,
  vcB_Left,
  vcB_Right,
  vcB_Up,
  vcB_Down,
  vcB_DecreaseCameraSpeed,
  vcB_IncreaseCameraSpeed,

  // Other Bindings
  vcB_Remove,
  vcB_Cancel,
  vcB_LockAltitude,
  vcB_GizmoTranslate,
  vcB_GizmoRotate,
  vcB_GizmoScale,
  vcB_GizmoLocalSpace,
  vcB_Fullscreen,
  vcB_RenameSceneItem,
  vcB_Save,
  vcB_Load,
  vcB_AddUDS,
  vcB_BindingsInterface,
  vcB_Undo,
  vcB_TakeScreenshot,
  vcB_ToggleSceneExplorer,

  vcB_ToggleSelectTool,
  vcB_ToggleMeasureLineTool,
  vcB_ToggleMeasureAreaTool,

  vcB_Count
};

namespace vcHotkey
{
  void ClearState();
  bool IsDown(int keyNum);
  bool IsDown(vcBind key);
  bool IsPressed(int keyNum, bool checkMod = true);
  bool IsPressed(vcBind key, bool checkMod = true);

  void GetKeyName(vcBind key, char *pBuffer, uint32_t bufferLen);
  template <size_t N>
  void GetKeyName(vcBind key, char(&buffer)[N]) { GetKeyName(key, buffer, N); };

  const char* GetBindName(vcBind key);
  vcBind BindFromName(const char* pName);
  int GetMod(int key);
  void Set(vcBind key, int value);
  int Get(vcBind key);

  void DisplayBindings(vcState *pProgramState);
  int DecodeKeyString(const char* pBind);
}

#endif //vcHotkey_h__
