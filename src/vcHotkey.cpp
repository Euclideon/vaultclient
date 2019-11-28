#include "vcHotkey.h"

#include "vcStrings.h"
#include "vcStringFormat.h"

#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL2/SDL.h>

namespace vcHotkey
{
  static int keyBinds[vcB_Count] = {};

  const char* bindNames[] =
  {
    "CameraForward",
    "CameraBackward",
    "CameraLeft",
    "CameraUp",
    "CameraRight",
    "CameraDown",
    "Remove",
    "Close",
    "LockAltitude",
    "GizmoTranslate",
    "GizmoRotate",
    "GizmoScale",
    "GizmoLocalSpace",
    "Fullscreen",
    "RenameSceneItem",
    "Save",
    "Load",
    "AddUDS",
    "MapMode",
    "BindingsInterface"
  };

  static const char *pModText[] =
  {
    "Shift",
    "Ctrl",
    "Alt",
    "Super"
  };

  static const modifierFlags modList[] =
  {
    vcMOD_Shift,
    vcMOD_Ctrl,
    vcMOD_Alt,
    vcMOD_Super
  };

  bool IsPressed(vcBind key)
  {
    ImGuiIO io = ImGui::GetIO();

    int keyNum = keyBinds[key];

    if ((keyNum & vcMOD_Shift) && !io.KeyShift)
      return false;
    if ((keyNum & vcMOD_Ctrl) && !io.KeyCtrl)
      return false;
    if ((keyNum & vcMOD_Alt) && !io.KeyAlt)
      return false;
    if ((keyNum & vcMOD_Super) && !io.KeySuper)
      return false;

    return ImGui::IsKeyPressed((keyNum & 0x1FF), false);
  }

  void GetKeyName(vcBind key, char *pBuffer, uint32_t bufferLen)
  {
    if (key == vcB_Count || keyBinds[key] == 0)
    {
      udStrcpy(pBuffer, (size_t)bufferLen, vcString::Get("bindingsClear"));
      return;
    }

    int mappedKey = keyBinds[key];

    const char *pStrings[5] = {};

    if ((mappedKey & vcMOD_Shift) == vcMOD_Shift)
      pStrings[0] = "Shift + ";
    else
      pStrings[0] = "";

    if ((mappedKey & vcMOD_Ctrl) == vcMOD_Ctrl)
      pStrings[1] = "Ctrl + ";
    else
      pStrings[1] = "";

    if ((mappedKey & vcMOD_Alt) == vcMOD_Alt)
      pStrings[2] = "Alt + ";
    else
      pStrings[2] = "";

    if ((mappedKey & vcMOD_Super) == vcMOD_Super)
    {
#ifdef UDPLATFORM_WINDOWS
      pStrings[3] = "Win + ";
#elif UDPLATFORM_OSX
      pStrings[3] = "Cmd + ";
#else
      pStrings[3] = "Super + ";
#endif
    }
    else
    {
      pStrings[3] = "";
    }

    pStrings[4] = SDL_GetScancodeName((SDL_Scancode)(mappedKey & 0x1FF));
   
    vcStringFormat(pBuffer, bufferLen, "{0}{1}{2}{3}{4}", pStrings, 5);
  }

  vcBind BindFromName(const char* pName)
  {
    int i = 0;
    for (; i < (int)udLengthOf(bindNames); ++i)
    {
      if (udStrEquali(bindNames[i], pName))
        break;
    }

    return (vcBind)i;
  }

  const char* GetBindName(vcBind key)
  {
    return bindNames[key];
  }

  int GetMod(int key)
  {
    // Only modifier keys in input
    if (key >= SDL_SCANCODE_LCTRL && key <= SDL_SCANCODE_RGUI)
      return 0;

    ImGuiIO io = ImGui::GetIO();
    int out = key;

    if (io.KeyShift)
      out |= vcMOD_Shift;
    if (io.KeyCtrl)
      out |= vcMOD_Ctrl;
    if (io.KeyAlt)
      out |= vcMOD_Alt;
    if (io.KeySuper)
      out |= vcMOD_Super;

    return out;
  }

  void Set(vcBind key, int value)
  {
    keyBinds[(int)key] = value;
  }

  int Get(vcBind key)
  {
    return keyBinds[key];
  }

  void DisplayBindings(vcState *pProgramState)
  {
    static int target = -1; // TODO: Document/reconsider limitation of 50 chars
    static const char *pError = "";

    if (target != -1 && pProgramState->currentKey)
    {
      for (int i = 0; i < vcB_Count; ++i)
      {
        if (keyBinds[i] == pProgramState->currentKey)
        {
          pError = vcString::Get("bindingsErrorUnbound");
          Set((vcBind)i, 0);
          break;
        }
      }

      Set((vcBind)target, pProgramState->currentKey);
      target = -1;
    }

    if (*pError != '\0')
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", pError);

    ImGui::Columns(3);
    ImGui::SetColumnWidth(0, 125);
    ImGui::SetColumnWidth(1, 125);
    ImGui::SetColumnWidth(2, 650);

    // Header Row
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextUnformatted(vcString::Get("bindingsColumnName"));
    ImGui::NextColumn();
    ImGui::TextUnformatted(vcString::Get("bindingsColumnKeyCombination"));
    ImGui::NextColumn();
    ImGui::TextUnformatted(vcString::Get("bindingsColumnDescription"));
    ImGui::NextColumn();
    ImGui::SetWindowFontScale(1.f);

    for (int i = 0; i < vcB_Count; ++i)
    {
      if (ImGui::Button(bindNames[i], ImVec2(-1, 0)))
      {
        pError = "";
        if (target == i)
        {
          Set((vcBind)i, 0);
          target = -1;
        }
        else
        {
          pProgramState->currentKey = 0;
          target = i;
        }
      }

      ImGui::NextColumn();

      char key[50];
      GetKeyName((vcBind)i, key, (uint32_t)udLengthOf(key));
      ImGui::TextUnformatted(key);

      ImGui::NextColumn();
      ImGui::TextUnformatted(vcString::Get(udTempStr("bindings%s", bindNames[i])));

      ImGui::NextColumn();
    }

    ImGui::EndColumns();
  }

  int DecodeKeyString(const char *pBind)
  {
    if (pBind == nullptr || *pBind == '\0')
      return 0;

    int len = (int)udStrlen(pBind);
    size_t index = 0;
    int value = 0;

    if (udStrrchr(pBind, "+", (size_t *)&index) != nullptr)
    {
      size_t modIndex = 0;

      for (int i = 0; i < (int)udLengthOf(modList); ++i)
      {
        udStrstr(pBind, len, pModText[i], &modIndex);
        if ((int)modIndex != len)
          value |= modList[i];
      }

      pBind += index + 2;
    }

    value += SDL_GetScancodeFromName(pBind);

    return value;
  }
}

UDCOMPILEASSERT(udLengthOf(vcHotkey::bindNames) == vcB_Count, "Hotkey count discrepancy.");
