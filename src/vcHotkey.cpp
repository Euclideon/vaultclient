#include "vcHotkey.h"

#include "vcStrings.h"
#include "vcStringFormat.h"

#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL2/SDL.h>

namespace vcHotkey
{
  static int target = -1;
  static int keyBinds[vcB_Count] = {};
  static int pendingKeyBinds[vcB_Count] = {};

  const char* bindNames[] =
  {
    "CameraForward",
    "CameraBackward",
    "CameraLeft",
    "CameraUp",
    "CameraRight",
    "CameraDown",
    "Remove",
    "Cancel",
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
    "BindingsInterface",
    "Undo",
    "TakeScreenshot"
  };
  UDCOMPILEASSERT(udLengthOf(vcHotkey::bindNames) == vcB_Count, "Hotkey count discrepancy.");

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

  enum KeyErrorStrings
  {
    vcKES_Unbound = 0,
    vcKES_Select = 1,
    vcKES_Count = 2
  };
  ImVec4 KeyErrorColours[] =
  {
    ImVec4(1, 0, 0, 1),
    ImVec4(1, 1, 1, 1)
  };
  static const char *pKeyErrors[] =
  {
    "bindingsErrorUnbound",
    "bindingsSelectKey"
  };

  bool HasPendingChanges()
  {
    return memcmp(keyBinds, pendingKeyBinds, sizeof(keyBinds));
  }

  void ApplyPendingChanges()
  {
    memcpy(keyBinds, pendingKeyBinds, sizeof(keyBinds));
  }

  void RevertPendingChanges()
  {
    memcpy(pendingKeyBinds, keyBinds, sizeof(keyBinds));
  }

  bool IsDown(int keyNum)
  {
    if (target != -1)
      return false;

    return ImGui::GetIO().KeysDown[keyNum];
  }

  bool IsDown(vcBind key)
  {
    return IsDown(keyBinds[key]);
  }

  bool IsPressed(int keyNum)
  {
    if (target != -1)
      return false;

    ImGuiIO io = ImGui::GetIO();

    if (((keyNum & vcMOD_Shift) == vcMOD_Shift) != io.KeyShift)
      return false;
    if (((keyNum & vcMOD_Ctrl) == vcMOD_Ctrl) != io.KeyCtrl)
      return false;
    if (((keyNum & vcMOD_Alt) == vcMOD_Alt) != io.KeyAlt)
      return false;
    if (((keyNum & vcMOD_Super) == vcMOD_Super) != io.KeySuper)
      return false;

    return ImGui::IsKeyPressed((keyNum & 0x1FF), false);
  }

  bool IsPressed(vcBind key)
  {
    return IsPressed(keyBinds[key]);
  }

  void NameFromKey(int key, char *pBuffer, uint32_t bufferLen)
  {
    if (key == 0)
    {
      udStrcpy(pBuffer, (size_t)bufferLen, vcString::Get("bindingsClear"));
      return;
    }

    const char *pStrings[5] = {};

    if ((key & vcMOD_Shift) == vcMOD_Shift)
      pStrings[0] = "Shift + ";
    else
      pStrings[0] = "";

    if ((key & vcMOD_Ctrl) == vcMOD_Ctrl)
      pStrings[1] = "Ctrl + ";
    else
      pStrings[1] = "";

    if ((key & vcMOD_Alt) == vcMOD_Alt)
      pStrings[2] = "Alt + ";
    else
      pStrings[2] = "";

    if ((key & vcMOD_Super) == vcMOD_Super)
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

    pStrings[4] = SDL_GetScancodeName((SDL_Scancode)(key & 0x1FF));

    vcStringFormat(pBuffer, bufferLen, "{0}{1}{2}{3}{4}", pStrings, 5);
  }

  void GetKeyName(vcBind key, char *pBuffer, uint32_t bufferLen)
  {
    int mappedKey;

    if (key == vcB_Count)
      mappedKey = 0;
    else
      mappedKey = keyBinds[key];

    NameFromKey(mappedKey, pBuffer, bufferLen);
  }

  void GetPendingKeyName(vcBind key, char *pBuffer, uint32_t bufferLen)
  {
    int mappedKey;

    if (key == vcB_Count)
      mappedKey = 0;
    else
      mappedKey = pendingKeyBinds[key];

    NameFromKey(mappedKey, pBuffer, bufferLen);
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
    pendingKeyBinds[(int)key] = value;
  }

  int Get(vcBind key)
  {
    return keyBinds[key];
  }

  int GetPending(vcBind key)
  {
    return pendingKeyBinds[key];
  }

  void DisplayBindings(vcState *pProgramState)
  {
    int errors = 0;

    if (target != -1)
    {
      errors |= (1 << vcKES_Select);

      if (pProgramState->currentKey)
      {
        if (target >= 0)
        {
          for (int i = 0; i < vcB_Count; ++i)
          {
            if (pendingKeyBinds[i] == pProgramState->currentKey)
            {
              Set((vcBind)i, 0);
              break;
            }
          }

          Set((vcBind)target, pProgramState->currentKey);
          target *= -1;
        }

        if (!ImGui::IsKeyPressed((pProgramState->currentKey & 0x1FF), true))
          target = -1;
      }
    }

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
        errors = 0;
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

      if (vcHotkey::GetPending((vcBind)i) == 0)
      {
        ImGui::GetForegroundDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::ColorConvertFloat4ToU32(KeyErrorColours[vcKES_Unbound]), 0.0f, ImDrawCornerFlags_All, 2.0f);

        errors |= (1 << vcKES_Unbound);
      }
      else if (target == i)
      {
        ImGui::GetForegroundDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::ColorConvertFloat4ToU32(KeyErrorColours[vcKES_Select]), 0.0f, ImDrawCornerFlags_All, 2.0f);
      }

      ImGui::NextColumn();

      // TODO: Document/reconsider limitation of 50 chars
      char key[50];
      GetPendingKeyName((vcBind)i, key, (uint32_t)udLengthOf(key));
      ImGui::TextUnformatted(key);

      ImGui::NextColumn();
      ImGui::TextUnformatted(vcString::Get(udTempStr("bindings%s", bindNames[i])));

      ImGui::NextColumn();
    }

    ImGui::EndColumns();

    if (errors != 0)
    {
      for (int i = 0; i < vcKES_Count; ++i)
      {
        if (errors & (1 << i))
          ImGui::TextColored(KeyErrorColours[i], "%s", vcString::Get(pKeyErrors[i]));
      }
    }
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
