#ifndef vcHotkey_h__
#define vcHotkey_h__

#include "udResult.h"
#include "vcState.h"

enum modifierFlags
{
  vcMOD_Shift = 1024,
  vcMOD_Ctrl = 2048,
  vcMOD_Alt = 4096,
  vcMOD_Super = 8192
};

enum vcBind
{
  vcB_Forward,
  vcB_Backward,
  vcB_Left,
  vcB_Up,
  vcB_Right,
  vcB_Down,
  vcB_Remove,
  vcB_Close,
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
  vcB_MapMode,
  vcB_BindingsInterface,
  vcB_Count
};

namespace vcHotkey
{
  bool IsPressed(vcBind key);
  void GetKeyName(vcBind key, char *pBuffer, uint32_t bufferLen);
  const char* GetBindName(vcBind key);
  vcBind BindFromName(const char* pName);
  int GetMod(int key);
  void Set(vcBind key, int value);
  int Get(vcBind key);

  void DisplayBindings(vcState *pProgramState);
  int DecodeKeyString(const char* pBind);
}

#endif //vcHotkey_h__
